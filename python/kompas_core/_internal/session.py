"""Shared Core session and object-lifecycle state."""

from __future__ import annotations

from collections.abc import Callable, Mapping
from typing import TypeVar, cast
from weakref import WeakValueDictionary

from kompas_bridge_transport import BridgeError

from kompas_core._internal.response import parse_result
from kompas_core.common.errors import (
    CoreObjectInvalidatedError,
    CoreSessionClosedError,
    translate_bridge_error,
)
from kompas_core.common.types import JsonObject
from kompas_core.ports.bridge import BridgeGateway


TObject = TypeVar("TObject", bound=object)


class CoreSession:
    """Coordinate all Core calls through one bridge session."""

    def __init__(self, gateway: BridgeGateway, *, owns_gateway: bool) -> None:
        self._gateway = gateway
        self._owns_gateway = owns_gateway
        self._closed = False
        self._objects: WeakValueDictionary[tuple[type[object], str], object] = (
            WeakValueDictionary()
        )
        self._document_children: dict[str, set[str]] = {}
        self._invalidated_handles: set[str] = set()

    @property
    def is_closed(self) -> bool:
        return self._closed

    def call(self, method: str, params: Mapping[str, object]) -> JsonObject:
        """Execute one bridge method and validate its result object."""
        if self._closed:
            raise CoreSessionClosedError("KOMPAS Core session is closed.")
        try:
            result = self._gateway.call(method, params)
        except BridgeError as error:
            raise translate_bridge_error(error) from error
        return parse_result(result, method)

    def get_or_create_object(
        self,
        object_type: type[TObject],
        handle: str,
        factory: Callable[[], TObject],
        *,
        document_id: str | None,
    ) -> TObject:
        """Return one cached facade or create it without retaining it strongly."""
        if self._closed:
            raise CoreSessionClosedError("KOMPAS Core session is closed.")
        if handle in self._invalidated_handles:
            raise CoreObjectInvalidatedError(f"Object {handle!r} is invalidated.")
        key = (cast(type[object], object_type), handle)
        cached = self._objects.get(key)
        if cached is not None:
            return cast(TObject, cached)
        instance = factory()
        self._objects[key] = instance
        if document_id is not None and document_id != handle:
            self._document_children.setdefault(document_id, set()).add(handle)
        return instance

    def invalidate_handle(self, handle: str) -> None:
        """Mark one bridge handle unusable in this Core session."""
        self._invalidated_handles.add(handle)

    def invalidate_document(self, document_id: str) -> None:
        """Invalidate a document and every known child facade."""
        self._invalidated_handles.add(document_id)
        self._invalidated_handles.update(self._document_children.pop(document_id, set()))

    def is_invalidated(self, handle: str) -> bool:
        return handle in self._invalidated_handles

    def ensure_same_session(self, *objects: object) -> None:
        """Reject CAD objects that belong to another Core session."""
        for item in objects:
            belongs_to = getattr(item, "_belongs_to", None)
            if not callable(belongs_to) or not belongs_to(self):
                raise ValueError("CAD objects belong to different Core sessions.")

    def close(self) -> None:
        """Close an owned gateway once and invalidate this session."""
        if self._closed:
            return
        self._closed = True
        self._invalidated_handles.update(handle for _, handle in self._objects.keys())
        if not self._owns_gateway:
            return
        try:
            self._gateway.close()
        except BridgeError as error:
            raise translate_bridge_error(error) from error
