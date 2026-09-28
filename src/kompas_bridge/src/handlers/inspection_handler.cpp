#include "kompas_bridge/handlers/inspection_handler.hpp"

#include <string>

#include "kompas_bridge/kompas/inspection_service.hpp"
#include "kompas_bridge/protocol/codec.hpp"

namespace kompas_bridge {
namespace {

std::string required_string(const nlohmann::json& params, const char* key) {
  if (!params.contains(key) || !params[key].is_string() ||
      params[key].get<std::string>().empty()) {
    throw ProtocolError(
        "invalid_params",
        std::string("Required string parameter is missing: ") + key);
  }
  return params[key].get<std::string>();
}

nlohmann::json to_json(const Point3D& point) {
  return {{"x", point.x}, {"y", point.y}, {"z", point.z}};
}

nlohmann::json to_json(const BoundingBox3D& box) {
  return {{"min", to_json(box.min)}, {"max", to_json(box.max)}};
}

nlohmann::json to_json(const BodyInfo& body) {
  return {{"body_id", body.bodyId},
          {"part_id", body.partId},
          {"solid", body.solid},
          {"bounding_box", to_json(body.boundingBox)},
          {"owner_feature_id", body.ownerFeatureId
                                   ? nlohmann::json(*body.ownerFeatureId)
                                   : nlohmann::json(nullptr)}};
}

nlohmann::json to_json(const FaceInfo& face) {
  return {
      {"face_id", face.faceId},
      {"part_id", face.partId},
      {"body_id",
       face.bodyId ? nlohmann::json(*face.bodyId) : nlohmann::json(nullptr)},
      {"surface_type", face.surfaceType},
      {"area_mm2", face.areaMm2},
      {"normal", face.normal ? to_json(*face.normal) : nlohmann::json(nullptr)},
      {"bounding_box", to_json(face.boundingBox)},
      {"radius_mm", face.radiusMm ? nlohmann::json(*face.radiusMm)
                                  : nlohmann::json(nullptr)},
      {"owner_feature_id", face.ownerFeatureId
                               ? nlohmann::json(*face.ownerFeatureId)
                               : nlohmann::json(nullptr)}};
}

nlohmann::json faces_json(const std::vector<FaceInfo>& faces) {
  nlohmann::json result = nlohmann::json::array();
  for (const auto& face : faces) {
    result.push_back(to_json(face));
  }
  return result;
}

}  // namespace

InspectionHandler::InspectionHandler(InspectionService& service)
    : service_(service) {}

nlohmann::json InspectionHandler::list_bodies(
    const nlohmann::json& params) const {
  const std::string partId = required_string(params, "part_id");
  nlohmann::json bodies = nlohmann::json::array();
  for (const auto& body : service_.list_bodies(partId)) {
    bodies.push_back(to_json(body));
  }
  return {{"part_id", partId}, {"bodies", std::move(bodies)}};
}

nlohmann::json InspectionHandler::get_body_info(
    const nlohmann::json& params) const {
  return to_json(service_.get_body_info(required_string(params, "body_id")));
}

nlohmann::json InspectionHandler::list_body_faces(
    const nlohmann::json& params) const {
  const std::string bodyId = required_string(params, "body_id");
  return {{"body_id", bodyId},
          {"faces", faces_json(service_.list_body_faces(bodyId))}};
}

nlohmann::json InspectionHandler::list_part_faces(
    const nlohmann::json& params) const {
  const std::string partId = required_string(params, "part_id");
  return {{"part_id", partId},
          {"faces", faces_json(service_.list_part_faces(partId))}};
}

nlohmann::json InspectionHandler::get_face_geometry(
    const nlohmann::json& params) const {
  return to_json(
      service_.get_face_geometry(required_string(params, "face_id")));
}

nlohmann::json InspectionHandler::get_mass_properties(
    const nlohmann::json& params) const {
  const MassProperties values =
      service_.get_mass_properties(required_string(params, "part_id"));
  return {{"mass_kg", values.massKg},
          {"volume_mm3", values.volumeMm3},
          {"area_mm2", values.areaMm2},
          {"density_kg_m3", values.densityKgM3},
          {"center_of_mass", to_json(values.centerOfMass)},
          {"moments_of_inertia",
           {{"jx", values.moments.jx},
            {"jy", values.moments.jy},
            {"jz", values.moments.jz},
            {"jxy", values.moments.jxy},
            {"jxz", values.moments.jxz},
            {"jyz", values.moments.jyz}}}};
}

nlohmann::json InspectionHandler::get_bounding_box(
    const nlohmann::json& params) const {
  return to_json(
      service_.get_bounding_box(required_string(params, "part_id")));
}

}  // namespace kompas_bridge
