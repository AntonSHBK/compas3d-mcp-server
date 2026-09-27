#include "kompas_bridge/protocol/request.hpp"

#include "kompas_bridge/protocol/codec.hpp"
#include "kompas_bridge/protocol/method.hpp"

namespace kompas_bridge {

namespace {

void validate_request(const Request& request) {
  if (request.protocolVersion != kProtocolVersion) {
    throw ProtocolError(
      "unsupported_protocol_version",
      "Unsupported protocol version."
    );
  }
  if (request.id.empty() || request.id.size() > kMaxIdBytes) {
    throw ProtocolError(
      "invalid_request",
      "Invalid request id."
    );
  }
  if (!is_supported_method(request.method)) {
    throw ProtocolError(
      "unknown_method",
      "Unknown method."
    );
  }
  if (!request.params.is_object()) {
    throw ProtocolError(
      "invalid_params",
      "Params must be an object."
    );
  }
}

}  // namespace

Request parse_request(const std::string& payload) {
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
        !data.contains("id") || !data["id"].is_string() ||
        !data.contains("method") || !data["method"].is_string() ||
        !data.contains("params")) {
      throw ProtocolError(
        "invalid_request",
        "Malformed request envelope."
      );
    }
    Request request{
      .protocolVersion = data["protocol_version"].get<int>(),
      .id = data["id"].get<std::string>(),
      .method = data["method"].get<std::string>(),
      .params = data["params"]
    };
    validate_request(request);
    return request;
  } catch (const nlohmann::json::exception&) {
    throw ProtocolError(
      "invalid_request",
      "Invalid JSON request."
    );
  }
}

std::string serialize_request(const Request& request) {
  validate_request(request);
  try {
    const nlohmann::json data = {
      {"protocol_version", request.protocolVersion},
      {"id", request.id},
      {"method", request.method},
      {"params", request.params}
    };
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
      "Invalid UTF-8 request."
    );
  }
}

}  // namespace kompas_bridge
