#include <WinSock2.h>

#include "kompas_bridge/kompas/kompas_document_service.hpp"

#include <filesystem>
#include <set>
#include <string>

#include <afxdisp.h>
#include <kapi5.h>
#include <kapi7.h>
#include <ksConstants.h>

#include "kompas_bridge/kompas/kompas_session.hpp"
#include "kompas_bridge/kompas/object_registry.hpp"
#include "kompas_bridge/protocol/codec.hpp"

namespace kompas_bridge {

namespace {

template <typename Function>
auto cad_call(Function&& function) -> decltype(function()) {
  try {
    return function();
  } catch (CException* error) {
    error->Delete();
    throw ProtocolError(
      "kompas_api_error",
      "KOMPAS document COM call failed."
    );
  }
}

std::wstring from_utf8(std::string_view text) {
  if (text.empty()) {
    throw ProtocolError(
      "invalid_params",
      "File path is empty."
    );
  }
  const int size = MultiByteToWideChar(
    CP_UTF8,
    MB_ERR_INVALID_CHARS,
    text.data(),
    static_cast<int>(text.size()),
    nullptr,
    0
  );
  if (size == 0) {
    throw ProtocolError(
      "invalid_params",
      "File path is not UTF-8."
    );
  }
  std::wstring result(
    size,
    L'\0'
  );
  MultiByteToWideChar(
    CP_UTF8,
    MB_ERR_INVALID_CHARS,
    text.data(),
    static_cast<int>(text.size()),
    result.data(),
    size
  );
  return result;
}

std::string to_utf8(const CString& value) {
  const std::wstring_view wide(
    value.GetString(),
    value.GetLength()
  );
  if (wide.empty()) {
    return {};
  }
  const int size = WideCharToMultiByte(
    CP_UTF8,
    WC_ERR_INVALID_CHARS,
    wide.data(),
    static_cast<int>(wide.size()),
    nullptr,
    0,
    nullptr,
    nullptr
  );
  if (size == 0) {
    throw ProtocolError(
      "kompas_api_error",
      "Invalid Unicode from KOMPAS."
    );
  }
  std::string result(
    size,
    '\0'
  );
  WideCharToMultiByte(
    CP_UTF8,
    WC_ERR_INVALID_CHARS,
    wide.data(),
    static_cast<int>(wide.size()),
    result.data(),
    size,
    nullptr,
    nullptr
  );
  return result;
}

std::filesystem::path checked_path(std::string_view text) {
  const std::filesystem::path path(from_utf8(text));
  if (!path.is_absolute()) {
    throw ProtocolError(
      "invalid_params",
      "File path must be absolute."
    );
  }
  return path;
}

}  // namespace

KompasDocumentService::KompasDocumentService(
  KompasSession& session,
  ObjectRegistry& registry
)
  : session_(session),
    registry_(registry) {}

void KompasDocumentService::require_connected() const {
  if (!session_.is_connected()) {
    throw ProtocolError(
      "kompas_not_running",
      "KOMPAS-3D is not connected."
    );
  }
}

DocumentInfo KompasDocumentService::info(IDispatch* dispatch) {
  IKompasDocument doc(dispatch);
  dispatch->AddRef();
  const std::string id = registry_.find_or_register_document(dispatch);
  const std::string path = to_utf8(doc.GetPathName());
  return {
    .documentId = id,
    .documentType = doc.GetDocumentType(),
    .name = to_utf8(doc.GetName()),
    .filePath = path.empty() ? std::nullopt
                             : std::optional<std::string>(path),
    .active = doc.GetActive() != FALSE,
    .changed = doc.GetChanged() != FALSE,
    .readOnly = doc.GetReadOnly() != FALSE
  };
}

Microsoft::WRL::ComPtr<IDispatch> KompasDocumentService::document(
  std::string_view id
) {
  sync_documents();
  auto object = registry_.get_object(
    id,
    ObjectKind::kDocument
  );
  Microsoft::WRL::ComPtr<IDispatch> dispatch;
  if (FAILED(object.As(&dispatch))) {
    throw ProtocolError(
      "kompas_api_error",
      "Document has no IDispatch interface."
    );
  }
  return dispatch;
}

void KompasDocumentService::sync_documents() {
  require_connected();
  IApplication app(session_.kompas_object().ksGetApplication7());
  IDocuments documents(app.GetDocuments());
  std::set<std::string> live;
  for (long index = 0; index < documents.GetCount(); ++index) {
    COleVariant item(index);
    IKompasDocument doc(documents.GetItem(item));
    if (doc.m_lpDispatch != nullptr) {
      live.insert(registry_.find_or_register_document(doc.m_lpDispatch));
    }
  }
  for (const auto& id : registry_.document_handles()) {
    if (!live.contains(id)) {
      registry_.invalidate_document(id);
    }
  }
}

std::vector<DocumentInfo> KompasDocumentService::list() {
  return cad_call([&] {
    sync_documents();
    IApplication app(session_.kompas_object().ksGetApplication7());
    IDocuments docs(app.GetDocuments());
    std::vector<DocumentInfo> result;
    for (long index = 0; index < docs.GetCount(); ++index) {
      COleVariant item(index);
      IKompasDocument doc(docs.GetItem(item));
      if (doc.m_lpDispatch != nullptr) {
        result.push_back(info(doc.m_lpDispatch));
      }
    }
    return result;
  });
}

std::optional<DocumentInfo> KompasDocumentService::get_active() {
  return cad_call([&]() -> std::optional<DocumentInfo> {
    sync_documents();
    IApplication app(session_.kompas_object().ksGetApplication7());
    IKompasDocument doc(app.GetActiveDocument());
    if (doc.m_lpDispatch == nullptr) {
      return std::nullopt;
    }
    return info(doc.m_lpDispatch);
  });
}

std::string KompasDocumentService::get_top_part(std::string_view documentId) {
  return cad_call([&] {
    auto dispatch = document(documentId);
    IKompasDocument doc(dispatch.Get());
    dispatch.Detach();
    const long type = doc.GetDocumentType();
    if (type != ksDocumentPart && type != ksDocumentAssembly) {
      throw ProtocolError(
        "invalid_params",
        "Document is not a 3D part or assembly."
      );
    }
    IKompasDocument3D doc3d(doc.m_lpDispatch);
    doc.m_lpDispatch->AddRef();
    COleDispatchDriver part(doc3d.GetTopPart());
    if (part.m_lpDispatch == nullptr) {
      throw ProtocolError(
        "kompas_api_error",
        "3D document has no top part."
      );
    }
    return registry_.find_or_register_child(
      ObjectKind::kPart,
      documentId,
      part.m_lpDispatch
    );
  });
}

DocumentResult KompasDocumentService::result(
  IDispatch* dispatch,
  bool isNew
) {
  DocumentInfo details = info(dispatch);
  std::optional<std::string> partId;
  if (details.documentType == ksDocumentPart ||
      details.documentType == ksDocumentAssembly) {
    partId = get_top_part(details.documentId);
  }
  return {
    .document = std::move(details),
    .partId = std::move(partId),
    .isNew = isNew
  };
}

DocumentResult KompasDocumentService::create_3d(bool visible) {
  return cad_call([&] {
    require_connected();
    IApplication app(session_.kompas_object().ksGetApplication7());
    IDocuments docs(app.GetDocuments());
    IDispatch* created = docs.Add(
      ksDocumentPart,
      visible ? TRUE : FALSE
    );
    IKompasDocument doc(created);
    if (doc.m_lpDispatch == nullptr) {
      throw ProtocolError(
        "kompas_api_error",
        "KOMPAS did not create a document."
      );
    }
    return result(
      doc.m_lpDispatch,
      true
    );
  });
}

DocumentResult KompasDocumentService::open(
  std::string_view path,
  bool visible,
  bool readOnly
) {
  return cad_call([&] {
    require_connected();
    const auto file = checked_path(path);
    if (!std::filesystem::is_regular_file(file)) {
      throw ProtocolError(
        "invalid_params",
        "Document file does not exist."
      );
    }
    IApplication app(session_.kompas_object().ksGetApplication7());
    IDocuments docs(app.GetDocuments());
    IDispatch* opened = docs.Open(
      file.c_str(),
      visible ? TRUE : FALSE,
      readOnly ? TRUE : FALSE
    );
    IKompasDocument doc(opened);
    if (doc.m_lpDispatch == nullptr) {
      throw ProtocolError(
        "kompas_api_error",
        "KOMPAS did not open the document."
      );
    }
    return result(
      doc.m_lpDispatch,
      false
    );
  });
}

DocumentInfo KompasDocumentService::activate(std::string_view documentId) {
  return cad_call([&] {
    auto dispatch = document(documentId);
    IKompasDocument doc(dispatch.Detach());
    doc.SetActive(TRUE);
    return info(doc.m_lpDispatch);
  });
}

DocumentInfo KompasDocumentService::save(std::string_view documentId) {
  return cad_call([&] {
    auto dispatch = document(documentId);
    IKompasDocument doc(dispatch.Detach());
    if (doc.GetReadOnly()) {
      throw ProtocolError(
        "invalid_params",
        "Document is read-only."
      );
    }
    if (doc.GetPathName().IsEmpty()) {
      throw ProtocolError(
        "invalid_params",
        "Unsaved document requires document.save_as."
      );
    }
    doc.Save();
    return info(doc.m_lpDispatch);
  });
}

DocumentInfo KompasDocumentService::save_as(
  std::string_view documentId,
  std::string_view path,
  bool overwrite
) {
  return cad_call([&] {
    const auto file = checked_path(path);
    if (std::filesystem::exists(file) && !overwrite) {
      throw ProtocolError(
        "invalid_params",
        "Target file exists; set overwrite=true explicitly."
      );
    }
    auto dispatch = document(documentId);
    IKompasDocument doc(dispatch.Detach());
    if (doc.GetReadOnly()) {
      throw ProtocolError(
        "invalid_params",
        "Document is read-only."
      );
    }
    doc.SaveAs(file.c_str());
    return info(doc.m_lpDispatch);
  });
}

void KompasDocumentService::close(
  std::string_view documentId,
  bool discardChanges
) {
  cad_call([&] {
    auto dispatch = document(documentId);
    IKompasDocument doc(dispatch.Detach());
    if (doc.GetChanged() && !discardChanges) {
      throw ProtocolError(
        "invalid_params",
        "Document has unsaved changes; save it or set discard_changes=true."
      );
    }
    if (!doc.Close(kdDoNotSaveChanges)) {
      throw ProtocolError(
        "kompas_api_error",
        "KOMPAS did not close the document."
      );
    }
    registry_.invalidate_document(documentId);
  });
}

}  // namespace kompas_bridge
