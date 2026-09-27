#pragma once

#include <nlohmann/json.hpp>

#include "kompas_bridge/kompas/application_service.hpp"

namespace kompas_bridge {
/** @brief Обрабатывает запрос состояния приложения. */
class ApplicationHandler {
 public:
  explicit ApplicationHandler(ApplicationService& service);
  [[nodiscard]] nlohmann::json get_status() const;
  [[nodiscard]] nlohmann::json connect(const nlohmann::json& params) const;
  [[nodiscard]] nlohmann::json disconnect() const;

 private:
  ApplicationService& service_;
};
}  // namespace kompas_bridge
