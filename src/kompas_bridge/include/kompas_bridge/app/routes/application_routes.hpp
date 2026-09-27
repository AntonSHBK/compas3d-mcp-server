#pragma once

namespace kompas_bridge {

class ApplicationHandler;
class RequestDispatcher;

/** @brief Registers application protocol methods. */
void register_application_routes(
  RequestDispatcher& dispatcher,
  ApplicationHandler& handler
);

}  // namespace kompas_bridge
