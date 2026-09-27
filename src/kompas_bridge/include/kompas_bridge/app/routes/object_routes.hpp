#pragma once

namespace kompas_bridge {

class ObjectHandler;
class RequestDispatcher;

/** @brief Registers opaque object protocol methods. */
void register_object_routes(
  RequestDispatcher& dispatcher,
  ObjectHandler& handler
);

}  // namespace kompas_bridge
