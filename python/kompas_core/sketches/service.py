"""Sketch operations backed by the bridge port."""

from kompas_bridge_transport import DocumentHandle, PartHandle, SketchHandle

from kompas_core._internal.session import CoreSession
from kompas_core.sketches.mapper import map_sketch
from kompas_core.sketches.models import (
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

    def close(self, handle: SketchHandle) -> SketchData:
        result = self._session.call(
            "sketch.close",
            {"sketch_id": handle.value},
        )
        return map_sketch(result)
