#include "kompas_bridge/app/application.hpp"

#include <iostream>
#include <stdexcept>
#include <string_view>

#include "kompas_bridge/commands/create_cube_command.hpp"
#include "kompas_bridge/commands/ping_command.hpp"
#include "kompas_bridge/common/bridge_error.hpp"
#include "kompas_bridge/kompas/kompas_session.hpp"

namespace kompas_bridge {
  
int Application::Run(int argc, char *argv[]) const {
  if (argc < 2) {
    std::cout << "Usage: kompas_bridge.exe <ping|create-cube>\n";
    return 2;
  }
  const std::string_view command(argv[1]);
  if (argc > 2 || (command != "ping" && command != "create-cube")) {
    std::cout << BridgeError{.code = "unknown_command",
                             .message = "Supported commands: ping, create-cube."}
                     .ToJson()
              << '\n';
    return 2;
  }
  try {
    KompasSession session = KompasSession::Connect();
    if (command == "ping") {
      std::cout << RunPing(session).ToJson() << '\n';
    } else {
      std::cout << CreateCube(session).ToJson() << '\n';
    }
    return 0;
  } catch (const std::exception &exception) {
    std::cout << BridgeError{.code = "ping_failed", .message = exception.what()}.ToJson() << '\n';
  } catch (...) {
    std::cout << BridgeError{.code = "ping_failed", .message = "An unknown native error occurred."}
                     .ToJson()
              << '\n';
  }
  return 1;
}
}  // namespace kompas_bridge
