#include "kompas_bridge/app/routes/feature_routes.hpp"
#include "kompas_bridge/app/request_dispatcher.hpp"
#include "kompas_bridge/handlers/feature_handler.hpp"
#include "kompas_bridge/protocol/methods/feature_methods.hpp"
#include "kompas_bridge/protocol/methods/model_methods.hpp"
namespace kompas_bridge {
void register_feature_routes(RequestDispatcher &dispatcher,
                             FeatureHandler &handler) {
  dispatcher.add_route(methods::kFeatureExtrude, [&handler](const auto &p) {
    return handler.extrude(p);
  });
  dispatcher.add_route(
      methods::kFeatureGetParameters,
      [&handler](const auto &p) { return handler.get_parameters(p); });
  dispatcher.add_route(
      methods::kFeatureUpdateExtrusion,
      [&handler](const auto &p) { return handler.update_extrusion(p); });
  dispatcher.add_route(methods::kModelRebuild, [&handler](const auto &p) {
    return handler.rebuild(p);
  });
}
} // namespace kompas_bridge
