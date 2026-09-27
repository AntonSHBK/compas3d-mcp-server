#pragma once

#include <string_view>

namespace kompas_bridge {

/** @brief Check a stable version-one wire error code. */
[[nodiscard]] bool is_supported_error_code(std::string_view code);

}  // namespace kompas_bridge
