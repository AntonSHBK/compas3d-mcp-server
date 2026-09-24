#pragma once

#include <string>
namespace kompas_bridge {
struct BridgeError {
  std::string code;
  std::string message;
  [[nodiscard]] std::string ToJson() const;
};
}  // namespace kompas_bridge
