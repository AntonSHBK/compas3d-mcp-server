#include "kompas_bridge/app/bridge_application.hpp"

#include <Windows.h>

#include <atomic>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "kompas_bridge/app/request_dispatcher.hpp"
#include "kompas_bridge/app/request_loop.hpp"
#include "kompas_bridge/handlers/application_handler.hpp"
#include "kompas_bridge/handlers/document_handler.hpp"
#include "kompas_bridge/handlers/object_handler.hpp"
#include "kompas_bridge/kompas/application_service.hpp"
#include "kompas_bridge/kompas/kompas_application_service.hpp"
#include "kompas_bridge/kompas/kompas_document_service.hpp"
#include "kompas_bridge/kompas/kompas_session.hpp"
#include "kompas_bridge/kompas/object_registry.hpp"
#include "kompas_bridge/protocol/codec.hpp"
#include "kompas_bridge/transport/named_pipe_server.hpp"
#include "kompas_bridge/transport/stdio_transport.hpp"

namespace kompas_bridge {
namespace {

std::atomic<NamedPipeServer*> activePipeServer{};

BOOL WINAPI handle_console_control(DWORD controlType) {
  if (controlType != CTRL_C_EVENT && controlType != CTRL_BREAK_EVENT &&
      controlType != CTRL_CLOSE_EVENT && controlType != CTRL_SHUTDOWN_EVENT) {
    return FALSE;
  }
  if (NamedPipeServer* server = activePipeServer.load(); server != nullptr) {
    server->request_stop();
    return TRUE;
  }
  return FALSE;
}

class ConsoleHandlerRegistration {
 public:
  explicit ConsoleHandlerRegistration(NamedPipeServer& server) {
    activePipeServer = &server;
    if (!SetConsoleCtrlHandler(handle_console_control, TRUE)) {
      activePipeServer = nullptr;
      throw std::runtime_error("Failed to install console shutdown handler.");
    }
  }

  ~ConsoleHandlerRegistration() {
    activePipeServer = nullptr;
    SetConsoleCtrlHandler(handle_console_control, FALSE);
  }

  ConsoleHandlerRegistration(const ConsoleHandlerRegistration&) = delete;
  ConsoleHandlerRegistration& operator=(const ConsoleHandlerRegistration&) =
    delete;
};

std::wstring utf8_to_wide(std::string_view value) {
  const int size = MultiByteToWideChar(
    CP_UTF8,
    MB_ERR_INVALID_CHARS,
    value.data(),
    static_cast<int>(value.size()),
    nullptr,
    0
  );
  if (size == 0) {
    throw std::invalid_argument("Named Pipe name must be valid UTF-8.");
  }
  std::wstring result(size, L'\0');
  if (MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        value.data(),
        static_cast<int>(value.size()),
        result.data(),
        size
      ) == 0) {
    throw std::invalid_argument("Named Pipe name must be valid UTF-8.");
  }
  return result;
}

}  // namespace

int BridgeApplication::run(
  int argc,
  char* argv[]
) const {
  if (argc < 2 || argc > 3) {
    std::cerr << "Usage: kompas_bridge.exe --pipe [pipe_name] | --stdio\n";
    return 2;
  }
  const std::string_view mode(argv[1]);
  if ((mode != "--pipe" && mode != "--stdio") ||
      (mode == "--stdio" && argc != 2)) {
    std::cerr << "Usage: kompas_bridge.exe --pipe [pipe_name] | --stdio\n";
    return 2;
  }
  try {
    KompasSession session;
    ObjectRegistry registry;
    KompasApplicationService service(
      session,
      registry
    );
    try {
      (void)service.connect(ConnectionPolicy::kAttachOnly);
    } catch (const ProtocolError& error) {
      if (error.code() != "kompas_not_running") {
        throw;
      }
    }
    ApplicationHandler handler(service);
    ObjectHandler objectHandler(registry);
    KompasDocumentService documentService(
      session,
      registry
    );
    DocumentHandler documentHandler(documentService);
    RequestDispatcher dispatcher(
      handler,
      objectHandler,
      documentHandler
    );
    if (mode == "--stdio") {
      StdioTransport transport;
      run_request_loop(
        transport,
        dispatcher
      );
      return 0;
    }

    const std::wstring pipeName = argc == 3
      ? utf8_to_wide(argv[2])
      : std::wstring(kDefaultPipeName);
    NamedPipeServer server(pipeName);
    ConsoleHandlerRegistration consoleHandler(server);
    std::wcerr << L"Named Pipe server started: " << server.pipe_name()
               << L"\nPress Ctrl+C to stop.\n";
    while (server.wait_for_client()) {
      try {
        run_request_loop(
          server,
          dispatcher
        );
      } catch (const std::exception& error) {
        std::cerr << "Named Pipe client disconnected: " << error.what() << '\n';
      }
      server.disconnect_client();
    }
    std::cerr << "Named Pipe server stopped.\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "Bridge stopped: " << error.what() << '\n';
    return 1;
  }
}

}  // namespace kompas_bridge
