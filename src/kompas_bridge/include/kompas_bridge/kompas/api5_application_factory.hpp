#pragma once

#include <stdexcept>

#include "kompas_bridge/kompas/connection_policy.hpp"

struct IDispatch;
namespace kompas_bridge {
/** @brief В таблице активных объектов нет работающего КОМПАСа. */
class KompasNotRunningError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

class Api5ApplicationFactory {
 public:
  /** @brief Возвращает COM-ссылку с владением вызывающей стороны. */
  [[nodiscard]] static IDispatch* CreateApplication(ConnectionPolicy policy);
};
}  // namespace kompas_bridge
