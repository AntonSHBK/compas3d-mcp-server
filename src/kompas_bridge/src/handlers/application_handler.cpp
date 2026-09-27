#include "kompas_bridge/handlers/application_handler.hpp"

#include <string>

#include "kompas_bridge/protocol/codec.hpp"

namespace kompas_bridge {

namespace {

nlohmann::json to_json(const ApplicationStatus& status) {
  nlohmann::json version = nullptr;
  if (status.kompasVersion) {
    const KompasVersion& value = *status.kompasVersion;
    version = {
      {"major", value.major},
      {"minor", value.minor},
      {"release", value.release},
      {"build", value.build}
    };
  }
  return {
    {"connected", status.connected},
    {"visible", status.visible ? nlohmann::json(*status.visible)
                               : nlohmann::json(nullptr)},
    {"kompas_version", version}
  };
}

ConnectionPolicy parse_policy(const nlohmann::json& params) {
  if (params.empty()) {
    return ConnectionPolicy::kAttachOnly;
  }
  if (params.size() != 1 || !params.contains("policy") ||
      !params["policy"].is_string()) {
    throw ProtocolError(
      "invalid_params",
      "application.connect accepts only a string policy."
    );
  }
  const std::string policy = params["policy"].get<std::string>();
  if (policy == "attach_only") {
    return ConnectionPolicy::kAttachOnly;
  }
  if (policy == "attach_or_start") {
    return ConnectionPolicy::kAttachOrStart;
  }
  if (policy == "start_new") {
    return ConnectionPolicy::kStartNew;
  }
  throw ProtocolError(
    "invalid_params",
    "Unknown application.connect policy."
  );
}

}  // namespace

ApplicationHandler::ApplicationHandler(ApplicationService& service)
  : service_(service) {}

nlohmann::json ApplicationHandler::get_status() const {
  return to_json(service_.get_status());
}

nlohmann::json ApplicationHandler::connect(const nlohmann::json& params) const {
  return to_json(service_.connect(parse_policy(params)));
}

nlohmann::json ApplicationHandler::disconnect() const {
  service_.disconnect();
  return to_json(service_.get_status());
}

}  // namespace kompas_bridge
