#pragma once

#include <nlohmann/json.hpp>

#include "kompas_bridge/kompas/document_service.hpp"

namespace kompas_bridge {

/** @brief Валидирует параметры JSON и вызывает документный сервис. */
class DocumentHandler {
 public:
  explicit DocumentHandler(DocumentService& service);
  [[nodiscard]] nlohmann::json list(const nlohmann::json& params) const;
  [[nodiscard]] nlohmann::json get_active(
    const nlohmann::json& params
  ) const;
  [[nodiscard]] nlohmann::json create_3d(
    const nlohmann::json& params
  ) const;
  [[nodiscard]] nlohmann::json open(const nlohmann::json& params) const;
  [[nodiscard]] nlohmann::json activate(const nlohmann::json& params) const;
  [[nodiscard]] nlohmann::json get_top_part(
    const nlohmann::json& params
  ) const;
  [[nodiscard]] nlohmann::json save(const nlohmann::json& params) const;
  [[nodiscard]] nlohmann::json save_as(const nlohmann::json& params) const;
  [[nodiscard]] nlohmann::json close(const nlohmann::json& params) const;

 private:
  DocumentService& service_;
};

}  // namespace kompas_bridge
