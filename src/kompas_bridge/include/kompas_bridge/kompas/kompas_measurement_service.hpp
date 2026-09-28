#pragma once

#include "kompas_bridge/kompas/measurement_service.hpp"

namespace kompas_bridge {

class ObjectRegistry;

class KompasMeasurementService final : public MeasurementService {
 public:
  explicit KompasMeasurementService(ObjectRegistry& registry);
  [[nodiscard]] DistanceMeasurement distance(
      std::string_view object1Id, std::string_view object2Id) override;
  [[nodiscard]] double angle_degrees(
      std::string_view object1Id, std::string_view object2Id) override;

 private:
  ObjectRegistry& registry_;
};

}  // namespace kompas_bridge
