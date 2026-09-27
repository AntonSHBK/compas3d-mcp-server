#include "kompas_bridge/app/request_loop.hpp"

#include "kompas_bridge/app/request_dispatcher.hpp"
#include "kompas_bridge/protocol/codec.hpp"
#include "kompas_bridge/protocol/response.hpp"
#include "kompas_bridge/transport/transport.hpp"

namespace kompas_bridge {

void run_request_loop(
  Transport& transport,
  const RequestDispatcher& dispatcher
) {
  while (true) {
    std::optional<std::string> message;
    try {
      message = transport.read_message();
    } catch (const ProtocolError& error) {
      const Response response{
        .ok = false,
        .error = BridgeError{
          .code = error.code(),
          .message = error.what()
        }
      };
      transport.write_message(serialize_response(response));
      continue;
    }
    if (!message) {
      return;
    }
    transport.write_message(serialize_response(dispatcher.dispatch(*message)));
  }
}

}  // namespace kompas_bridge
