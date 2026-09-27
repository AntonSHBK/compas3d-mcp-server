#include <WinSock2.h>

#include "kompas_bridge/kompas/kompas_feature_service.hpp"

#include <afxdisp.h>
#include <kapi5.h>
#include <ksConstants3D.h>

#include "kompas_bridge/kompas/object_registry.hpp"
#include "kompas_bridge/protocol/codec.hpp"

namespace kompas_bridge {
namespace {
Microsoft::WRL::ComPtr<IDispatch> object_dispatch(ObjectRegistry &registry,
                                                  std::string_view handle,
                                                  ObjectKind kind) {
  auto object = registry.get_object(handle, kind);
  Microsoft::WRL::ComPtr<IDispatch> dispatch;
  if (FAILED(object.As(&dispatch))) {
    throw ProtocolError("kompas_api_error",
                        "CAD object has no IDispatch interface.");
  }
  return dispatch;
}

short direction_value(ExtrusionDirection value) {
  switch (value) {
  case ExtrusionDirection::kForward:
    return dtNormal;
  case ExtrusionDirection::kReverse:
    return dtReverse;
  case ExtrusionDirection::kBoth:
    return dtBoth;
  }
  throw ProtocolError("invalid_params", "Unsupported extrusion direction.");
}

short entity_type(BooleanOperation operation) {
  switch (operation) {
  case BooleanOperation::kNewBody:
    return o3d_baseExtrusion;
  case BooleanOperation::kJoin:
    return o3d_bossExtrusion;
  case BooleanOperation::kCut:
    return o3d_cutExtrusion;
  }
  throw ProtocolError("invalid_params", "Unsupported boolean operation.");
}

template <typename Definition>
void configure_definition(Definition &definition, IDispatch *sketch,
                          const ExtrusionParameters &parameters) {
  definition.SetDirectionType(direction_value(parameters.direction));
  if ((sketch != nullptr && !definition.SetSketch(sketch)) ||
      !definition.SetSideParam(TRUE, etBlind, parameters.distance, 0.0,
                               FALSE)) {
    throw ProtocolError("kompas_api_error",
                        "KOMPAS rejected extrusion parameters.");
  }
  if (parameters.direction == ExtrusionDirection::kBoth &&
      !definition.SetSideParam(FALSE, etBlind, parameters.distance, 0.0,
                               FALSE)) {
    throw ProtocolError("kompas_api_error",
                        "KOMPAS rejected reverse extrusion parameters.");
  }
}

template <typename Definition>
ExtrusionParameters read_definition(Definition &definition,
                                    BooleanOperation operation) {
  short type{};
  double depth{};
  double draft{};
  BOOL outward{};
  if (!definition.GetSideParam(TRUE, &type, &depth, &draft, &outward)) {
    throw ProtocolError("kompas_api_error",
                        "KOMPAS did not return extrusion parameters.");
  }
  ExtrusionDirection direction = ExtrusionDirection::kForward;
  switch (definition.GetDirectionType()) {
  case dtNormal:
    direction = ExtrusionDirection::kForward;
    break;
  case dtReverse:
    direction = ExtrusionDirection::kReverse;
    break;
  case dtBoth:
    direction = ExtrusionDirection::kBoth;
    break;
  default:
    throw ProtocolError("kompas_api_error",
                        "KOMPAS returned an unknown extrusion direction.");
  }
  return {.distance = depth, .direction = direction, .operation = operation};
}

template <typename Function>
auto cad_call(Function &&function) -> decltype(function()) {
  try {
    return function();
  } catch (CException *error) {
    error->Delete();
    throw ProtocolError("kompas_api_error", "KOMPAS feature operation failed.");
  }
}
} // namespace

std::string_view direction_name(ExtrusionDirection value) {
  switch (value) {
  case ExtrusionDirection::kForward:
    return "forward";
  case ExtrusionDirection::kReverse:
    return "reverse";
  case ExtrusionDirection::kBoth:
    return "both";
  }
  return "unknown";
}

std::string_view operation_name(BooleanOperation value) {
  switch (value) {
  case BooleanOperation::kNewBody:
    return "new_body";
  case BooleanOperation::kJoin:
    return "join";
  case BooleanOperation::kCut:
    return "cut";
  }
  return "unknown";
}

KompasFeatureService::KompasFeatureService(ObjectRegistry &registry)
    : registry_(registry) {}

ExtrusionResult KompasFeatureService::extrude(
    std::string_view documentId, std::string_view partId,
    std::string_view sketchId, const ExtrusionParameters &parameters) {
  if (parameters.distance <= 0.0) {
    throw ProtocolError("invalid_params",
                        "Extrusion distance must be positive.");
  }
  return cad_call([&] {
    const auto partInfo = registry_.get_info(partId);
    const auto sketchInfo = registry_.get_info(sketchId);
    if (partInfo.documentId != documentId ||
        sketchInfo.documentId != documentId) {
      throw ProtocolError("invalid_params",
                          "Part and sketch must belong to the document.");
    }
    auto partDispatch = object_dispatch(registry_, partId, ObjectKind::kPart);
    auto sketchDispatch =
        object_dispatch(registry_, sketchId, ObjectKind::kSketch);
    ksPart part(partDispatch.Detach());
    ksEntity entity(part.NewEntity(entity_type(parameters.operation)));
    if (entity.m_lpDispatch == nullptr) {
      throw ProtocolError("kompas_api_error",
                          "KOMPAS did not allocate extrusion.");
    }
    IDispatch *definitionDispatch = entity.GetDefinition();
    if (parameters.operation == BooleanOperation::kNewBody) {
      ksBaseExtrusionDefinition definition(definitionDispatch);
      configure_definition(definition, sketchDispatch.Get(), parameters);
    } else if (parameters.operation == BooleanOperation::kJoin) {
      ksBossExtrusionDefinition definition(definitionDispatch);
      configure_definition(definition, sketchDispatch.Get(), parameters);
    } else {
      ksCutExtrusionDefinition definition(definitionDispatch);
      configure_definition(definition, sketchDispatch.Get(), parameters);
    }
    if (!entity.Create()) {
      throw ProtocolError("kompas_api_error",
                          "KOMPAS did not create extrusion.");
    }
    const std::string featureId = registry_.register_child(
        ObjectKind::kFeature, documentId, entity.m_lpDispatch);
    metadata_[featureId] = {.partId = std::string(partId),
                            .sketchId = std::string(sketchId),
                            .operation = parameters.operation};
    return ExtrusionResult{.featureId = featureId,
                           .documentId = std::string(documentId),
                           .partId = std::string(partId),
                           .sketchId = std::string(sketchId),
                           .parameters = parameters};
  });
}

ExtrusionResult
KompasFeatureService::get_parameters(std::string_view featureId) {
  return cad_call([&] {
    const auto metadata = metadata_.find(std::string(featureId));
    if (metadata == metadata_.end()) {
      throw ProtocolError(
          "object_not_found",
          "Extrusion metadata is not available in this bridge session.");
    }
    const auto info = registry_.get_info(featureId);
    auto dispatch = object_dispatch(registry_, featureId, ObjectKind::kFeature);
    ksEntity entity(dispatch.Detach());
    IDispatch *definitionDispatch = entity.GetDefinition();
    ExtrusionParameters parameters;
    if (metadata->second.operation == BooleanOperation::kNewBody) {
      ksBaseExtrusionDefinition definition(definitionDispatch);
      parameters = read_definition(definition, metadata->second.operation);
    } else if (metadata->second.operation == BooleanOperation::kJoin) {
      ksBossExtrusionDefinition definition(definitionDispatch);
      parameters = read_definition(definition, metadata->second.operation);
    } else {
      ksCutExtrusionDefinition definition(definitionDispatch);
      parameters = read_definition(definition, metadata->second.operation);
    }
    return ExtrusionResult{.featureId = std::string(featureId),
                           .documentId = info.documentId.value_or(""),
                           .partId = metadata->second.partId,
                           .sketchId = metadata->second.sketchId,
                           .parameters = parameters};
  });
}

ExtrusionResult
KompasFeatureService::update_extrusion(std::string_view featureId,
                                       double distance) {
  if (distance <= 0.0) {
    throw ProtocolError("invalid_params",
                        "Extrusion distance must be positive.");
  }
  return cad_call([&] {
    ExtrusionResult current = get_parameters(featureId);
    current.parameters.distance = distance;
    auto dispatch = object_dispatch(registry_, featureId, ObjectKind::kFeature);
    ksEntity entity(dispatch.Detach());
    IDispatch *definitionDispatch = entity.GetDefinition();
    if (current.parameters.operation == BooleanOperation::kNewBody) {
      ksBaseExtrusionDefinition definition(definitionDispatch);
      configure_definition(definition, nullptr, current.parameters);
    } else if (current.parameters.operation == BooleanOperation::kJoin) {
      ksBossExtrusionDefinition definition(definitionDispatch);
      configure_definition(definition, nullptr, current.parameters);
    } else {
      ksCutExtrusionDefinition definition(definitionDispatch);
      configure_definition(definition, nullptr, current.parameters);
    }
    if (!entity.Update()) {
      throw ProtocolError("kompas_api_error",
                          "KOMPAS did not update extrusion.");
    }
    return current;
  });
}

void KompasFeatureService::rebuild(std::string_view partId) {
  cad_call([&] {
    auto dispatch = object_dispatch(registry_, partId, ObjectKind::kPart);
    ksPart part(dispatch.Detach());
    if (!part.RebuildModel()) {
      throw ProtocolError("kompas_api_error",
                          "KOMPAS did not rebuild the model.");
    }
  });
}
} // namespace kompas_bridge
