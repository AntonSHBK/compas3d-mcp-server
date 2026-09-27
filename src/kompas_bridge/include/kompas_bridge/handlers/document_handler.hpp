#pragma once

#include <string_view>

#include <nlohmann/json.hpp>

#include "kompas_bridge/kompas/document_service.hpp"

namespace kompas_bridge {

/** @brief Валидирует параметры JSON и вызывает документный сервис. */
class DocumentHandler {
 public:
  explicit DocumentHandler(DocumentService& service);
  [[nodiscard]] nlohmann::json handle(
    std::string_view method,
    const nlohmann::json& params
  ) const;

 private:
  DocumentService& service_;
};

}  // namespace kompas_bridge
