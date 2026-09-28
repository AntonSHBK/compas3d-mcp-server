#pragma once

namespace kompas_bridge {
class MeasurementHandler;
class RequestDispatcher;
void register_measurement_routes(RequestDispatcher& dispatcher,
                                 MeasurementHandler& handler);
}  // namespace kompas_bridge
