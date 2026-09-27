#include "kompas_bridge/handlers/sketch_handler.hpp"

#include "kompas_bridge/kompas/sketch_service.hpp"
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
Point2d required_point(const nlohmann::json &params, const char *key) {
  if (!params.contains(key) || !params[key].is_object()) {
    throw ProtocolError("invalid_params",
                        std::string("Required point parameter is missing: ") +
                            key);
  }
  return {required_number(params[key], "x"), required_number(params[key], "y")};
}
nlohmann::json to_json(const SketchResult &value) {
  return {
      {"sketch_id", value.sketchId}, {"document_id", value.documentId},
      {"part_id", value.partId},     {"plane", value.plane},
      {"closed", !value.editing},    {"geometry_count", value.geometryCount}};
}
} // namespace

SketchHandler::SketchHandler(SketchService &service) : service_(service) {}
nlohmann::json SketchHandler::create(const nlohmann::json &params) const {
  return to_json(service_.create(required_string(params, "document_id"),
                                 required_string(params, "part_id"),
                                 required_string(params, "plane")));
}
nlohmann::json SketchHandler::begin_edit(const nlohmann::json &params) const {
  return to_json(service_.begin_edit(required_string(params, "sketch_id")));
}
nlohmann::json SketchHandler::end_edit(const nlohmann::json &params) const {
  return to_json(service_.end_edit(required_string(params, "sketch_id")));
}
nlohmann::json SketchHandler::add_line(const nlohmann::json &params) const {
  return to_json(service_.add_line(required_string(params, "sketch_id"),
                                   required_point(params, "start"),
                                   required_point(params, "end")));
}
nlohmann::json SketchHandler::add_circle(const nlohmann::json &params) const {
  return to_json(service_.add_circle(required_string(params, "sketch_id"),
                                     required_point(params, "center"),
                                     required_number(params, "radius")));
}
} // namespace kompas_bridge
