#pragma once
#include <string_view>
namespace kompas_bridge::methods {
inline constexpr std::string_view kFeatureExtrude = "feature.extrude";
inline constexpr std::string_view kFeatureGetParameters =
    "feature.get_parameters";
inline constexpr std::string_view kFeatureUpdateExtrusion =
    "feature.update_extrusion";
inline constexpr std::string_view kPartListFeatures = "part.list_features";
inline constexpr std::string_view kFeatureGetInfo = "feature.get_info";
} // namespace kompas_bridge::methods
