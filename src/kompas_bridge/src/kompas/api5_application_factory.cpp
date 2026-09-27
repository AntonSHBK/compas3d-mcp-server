#include "kompas_bridge/kompas/api5_application_factory.hpp"

#include <format>
#include <stdexcept>

#include <Windows.h>

namespace kompas_bridge {

namespace {

[[noreturn]] void ThrowComError(
  const char* operation,
  HRESULT result
) {
  throw std::runtime_error(std::format(
    "{} failed with HRESULT 0x{:08X}.",
    operation,
    static_cast<unsigned long>(result)
  ));
}

IDispatch* AttachToActive(REFCLSID classId) {
  IUnknown* activeObject = nullptr;
  const HRESULT result = GetActiveObject(
    classId,
    nullptr,
    &activeObject
  );
  if (result == MK_E_UNAVAILABLE) {
    throw KompasNotRunningError("KOMPAS-3D is not running.");
  }
  if (FAILED(result)) {
    ThrowComError(
      "GetActiveObject(Kompas.Application.5)",
      result
    );
  }
  IDispatch* application = nullptr;
  const HRESULT queryResult = activeObject->QueryInterface(
    IID_IDispatch,
    reinterpret_cast<void**>(&application)
  );
  activeObject->Release();
  if (FAILED(queryResult)) {
    ThrowComError(
      "QueryInterface(IID_IDispatch)",
      queryResult
    );
  }
  if (application == nullptr) {
    throw std::runtime_error("Active KOMPAS-3D object has no IDispatch.");
  }
  return application;
}

IDispatch* ActivateServer(REFCLSID classId) {
  IDispatch* application = nullptr;
  const HRESULT result = CoCreateInstance(
    classId,
    nullptr,
    CLSCTX_LOCAL_SERVER,
    IID_IDispatch,
    reinterpret_cast<void**>(&application)
  );
  if (FAILED(result)) {
    ThrowComError(
      "CoCreateInstance(Kompas.Application.5)",
      result
    );
  }
  if (application == nullptr) {
    throw std::runtime_error("COM activation returned a null IDispatch.");
  }
  return application;
}

}  // namespace

IDispatch* Api5ApplicationFactory::CreateApplication(ConnectionPolicy policy) {
  CLSID classId{};
  const HRESULT result = CLSIDFromProgID(
    L"Kompas.Application.5",
    &classId
  );
  if (FAILED(result)) {
    ThrowComError(
      "CLSIDFromProgID(Kompas.Application.5)",
      result
    );
  }
  switch (policy) {
    case ConnectionPolicy::kAttachOnly:
      return AttachToActive(classId);
    case ConnectionPolicy::kAttachOrStart:
      try {
        return AttachToActive(classId);
      } catch (const KompasNotRunningError&) {
        return ActivateServer(classId);
      }
    case ConnectionPolicy::kStartNew:
      return ActivateServer(classId);
  }
  throw std::invalid_argument("Unknown KOMPAS connection policy.");
}

}  // namespace kompas_bridge
