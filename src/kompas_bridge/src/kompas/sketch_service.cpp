#include <WinSock2.h>

#include "kompas_bridge/kompas/kompas_sketch_service.hpp"

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

short plane_type(std::string_view plane) {
  if (plane == "xoy" || plane == "xy" || plane == "XOY" || plane == "XY")
    return o3d_planeXOY;
  if (plane == "xoz" || plane == "xz" || plane == "XOZ" || plane == "XZ")
    return o3d_planeXOZ;
  if (plane == "yoz" || plane == "yz" || plane == "YOZ" || plane == "YZ")
    return o3d_planeYOZ;
  throw ProtocolError("invalid_params", "Plane must be xoy, xoz, or yoz.");
}

template <typename Function>
auto cad_call(Function &&function) -> decltype(function()) {
  try {
    return function();
  } catch (CException *error) {
    error->Delete();
    throw ProtocolError("kompas_api_error", "KOMPAS sketch operation failed.");
  }
}
} // namespace

KompasSketchService::KompasSketchService(ObjectRegistry &registry)
    : registry_(registry) {}

SketchResult KompasSketchService::create(std::string_view documentId,
                                         std::string_view partId,
                                         std::string_view plane) {
  return cad_call([&] {
    const auto partInfo = registry_.get_info(partId);
    if (partInfo.documentId != documentId) {
      throw ProtocolError("invalid_params",
                          "Part does not belong to the document.");
    }
    auto partDispatch = object_dispatch(registry_, partId, ObjectKind::kPart);
    ksPart part(partDispatch.Detach());
    ksEntity sketch(part.NewEntity(o3d_sketch));
    ksEntity basePlane(part.GetDefaultEntity(plane_type(plane)));
    if (sketch.m_lpDispatch == nullptr || basePlane.m_lpDispatch == nullptr) {
      throw ProtocolError("kompas_api_error",
                          "KOMPAS did not create sketch objects.");
    }
    ksSketchDefinition definition(sketch.GetDefinition());
    if (definition.m_lpDispatch == nullptr ||
        !definition.SetPlane(basePlane.m_lpDispatch) || !sketch.Create()) {
      throw ProtocolError("kompas_api_error",
                          "KOMPAS did not create the sketch.");
    }
    const std::string id = registry_.register_model_child(
        ObjectKind::kSketch, documentId, partId, sketch.m_lpDispatch);
    metadata_[id] = {.documentId = std::string(documentId),
                     .partId = std::string(partId),
                     .plane = std::string(plane),
                     .geometryCount = 0};
    return SketchResult{.sketchId = id,
                        .documentId = std::string(documentId),
                        .partId = std::string(partId),
                        .plane = std::string(plane),
                        .editing = false,
                        .geometryCount = 0};
  });
}

SketchResult KompasSketchService::begin_edit(std::string_view sketchId) {
  return cad_call([&] {
    if (edits_.contains(std::string(sketchId))) {
      throw ProtocolError("invalid_params", "Sketch is already being edited.");
    }
    auto sketchDispatch =
        object_dispatch(registry_, sketchId, ObjectKind::kSketch);
    ksEntity sketch(sketchDispatch.Detach());
    ksSketchDefinition definition(sketch.GetDefinition());
    Microsoft::WRL::ComPtr<IDispatch> document;
    document.Attach(definition.BeginEdit());
    if (document == nullptr) {
      throw ProtocolError("kompas_api_error",
                          "KOMPAS did not enter sketch edit mode.");
    }
    edits_.emplace(std::string(sketchId), std::move(document));
    return result(sketchId, true);
  });
}

SketchResult KompasSketchService::end_edit(std::string_view sketchId) {
  return cad_call([&] {
    (void)edit_document(sketchId);
    auto sketchDispatch =
        object_dispatch(registry_, sketchId, ObjectKind::kSketch);
    ksEntity sketch(sketchDispatch.Detach());
    ksSketchDefinition definition(sketch.GetDefinition());
    if (!definition.EndEdit()) {
      throw ProtocolError("kompas_api_error",
                          "KOMPAS did not finish sketch editing.");
    }
    edits_.erase(std::string(sketchId));
    return result(sketchId, false);
  });
}

IDispatch *KompasSketchService::edit_document(std::string_view sketchId) const {
  const auto found = edits_.find(std::string(sketchId));
  if (found == edits_.end()) {
    throw ProtocolError("invalid_params", "Sketch is not in edit mode.");
  }
  return found->second.Get();
}

SketchResult KompasSketchService::add_line(std::string_view sketchId,
                                           Point2d start, Point2d end) {
  return cad_call([&] {
    (void)registry_.get_object(sketchId, ObjectKind::kSketch);
    IDispatch *dispatch = edit_document(sketchId);
    dispatch->AddRef();
    ksDocument2D document(dispatch);
    const long reference =
        document.ksLineSeg(start.x, start.y, end.x, end.y, 1);
    if (reference == 0) {
      throw ProtocolError("kompas_api_error",
                          "KOMPAS did not create the line.");
    }
    ++metadata_.at(std::string(sketchId)).geometryCount;
    return result(sketchId, true);
  });
}

SketchResult KompasSketchService::add_circle(std::string_view sketchId,
                                             Point2d center, double radius) {
  if (radius <= 0.0) {
    throw ProtocolError("invalid_params", "Circle radius must be positive.");
  }
  return cad_call([&] {
    (void)registry_.get_object(sketchId, ObjectKind::kSketch);
    IDispatch *dispatch = edit_document(sketchId);
    dispatch->AddRef();
    ksDocument2D document(dispatch);
    const long reference = document.ksCircle(center.x, center.y, radius, 1);
    if (reference == 0) {
      throw ProtocolError("kompas_api_error",
                          "KOMPAS did not create the circle.");
    }
    ++metadata_.at(std::string(sketchId)).geometryCount;
    return result(sketchId, true);
  });
}

SketchResult KompasSketchService::result(std::string_view sketchId,
                                         bool editing) const {
  const auto found = metadata_.find(std::string(sketchId));
  if (found == metadata_.end()) {
    throw ProtocolError(
        "object_not_found",
        "Sketch metadata is not available in this bridge session.");
  }
  return {.sketchId = std::string(sketchId),
          .documentId = found->second.documentId,
          .partId = found->second.partId,
          .plane = found->second.plane,
          .editing = editing,
          .geometryCount = found->second.geometryCount};
}
} // namespace kompas_bridge
