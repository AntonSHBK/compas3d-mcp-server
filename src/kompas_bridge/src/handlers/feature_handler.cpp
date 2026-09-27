#include "kompas_bridge/handlers/feature_handler.hpp"

#include "kompas_bridge/kompas/feature_service.hpp"
#include "kompas_bridge/protocol/codec.hpp"
#include <string>

namespace kompas_bridge {
namespace {
std::string required_string(const nlohmann::json &params, const char *key) {
  if (!params.contains(key) || !params[key].is_string() ||
      params[key].get<std::string>().empty()) {
    throw ProtocolError("invalid_params",
                        std::string("Required string parameter is missing: ") +
                            key);
  }
  return params[key].get<std::string>();
}
double required_number(const nlohmann::json &params, const char *key) {
  if (!params.contains(key) || !params[key].is_number()) {
    throw ProtocolError("invalid_params",
                        std::string("Required number parameter is missing: ") +
                            key);
  }
  return params[key].get<double>();
}
ExtrusionDirection parse_direction(std::string_view value) {
  if (value == "forward")
    return ExtrusionDirection::kForward;
  if (value == "reverse")
    return ExtrusionDirection::kReverse;
  if (value == "both")
    return ExtrusionDirection::kBoth;
  throw ProtocolError("invalid_params", "Unknown extrusion direction.");
}
BooleanOperation parse_operation(std::string_view value) {
  if (value == "new_body")
    return BooleanOperation::kNewBody;
  if (value == "join")
    return BooleanOperation::kJoin;
  if (value == "cut")
    return BooleanOperation::kCut;
  throw ProtocolError("invalid_params",
                      "Supported operations are new_body, join, and cut.");
}
nlohmann::json to_json(const ExtrusionResult &value) {
  return {{"feature_id", value.featureId},
          {"document_id", value.documentId},
          {"part_id", value.partId},
          {"sketch_id", value.sketchId},
          {"kind", "extrusion"},
          {"distance", value.parameters.distance},
          {"direction", direction_name(value.parameters.direction)},
          {"operation", operation_name(value.parameters.operation)}};
}
} // namespace

FeatureHandler::FeatureHandler(FeatureService &service) : service_(service) {}
nlohmann::json FeatureHandler::extrude(const nlohmann::json &params) const {
  const ExtrusionParameters values{
      .distance = required_number(params, "distance"),
      .direction = parse_direction(required_string(params, "direction")),
      .operation = parse_operation(required_string(params, "operation"))};
  return to_json(service_.extrude(required_string(params, "document_id"),
                                  required_string(params, "part_id"),
                                  required_string(params, "sketch_id"),
                                  values));
}
nlohmann::json
FeatureHandler::get_parameters(const nlohmann::json &params) const {
  return to_json(
      service_.get_parameters(required_string(params, "feature_id")));
}
nlohmann::json
FeatureHandler::update_extrusion(const nlohmann::json &params) const {
  return to_json(
      service_.update_extrusion(required_string(params, "feature_id"),
                                required_number(params, "distance")));
}
nlohmann::json FeatureHandler::rebuild(const nlohmann::json &params) const {
  const std::string partId = required_string(params, "part_id");
  service_.rebuild(partId);
  return {{"part_id", partId}, {"rebuilt", true}};
}
} // namespace kompas_bridge
