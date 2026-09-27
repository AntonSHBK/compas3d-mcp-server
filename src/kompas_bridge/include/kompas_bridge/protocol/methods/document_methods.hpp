#pragma once

#include <string_view>

namespace kompas_bridge::methods {

inline constexpr std::string_view kDocumentList = "document.list";
inline constexpr std::string_view kDocumentGetActive = "document.get_active";
inline constexpr std::string_view kDocumentCreate3d = "document.create_3d";
inline constexpr std::string_view kDocumentOpen = "document.open";
inline constexpr std::string_view kDocumentActivate = "document.activate";
inline constexpr std::string_view kDocumentGetTopPart =
  "document.get_top_part";
inline constexpr std::string_view kDocumentSave = "document.save";
inline constexpr std::string_view kDocumentSaveAs = "document.save_as";
inline constexpr std::string_view kDocumentClose = "document.close";

}  // namespace kompas_bridge::methods
