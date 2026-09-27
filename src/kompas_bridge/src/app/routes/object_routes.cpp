#include "kompas_bridge/app/routes/object_routes.hpp"

#include "kompas_bridge/app/request_dispatcher.hpp"
#include "kompas_bridge/handlers/object_handler.hpp"
#include "kompas_bridge/protocol/methods/object_methods.hpp"

namespace kompas_bridge {

void register_object_routes(
  RequestDispatcher& dispatcher,
  ObjectHandler& handler
) {
  dispatcher.add_route(
    methods::kObjectGetInfo,
    [&handler](const auto& params) { return handler.get_info(params); }
  );
  dispatcher.add_route(
    methods::kObjectRelease,
    [&handler](const auto& params) { return handler.release(params); }
  );
}

}  // namespace kompas_bridge
