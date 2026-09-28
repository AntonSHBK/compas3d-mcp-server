#include "kompas_bridge/app/routes/inspection_routes.hpp"

#include "kompas_bridge/app/request_dispatcher.hpp"
#include "kompas_bridge/handlers/inspection_handler.hpp"
#include "kompas_bridge/protocol/methods/inspection_methods.hpp"

namespace kompas_bridge {

void register_inspection_routes(RequestDispatcher& dispatcher,
                                InspectionHandler& handler) {
  dispatcher.add_route(
      methods::kPartListBodies,
      [&handler](const auto& params) { return handler.list_bodies(params); });
  dispatcher.add_route(methods::kBodyGetInfo, [&handler](const auto& params) {
    return handler.get_body_info(params);
  });
  dispatcher.add_route(methods::kBodyListFaces, [&handler](const auto& params) {
    return handler.list_body_faces(params);
  });
  dispatcher.add_route(methods::kPartListFaces, [&handler](const auto& params) {
    return handler.list_part_faces(params);
  });
  dispatcher.add_route(methods::kFaceGetGeometry,
                       [&handler](const auto& params) {
                         return handler.get_face_geometry(params);
                       });
  dispatcher.add_route(methods::kPartGetMassProperties,
                       [&handler](const auto& params) {
                         return handler.get_mass_properties(params);
                       });
  dispatcher.add_route(methods::kPartGetBoundingBox,
                       [&handler](const auto& params) {
                         return handler.get_bounding_box(params);
                       });
}

}  // namespace kompas_bridge
