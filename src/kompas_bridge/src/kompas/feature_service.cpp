#include <WinSock2.h>

#include "kompas_bridge/kompas/kompas_feature_service.hpp"

#include <afxdisp.h>
#include <kapi5.h>
#include <ksConstants3D.h>

#include "kompas_bridge/kompas/object_registry.hpp"
#include "kompas_bridge/protocol/codec.hpp"

namespace kompas_bridge {
namespace {
std::string to_utf8(const CString &value) {
  if (value.IsEmpty()) {
    return {};
  }
  const int size =
      WideCharToMultiByte(CP_UTF8, 0, value.GetString(), value.GetLength(),
                          nullptr, 0, nullptr, nullptr);
  if (size <= 0) {
    throw ProtocolError("kompas_api_error",
                        "Feature name is not valid UTF-16.");
  }
  std::string result(static_cast<std::size_t>(size), '\0');
  WideCharToMultiByte(CP_UTF8, 0, value.GetString(), value.GetLength(),
                      result.data(), size, nullptr, nullptr);
  return result;
}

std::string_view feature_type_name(short type) {
  switch (type) {
  case o3d_baseExtrusion:
  case o3d_bossExtrusion:
  case o3d_cutExtrusion:
    return "extrusion";
  case o3d_sketch:
    return "sketch";
  default:
    return "unknown";
  }
}

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
    const std::string featureId = registry_.register_model_child(
        ObjectKind::kFeature, documentId, partId, entity.m_lpDispatch);
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

std::vector<FeatureInfo>
KompasFeatureService::list_features(std::string_view partId) {
  return cad_call([&] {
    const auto partInfo = registry_.get_info(partId);
    const std::string documentId = partInfo.documentId.value_or("");
    auto dispatch = object_dispatch(registry_, partId, ObjectKind::kPart);
    ksPart part(dispatch.Detach());
    ksFeature root(part.GetFeature());
    if (root.m_lpDispatch == nullptr) {
      throw ProtocolError("kompas_api_error",
                          "KOMPAS did not return the feature tree.");
    }
    ksFeatureCollection collection(root.SubFeatureCollection(TRUE, FALSE));
    std::vector<FeatureInfo> result;
    if (collection.m_lpDispatch == nullptr) {
      return result;
    }
    collection.refresh();
    const long count = collection.GetCount();
    result.reserve(static_cast<std::size_t>(count));
    for (long index = 0; index < count; ++index) {
      ksFeature feature(collection.GetByIndex(index));
      if (feature.m_lpDispatch == nullptr) {
        continue;
      }
      ksEntity object(feature.GetObject());
      if (object.m_lpDispatch == nullptr) {
        continue;
      }
      const std::string featureId = registry_.find_or_register_model_child(
          ObjectKind::kFeature, documentId, partId, object.m_lpDispatch);
      std::optional<std::string> ownerId;
      ksFeature owner(feature.GetOwnerFeature());
      if (owner.m_lpDispatch != nullptr) {
        ksEntity ownerObject(owner.GetObject());
        if (ownerObject.m_lpDispatch != nullptr) {
          ownerId = registry_.find_or_register_model_child(
              ObjectKind::kFeature, documentId, partId,
              ownerObject.m_lpDispatch);
        }
      }
      result.push_back(
          {.featureId = featureId,
           .documentId = documentId,
           .partId = std::string(partId),
           .name = to_utf8(feature.GetName()),
           .featureType = std::string(feature_type_name(feature.GetType())),
           .excluded = feature.GetExcluded() != FALSE,
           .valid = feature.IsValid() != FALSE,
           .ownerFeatureId = std::move(ownerId),
           .updateStamp = feature.GetUpdateStamp()});
    }
    return result;
  });
}

FeatureInfo KompasFeatureService::get_info(std::string_view featureId) {
  return cad_call([&] {
    const auto info = registry_.get_info(featureId);
    auto dispatch = object_dispatch(registry_, featureId, ObjectKind::kFeature);
    ksEntity entity(dispatch.Detach());
    ksFeature feature(entity.GetFeature());
    if (feature.m_lpDispatch == nullptr || !info.documentId || !info.partId) {
      throw ProtocolError("kompas_api_error",
                          "KOMPAS did not return feature information.");
    }
    std::optional<std::string> ownerId;
    ksFeature owner(feature.GetOwnerFeature());
    if (owner.m_lpDispatch != nullptr) {
      ksEntity ownerObject(owner.GetObject());
      if (ownerObject.m_lpDispatch != nullptr) {
        ownerId = registry_.find_or_register_model_child(
            ObjectKind::kFeature, *info.documentId, *info.partId,
            ownerObject.m_lpDispatch);
      }
    }
    return FeatureInfo{.featureId = std::string(featureId),
                       .documentId = *info.documentId,
                       .partId = *info.partId,
                       .name = to_utf8(feature.GetName()),
                       .featureType =
                           std::string(feature_type_name(feature.GetType())),
                       .excluded = feature.GetExcluded() != FALSE,
                       .valid = feature.IsValid() != FALSE,
                       .ownerFeatureId = std::move(ownerId),
                       .updateStamp = feature.GetUpdateStamp()};
  });
}

std::uint64_t KompasFeatureService::rebuild(std::string_view partId) {
  return cad_call([&] {
    auto dispatch = object_dispatch(registry_, partId, ObjectKind::kPart);
    ksPart part(dispatch.Detach());
    if (!part.RebuildModel()) {
      throw ProtocolError("kompas_api_error",
                          "KOMPAS did not rebuild the model.");
    }
    return registry_.bump_model_revision(partId);
  });
}
} // namespace kompas_bridge
