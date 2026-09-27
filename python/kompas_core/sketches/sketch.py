"""High-level KOMPAS sketch object."""

from __future__ import annotations

from typing import ClassVar

from kompas_core._internal.session import CoreSession
from kompas_core.common.errors import CoreProtocolError
from kompas_core.common.types import ObjectKind
from kompas_core.objects import KompasObject
from kompas_core.sketches.models import Circle, LineSegment, Point2D, SketchData, SketchState
from kompas_core.sketches.service import SketchService


class Sketch(KompasObject):
    """High-level facade for one KOMPAS sketch."""

    object_kind: ClassVar[ObjectKind] = "sketch"

    def __init__(
        self,
        session: CoreSession,
        service: SketchService,
        data: SketchData,
    ) -> None:
        super().__init__(session, data.handle, document_id=data.document_handle.value)
        self._service = service
        self._state = data.state

    @property
    def state(self) -> SketchState:
        return self._state

    def add_line(self, start: Point2D, end: Point2D) -> Sketch:
        """Add one validated line segment to this open sketch."""
        self._ensure_usable()
        if self._state.closed:
            raise ValueError("Cannot add geometry to a closed sketch.")
        self._update(self._service.add_line(self._handle, LineSegment(start, end)))
        return self

    def close(self) -> Sketch:
        """Finish sketch editing while keeping the sketch object valid."""
        self._ensure_usable()
        if self._state.closed:
            return self
        self._update(self._service.close(self._handle))
        if not self._state.closed:
            raise CoreProtocolError("Bridge did not close the sketch.")
        return self

    def add_circle(self, center: Point2D, radius: float) -> Sketch:
        """Add one circle to this open sketch."""
        self._ensure_usable()
        if self._state.closed:
            raise ValueError("Cannot add geometry to a closed sketch.")
        self._update(self._service.add_circle(self._handle, Circle(center, radius)))
        return self

    def rectangle(self, width: float, height: float) -> Sketch:
        """Add a centered axis-aligned rectangle and return this sketch."""
        if width <= 0 or height <= 0:
            raise ValueError("Rectangle width and height must be positive.")
        half_width = width / 2.0
        half_height = height / 2.0
        points = (
            Point2D(-half_width, -half_height),
            Point2D(half_width, -half_height),
            Point2D(half_width, half_height),
            Point2D(-half_width, half_height),
        )
        for index, start in enumerate(points):
            self.add_line(start, points[(index + 1) % len(points)])
        return self

    def _update(self, data: SketchData) -> None:
        if data.handle != self._handle or data.document_handle.value != self.document_id:
            raise CoreProtocolError("Bridge returned another sketch object.")
        self._state = data.state
