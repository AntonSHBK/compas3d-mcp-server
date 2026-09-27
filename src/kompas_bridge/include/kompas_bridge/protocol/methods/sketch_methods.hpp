#pragma once
#include <string_view>
namespace kompas_bridge::methods {
inline constexpr std::string_view kSketchCreate = "sketch.create";
inline constexpr std::string_view kSketchBeginEdit = "sketch.begin_edit";
inline constexpr std::string_view kSketchEndEdit = "sketch.end_edit";
inline constexpr std::string_view kSketchAddLine = "sketch.add_line";
inline constexpr std::string_view kSketchAddCircle = "sketch.add_circle";
} // namespace kompas_bridge::methods
