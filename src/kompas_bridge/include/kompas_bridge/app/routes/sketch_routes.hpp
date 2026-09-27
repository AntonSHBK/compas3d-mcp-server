#pragma once
namespace kompas_bridge {
class RequestDispatcher;
class SketchHandler;
void register_sketch_routes(RequestDispatcher &dispatcher,
                            SketchHandler &handler);
} // namespace kompas_bridge
