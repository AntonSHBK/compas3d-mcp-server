#include "kompas_bridge/handlers/document_handler.hpp"

#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>

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

nlohmann::json DocumentHandler::list(const nlohmann::json& params) const {
  only_keys(params, {});
  nlohmann::json documents = nlohmann::json::array();
  for (const auto& document : service_.list()) {
    documents.push_back(to_json(document));
  }
  return {{"documents", std::move(documents)}};
}

nlohmann::json DocumentHandler::get_active(
  const nlohmann::json& params
) const {
  only_keys(params, {});
  const auto document = service_.get_active();
  if (!document) {
    throw ProtocolError(
      "no_active_document",
      "No active document is available."
    );
  }
  return to_json(*document);
}

nlohmann::json DocumentHandler::create_3d(
  const nlohmann::json& params
) const {
  only_keys(params, {"visible"});
  return to_json(service_.create_3d(optional_bool(params, "visible", true)));
}

nlohmann::json DocumentHandler::open(const nlohmann::json& params) const {
  only_keys(params, {"file_path", "visible", "read_only"});
  return to_json(service_.open(
    required_string(params, "file_path"),
    optional_bool(params, "visible", true),
    optional_bool(params, "read_only", false)
  ));
}

nlohmann::json DocumentHandler::activate(const nlohmann::json& params) const {
  only_keys(params, {"document_id"});
  return to_json(service_.activate(required_string(params, "document_id")));
}

nlohmann::json DocumentHandler::get_top_part(
  const nlohmann::json& params
) const {
  only_keys(params, {"document_id"});
  const std::string id = required_string(params, "document_id");
  return {
    {"document_id", id},
    {"part_id", service_.get_top_part(id)}
  };
}

nlohmann::json DocumentHandler::save(const nlohmann::json& params) const {
  only_keys(params, {"document_id"});
  return to_json(service_.save(required_string(params, "document_id")));
}

nlohmann::json DocumentHandler::save_as(const nlohmann::json& params) const {
  only_keys(params, {"document_id", "file_path", "overwrite"});
  return to_json(service_.save_as(
    required_string(params, "document_id"),
    required_string(params, "file_path"),
    optional_bool(params, "overwrite", false)
  ));
}

nlohmann::json DocumentHandler::close(const nlohmann::json& params) const {
  only_keys(params, {"document_id", "discard_changes"});
  const std::string id = required_string(params, "document_id");
  service_.close(
    id,
    optional_bool(params, "discard_changes", false)
  );
  return {
    {"document_id", id},
    {"closed", true}
  };
}

}  // namespace kompas_bridge
