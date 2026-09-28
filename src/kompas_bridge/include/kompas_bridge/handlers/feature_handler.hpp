#pragma once
#include <nlohmann/json.hpp>
namespace kompas_bridge {
class FeatureService;
class FeatureHandler {
public:
  explicit FeatureHandler(FeatureService &service);
  [[nodiscard]] nlohmann::json extrude(const nlohmann::json &params) const;
  [[nodiscard]] nlohmann::json
  get_parameters(const nlohmann::json &params) const;
  [[nodiscard]] nlohmann::json
  update_extrusion(const nlohmann::json &params) const;
  [[nodiscard]] nlohmann::json
  list_features(const nlohmann::json &params) const;
  [[nodiscard]] nlohmann::json get_info(const nlohmann::json &params) const;
  [[nodiscard]] nlohmann::json rebuild(const nlohmann::json &params) const;

private:
  FeatureService &service_;
};
} // namespace kompas_bridge
