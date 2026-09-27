"""High-level KOMPAS part object."""

from __future__ import annotations

from typing import ClassVar

from kompas_bridge_transport import DocumentHandle, PartHandle

from kompas_core._internal.session import CoreSession
from kompas_core.common.errors import CoreProtocolError
from kompas_core.common.types import ObjectKind
from kompas_core.features.feature import Feature
from kompas_core.features.models import (
    BooleanOperation,
    ExtrusionDirection,
    ExtrusionParameters,
)
from kompas_core.features.service import FeatureService
from kompas_core.objects import KompasObject
from kompas_core.sketches.models import (
    CreateSketchParameters,
    SketchPlane,
    parse_sketch_plane,
)
from kompas_core.sketches.service import SketchService
from kompas_core.sketches.sketch import Sketch


class Part(KompasObject):
    """High-level facade for one KOMPAS part."""

    object_kind: ClassVar[ObjectKind] = "part"

    def __init__(
        self,
        session: CoreSession,
        handle: PartHandle,
        document_handle: DocumentHandle,
    ) -> None:
        super().__init__(session, handle, document_id=document_handle.value)
        self._document_handle = document_handle
        self._sketches = SketchService(session)
        self._features = FeatureService(session)

    @property
    def document_id(self) -> str:
        return self._document_handle.value

    def create_sketch(self, plane: SketchPlane | str) -> Sketch:
        """Create a sketch facade on a standard reference plane."""
        self._ensure_usable()
        parameters = CreateSketchParameters(plane=parse_sketch_plane(plane))
        data = self._sketches.create(self._handle, self._document_handle, parameters)
        if data.document_handle != self._document_handle:
            raise CoreProtocolError("Created sketch belongs to another document.")
        sketch = self._session.get_or_create_object(
            Sketch,
            data.handle.value,
            lambda: Sketch(self._session, self._sketches, data),
            document_id=self.document_id,
        )
        sketch._update(data)
        return sketch

    def extrude(
        self,
        sketch: Sketch,
        *,
        distance: float,
        direction: ExtrusionDirection = ExtrusionDirection.FORWARD,
        operation: BooleanOperation = BooleanOperation.NEW_BODY,
    ) -> Feature:
        """Create an extrusion feature from a closed sketch."""
        self._ensure_usable()
        self._ensure_same_session(sketch)
        sketch._ensure_usable()
        if sketch.document_id != self.document_id:
            raise ValueError("Sketch and part belong to different documents.")
        if not sketch.state.closed:
            raise ValueError("Sketch must be closed before extrusion.")
        parameters = ExtrusionParameters(
            distance=distance,
            direction=direction,
            operation=operation,
        )
        data = self._features.extrude(
            self._handle,
            self._document_handle,
            sketch._handle,
            parameters,
        )
        if data.document_handle != self._document_handle:
            raise CoreProtocolError("Created feature belongs to another document.")
        feature = self._session.get_or_create_object(
            Feature,
            data.handle.value,
            lambda: Feature(self._session, self._features, data),
            document_id=self.document_id,
        )
        feature._update(data)
        return feature

    def rebuild(self) -> Part:
        """Rebuild this part after feature changes."""
        self._ensure_usable()
        self._features.rebuild(self._handle)
        return self
