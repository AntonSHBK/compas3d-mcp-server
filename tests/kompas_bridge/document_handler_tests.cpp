#include "kompas_bridge/handlers/document_handler.hpp"

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "kompas_bridge/protocol/codec.hpp"

namespace {

void check(bool value) {
  if (!value) throw std::runtime_error("Document handler test failed.");
}

class FakeDocuments final : public kompas_bridge::DocumentService {
 public:
  std::vector<kompas_bridge::DocumentInfo> list() override { return documents; }
  std::optional<kompas_bridge::DocumentInfo> get_active() override {
    for (const auto& item : documents) if (item.active) return item;
    return std::nullopt;
  }
  kompas_bridge::DocumentResult create_3d(bool visible) override {
    lastVisible = visible;
    documents.push_back({.documentId = "doc_1", .documentType = 4,
                         .name = "Part", .active = true});
    return {.document = documents.back(), .partId = "part_1", .isNew = true};
  }
  kompas_bridge::DocumentResult open(std::string_view path, bool visible,
                                      bool readOnly) override {
    lastVisible = visible;
    documents.push_back({.documentId = "doc_2", .documentType = 4,
                         .name = "Part2", .filePath = std::string(path),
                         .active = false, .readOnly = readOnly});
    return {.document = documents.back(), .partId = "part_2"};
  }
  kompas_bridge::DocumentInfo activate(std::string_view id) override {
    for (auto& doc : documents) doc.active = doc.documentId == id;
    for (const auto& doc : documents) if (doc.active) return doc;
    throw std::runtime_error("Missing test document.");
  }
  std::string get_top_part(std::string_view id) override {
    return id == "doc_1" ? "part_1" : "part_2";
  }
  kompas_bridge::DocumentInfo save(std::string_view id) override {
    for (auto& doc : documents) if (doc.documentId == id) return doc;
    throw std::runtime_error("Missing test document.");
  }
  kompas_bridge::DocumentInfo save_as(std::string_view id, std::string_view path,
                                       bool overwrite) override {
    lastOverwrite = overwrite;
    for (auto& doc : documents) {
      if (doc.documentId == id) {
        doc.filePath = path;
        return doc;
      }
    }
    throw std::runtime_error("Missing test document.");
  }
  void close(std::string_view id, bool discardChanges) override {
    lastDiscard = discardChanges;
    closed = std::string(id);
  }

  std::vector<kompas_bridge::DocumentInfo> documents;
  bool lastVisible{};
  bool lastOverwrite{};
  bool lastDiscard{};
  std::string closed;
};

template <typename Action>
void invalid(Action&& action) {
  try {
    action();
  } catch (const kompas_bridge::ProtocolError& error) {
    check(error.code() == "invalid_params");
    return;
  }
  throw std::runtime_error("Expected invalid_params.");
}

}  // namespace

int main() {
  FakeDocuments service;
  kompas_bridge::DocumentHandler handler(service);
  const auto created = handler.create_3d({{"visible", true}});
  check(created["document_id"] == "doc_1" && created["part_id"] == "part_1");
  check(created["is_new"] == true && created["file_path"].is_null());
  check(handler.list(nlohmann::json::object())["documents"].size() == 1);
  check(handler.get_active(nlohmann::json::object())["document_id"] == "doc_1");
  const auto opened = handler.open({{"file_path", "D:/test.m3d"},
                                    {"visible", false},
                                    {"read_only", true}});
  check(opened["document_id"] == "doc_2" && opened["is_new"] == false);
  check(!service.lastVisible && opened["read_only"] == true);
  check(handler.activate({{"document_id", "doc_2"}})["active"] == true);
  check(handler.activate({{"document_id", "doc_1"}})["document_id"] == "doc_1");
  check(handler.get_top_part({{"document_id", "doc_1"}})["part_id"] == "part_1");
  check(handler.save({{"document_id", "doc_1"}})["document_id"] == "doc_1");
  check(handler.save_as({{"document_id", "doc_1"},
                         {"file_path", "D:/saved.m3d"},
                         {"overwrite", true}})["file_path"] == "D:/saved.m3d");
  check(service.lastOverwrite);
  check(handler.close({{"document_id", "doc_2"}})["closed"] == true);
  check(service.closed == "doc_2" && !service.lastDiscard);
  invalid([&] { (void)handler.create_3d({{"visible", "yes"}}); });
  invalid([&] { (void)handler.open({{"visible", true}}); });
  invalid([&] {
    (void)handler.close({{"document_id", "doc_1"}, {"unexpected", 1}});
  });
}
