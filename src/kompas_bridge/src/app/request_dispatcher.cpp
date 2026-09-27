#include "kompas_bridge/app/request_dispatcher.hpp"

#include <exception>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#include "kompas_bridge/protocol/codec.hpp"
#include "kompas_bridge/protocol/request.hpp"

namespace kompas_bridge {
namespace {

std::optional<std::string> extract_request_id(std::string_view payload) {
  if (payload.size() > kMaxMessageBytes) {
    return std::nullopt;
  }
  const auto data = nlohmann::json::parse(payload, nullptr, false);
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

void RequestDispatcher::add_route(std::string_view method, Route route) {
  if (method.empty() || !route) {
    throw std::invalid_argument("A route requires a method and callback.");
  }
  const auto result = routes_.emplace(method, std::move(route));
  if (!result.second) {
    throw std::logic_error("Duplicate bridge method registration.");
  }
}

Response RequestDispatcher::dispatch(std::string_view payload) const {
  std::optional<std::string> id;
  try {
    const Request request = parse_request(std::string(payload));
    id = request.id;
    const auto route = routes_.find(request.method);
    if (route == routes_.end()) {
      throw ProtocolError("unknown_method", "Unknown method.");
    }
    return Response{
      .id = request.id,
      .ok = true,
      .result = route->second(request.params)
    };
  } catch (const ProtocolError& error) {
    if (!id) {
      id = extract_request_id(payload);
    }
    return error_response(std::move(id), error.code(), error.what());
  } catch (const std::exception&) {
    return error_response(
      std::move(id),
      "internal_error",
      "Request processing failed."
    );
  }
}

}  // namespace kompas_bridge
