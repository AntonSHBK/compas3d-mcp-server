#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace kompas_bridge {
/** @brief Передаёт сообщения без знания об операциях КОМПАСа. */
class Transport {
 public:
  virtual ~Transport() = default;
  [[nodiscard]] virtual std::optional<std::string> read_message() = 0;
  virtual void write_message(std::string_view message) = 0;
};
}  // namespace kompas_bridge
