#include "kompas_bridge/handlers/measurement_handler.hpp"

#include <string>

#include "kompas_bridge/kompas/measurement_service.hpp"
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

nlohmann::json point_json(const Point3D& point) {
  return {{"x", point.x}, {"y", point.y}, {"z", point.z}};
}

nlohmann::json optional_point_json(const std::optional<Point3D>& point) {
  return point ? point_json(*point) : nlohmann::json(nullptr);
}

}  // namespace

MeasurementHandler::MeasurementHandler(MeasurementService& service)
    : service_(service) {}

nlohmann::json MeasurementHandler::distance(
    const nlohmann::json& params) const {
  const auto result = service_.distance(required_string(params, "object1_id"),
                                        required_string(params, "object2_id"));
  return {{"distance_mm", result.distanceMm},
          {"point1", point_json(result.point1)},
          {"point2", point_json(result.point2)},
          {"maximum_distance_mm", result.maximumDistanceMm},
          {"maximum_point1", optional_point_json(result.maximumPoint1)},
          {"maximum_point2", optional_point_json(result.maximumPoint2)},
          {"normal_distance_mm", result.normalDistanceMm},
          {"normal_point1", optional_point_json(result.normalPoint1)},
          {"normal_point2", optional_point_json(result.normalPoint2)}};
}

nlohmann::json MeasurementHandler::angle(const nlohmann::json& params) const {
  return {{"angle_degrees",
           service_.angle_degrees(required_string(params, "object1_id"),
                                  required_string(params, "object2_id"))}};
}

}  // namespace kompas_bridge
