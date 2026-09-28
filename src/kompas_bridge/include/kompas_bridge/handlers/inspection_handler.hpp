#pragma once

#include <nlohmann/json.hpp>

namespace kompas_bridge {

class InspectionService;

class InspectionHandler {
 public:
  explicit InspectionHandler(InspectionService& service);
  [[nodiscard]] nlohmann::json list_bodies(const nlohmann::json& params) const;
  [[nodiscard]] nlohmann::json get_body_info(
      const nlohmann::json& params) const;
  [[nodiscard]] nlohmann::json list_body_faces(
      const nlohmann::json& params) const;
  [[nodiscard]] nlohmann::json list_part_faces(
      const nlohmann::json& params) const;
  [[nodiscard]] nlohmann::json get_face_geometry(
      const nlohmann::json& params) const;
  [[nodiscard]] nlohmann::json get_mass_properties(
      const nlohmann::json& params) const;
  [[nodiscard]] nlohmann::json get_bounding_box(
      const nlohmann::json& params) const;

 private:
  InspectionService& service_;
};

}  // namespace kompas_bridge
