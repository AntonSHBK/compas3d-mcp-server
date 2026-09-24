#include "kompas_bridge/kompas/api5_application_factory.hpp"

#include <Windows.h>

#include <format>
#include <stdexcept>
namespace kompas_bridge {
namespace {
[[noreturn]] void ThrowComError(const char *operation, HRESULT result) {
  throw std::runtime_error(std::format("{} failed with HRESULT 0x{:08X}.", operation,
                                       static_cast<unsigned long>(result)));
}
}  // namespace
IDispatch *Api5ApplicationFactory::CreateApplication() {
  CLSID application_class_id{};
  HRESULT result = CLSIDFromProgID(L"Kompas.Application.5", &application_class_id);
  if (FAILED(result)) ThrowComError("CLSIDFromProgID(Kompas.Application.5)", result);
  IDispatch *application = nullptr;
  result = CoCreateInstance(application_class_id, nullptr, CLSCTX_LOCAL_SERVER, IID_IDispatch,
                            reinterpret_cast<void **>(&application));
  if (FAILED(result)) ThrowComError("CoCreateInstance(Kompas.Application.5)", result);
  if (application == nullptr)
    throw std::runtime_error("CoCreateInstance(Kompas.Application.5) returned a null IDispatch.");
  return application;
}
}  // namespace kompas_bridge
