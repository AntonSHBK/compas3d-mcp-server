#include "kompas_bridge/kompas/kompas_session.hpp"

#include <WinSock2.h>
#include <Windows.h>
#include <afxdisp.h>
#include <kapi5.h>

#include <memory>
#include <stdexcept>
#include <string>

#include "kompas_bridge/kompas/api5_application_factory.hpp"
namespace kompas_bridge {
namespace {
void InitializeComApartment() {
  const HRESULT result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  if (FAILED(result))
    throw std::runtime_error("CoInitializeEx(COINIT_APARTMENTTHREADED) failed with HRESULT " +
                             std::to_string(static_cast<long>(result)) + ".");
}
}  // namespace
class KompasSession::Implementation {
 public:
  Implementation() {
    InitializeComApartment();
    com_initialized_ = true;
    try {
      kompas_object_ = std::make_unique<KompasObject>(Api5ApplicationFactory::CreateApplication());
      kompas_object_->SetVisible(TRUE);
    } catch (...) {
      ReleaseResources();
      throw;
    }
  }
  ~Implementation() { ReleaseResources(); }
  [[nodiscard]] bool is_connected() const noexcept { return kompas_object_ != nullptr; }
  [[nodiscard]] KompasObject &kompas_object() const {
    if (kompas_object_ == nullptr) throw std::logic_error("KOMPAS-3D session is not connected.");
    return *kompas_object_;
  }

 private:
  void ReleaseResources() noexcept {
    kompas_object_.reset();
    if (com_initialized_) {
      CoUninitialize();
      com_initialized_ = false;
    }
  }
  bool com_initialized_{};
  std::unique_ptr<KompasObject> kompas_object_;
};
KompasSession::KompasSession() : implementation_(std::make_unique<Implementation>()) {}
KompasSession::~KompasSession() = default;
KompasSession KompasSession::Connect() { return KompasSession(); }
bool KompasSession::is_connected() const noexcept {
  return implementation_ != nullptr && implementation_->is_connected();
}
KompasObject &KompasSession::kompas_object() const {
  if (implementation_ == nullptr) throw std::logic_error("KOMPAS-3D session was moved from.");
  return implementation_->kompas_object();
}
}  // namespace kompas_bridge
