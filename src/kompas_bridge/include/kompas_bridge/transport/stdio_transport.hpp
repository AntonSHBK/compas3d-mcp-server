#pragma once

#include "kompas_bridge/transport/transport.hpp"

namespace kompas_bridge {
/** @brief Временный транспорт для разработки и ручной проверки. */
class StdioTransport final : public Transport {
 public:
  [[nodiscard]] std::optional<std::string> read_message() override;
  void write_message(std::string_view message) override;
};
}  // namespace kompas_bridge
