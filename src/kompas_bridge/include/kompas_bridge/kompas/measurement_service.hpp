#pragma once

#include <optional>
#include <string_view>

#include "kompas_bridge/kompas/inspection_service.hpp"

namespace kompas_bridge {

struct DistanceMeasurement {
  double distanceMm{};
  Point3D point1;
  Point3D point2;
  std::optional<double> maximumDistanceMm;
  std::optional<Point3D> maximumPoint1;
  std::optional<Point3D> maximumPoint2;
  std::optional<double> normalDistanceMm;
  std::optional<Point3D> normalPoint1;
  std::optional<Point3D> normalPoint2;
};

class MeasurementService {
 public:
  virtual ~MeasurementService() = default;
  [[nodiscard]] virtual DistanceMeasurement distance(
      std::string_view object1Id, std::string_view object2Id) = 0;
  [[nodiscard]] virtual double angle_degrees(
      std::string_view object1Id, std::string_view object2Id) = 0;
};

}  // namespace kompas_bridge
