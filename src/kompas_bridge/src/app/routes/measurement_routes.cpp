#include "kompas_bridge/app/routes/measurement_routes.hpp"

#include "kompas_bridge/app/request_dispatcher.hpp"
#include "kompas_bridge/handlers/measurement_handler.hpp"
#include "kompas_bridge/protocol/methods/measurement_methods.hpp"

namespace kompas_bridge {

void register_measurement_routes(RequestDispatcher& dispatcher,
                                 MeasurementHandler& handler) {
  dispatcher.add_route(methods::kMeasurementDistance,
                       [&handler](const auto& params) {
                         return handler.distance(params);
                       });
  dispatcher.add_route(methods::kMeasurementAngle,
                       [&handler](const auto& params) {
                         return handler.angle(params);
                       });
}

}  // namespace kompas_bridge
