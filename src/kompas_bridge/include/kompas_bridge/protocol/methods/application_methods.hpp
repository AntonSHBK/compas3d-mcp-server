#pragma once

#include <string_view>

namespace kompas_bridge::methods {

inline constexpr std::string_view kApplicationStatus = "application.status";
inline constexpr std::string_view kApplicationConnect = "application.connect";
inline constexpr std::string_view kApplicationDisconnect =
  "application.disconnect";

}  // namespace kompas_bridge::methods
