#pragma once

#include "kompas_bridge/kompas/inspection_service.hpp"

namespace kompas_bridge {

class ObjectRegistry;

class KompasInspectionService final : public InspectionService {
 public:
  explicit KompasInspectionService(ObjectRegistry& registry);

  [[nodiscard]] std::vector<BodyInfo> list_bodies(
      std::string_view partId) override;
  [[nodiscard]] BodyInfo get_body_info(std::string_view bodyId) override;
  [[nodiscard]] std::vector<FaceInfo> list_body_faces(
      std::string_view bodyId) override;
  [[nodiscard]] std::vector<FaceInfo> list_part_faces(
      std::string_view partId) override;
  [[nodiscard]] FaceInfo get_face_geometry(std::string_view faceId) override;
  [[nodiscard]] MassProperties get_mass_properties(
      std::string_view partId) override;
  [[nodiscard]] BoundingBox3D get_bounding_box(
      std::string_view partId) override;

 private:
  ObjectRegistry& registry_;
};

}  // namespace kompas_bridge
