#pragma once

#include "kompas_bridge/kompas/application_service.hpp"

namespace kompas_bridge {
class KompasSession;
class ObjectRegistry;

/** @brief Реализация операций приложения через API5. */
class KompasApplicationService final : public ApplicationService {
 public:
  KompasApplicationService(
    KompasSession& session,
    ObjectRegistry& registry
  );
  [[nodiscard]] ApplicationStatus get_status() override;
  [[nodiscard]] ApplicationStatus connect(ConnectionPolicy policy) override;
  void disconnect() override;

 private:
  KompasSession& session_;
  ObjectRegistry& registry_;
};

}  // namespace kompas_bridge
