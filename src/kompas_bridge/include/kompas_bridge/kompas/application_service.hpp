#pragma once

#include <optional>

#include "kompas_bridge/kompas/connection_policy.hpp"

namespace kompas_bridge {

/** @brief Четыре числовые части версии КОМПАСа. */
struct KompasVersion {
  long major{};
  long minor{};
  long release{};
  long build{};
};

/** @brief Снимок состояния подключения. */
struct ApplicationStatus {
  bool connected{};
  std::optional<bool> visible;
  std::optional<KompasVersion> kompasVersion;
};

/** @brief Граница между обработчиком и COM-реализацией. */
class ApplicationService {
 public:
  virtual ~ApplicationService() = default;
  [[nodiscard]] virtual ApplicationStatus get_status() = 0;
  [[nodiscard]] virtual ApplicationStatus connect(ConnectionPolicy policy) = 0;
  virtual void disconnect() = 0;
};

}  // namespace kompas_bridge
