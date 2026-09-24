#include "kompas_bridge/common/bridge_error.hpp"

#include <format>
#include <string_view>

namespace kompas_bridge {
namespace {
std::string EscapeJson(std::string_view value) {
  std::string escaped;
  escaped.reserve(value.size());
  for (const unsigned char character : value) {
    switch (character) {
      case '"':
        escaped += "\\\"";
        break;
      case '\\':
        escaped += "\\\\";
        break;
      case '\b':
        escaped += "\\b";
        break;
      case '\f':
        escaped += "\\f";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        if (character < 0x20)
          escaped += std::format("\\u{:04x}", character);
        else
          escaped += static_cast<char>(character);
    }
  }
  return escaped;
}
}  // namespace
std::string BridgeError::ToJson() const {
  return R"({"ok":false,"error":{"code":")" + EscapeJson(code) + R"(","message":")" +
         EscapeJson(message) + R"("}})";
}
}  // namespace kompas_bridge
