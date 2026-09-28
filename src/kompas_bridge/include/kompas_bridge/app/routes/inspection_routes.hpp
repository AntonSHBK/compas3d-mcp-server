#pragma once

namespace kompas_bridge {
class InspectionHandler;
class RequestDispatcher;
void register_inspection_routes(RequestDispatcher& dispatcher,
                                InspectionHandler& handler);
}  // namespace kompas_bridge
