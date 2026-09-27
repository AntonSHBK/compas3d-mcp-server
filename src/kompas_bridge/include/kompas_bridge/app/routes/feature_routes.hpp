#pragma once
namespace kompas_bridge {
class RequestDispatcher;
class FeatureHandler;
void register_feature_routes(RequestDispatcher &dispatcher,
                             FeatureHandler &handler);
} // namespace kompas_bridge
