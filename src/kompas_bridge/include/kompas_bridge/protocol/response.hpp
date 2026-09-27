#pragma once

#include <optional>
#include <string>

#include <nlohmann/json.hpp>

#include "kompas_bridge/protocol/bridge_error.hpp"

namespace kompas_bridge {

/** @brief Метаданные ответа внутреннего протокола. */
struct Response {
  int protocolVersion{1};
  std::optional<std::string> id;
  bool ok{};
  std::optional<nlohmann::json> result;
  std::optional<BridgeError> error;
};

[[nodiscard]] Response parse_response(const std::string& payload);
[[nodiscard]] std::string serialize_response(const Response& response);

}  // namespace kompas_bridge
