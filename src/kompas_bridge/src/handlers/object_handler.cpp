#include "kompas_bridge/handlers/object_handler.hpp"

#include <string>

#include "kompas_bridge/kompas/object_registry.hpp"
#include "kompas_bridge/protocol/codec.hpp"

namespace kompas_bridge {
namespace {

std::string parse_handle(const nlohmann::json &params) {
  if (params.size() != 1 || !params.contains("handle") ||
      !params["handle"].is_string()) {
    throw ProtocolError("invalid_params",
                        "Object method requires one string handle.");
  }
  const std::string handle = params["handle"].get<std::string>();
  if (handle.empty()) {
    throw ProtocolError("invalid_params", "Object handle cannot be empty.");
  }
  return handle;
}

} // namespace

ObjectHandler::ObjectHandler(ObjectRegistry &registry) : registry_(registry) {}

nlohmann::json ObjectHandler::get_info(const nlohmann::json &params) const {
  const ObjectInfo info = registry_.get_info(parse_handle(params));
  return {{"handle", info.handle},
          {"kind", object_kind_name(info.kind)},
          {"document_id", info.documentId ? nlohmann::json(*info.documentId)
                                          : nlohmann::json(nullptr)},
          {"part_id", info.partId ? nlohmann::json(*info.partId)
                                  : nlohmann::json(nullptr)},
          {"revision", info.revision}};
}

nlohmann::json ObjectHandler::release(const nlohmann::json &params) const {
  registry_.release(parse_handle(params));
  return {{"released", true}};
}

} // namespace kompas_bridge
