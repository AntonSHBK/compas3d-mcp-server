#pragma once

namespace kompas_bridge {

class DocumentHandler;
class RequestDispatcher;

/** @brief Registers document protocol methods. */
void register_document_routes(
  RequestDispatcher& dispatcher,
  DocumentHandler& handler
);

}  // namespace kompas_bridge
