#include "kompas_bridge/app/routes/application_routes.hpp"

#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include "kompas_bridge/app/request_dispatcher.hpp"
#include "kompas_bridge/handlers/application_handler.hpp"
#include "kompas_bridge/protocol/codec.hpp"
#include "kompas_bridge/protocol/methods/application_methods.hpp"

namespace kompas_bridge {
namespace {

void require_empty_params(
  const nlohmann::json& params,
  std::string_view method
) {
  if (!params.empty()) {
    throw ProtocolError(
      "invalid_params",
      std::string(method) + " does not accept params."
    );
  }
}

}  // namespace

void register_application_routes(
  RequestDispatcher& dispatcher,
  ApplicationHandler& handler
) {
  dispatcher.add_route(
    methods::kApplicationStatus,
    [&handler](const auto& params) {
      require_empty_params(params, methods::kApplicationStatus);
      return handler.get_status();
    }
  );
  dispatcher.add_route(
    methods::kApplicationConnect,
    [&handler](const auto& params) { return handler.connect(params); }
  );
  dispatcher.add_route(
    methods::kApplicationDisconnect,
    [&handler](const auto& params) {
      require_empty_params(params, methods::kApplicationDisconnect);
      return handler.disconnect();
    }
  );
}

}  // namespace kompas_bridge
