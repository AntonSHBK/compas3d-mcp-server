"""Sketch operations backed by the bridge port."""

from kompas_bridge_transport import DocumentHandle, PartHandle, SketchHandle

from kompas_core._internal.session import CoreSession
from kompas_core.common.errors import CoreProtocolError
from kompas_core.sketches.mapper import map_sketch
from kompas_core.sketches.models import (
    Circle,
    CreateSketchParameters,
    LineSegment,
    SketchData,
)


class SketchService:
    """Translate typed sketch operations into bridge calls."""

    def __init__(self, session: CoreSession) -> None:
        self._session = session

    def create(
        self,
        part_handle: PartHandle,
        document_handle: DocumentHandle,
        parameters: CreateSketchParameters,
    ) -> SketchData:
        result = self._session.call(
            "sketch.create",
            {
                "part_id": part_handle.value,
                "document_id": document_handle.value,
                "plane": parameters.plane.value,
            },
        )
        result = self._session.call(
            "sketch.begin_edit",
            {"sketch_id": _sketch_id(result)},
        )
        return map_sketch(result)

    def add_line(self, handle: SketchHandle, line: LineSegment) -> SketchData:
        result = self._session.call(
            "sketch.add_line",
            {
                "sketch_id": handle.value,
                "start": {"x": line.start.x, "y": line.start.y},
                "end": {"x": line.end.x, "y": line.end.y},
            },
        )
        return map_sketch(result)

    def add_circle(self, handle: SketchHandle, circle: Circle) -> SketchData:
        result = self._session.call(
            "sketch.add_circle",
            {
                "sketch_id": handle.value,
                "center": {"x": circle.center.x, "y": circle.center.y},
                "radius": circle.radius,
            },
        )
        return map_sketch(result)

    def close(self, handle: SketchHandle) -> SketchData:
        result = self._session.call(
            "sketch.end_edit",
            {"sketch_id": handle.value},
        )
        return map_sketch(result)


def _sketch_id(result: dict[str, object]) -> str:
    value = result.get("sketch_id")
    if not isinstance(value, str) or not value:
        raise CoreProtocolError("Bridge sketch.create response has no sketch_id.")
    return value
