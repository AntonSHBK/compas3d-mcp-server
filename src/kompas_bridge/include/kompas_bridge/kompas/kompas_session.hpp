#pragma once

#include <memory>

#include "kompas_bridge/kompas/connection_policy.hpp"

class KompasObject;
namespace kompas_bridge {
class KompasSession {
 public:
  KompasSession();
  ~KompasSession();
  KompasSession(const KompasSession&) = delete;
  KompasSession& operator=(const KompasSession&) = delete;
  KompasSession(KompasSession&&) = delete;
  KompasSession& operator=(KompasSession&&) = delete;
  /** @brief Присоединяет приложение согласно явной политике. */
  void connect(ConnectionPolicy policy);
  /** @brief Освобождает COM-ссылку, не закрывая КОМПАС. */
  void disconnect() noexcept;
  [[nodiscard]] bool is_connected() const noexcept;
  [[nodiscard]] KompasObject& kompas_object() const;

 private:
  class Implementation;
  std::unique_ptr<Implementation> implementation_;
};
}  // namespace kompas_bridge
