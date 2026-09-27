"""Common object model for KOMPAS Core CAD entities."""

from __future__ import annotations

from dataclasses import dataclass
from typing import ClassVar

from kompas_bridge_transport import BridgeHandle

from kompas_core._internal.session import CoreSession
from kompas_core.common.errors import CoreObjectInvalidatedError, CoreProtocolError
from kompas_core.common.types import ObjectKind


@dataclass(frozen=True, slots=True)
class ObjectInfo:
    """Typed diagnostic information returned by object.get_info."""

    id: str
    kind: ObjectKind
    document_id: str | None


class KompasObject:
    """Base class for a CAD object owned by one Core session."""

    object_kind: ClassVar[ObjectKind]

    def __init__(
        self,
        session: CoreSession,
        handle: BridgeHandle,
        *,
        document_id: str | None,
    ) -> None:
        self._session = session
        self._handle = handle
        self._document_id = document_id
        self._released = False
        self._info: ObjectInfo | None = None

    @property
    def id(self) -> str:
        return self._handle.value

    @property
    def document_id(self) -> str | None:
        return self._document_id

    @property
    def is_released(self) -> bool:
        return self._released

    @property
    def info(self) -> ObjectInfo | None:
        return self._info

    def get_info(self) -> ObjectInfo:
        """Load typed diagnostic information from the bridge registry."""
        self._ensure_usable()
        result = self._session.call("object.get_info", {"handle": self.id})
        handle = result.get("handle")
        kind = result.get("kind")
        document_id = result.get("document_id")
        if handle != self.id:
            raise CoreProtocolError("Object info response contains another handle.")
        if kind != self.object_kind:
            raise CoreProtocolError("Object info response contains an unexpected kind.")
        if document_id is not None and not isinstance(document_id, str):
            raise CoreProtocolError("Object document_id must be null or a string.")
        if document_id != self._document_id:
            raise CoreProtocolError("Object info response contains another document_id.")
        self._info = ObjectInfo(
            id=self.id,
            kind=self.object_kind,
            document_id=document_id,
        )
        return self._info

    def refresh(self) -> KompasObject:
        """Validate this object against the bridge registry."""
        self.get_info()
        return self

    def release(self) -> None:
        """Release the bridge handle once and invalidate this facade."""
        if self._released:
            return
        if self._session.is_invalidated(self.id):
            self._released = True
            return
        self._ensure_usable()
        result = self._session.call("object.release", {"handle": self.id})
        if result.get("released") is not True:
            raise CoreProtocolError("Bridge did not confirm object release.")
        self._released = True
        if self.object_kind == "document":
            self._session.invalidate_document(self.id)
        else:
            self._session.invalidate_handle(self.id)

    def _ensure_usable(self) -> None:
        if self._released or self._session.is_invalidated(self.id):
            raise CoreObjectInvalidatedError(f"Object {self.id!r} is invalidated.")

    def _belongs_to(self, session: CoreSession) -> bool:
        return self._session is session

    def _ensure_same_session(self, *others: KompasObject) -> None:
        self._session.ensure_same_session(self, *others)

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, KompasObject):
            return NotImplemented
        return self._session is other._session and self.id == other.id

    def __hash__(self) -> int:
        return hash((id(self._session), self.id))

    def __repr__(self) -> str:
        return f"{type(self).__name__}(id={self.id!r})"
