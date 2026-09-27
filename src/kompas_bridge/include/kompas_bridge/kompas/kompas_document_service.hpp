#pragma once

#include <wrl/client.h>

#include "kompas_bridge/kompas/document_service.hpp"

struct IDispatch;

namespace kompas_bridge {
class KompasSession;
class ObjectRegistry;

/** @brief Реализация документных операций через API7 в STA bridge. */
class KompasDocumentService final : public DocumentService {
 public:
  KompasDocumentService(
    KompasSession& session,
    ObjectRegistry& registry
  );
  [[nodiscard]] std::vector<DocumentInfo> list() override;
  [[nodiscard]] std::optional<DocumentInfo> get_active() override;
  [[nodiscard]] DocumentResult create_3d(bool visible) override;
  [[nodiscard]] DocumentResult open(
    std::string_view path,
    bool visible,
    bool readOnly
  ) override;
  [[nodiscard]] DocumentInfo activate(std::string_view documentId) override;
  [[nodiscard]] std::string get_top_part(std::string_view documentId) override;
  [[nodiscard]] DocumentInfo save(std::string_view documentId) override;
  [[nodiscard]] DocumentInfo save_as(
    std::string_view documentId,
    std::string_view path,
    bool overwrite
  ) override;
  void close(
    std::string_view documentId,
    bool discardChanges
  ) override;

 private:
  void require_connected() const;
  void sync_documents();
  [[nodiscard]] DocumentInfo info(IDispatch* dispatch);
  [[nodiscard]] DocumentResult result(
    IDispatch* dispatch,
    bool isNew
  );
  [[nodiscard]] Microsoft::WRL::ComPtr<IDispatch> document(std::string_view id);

  KompasSession& session_;
  ObjectRegistry& registry_;
};
}  // namespace kompas_bridge
