#pragma once

#include <string>

#include <nlohmann/json.hpp>

namespace kompas_bridge {

/** @brief Ошибка bridge с машиночитаемым кодом. */
struct BridgeError {
  std::string code;
  std::string message;
  nlohmann::json details = nlohmann::json::object();
};

}  // namespace kompas_bridge
