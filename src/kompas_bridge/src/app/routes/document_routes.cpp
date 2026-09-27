#include "kompas_bridge/app/routes/document_routes.hpp"

#include "kompas_bridge/app/request_dispatcher.hpp"
#include "kompas_bridge/handlers/document_handler.hpp"
#include "kompas_bridge/protocol/methods/document_methods.hpp"

namespace kompas_bridge {

void register_document_routes(
  RequestDispatcher& dispatcher,
  DocumentHandler& handler
) {
  dispatcher.add_route(
    methods::kDocumentList,
    [&handler](const auto& params) { return handler.list(params); }
  );
  dispatcher.add_route(
    methods::kDocumentGetActive,
    [&handler](const auto& params) { return handler.get_active(params); }
  );
  dispatcher.add_route(
    methods::kDocumentCreate3d,
    [&handler](const auto& params) { return handler.create_3d(params); }
  );
  dispatcher.add_route(
    methods::kDocumentOpen,
    [&handler](const auto& params) { return handler.open(params); }
  );
  dispatcher.add_route(
    methods::kDocumentActivate,
    [&handler](const auto& params) { return handler.activate(params); }
  );
  dispatcher.add_route(
    methods::kDocumentGetTopPart,
    [&handler](const auto& params) { return handler.get_top_part(params); }
  );
  dispatcher.add_route(
    methods::kDocumentSave,
    [&handler](const auto& params) { return handler.save(params); }
  );
  dispatcher.add_route(
    methods::kDocumentSaveAs,
    [&handler](const auto& params) { return handler.save_as(params); }
  );
  dispatcher.add_route(
    methods::kDocumentClose,
    [&handler](const auto& params) { return handler.close(params); }
  );
}

}  // namespace kompas_bridge
