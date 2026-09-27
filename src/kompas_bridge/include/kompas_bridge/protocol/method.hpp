#pragma once

#include <string_view>

namespace kompas_bridge {
/** @brief Методы первого вертикального среза протокола. */
enum class Method { kApplicationStatus, kDocumentList, kDocumentGetActive };
[[nodiscard]] bool is_supported_method(std::string_view method);
}  // namespace kompas_bridge
