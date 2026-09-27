#include "kompas_bridge/transport/stdio_transport.hpp"

#include <iostream>
#include <stdexcept>

#include "kompas_bridge/protocol/codec.hpp"

namespace kompas_bridge {

std::optional<std::string> StdioTransport::read_message() {
  std::string line;
  bool overflow = false;
  while (true) {
    const int character = std::cin.get();
    if (character == std::char_traits<char>::eof()) {
      if (std::cin.bad()) {
        throw std::runtime_error("Failed to read a message from stdin.");
      }
      if (line.empty() && !overflow) {
        return std::nullopt;
      }
      break;
    }
    if (character == '\n') {
      break;
    }
    if (line.size() < kMaxMessageBytes) {
      line.push_back(static_cast<char>(character));
    } else {
      overflow = true;
    }
  }
  if (overflow) {
    throw ProtocolError(
      "invalid_request",
      "Stdio message exceeds size limit."
    );
  }
  if (!line.empty() && line.back() == '\r') {
    line.pop_back();
  }
  return line;
}

void StdioTransport::write_message(std::string_view message) {
  std::cout << message << '\n' << std::flush;
  if (!std::cout) {
    throw std::runtime_error("Failed to write a message to stdout.");
  }
}

}  // namespace kompas_bridge
