"""Document operations backed by the bridge port."""

from pathlib import Path

from kompas_bridge_transport import DocumentHandle, PartHandle

from kompas_core._internal.session import CoreSession
from kompas_core.common.errors import CoreObjectNotFoundError, CoreProtocolError
from kompas_core.documents.mapper import map_document, map_document_list, map_top_part
from kompas_core.documents.models import DocumentData


class DocumentService:
    """Execute low-level document operations for object facades."""

    def __init__(self, session: CoreSession) -> None:
        self._session = session

    def list(self) -> list[DocumentData]:
        return map_document_list(self._session.call("document.list", {}))

    def get_active(self) -> DocumentData:
        return map_document(self._session.call("document.get_active", {}))

    def create_3d(self, *, visible: bool) -> DocumentData:
        result = self._session.call("document.create_3d", {"visible": visible})
        return map_document(result)

    def open(self, file_path: Path, *, visible: bool, read_only: bool) -> DocumentData:
        result = self._session.call(
            "document.open",
            {
                "file_path": str(file_path),
                "visible": visible,
                "read_only": read_only,
            },
        )
        return map_document(result)

    def activate(self, handle: DocumentHandle) -> DocumentData:
        result = self._session.call(
            "document.activate",
            {"document_id": handle.value},
        )
        return map_document(result)

    def get_top_part(self, handle: DocumentHandle) -> PartHandle:
        result = self._session.call(
            "document.get_top_part",
            {"document_id": handle.value},
        )
        return map_top_part(result, handle.value)

    def save(self, handle: DocumentHandle) -> DocumentData:
        result = self._session.call(
            "document.save",
            {"document_id": handle.value},
        )
        return map_document(result)

    def save_as(
        self,
        handle: DocumentHandle,
        file_path: Path,
        *,
        overwrite: bool,
    ) -> DocumentData:
        result = self._session.call(
            "document.save_as",
            {
                "document_id": handle.value,
                "file_path": str(file_path),
                "overwrite": overwrite,
            },
        )
        return map_document(result)

    def close(self, handle: DocumentHandle, *, discard_changes: bool) -> None:
        result = self._session.call(
            "document.close",
            {
                "document_id": handle.value,
                "discard_changes": discard_changes,
            },
        )
        if result.get("document_id") != handle.value or result.get("closed") is not True:
            raise CoreProtocolError("Bridge did not confirm document closure.")

    def refresh(self, handle: DocumentHandle) -> DocumentData:
        for document in self.list():
            if document.handle == handle:
                return document
        raise CoreObjectNotFoundError(f"Document {handle.value!r} is not open.")
