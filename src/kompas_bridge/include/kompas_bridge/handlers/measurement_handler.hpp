#pragma once

#include <nlohmann/json.hpp>

namespace kompas_bridge {

class MeasurementService;

class MeasurementHandler {
 public:
  explicit MeasurementHandler(MeasurementService& service);
  [[nodiscard]] nlohmann::json distance(const nlohmann::json& params) const;
  [[nodiscard]] nlohmann::json angle(const nlohmann::json& params) const;

 private:
  MeasurementService& service_;
};

}  // namespace kompas_bridge
