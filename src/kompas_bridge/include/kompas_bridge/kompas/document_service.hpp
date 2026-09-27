#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace kompas_bridge {

struct DocumentInfo {
  std::string documentId;
  long documentType{};
  std::string name;
  std::optional<std::string> filePath;
  bool active{};
  bool changed{};
  bool readOnly{};
};

struct DocumentResult {
  DocumentInfo document;
  std::optional<std::string> partId;
  bool isNew{};
};

/** @brief Граница документных операций для handler и COM-реализации. */
class DocumentService {
 public:
  virtual ~DocumentService() = default;
  [[nodiscard]] virtual std::vector<DocumentInfo> list() = 0;
  [[nodiscard]] virtual std::optional<DocumentInfo> get_active() = 0;
  [[nodiscard]] virtual DocumentResult create_3d(bool visible) = 0;
  [[nodiscard]] virtual DocumentResult open(
    std::string_view path,
    bool visible,
    bool readOnly
  ) = 0;
  [[nodiscard]] virtual DocumentInfo activate(std::string_view documentId) = 0;
  [[nodiscard]] virtual std::string get_top_part(std::string_view documentId) = 0;
  [[nodiscard]] virtual DocumentInfo save(std::string_view documentId) = 0;
  [[nodiscard]] virtual DocumentInfo save_as(
    std::string_view documentId,
    std::string_view path,
    bool overwrite
  ) = 0;
  virtual void close(
    std::string_view documentId,
    bool discardChanges
  ) = 0;
};

}  // namespace kompas_bridge
