"""High-level KOMPAS application facade."""

from __future__ import annotations

from os import PathLike

from kompas_core._internal.session import CoreSession
from kompas_core.application.mapper import map_application_status
from kompas_core.application.models import ApplicationStatus
from kompas_core.common.types import ConnectionPolicy
from kompas_core.common.validation import validate_file_path
from kompas_core.documents.document import Document
from kompas_core.documents.models import DocumentData
from kompas_core.documents.service import DocumentService


class KompasApplication:
    """Public entry point for one KOMPAS Core session."""

    def __init__(self, session: CoreSession) -> None:
        self._session = session
        self._documents = DocumentService(session)

    @property
    def is_closed(self) -> bool:
        return self._session.is_closed

    def status(self) -> ApplicationStatus:
        """Return the current KOMPAS connection status."""
        return map_application_status(self._session.call("application.status", {}))

    def documents(self) -> list[Document]:
        """Return facades for all open KOMPAS documents."""
        return [self._make_document(data) for data in self._documents.list()]

    def active_document(self) -> Document:
        """Return the currently active KOMPAS document."""
        return self._make_document(self._documents.get_active())

    def create_document_3d(self, *, visible: bool = True) -> Document:
        """Create and return a new 3D document."""
        return self._make_document(self._documents.create_3d(visible=visible))

    def open_document(
        self,
        file_path: str | PathLike[str],
        *,
        visible: bool = True,
        read_only: bool = False,
    ) -> Document:
        """Open and return an existing KOMPAS document."""
        data = self._documents.open(
            validate_file_path(file_path),
            visible=visible,
            read_only=read_only,
        )
        return self._make_document(data)

    def close(self) -> None:
        """Close this Core session and its owned bridge client."""
        self._session.close()

    def __enter__(self) -> KompasApplication:
        return self

    def __exit__(self, *_: object) -> None:
        self.close()

    def _connect(self, policy: ConnectionPolicy) -> ApplicationStatus:
        result = self._session.call("application.connect", {"policy": policy})
        return map_application_status(result)

    def _make_document(self, data: DocumentData) -> Document:
        document = self._session.get_or_create_object(
            Document,
            data.handle.value,
            lambda: Document(self._session, self._documents, data),
            document_id=data.handle.value,
        )
        document._update(data)
        return document
