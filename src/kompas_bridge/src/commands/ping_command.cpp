#include "kompas_bridge/commands/ping_command.hpp"

#include <WinSock2.h>
#include <afxdisp.h>
#include <kapi5.h>

#include <format>
#include <limits>
#include <stdexcept>

#include "kompas_bridge/kompas/kompas_session.hpp"
namespace kompas_bridge {
std::string PingResponse::ToJson() const {
  return R"({"ok":true,"command":"ping","kompas_version":{"major":)" +
         std::to_string(kompas_version.major) + R"(,"minor":)" +
         std::to_string(kompas_version.minor) + R"(,"release":)" +
         std::to_string(kompas_version.release) + R"(,"build":)" +
         std::to_string(kompas_version.build) + "}}";
}
PingResponse RunPing(KompasSession &session) {
  if (!session.is_connected()) throw std::runtime_error("KOMPAS-3D session is not connected.");
  constexpr long kUnsetVersionPart = (std::numeric_limits<long>::min)();
  KompasVersion version{.major = kUnsetVersionPart,
                        .minor = kUnsetVersionPart,
                        .release = kUnsetVersionPart,
                        .build = kUnsetVersionPart};
  KompasObject &kompas = session.kompas_object();
  if (kompas.ksGetSystemVersion(&version.major, &version.minor, &version.release, &version.build) ==
      0)
    throw std::runtime_error("ksGetSystemVersion reported failure.");
  const long kompas_error = kompas.ksReturnResult();
  if (kompas_error != 0)
    throw std::runtime_error(
        std::format("ksGetSystemVersion failed with KOMPAS error {}.", kompas_error));
  if (version.major == kUnsetVersionPart || version.minor == kUnsetVersionPart ||
      version.release == kUnsetVersionPart || version.build == kUnsetVersionPart)
    throw std::runtime_error("ksGetSystemVersion did not initialize every version field.");
  return PingResponse{.kompas_version = version};
}
}  // namespace kompas_bridge
