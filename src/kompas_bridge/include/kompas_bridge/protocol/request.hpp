#pragma once

#include <string>

#include <nlohmann/json.hpp>

namespace kompas_bridge {

/** @brief Метаданные запроса внутреннего протокола. */
struct Request {
  int protocolVersion{1};
  std::string id;
  std::string method;
  nlohmann::json params = nlohmann::json::object();
};

[[nodiscard]] Request parse_request(const std::string& payload);
[[nodiscard]] std::string serialize_request(const Request& request);

}  // namespace kompas_bridge
