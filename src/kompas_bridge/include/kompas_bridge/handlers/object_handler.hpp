#pragma once

#include <nlohmann/json.hpp>

namespace kompas_bridge {
class ObjectRegistry;

/** @brief Публичные операции над opaque handles. */
class ObjectHandler {
 public:
  explicit ObjectHandler(ObjectRegistry& registry);
  [[nodiscard]] nlohmann::json get_info(const nlohmann::json& params) const;
  [[nodiscard]] nlohmann::json release(const nlohmann::json& params) const;

 private:
  ObjectRegistry& registry_;
};

}  // namespace kompas_bridge
