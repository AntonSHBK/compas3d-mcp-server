#include "kompas_bridge/app/routes/sketch_routes.hpp"
#include "kompas_bridge/app/request_dispatcher.hpp"
#include "kompas_bridge/handlers/sketch_handler.hpp"
#include "kompas_bridge/protocol/methods/sketch_methods.hpp"
namespace kompas_bridge {
void register_sketch_routes(RequestDispatcher &dispatcher,
                            SketchHandler &handler) {
  dispatcher.add_route(methods::kSketchCreate,
                       [&handler](const auto &p) { return handler.create(p); });
  dispatcher.add_route(methods::kSketchBeginEdit, [&handler](const auto &p) {
    return handler.begin_edit(p);
  });
  dispatcher.add_route(methods::kSketchEndEdit, [&handler](const auto &p) {
    return handler.end_edit(p);
  });
  dispatcher.add_route(methods::kSketchAddLine, [&handler](const auto &p) {
    return handler.add_line(p);
  });
  dispatcher.add_route(methods::kSketchAddCircle, [&handler](const auto &p) {
    return handler.add_circle(p);
  });
}
} // namespace kompas_bridge
