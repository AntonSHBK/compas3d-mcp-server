#pragma once
#include <nlohmann/json.hpp>
namespace kompas_bridge {
class SketchService;
class SketchHandler {
public:
  explicit SketchHandler(SketchService &service);
  [[nodiscard]] nlohmann::json create(const nlohmann::json &params) const;
  [[nodiscard]] nlohmann::json begin_edit(const nlohmann::json &params) const;
  [[nodiscard]] nlohmann::json end_edit(const nlohmann::json &params) const;
  [[nodiscard]] nlohmann::json add_line(const nlohmann::json &params) const;
  [[nodiscard]] nlohmann::json add_circle(const nlohmann::json &params) const;

private:
  SketchService &service_;
};
} // namespace kompas_bridge
