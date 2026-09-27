#include "kompas_bridge/app/request_dispatcher.hpp"
#include "kompas_bridge/app/routes/object_routes.hpp"
#include "kompas_bridge/handlers/object_handler.hpp"
#include "kompas_bridge/kompas/object_registry.hpp"
#include "kompas_bridge/protocol/codec.hpp"

#include <Unknwn.h>

#include <atomic>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

void check(bool condition) {
  if (!condition) {
    throw std::runtime_error("Object registry test failed.");
  }
}

class FakeUnknown final : public IUnknown {
 public:
  explicit FakeUnknown(int& destroyed) : destroyed_(destroyed) {}

  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** object) override {
    if (object == nullptr) {
      return E_POINTER;
    }
    *object = nullptr;
    if (!IsEqualIID(iid, IID_IUnknown)) {
      return E_NOINTERFACE;
    }
    *object = static_cast<IUnknown*>(this);
    AddRef();
    return S_OK;
  }

  ULONG STDMETHODCALLTYPE AddRef() override { return ++references_; }

  ULONG STDMETHODCALLTYPE Release() override {
    const ULONG remaining = --references_;
    if (remaining == 0) {
      delete this;
    }
    return remaining;
  }

 private:
  ~FakeUnknown() { ++destroyed_; }
  std::atomic<ULONG> references_{1};
  int& destroyed_;
};

std::string object_request(std::string_view method, std::string_view handle) {
  return nlohmann::json{{"protocol_version", 1},
                        {"id", "test"},
                        {"method", method},
                        {"params", {{"handle", handle}}}}.dump();
}

}  // namespace

int main() {
  int destroyed = 0;
  kompas_bridge::ObjectRegistry registry;
  auto* documentObject = new FakeUnknown(destroyed);
  const std::string document = registry.find_or_register_document(documentObject);
  check(registry.find_or_register_document(documentObject) == document);
  documentObject->Release();
  auto add_child = [&](kompas_bridge::ObjectKind kind) {
    auto* object = new FakeUnknown(destroyed);
    const std::string handle = registry.find_or_register_child(kind, document, object);
    check(registry.find_or_register_child(kind, document, object) == handle);
    object->Release();
    return handle;
  };
  const std::string part = add_child(kompas_bridge::ObjectKind::kPart);
  const std::string sketch = add_child(kompas_bridge::ObjectKind::kSketch);
  const std::string feature = add_child(kompas_bridge::ObjectKind::kFeature);
  check(document.starts_with("doc_"));
  check(part.starts_with("part_"));
  check(sketch.starts_with("sketch_"));
  check(feature.starts_with("feat_"));

  kompas_bridge::ObjectHandler objectHandler(registry);
  kompas_bridge::RequestDispatcher dispatcher;
  kompas_bridge::register_object_routes(dispatcher, objectHandler);
  const auto first = dispatcher.dispatch(object_request("object.get_info", sketch));
  const auto second = dispatcher.dispatch(object_request("object.get_info", sketch));
  check(first.ok && second.ok);
  check((*first.result)["handle"] == sketch);
  check((*second.result)["document_id"] == document);
  check((*second.result)["kind"] == "sketch");

  registry.invalidate_document(document);
  check(destroyed == 4);
  const auto invalid = dispatcher.dispatch(object_request("object.get_info", sketch));
  check(!invalid.ok && invalid.error->code == "object_invalidated");
  const auto unknown = dispatcher.dispatch(object_request("object.get_info", "part_missing"));
  check(!unknown.ok && unknown.error->code == "object_not_found");
  const auto released = dispatcher.dispatch(object_request("object.release", part));
  check(!released.ok && released.error->code == "object_invalidated");

  kompas_bridge::ObjectRegistry otherSession;
  auto* foreignDocument = new FakeUnknown(destroyed);
  const std::string foreign = otherSession.register_document(foreignDocument);
  foreignDocument->Release();
  check(foreign != document);
  try {
    (void)otherSession.get_info(document);
    throw std::runtime_error("Foreign-session handle was accepted.");
  } catch (const kompas_bridge::ProtocolError& error) {
    check(error.code() == "object_not_found");
  }

  auto* anotherDocument = new FakeUnknown(destroyed);
  const std::string another = registry.register_document(anotherDocument);
  anotherDocument->Release();
  const auto releaseResponse = dispatcher.dispatch(
      object_request("object.release", another));
  check(releaseResponse.ok && (*releaseResponse.result)["released"] == true);
  check(destroyed == 5);
  const auto repeatedRelease = dispatcher.dispatch(
      object_request("object.release", another));
  check(!repeatedRelease.ok &&
        repeatedRelease.error->code == "object_invalidated");

  auto* activeDocument = new FakeUnknown(destroyed);
  const std::string active = registry.register_document(activeDocument);
  activeDocument->Release();
  const std::string activePart = [&] {
    auto* object = new FakeUnknown(destroyed);
    const std::string handle = registry.register_child(
        kompas_bridge::ObjectKind::kPart, active, object);
    object->Release();
    return handle;
  }();
  const auto releasePart = dispatcher.dispatch(
      object_request("object.release", activePart));
  check(releasePart.ok);
  check(registry.get_info(active).kind ==
        kompas_bridge::ObjectKind::kDocument);
  registry.invalidate_all();
  const auto invalidAfterDisconnect = dispatcher.dispatch(
      object_request("object.get_info", active));
  check(!invalidAfterDisconnect.ok &&
        invalidAfterDisconnect.error->code == "object_invalidated");
  otherSession.invalidate_all();
  check(destroyed == 8);
}
