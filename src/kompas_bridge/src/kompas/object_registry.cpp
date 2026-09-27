#include "kompas_bridge/kompas/object_registry.hpp"

#include <Windows.h>

#include <format>
#include <stdexcept>
#include <utility>

#include "kompas_bridge/protocol/codec.hpp"

namespace kompas_bridge {

namespace {

std::string make_session_id() {
  GUID guid{};
  if (FAILED(CoCreateGuid(&guid))) {
    throw std::runtime_error("Failed to create a bridge session ID.");
  }
  return std::format(
    "{:08x}{:04x}{:04x}{:02x}{:02x}{:02x}{:02x}"
    "{:02x}{:02x}{:02x}{:02x}",
    guid.Data1,
    guid.Data2,
    guid.Data3,
    guid.Data4[0],
    guid.Data4[1],
    guid.Data4[2],
    guid.Data4[3],
    guid.Data4[4],
    guid.Data4[5],
    guid.Data4[6],
    guid.Data4[7]
  );
}

std::string_view kind_prefix(ObjectKind kind) {
  switch (kind) {
    case ObjectKind::kDocument:
      return "doc";
    case ObjectKind::kPart:
      return "part";
    case ObjectKind::kSketch:
      return "sketch";
    case ObjectKind::kFeature:
      return "feat";
  }
  throw std::invalid_argument("Unknown registry object kind.");
}

}  // namespace

std::string_view object_kind_name(ObjectKind kind) {
  switch (kind) {
    case ObjectKind::kDocument:
      return "document";
    case ObjectKind::kPart:
      return "part";
    case ObjectKind::kSketch:
      return "sketch";
    case ObjectKind::kFeature:
      return "feature";
  }
  throw std::invalid_argument("Unknown registry object kind.");
}

ObjectRegistry::ObjectRegistry()
  : ownerThread_(std::this_thread::get_id()),
    sessionId_(make_session_id()) {}

void ObjectRegistry::check_thread() const {
  if (std::this_thread::get_id() != ownerThread_) {
    throw std::logic_error(
      "ObjectRegistry must be used on its owner STA thread."
    );
  }
}

std::string ObjectRegistry::next_handle(ObjectKind kind) {
  if (nextId_ == 0) {
    throw std::overflow_error("Object handle counter exhausted.");
  }
  return std::format(
    "{}_{}_{}",
    kind_prefix(kind),
    sessionId_,
    nextId_++
  );
}

std::string ObjectRegistry::register_document(IUnknown* object) {
  check_thread();
  if (object == nullptr) {
    throw std::invalid_argument("Cannot register a null COM document.");
  }
  std::string handle = next_handle(ObjectKind::kDocument);
  Entry entry{
    .info = ObjectInfo{
      .handle = handle,
      .kind = ObjectKind::kDocument
    }
  };
  entry.object = object;
  entries_.emplace(
    handle,
    std::move(entry)
  );
  return handle;
}

std::string ObjectRegistry::find_or_register_document(IUnknown* object) {
  check_thread();
  if (object == nullptr) {
    throw std::invalid_argument("Null COM document.");
  }
  Microsoft::WRL::ComPtr<IUnknown> identity;
  if (FAILED(object->QueryInterface(IID_PPV_ARGS(&identity)))) {
    throw ProtocolError(
      "kompas_api_error",
      "Document has no COM identity."
    );
  }
  for (const auto& [handle, entry] : entries_) {
    if (!entry.invalidated && entry.info.kind == ObjectKind::kDocument &&
        entry.object.Get() == identity.Get()) {
      return handle;
    }
  }
  return register_document(identity.Get());
}

std::string ObjectRegistry::register_child(
  ObjectKind kind,
  std::string_view documentId,
  IUnknown* object
) {
  check_thread();
  if (kind == ObjectKind::kDocument || object == nullptr) {
    throw std::invalid_argument("Invalid child object registration.");
  }
  const Entry& document = find_entry(documentId);
  if (document.info.kind != ObjectKind::kDocument) {
    throw ProtocolError(
      "invalid_params",
      "Parent handle is not a document."
    );
  }
  std::string handle = next_handle(kind);
  Entry entry{
    .info = ObjectInfo{
      .handle = handle,
      .kind = kind,
      .documentId = std::string(documentId)
    }
  };
  entry.object = object;
  entries_.emplace(
    handle,
    std::move(entry)
  );
  return handle;
}

std::string ObjectRegistry::find_or_register_child(
  ObjectKind kind,
  std::string_view documentId,
  IUnknown* object
) {
  check_thread();
  if (object == nullptr) {
    throw std::invalid_argument("Null COM child.");
  }
  Microsoft::WRL::ComPtr<IUnknown> identity;
  if (FAILED(object->QueryInterface(IID_PPV_ARGS(&identity)))) {
    throw ProtocolError(
      "kompas_api_error",
      "Child has no COM identity."
    );
  }
  (void)get_object(
    documentId,
    ObjectKind::kDocument
  );
  for (const auto& [handle, entry] : entries_) {
    if (!entry.invalidated && entry.info.kind == kind &&
        entry.info.documentId == documentId &&
        entry.object.Get() == identity.Get()) {
      return handle;
    }
  }
  return register_child(
    kind,
    documentId,
    identity.Get()
  );
}

std::vector<std::string> ObjectRegistry::document_handles() const {
  check_thread();
  std::vector<std::string> result;
  for (const auto& [handle, entry] : entries_) {
    if (!entry.invalidated && entry.info.kind == ObjectKind::kDocument) {
      result.push_back(handle);
    }
  }
  return result;
}

const ObjectRegistry::Entry& ObjectRegistry::find_entry(
  std::string_view handle
) const {
  check_thread();
  const auto entry = entries_.find(std::string(handle));
  if (entry == entries_.end()) {
    throw ProtocolError(
      "object_not_found",
      "Object handle does not exist."
    );
  }
  if (entry->second.invalidated) {
    throw ProtocolError(
      "object_invalidated",
      "Object handle is invalidated."
    );
  }
  return entry->second;
}

ObjectRegistry::Entry& ObjectRegistry::find_entry(std::string_view handle) {
  return const_cast<Entry&>(std::as_const(*this).find_entry(handle));
}

ObjectInfo ObjectRegistry::get_info(std::string_view handle) const {
  return find_entry(handle).info;
}

Microsoft::WRL::ComPtr<IUnknown> ObjectRegistry::get_object(
  std::string_view handle,
  ObjectKind expectedKind
) const {
  const Entry& entry = find_entry(handle);
  if (entry.info.kind != expectedKind) {
    throw ProtocolError(
      "invalid_params",
      "Object handle has the wrong kind."
    );
  }
  return entry.object;
}

void ObjectRegistry::invalidate_entry(Entry& entry) {
  if (entry.invalidated) {
    return;
  }
  entry.object.Reset();
  entry.invalidated = true;
  tombstones_.push_back(entry.info.handle);
}

void ObjectRegistry::prune_tombstones() {
  constexpr std::size_t kMaxTombstones = 4096;
  while (tombstones_.size() > kMaxTombstones) {
    entries_.erase(tombstones_.front());
    tombstones_.pop_front();
  }
}

void ObjectRegistry::invalidate_document(std::string_view documentId) {
  check_thread();
  Entry& document = find_entry(documentId);
  if (document.info.kind != ObjectKind::kDocument) {
    throw ProtocolError(
      "invalid_params",
      "Handle is not a document."
    );
  }
  for (auto& [handle, entry] : entries_) {
    if (entry.info.documentId == documentId) {
      invalidate_entry(entry);
    }
  }
  invalidate_entry(document);
  prune_tombstones();
}

void ObjectRegistry::release(std::string_view handle) {
  check_thread();
  Entry& entry = find_entry(handle);
  if (entry.info.kind == ObjectKind::kDocument) {
    invalidate_document(handle);
  } else {
    invalidate_entry(entry);
    prune_tombstones();
  }
}

void ObjectRegistry::invalidate_all() {
  check_thread();
  for (auto& [handle, entry] : entries_) {
    invalidate_entry(entry);
  }
  prune_tombstones();
}

}  // namespace kompas_bridge
