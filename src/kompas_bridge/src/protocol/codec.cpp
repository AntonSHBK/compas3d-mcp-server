#include "kompas_bridge/protocol/codec.hpp"

#include <utility>

namespace kompas_bridge {

ProtocolError::ProtocolError(
  std::string code,
  std::string message
)
  : std::runtime_error(std::move(message)),
    code_(std::move(code)) {}

const std::string& ProtocolError::code() const noexcept {
  return code_;
}

void validate_utf8(std::string_view payload) {
  for (std::size_t index = 0; index < payload.size();) {
    const auto first = static_cast<unsigned char>(payload[index]);
    std::size_t count = 0;
    std::uint32_t codepoint = 0;
    if (first < 0x80) {
      count = 1;
      codepoint = first;
    } else if (first >= 0xC2 && first <= 0xDF) {
      count = 2;
      codepoint = first & 0x1F;
    } else if (first >= 0xE0 && first <= 0xEF) {
      count = 3;
      codepoint = first & 0x0F;
    } else if (first >= 0xF0 && first <= 0xF4) {
      count = 4;
      codepoint = first & 0x07;
    } else {
      throw ProtocolError(
        "invalid_request",
        "Invalid UTF-8."
      );
    }
    if (index + count > payload.size()) {
      throw ProtocolError(
        "invalid_request",
        "Invalid UTF-8."
      );
    }
    for (std::size_t offset = 1; offset < count; ++offset) {
      const auto next = static_cast<unsigned char>(payload[index + offset]);
      if ((next & 0xC0) != 0x80) {
        throw ProtocolError(
          "invalid_request",
          "Invalid UTF-8."
        );
      }
      codepoint = (codepoint << 6) | (next & 0x3F);
    }
    if ((count == 2 && codepoint < 0x80) ||
        (count == 3 && codepoint < 0x800) ||
        (count == 4 && codepoint < 0x10000) ||
        (codepoint >= 0xD800 && codepoint <= 0xDFFF) ||
        codepoint > 0x10FFFF) {
      throw ProtocolError(
        "invalid_request",
        "Invalid UTF-8."
      );
    }
    index += count;
  }
}

std::vector<std::uint8_t> encode_frame(std::string_view payload) {
  if (payload.empty() || payload.size() > kMaxMessageBytes) {
    throw ProtocolError(
      "invalid_request",
      "Invalid message size."
    );
  }
  validate_utf8(payload);
  const auto size = static_cast<std::uint32_t>(payload.size());
  std::vector<std::uint8_t> frame;
  frame.reserve(4 + size);
  for (int shift = 0; shift < 32; shift += 8) {
    frame.push_back(static_cast<std::uint8_t>(size >> shift));
  }
  frame.insert(
    frame.end(),
    payload.begin(),
    payload.end()
  );
  return frame;
}

std::string decode_frame(std::span<const std::uint8_t> frame) {
  if (frame.size() < 4) {
    throw ProtocolError(
      "invalid_request",
      "Incomplete frame header."
    );
  }
  std::uint32_t size = 0;
  for (int index = 0; index < 4; ++index) {
    size |= static_cast<std::uint32_t>(frame[index]) << (index * 8);
  }
  if (size == 0 || size > kMaxMessageBytes || frame.size() != 4 + size) {
    throw ProtocolError(
      "invalid_request",
      "Invalid frame length."
    );
  }
  std::string payload{
    reinterpret_cast<const char*>(frame.data() + 4),
    size
  };
  validate_utf8(payload);
  return payload;
}

}  // namespace kompas_bridge
