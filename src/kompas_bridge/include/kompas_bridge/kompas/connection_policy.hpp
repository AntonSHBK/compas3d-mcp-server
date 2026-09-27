#pragma once

namespace kompas_bridge {

/** @brief Политика получения COM-объекта приложения. */
enum class ConnectionPolicy { kAttachOnly, kAttachOrStart, kStartNew };

}  // namespace kompas_bridge
