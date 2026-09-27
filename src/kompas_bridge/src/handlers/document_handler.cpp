#include "kompas_bridge/handlers/document_handler.hpp"

#include <initializer_list>
#include <string>
#include <string_view>

#include "kompas_bridge/protocol/codec.hpp"

namespace kompas_bridge {

namespace {

void only_keys(
  const nlohmann::json& params,
  std::initializer_list<std::string_view> keys
) {
  for (auto it = params.begin(); it != params.end(); ++it) {
    bool allowed = false;
    for (std::string_view key : keys) {
      allowed |= it.key() == key;
    }
    if (!allowed) {
      throw ProtocolError(
        "invalid_params",
        "Unknown document parameter."
      );
    }
  }
}

std::string required_string(
  const nlohmann::json& params,
  std::string_view key
) {
  const std::string name(key);
  if (!params.contains(name) || !params[name].is_string() ||
      params[name].get<std::string>().empty()) {
    throw ProtocolError(
      "invalid_params",
      "Required nonempty string parameter is missing: " + name
    );
  }
  return params[name].get<std::string>();
}

bool optional_bool(
  const nlohmann::json& params,
  std::string_view key,
  bool defaultValue
) {
  const std::string name(key);
  if (!params.contains(name)) {
    return defaultValue;
  }
  if (!params[name].is_boolean()) {
    throw ProtocolError(
      "invalid_params",
      "Expected boolean parameter: " + name
    );
  }
  return params[name].get<bool>();
}

nlohmann::json to_json(const DocumentInfo& info) {
  return {
    {"document_id", info.documentId},
    {"document_type", info.documentType},
    {"name", info.name},
    {"file_path", info.filePath},
    {"active", info.active},
    {"changed", info.changed},
    {"read_only", info.readOnly}
  };
}

nlohmann::json to_json(const DocumentResult& result) {
  nlohmann::json data = to_json(result.document);
  data["part_id"] = result.partId;
  data["is_new"] = result.isNew;
  return data;
}

}  // namespace

DocumentHandler::DocumentHandler(DocumentService& service)
  : service_(service) {}

nlohmann::json DocumentHandler::handle(
  std::string_view method,
  const nlohmann::json& params
) const {
  if (method == "document.list") {
    only_keys(
      params,
      {}
    );
    nlohmann::json documents = nlohmann::json::array();
    for (const auto& document : service_.list()) {
      documents.push_back(to_json(document));
    }
    return {{"documents", std::move(documents)}};
  }
  if (method == "document.get_active") {
    only_keys(
      params,
      {}
    );
    const auto document = service_.get_active();
    if (!document) {
      throw ProtocolError(
        "no_active_document",
        "No active document is available."
      );
    }
    return to_json(*document);
  }
  if (method == "document.create_3d") {
    only_keys(
      params,
      {"visible"}
    );
    const bool visible = optional_bool(
      params,
      "visible",
      true
    );
    return to_json(service_.create_3d(visible));
  }
  if (method == "document.open") {
    only_keys(
      params,
      {"file_path", "visible", "read_only"}
    );
    const std::string filePath = required_string(
      params,
      "file_path"
    );
    const bool visible = optional_bool(
      params,
      "visible",
      true
    );
    const bool readOnly = optional_bool(
      params,
      "read_only",
      false
    );
    const DocumentResult result = service_.open(
      filePath,
      visible,
      readOnly
    );
    return to_json(result);
  }
  if (method == "document.activate" || method == "document.get_top_part" ||
      method == "document.save" || method == "document.save_as" ||
      method == "document.close") {
    if (method == "document.save_as") {
      only_keys(
        params,
        {"document_id", "file_path", "overwrite"}
      );
      const std::string documentId = required_string(
        params,
        "document_id"
      );
      const std::string filePath = required_string(
        params,
        "file_path"
      );
      const bool overwrite = optional_bool(
        params,
        "overwrite",
        false
      );
      const DocumentInfo result = service_.save_as(
        documentId,
        filePath,
        overwrite
      );
      return to_json(result);
    }
    if (method == "document.close") {
      only_keys(
        params,
        {"document_id", "discard_changes"}
      );
      const std::string id = required_string(
        params,
        "document_id"
      );
      const bool discardChanges = optional_bool(
        params,
        "discard_changes",
        false
      );
      service_.close(
        id,
        discardChanges
      );
      return {
        {"document_id", id},
        {"closed", true}
      };
    }
    only_keys(
      params,
      {"document_id"}
    );
    const std::string id = required_string(
      params,
      "document_id"
    );
    if (method == "document.activate") {
      return to_json(service_.activate(id));
    }
    if (method == "document.get_top_part") {
      return {
        {"document_id", id},
        {"part_id", service_.get_top_part(id)}
      };
    }
    return to_json(service_.save(id));
  }
  throw ProtocolError(
    "unknown_method",
    "Document method is not implemented."
  );
}

}  // namespace kompas_bridge
