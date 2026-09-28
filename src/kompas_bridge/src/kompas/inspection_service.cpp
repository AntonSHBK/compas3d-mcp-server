#include <WinSock2.h>
#include <afxdisp.h>
#include <kapi5.h>

#include <cmath>
#include <iterator>
#include <utility>

#include "kompas_bridge/kompas/kompas_inspection_service.hpp"
#include "kompas_bridge/kompas/object_registry.hpp"
#include "kompas_bridge/protocol/codec.hpp"

namespace kompas_bridge {
namespace {

constexpr double kCentimetersToMillimeters = 10.0;
constexpr double kSquareCentimetersToSquareMillimeters = 100.0;
constexpr double kCubicCentimetersToCubicMillimeters = 1000.0;
constexpr double kGramsToKilograms = 0.001;
constexpr double kGramsPerCubicCentimeterToKilogramsPerCubicMeter = 1000.0;
constexpr double kGramSquareCentimetersToKilogramSquareMillimeters = 0.1;

Point3D centimeters_to_millimeters(Point3D point) {
  point.x *= kCentimetersToMillimeters;
  point.y *= kCentimetersToMillimeters;
  point.z *= kCentimetersToMillimeters;
  return point;
}

Microsoft::WRL::ComPtr<IDispatch> object_dispatch(ObjectRegistry& registry,
                                                  std::string_view handle,
                                                  ObjectKind kind) {
  auto object = registry.get_object(handle, kind);
  Microsoft::WRL::ComPtr<IDispatch> dispatch;
  if (FAILED(object.As(&dispatch))) {
    throw ProtocolError("kompas_api_error",
                        "CAD inspection object has no IDispatch interface.");
  }
  return dispatch;
}

template <typename Function>
auto cad_call(Function&& function) -> decltype(function()) {
  try {
    return function();
  } catch (CException* error) {
    error->Delete();
    throw ProtocolError("kompas_api_error",
                        "KOMPAS inspection operation failed.");
  }
}

BoundingBox3D body_box(ksBody& body) {
  BoundingBox3D box;
  if (!body.GetGabarit(&box.min.x, &box.min.y, &box.min.z, &box.max.x,
                       &box.max.y, &box.max.z)) {
    throw ProtocolError("kompas_api_error",
                        "KOMPAS did not return body bounds.");
  }
  return box;
}

BoundingBox3D surface_box(ksSurface& surface) {
  BoundingBox3D box;
  if (!surface.GetGabarit(&box.min.x, &box.min.y, &box.min.z, &box.max.x,
                          &box.max.y, &box.max.z)) {
    throw ProtocolError("kompas_api_error",
                        "KOMPAS did not return face bounds.");
  }
  return box;
}

std::string surface_type(ksFaceDefinition& face) {
  if (face.IsPlanar()) {
    return "plane";
  }
  if (face.IsCylinder()) {
    return "cylinder";
  }
  if (face.IsCone()) {
    return "cone";
  }
  if (face.IsSphere()) {
    return "sphere";
  }
  if (face.IsTorus()) {
    return "torus";
  }
  if (face.IsNurbsSurface()) {
    return "nurbs";
  }
  if (face.IsRevolved()) {
    return "revolved";
  }
  if (face.IsSwept()) {
    return "swept";
  }
  return "unknown";
}

std::optional<Point3D> planar_normal(ksFaceDefinition& face,
                                     ksSurface& surface) {
  if (!face.IsPlanar()) {
    return std::nullopt;
  }
  Point3D normal;
  const double u = (surface.GetParamUMin() + surface.GetParamUMax()) / 2.0;
  const double v = (surface.GetParamVMin() + surface.GetParamVMax()) / 2.0;
  if (!surface.GetNormal(u, v, &normal.x, &normal.y, &normal.z)) {
    throw ProtocolError("kompas_api_error",
                        "KOMPAS did not return face normal.");
  }
  if (!face.GetNormalOrientation()) {
    normal.x = -normal.x;
    normal.y = -normal.y;
    normal.z = -normal.z;
  }
  const double length = std::sqrt(normal.x * normal.x + normal.y * normal.y +
                                  normal.z * normal.z);
  if (length <= 0.0) {
    throw ProtocolError("kompas_api_error",
                        "KOMPAS returned a zero face normal.");
  }
  normal.x /= length;
  normal.y /= length;
  normal.z /= length;
  return normal;
}

std::optional<double> cylinder_radius(ksFaceDefinition& face) {
  if (!face.IsCylinder()) {
    return std::nullopt;
  }
  double height{};
  double radius{};
  if (!face.GetCylinderParam(&height, &radius)) {
    throw ProtocolError("kompas_api_error",
                        "KOMPAS did not return cylinder parameters.");
  }
  return radius;
}

std::optional<std::string> register_owner_feature(ObjectRegistry& registry,
                                                  std::string_view documentId,
                                                  std::string_view partId,
                                                  IDispatch* featureDispatch) {
  ksFeature feature(featureDispatch);
  if (feature.m_lpDispatch == nullptr) {
    return std::nullopt;
  }
  ksEntity object(feature.GetObject());
  if (object.m_lpDispatch == nullptr) {
    return std::nullopt;
  }
  return registry.find_or_register_model_child(ObjectKind::kFeature, documentId,
                                               partId, object.m_lpDispatch);
}

BodyInfo make_body_info(ObjectRegistry& registry, std::string_view bodyId,
                        std::string_view partId, std::string_view documentId,
                        ksBody& body) {
  return BodyInfo{.bodyId = std::string(bodyId),
                  .partId = std::string(partId),
                  .solid = body.IsSolid() != FALSE,
                  .boundingBox = body_box(body),
                  .ownerFeatureId = register_owner_feature(
                      registry, documentId, partId, body.GetFeature())};
}

FaceInfo make_face_info(ObjectRegistry& registry, std::string_view faceId,
                        std::string_view partId, std::string_view documentId,
                        std::optional<std::string> bodyId,
                        ksFaceDefinition& face) {
  ksSurface surface(face.GetSurface());
  if (surface.m_lpDispatch == nullptr) {
    throw ProtocolError("kompas_api_error",
                        "KOMPAS did not return face surface.");
  }
  ksEntity owner(face.GetOwnerEntity());
  std::optional<std::string> ownerFeatureId;
  if (owner.m_lpDispatch != nullptr) {
    ownerFeatureId = registry.find_or_register_model_child(
        ObjectKind::kFeature, documentId, partId, owner.m_lpDispatch);
  }
  return FaceInfo{
      .faceId = std::string(faceId),
      .partId = std::string(partId),
      .bodyId = std::move(bodyId),
      .surfaceType = surface_type(face),
      .areaMm2 = face.GetArea(0) * kSquareCentimetersToSquareMillimeters,
      .normal = planar_normal(face, surface),
      .boundingBox = surface_box(surface),
      .radiusMm = cylinder_radius(face),
      .ownerFeatureId = std::move(ownerFeatureId)};
}

}  // namespace

KompasInspectionService::KompasInspectionService(ObjectRegistry& registry)
    : registry_(registry) {}

std::vector<BodyInfo> KompasInspectionService::list_bodies(
    std::string_view partId) {
  return cad_call([&] {
    const auto partInfo = registry_.get_info(partId);
    const std::string documentId = partInfo.documentId.value_or("");
    auto dispatch = object_dispatch(registry_, partId, ObjectKind::kPart);
    ksPart part(dispatch.Detach());
    ksBodyCollection collection(part.BodyCollection());
    std::vector<BodyInfo> result;
    if (collection.m_lpDispatch == nullptr) {
      return result;
    }
    collection.refresh();
    const long count = collection.GetCount();
    result.reserve(static_cast<std::size_t>(count));
    for (long index = 0; index < count; ++index) {
      ksBody body(collection.GetByIndex(index));
      if (body.m_lpDispatch == nullptr) {
        continue;
      }
      const std::string bodyId = registry_.find_or_register_model_child(
          ObjectKind::kBody, documentId, partId, body.m_lpDispatch);
      result.push_back(
          make_body_info(registry_, bodyId, partId, documentId, body));
    }
    return result;
  });
}

BodyInfo KompasInspectionService::get_body_info(std::string_view bodyId) {
  return cad_call([&] {
    const auto info = registry_.get_info(bodyId);
    if (!info.documentId || !info.partId) {
      throw ProtocolError("object_invalidated",
                          "Body ownership is unavailable.");
    }
    auto dispatch = object_dispatch(registry_, bodyId, ObjectKind::kBody);
    ksBody body(dispatch.Detach());
    return make_body_info(registry_, bodyId, *info.partId, *info.documentId,
                          body);
  });
}

std::vector<FaceInfo> KompasInspectionService::list_body_faces(
    std::string_view bodyId) {
  return cad_call([&] {
    const auto info = registry_.get_info(bodyId);
    if (!info.documentId || !info.partId) {
      throw ProtocolError("object_invalidated",
                          "Body ownership is unavailable.");
    }
    auto dispatch = object_dispatch(registry_, bodyId, ObjectKind::kBody);
    ksBody body(dispatch.Detach());
    ksFaceCollection collection(body.FaceCollection());
    std::vector<FaceInfo> result;
    if (collection.m_lpDispatch == nullptr) {
      return result;
    }
    collection.refresh();
    const long count = collection.GetCount();
    result.reserve(static_cast<std::size_t>(count));
    for (long index = 0; index < count; ++index) {
      ksFaceDefinition face(collection.GetByIndex(index));
      if (face.m_lpDispatch == nullptr) {
        continue;
      }
      const std::string faceId = registry_.find_or_register_model_child(
          ObjectKind::kFace, *info.documentId, *info.partId, face.m_lpDispatch);
      result.push_back(make_face_info(registry_, faceId, *info.partId,
                                      *info.documentId, std::string(bodyId),
                                      face));
    }
    return result;
  });
}

std::vector<FaceInfo> KompasInspectionService::list_part_faces(
    std::string_view partId) {
  std::vector<FaceInfo> result;
  for (const auto& body : list_bodies(partId)) {
    auto faces = list_body_faces(body.bodyId);
    result.insert(result.end(), std::make_move_iterator(faces.begin()),
                  std::make_move_iterator(faces.end()));
  }
  return result;
}

FaceInfo KompasInspectionService::get_face_geometry(std::string_view faceId) {
  return cad_call([&] {
    const auto info = registry_.get_info(faceId);
    if (!info.documentId || !info.partId) {
      throw ProtocolError("object_invalidated",
                          "Face ownership is unavailable.");
    }
    auto dispatch = object_dispatch(registry_, faceId, ObjectKind::kFace);
    ksFaceDefinition face(dispatch.Detach());
    return make_face_info(registry_, faceId, *info.partId, *info.documentId,
                          std::nullopt, face);
  });
}

MassProperties KompasInspectionService::get_mass_properties(
    std::string_view partId) {
  return cad_call([&] {
    auto dispatch = object_dispatch(registry_, partId, ObjectKind::kPart);
    ksPart part(dispatch.Detach());
    ksMassInertiaParam values(part.CalcMassInertiaProperties(0));
    if (values.m_lpDispatch == nullptr) {
      throw ProtocolError("kompas_api_error",
                          "KOMPAS did not calculate mass properties.");
    }
    return MassProperties{
        .massKg = values.GetM() * kGramsToKilograms,
        .volumeMm3 = values.GetV() * kCubicCentimetersToCubicMillimeters,
        .areaMm2 = values.GetF() * kSquareCentimetersToSquareMillimeters,
        .densityKgM3 = part.GetDensity() *
                       kGramsPerCubicCentimeterToKilogramsPerCubicMeter,
        .centerOfMass = centimeters_to_millimeters(
            {values.GetXc(), values.GetYc(), values.GetZc()}),
        .moments = {
            values.GetJx() * kGramSquareCentimetersToKilogramSquareMillimeters,
            values.GetJy() * kGramSquareCentimetersToKilogramSquareMillimeters,
            values.GetJz() * kGramSquareCentimetersToKilogramSquareMillimeters,
            values.GetJxy() * kGramSquareCentimetersToKilogramSquareMillimeters,
            values.GetJxz() * kGramSquareCentimetersToKilogramSquareMillimeters,
            values.GetJyz() *
                kGramSquareCentimetersToKilogramSquareMillimeters}};
  });
}

BoundingBox3D KompasInspectionService::get_bounding_box(
    std::string_view partId) {
  return cad_call([&] {
    auto dispatch = object_dispatch(registry_, partId, ObjectKind::kPart);
    ksPart part(dispatch.Detach());
    BoundingBox3D box;
    if (!part.GetGabarit(TRUE, FALSE, &box.min.x, &box.min.y, &box.min.z,
                         &box.max.x, &box.max.y, &box.max.z)) {
      throw ProtocolError("kompas_api_error",
                          "KOMPAS did not return part bounds.");
    }
    return box;
  });
}

}  // namespace kompas_bridge
