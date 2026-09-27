#include "kompas_bridge/kompas/kompas_application_service.hpp"

#include <WinSock2.h>
#include <afxdisp.h>
#include <afxconv.h>
#include <kapi5.h>

#include <limits>
#include <stdexcept>

#include "kompas_bridge/kompas/api5_application_factory.hpp"
#include "kompas_bridge/kompas/kompas_session.hpp"
#include "kompas_bridge/kompas/object_registry.hpp"
#include "kompas_bridge/protocol/codec.hpp"

namespace kompas_bridge {

KompasApplicationService::KompasApplicationService(
  KompasSession& session,
  ObjectRegistry& registry
)
  : session_(session),
    registry_(registry) {}

ApplicationStatus KompasApplicationService::get_status() {
  if (!session_.is_connected()) {
    return {};
  }
  try {
    KompasObject& kompas = session_.kompas_object();
    const bool visible = kompas.GetVisible() != FALSE;
    constexpr long kUnset = (std::numeric_limits<long>::min)();
    KompasVersion version{
      .major = kUnset,
      .minor = kUnset,
      .release = kUnset,
      .build = kUnset
    };
    const long result = kompas.ksGetSystemVersion(
      &version.major,
      &version.minor,
      &version.release,
      &version.build
    );
    if (result == 0 || kompas.ksReturnResult() != 0 ||
        version.major == kUnset || version.minor == kUnset ||
        version.release == kUnset || version.build == kUnset) {
      throw ProtocolError(
        "kompas_api_error",
        "ksGetSystemVersion failed or returned incomplete data."
      );
    }
    return ApplicationStatus{
      .connected = true,
      .visible = visible,
      .kompasVersion = version
    };
  } catch (CException* error) {
    TCHAR message[512]{};
    error->GetErrorMessage(message, static_cast<UINT>(std::size(message)), 0);
    const std::string diagnostic(CW2A(message, CP_UTF8));
    error->Delete();
    throw ProtocolError(
      "kompas_api_error",
      "COM call failed while reading KOMPAS status: " + diagnostic
    );
  }
}

ApplicationStatus KompasApplicationService::connect(ConnectionPolicy policy) {
  if (session_.is_connected()) {
    if (policy == ConnectionPolicy::kStartNew) {
      throw ProtocolError(
        "invalid_params",
        "Disconnect before requesting start_new."
      );
    }
    return get_status();
  }
  try {
    session_.connect(policy);
  } catch (const KompasNotRunningError&) {
    throw ProtocolError(
      "kompas_not_running",
      "KOMPAS-3D is not running."
    );
  } catch (CException* error) {
    error->Delete();
    throw ProtocolError(
      "kompas_api_error",
      "COM activation of KOMPAS failed."
    );
  } catch (const std::exception& error) {
    throw ProtocolError(
      "kompas_api_error",
      error.what()
    );
  }
  return get_status();
}

void KompasApplicationService::disconnect() {
  registry_.invalidate_all();
  session_.disconnect();
}

}  // namespace kompas_bridge
