#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace kompas_bridge {

inline constexpr int kProtocolVersion = 1;
inline constexpr std::size_t kMaxMessageBytes = 1024 * 1024;
inline constexpr std::size_t kMaxIdBytes = 128;

/** @brief Validation failure with a stable protocol error code. */
class ProtocolError : public std::runtime_error {
 public:
  ProtocolError(
    std::string code,
    std::string message
  );
  [[nodiscard]] const std::string& code() const noexcept;

 private:
  std::string code_;
};

/** @brief Add four-byte little-endian byte length to a JSON payload. */
[[nodiscard]] std::vector<std::uint8_t> encode_frame(std::string_view payload);
/** @brief Decode exactly one complete frame. */
[[nodiscard]] std::string decode_frame(std::span<const std::uint8_t> frame);
/** @brief Reject malformed UTF-8 byte sequences. */
void validate_utf8(std::string_view payload);

}  // namespace kompas_bridge
