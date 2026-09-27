#pragma once

#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

#include <wrl/client.h>

struct IUnknown;

namespace kompas_bridge {

/** @brief Тип зарегистрированного CAD-объекта. */
enum class ObjectKind { kDocument, kPart, kSketch, kFeature };

/** @brief Публичные метаданные без COM-указателей. */
struct ObjectInfo {
  std::string handle;
  ObjectKind kind{};
  std::optional<std::string> documentId;
};

/** @brief Владеет COM-ссылками и opaque handles одного bridge-процесса. */
class ObjectRegistry {
 public:
  ObjectRegistry();
  ObjectRegistry(const ObjectRegistry&) = delete;
  ObjectRegistry& operator=(const ObjectRegistry&) = delete;

  /** @brief Регистрирует документ, сохраняя собственную COM-ссылку. */
  [[nodiscard]] std::string register_document(IUnknown* object);
  [[nodiscard]] std::string find_or_register_document(IUnknown* object);
  /** @brief Регистрирует объект, принадлежащий указанному документу. */
  [[nodiscard]] std::string register_child(
    ObjectKind kind,
    std::string_view documentId,
    IUnknown* object
  );
  [[nodiscard]] std::string find_or_register_child(
    ObjectKind kind,
    std::string_view documentId,
    IUnknown* object
  );
  [[nodiscard]] std::vector<std::string> document_handles() const;
  /** @brief Возвращает метаданные или стабильную ошибку handle. */
  [[nodiscard]] ObjectInfo get_info(std::string_view handle) const;
  /** @brief Даёт временную COM-ссылку внутреннему сервису. */
  [[nodiscard]] Microsoft::WRL::ComPtr<IUnknown> get_object(
    std::string_view handle,
    ObjectKind expectedKind
  ) const;
  /** @brief Освобождает ссылку и инвалидирует потомков документа. */
  void release(std::string_view handle);
  /** @brief Вызывается при закрытии документа в DocumentService. */
  void invalidate_document(std::string_view documentId);
  /** @brief Освобождает все ссылки при смене COM-подключения. */
  void invalidate_all();

 private:
  struct Entry {
    ObjectInfo info;
    Microsoft::WRL::ComPtr<IUnknown> object;
    bool invalidated{};
  };

  void check_thread() const;
  [[nodiscard]] Entry& find_entry(std::string_view handle);
  [[nodiscard]] const Entry& find_entry(std::string_view handle) const;
  void invalidate_entry(Entry& entry);
  void prune_tombstones();
  [[nodiscard]] std::string next_handle(ObjectKind kind);

  std::thread::id ownerThread_;
  std::string sessionId_;
  std::uint64_t nextId_{1};
  std::unordered_map<std::string, Entry> entries_;
  std::deque<std::string> tombstones_;
};

/** @brief Публичное имя типа объекта. */
[[nodiscard]] std::string_view object_kind_name(ObjectKind kind);

}  // namespace kompas_bridge
