#pragma once
#include <string_view>
namespace kompas_bridge::methods {
inline constexpr std::string_view kFeatureExtrude = "feature.extrude";
inline constexpr std::string_view kFeatureGetParameters =
    "feature.get_parameters";
inline constexpr std::string_view kFeatureUpdateExtrusion =
    "feature.update_extrusion";
} // namespace kompas_bridge::methods
