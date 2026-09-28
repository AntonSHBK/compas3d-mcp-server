#include "kompas_bridge/handlers/measurement_handler.hpp"

#include <stdexcept>
#include <string_view>

#include "kompas_bridge/kompas/measurement_service.hpp"

namespace {

void check(bool condition) {
  if (!condition) {
    throw std::runtime_error("Measurement handler test failed.");
  }
}

class FakeMeasurementService final : public kompas_bridge::MeasurementService {
 public:
  kompas_bridge::DistanceMeasurement distance(
      std::string_view, std::string_view) override {
    return {.distanceMm = 50.0,
            .point1 = {0, 0, 0},
            .point2 = {0, 0, 50},
            .maximumDistanceMm = 70.0,
            .maximumPoint1 = kompas_bridge::Point3D{-25, -25, 0},
            .maximumPoint2 = kompas_bridge::Point3D{25, 25, 50},
            .normalDistanceMm = 50.0,
            .normalPoint1 = kompas_bridge::Point3D{0, 0, 0},
            .normalPoint2 = kompas_bridge::Point3D{0, 0, 50}};
  }

  double angle_degrees(std::string_view, std::string_view) override {
    return 90.0;
  }
};

}  // namespace

int main() {
  FakeMeasurementService service;
  kompas_bridge::MeasurementHandler handler(service);
  const auto params =
      nlohmann::json{{"object1_id", "face_1"}, {"object2_id", "face_2"}};

  const auto distance = handler.distance(params);
  check(distance["distance_mm"] == 50.0);
  check(distance["normal_point2"]["z"] == 50.0);

  const auto angle = handler.angle(params);
  check(angle["angle_degrees"] == 90.0);
}
