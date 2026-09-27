#include "kompas_bridge/protocol/response.hpp"

#include "kompas_bridge/protocol/codec.hpp"
#include "kompas_bridge/protocol/error_code.hpp"

namespace kompas_bridge {

namespace {

void validate_response(const Response& response) {
  if (response.protocolVersion != kProtocolVersion) {
    throw ProtocolError(
      "unsupported_protocol_version",
      "Unsupported protocol version."
    );
  }
  if (response.id &&
      (response.id->empty() || response.id->size() > kMaxIdBytes)) {
    throw ProtocolError(
      "invalid_request",
      "Invalid response id."
    );
  }
  if (response.ok) {
    if (!response.id || !response.result || !response.result->is_object() ||
        response.error) {
      throw ProtocolError(
        "invalid_request",
        "Malformed success response."
      );
    }
  } else if (response.result || !response.error ||
             !is_supported_error_code(response.error->code) ||
             response.error->message.empty() ||
             !response.error->details.is_object()) {
    throw ProtocolError(
      "invalid_request",
      "Malformed error response."
    );
  }
}

}  // namespace

Response parse_response(const std::string& payload) {
  if (payload.empty() || payload.size() > kMaxMessageBytes) {
    throw ProtocolError(
      "invalid_request",
      "Invalid message size."
    );
  }
  validate_utf8(payload);
  try {
    const auto data = nlohmann::json::parse(payload);
    if (!data.is_object() || !data.contains("protocol_version") ||
        !data["protocol_version"].is_number_integer() ||
        !data.contains("id") ||
        !(data["id"].is_null() || data["id"].is_string()) ||
        !data.contains("ok") || !data["ok"].is_boolean()) {
      throw ProtocolError(
        "invalid_request",
        "Malformed response envelope."
      );
    }
    Response response;
    response.protocolVersion = data["protocol_version"].get<int>();
    if (data["id"].is_string()) {
      response.id = data["id"].get<std::string>();
    }
    response.ok = data["ok"].get<bool>();
    if (data.contains("result")) {
      response.result = data["result"];
    }
    if (data.contains("error")) {
      const auto& error = data["error"];
      if (!error.is_object() || !error.contains("code") ||
          !error["code"].is_string() || !error.contains("message") ||
          !error["message"].is_string() || !error.contains("details")) {
        throw ProtocolError(
          "invalid_request",
          "Malformed error payload."
        );
      }
      response.error = BridgeError{
        .code = error["code"].get<std::string>(),
        .message = error["message"].get<std::string>(),
        .details = error["details"]
      };
    }
    validate_response(response);
    return response;
  } catch (const nlohmann::json::exception&) {
    throw ProtocolError(
      "invalid_request",
      "Invalid JSON response."
    );
  }
}

std::string serialize_response(const Response& response) {
  validate_response(response);
  try {
    nlohmann::json data{
      {"protocol_version", response.protocolVersion},
      {"id", response.id ? nlohmann::json(*response.id)
                         : nlohmann::json(nullptr)},
      {"ok", response.ok}
    };
    if (response.ok) {
      data["result"] = *response.result;
    } else {
      data["error"] = {
        {"code", response.error->code},
        {"message", response.error->message},
        {"details", response.error->details}
      };
    }
    auto payload = data.dump();
    if (payload.size() > kMaxMessageBytes) {
      throw ProtocolError(
        "invalid_request",
        "Message exceeds size limit."
      );
    }
    validate_utf8(payload);
    return payload;
  } catch (const nlohmann::json::exception&) {
    throw ProtocolError(
      "invalid_request",
      "Invalid UTF-8 response."
    );
  }
}

}  // namespace kompas_bridge
