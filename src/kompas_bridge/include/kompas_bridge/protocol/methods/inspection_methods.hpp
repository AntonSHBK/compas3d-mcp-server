#pragma once

#include <string_view>

namespace kompas_bridge::methods {
inline constexpr std::string_view kPartListBodies = "part.list_bodies";
inline constexpr std::string_view kBodyGetInfo = "body.get_info";
inline constexpr std::string_view kBodyListFaces = "body.list_faces";
inline constexpr std::string_view kPartListFaces = "part.list_faces";
inline constexpr std::string_view kFaceGetGeometry = "face.get_geometry";
inline constexpr std::string_view kPartGetMassProperties =
    "part.get_mass_properties";
inline constexpr std::string_view kPartGetBoundingBox =
    "part.get_bounding_box";
}  // namespace kompas_bridge::methods
