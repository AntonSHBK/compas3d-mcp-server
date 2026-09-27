#include "kompas_bridge/app/request_dispatcher.hpp"

#include <exception>
#include <optional>
#include <string>
#include <utility>

#include "kompas_bridge/handlers/application_handler.hpp"
#include "kompas_bridge/handlers/document_handler.hpp"
#include "kompas_bridge/handlers/object_handler.hpp"
#include "kompas_bridge/protocol/codec.hpp"
#include "kompas_bridge/protocol/request.hpp"

namespace kompas_bridge {
namespace {

std::optional<std::string> extract_request_id(std::string_view payload) {
  if (payload.size() > kMaxMessageBytes) {
    return std::nullopt;
  }
  const auto data = nlohmann::json::parse(
    payload,
    nullptr,
    false
  );
  if (!data.is_object() || !data.contains("id") || !data["id"].is_string()) {
    return std::nullopt;
  }
  auto id = data["id"].get<std::string>();
  if (id.empty() || id.size() > kMaxIdBytes) {
    return std::nullopt;
  }
  return id;
}

Response error_response(
  std::optional<std::string> id,
  std::string code,
  std::string message
) {
  return Response{
    .id = std::move(id),
    .ok = false,
    .error = BridgeError{
      .code = std::move(code),
      .message = std::move(message)
    }
  };
}

}  // namespace

RequestDispatcher::RequestDispatcher(
  ApplicationHandler& applicationHandler,
  ObjectHandler& objectHandler,
  DocumentHandler& documentHandler
)
  : applicationHandler_(applicationHandler),
    objectHandler_(objectHandler),
    documentHandler_(&documentHandler) {}

RequestDispatcher::RequestDispatcher(
  ApplicationHandler& applicationHandler,
  ObjectHandler& objectHandler
)
  : applicationHandler_(applicationHandler),
    objectHandler_(objectHandler),
    documentHandler_(nullptr) {}

Response RequestDispatcher::dispatch(std::string_view payload) const {
  std::optional<std::string> id;
  try {
    const Request request = parse_request(std::string(payload));
    id = request.id;
    if (request.method == "application.status") {
      if (!request.params.empty()) {
        throw ProtocolError(
          "invalid_params",
          "application.status does not accept params."
        );
      }
      return Response{
        .id = request.id,
        .ok = true,
        .result = applicationHandler_.get_status()
      };
    }
    if (request.method == "application.connect") {
      return Response{
        .id = request.id,
        .ok = true,
        .result = applicationHandler_.connect(request.params)
      };
    }
    if (request.method == "application.disconnect") {
      if (!request.params.empty()) {
        throw ProtocolError(
          "invalid_params",
          "application.disconnect does not accept params."
        );
      }
      return Response{
        .id = request.id,
        .ok = true,
        .result = applicationHandler_.disconnect()
      };
    }
    if (request.method == "object.get_info") {
      return Response{
        .id = request.id,
        .ok = true,
        .result = objectHandler_.get_info(request.params)
      };
    }
    if (request.method == "object.release") {
      return Response{
        .id = request.id,
        .ok = true,
        .result = objectHandler_.release(request.params)
      };
    }
    if (request.method.starts_with("document.")) {
      if (documentHandler_ == nullptr) {
        throw ProtocolError(
          "unknown_method",
          "Document service is unavailable."
        );
      }
      return Response{
        .id = request.id,
        .ok = true,
        .result = documentHandler_->handle(
          request.method,
          request.params
        )
      };
    }
    throw ProtocolError(
      "unknown_method",
      "Method is not implemented yet."
    );
  } catch (const ProtocolError& error) {
    if (!id) {
      id = extract_request_id(payload);
    }
    return error_response(
      std::move(id),
      error.code(),
      error.what()
    );
  } catch (const std::exception&) {
    return error_response(
      std::move(id),
      "internal_error",
      "Request processing failed."
    );
  }
}

}  // namespace kompas_bridge
