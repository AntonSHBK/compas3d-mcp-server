"""High-level KOMPAS document object."""

from __future__ import annotations

from os import PathLike
from pathlib import Path
from typing import ClassVar

from kompas_core._internal.session import CoreSession
from kompas_core.common.errors import CoreObjectInvalidatedError
from kompas_core.common.types import ObjectKind
from kompas_core.common.validation import validate_file_path
from kompas_core.documents.models import DocumentData, DocumentState
from kompas_core.documents.service import DocumentService
from kompas_core.objects import KompasObject
from kompas_core.parts.part import Part


class Document(KompasObject):
    """High-level facade for one open KOMPAS document."""

    object_kind: ClassVar[ObjectKind] = "document"

    def __init__(
        self,
        session: CoreSession,
        service: DocumentService,
        data: DocumentData,
    ) -> None:
        super().__init__(session, data.handle, document_id=None)
        self._service = service
        self._state = data.state
        self._top_part_handle = data.top_part_handle
        self._closed = False

    @property
    def state(self) -> DocumentState:
        return self._state

    @property
    def name(self) -> str:
        return self._state.name

    @property
    def file_path(self) -> Path | None:
        return self._state.file_path

    @property
    def is_closed(self) -> bool:
        return self._closed

    def activate(self) -> Document:
        """Make this document active and refresh its state."""
        self._ensure_open()
        self._update(self._service.activate(self._handle))
        return self

    def top_part(self) -> Part:
        """Return the top-level part of this 3D document."""
        self._ensure_open()
        if self._top_part_handle is None:
            self._top_part_handle = self._service.get_top_part(self._handle)
        handle = self._top_part_handle
        return self._session.get_or_create_object(
            Part,
            handle.value,
            lambda: Part(self._session, handle, self._handle),
            document_id=self.id,
        )

    def save(self) -> Document:
        """Save this document to its current file path."""
        self._ensure_open()
        self._update(self._service.save(self._handle))
        return self

    def save_as(
        self,
        file_path: str | PathLike[str],
        *,
        overwrite: bool = False,
    ) -> Document:
        """Save this document under another file path."""
        self._ensure_open()
        path = validate_file_path(file_path)
        self._update(
            self._service.save_as(
                self._handle,
                path,
                overwrite=overwrite,
            )
        )
        return self

    def close(self, *, discard_changes: bool = False) -> None:
        """Close this document and invalidate this facade."""
        if self._closed:
            return
        self._service.close(self._handle, discard_changes=discard_changes)
        self._closed = True
        self._session.invalidate_document(self.id)

    def refresh(self) -> Document:
        """Reload this document state from the list of open documents."""
        self._ensure_open()
        self._update(self._service.refresh(self._handle))
        return self

    def _ensure_open(self) -> None:
        if self._closed:
            raise CoreObjectInvalidatedError(f"Document {self.id!r} is closed.")
        self._ensure_usable()

    def _update(self, data: DocumentData) -> None:
        if data.handle != self._handle:
            raise CoreObjectInvalidatedError("Bridge returned another document handle.")
        self._state = data.state
        if data.top_part_handle is not None:
            self._top_part_handle = data.top_part_handle
