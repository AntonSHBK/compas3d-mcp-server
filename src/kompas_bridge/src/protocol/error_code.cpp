#include "kompas_bridge/protocol/error_code.hpp"

namespace kompas_bridge {

bool is_supported_error_code(std::string_view code) {
  return code == "invalid_request" ||
    code == "unsupported_protocol_version" ||
    code == "unknown_method" ||
    code == "invalid_params" ||
    code == "kompas_not_running" ||
    code == "kompas_api_error" ||
    code == "no_active_document" ||
    code == "object_not_found" ||
    code == "object_invalidated" ||
    code == "internal_error";
}

}  // namespace kompas_bridge
