#include "kompas_bridge/protocol/method.hpp"

namespace kompas_bridge {

bool is_supported_method(std::string_view method) {
  return method == "application.status" ||
    method == "application.connect" ||
    method == "application.disconnect" ||
    method == "object.get_info" ||
    method == "object.release" ||
    method == "document.list" ||
    method == "document.get_active" ||
    method == "document.create_3d" ||
    method == "document.open" ||
    method == "document.activate" ||
    method == "document.get_top_part" ||
    method == "document.save" ||
    method == "document.save_as" ||
    method == "document.close";
}

}  // namespace kompas_bridge
