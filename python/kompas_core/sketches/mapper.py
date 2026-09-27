"""Mapping between bridge responses and sketch models."""

from kompas_bridge_transport import DocumentHandle, SketchHandle

from kompas_core.common.errors import CoreProtocolError
from kompas_core.common.types import JsonObject
from kompas_core.common.validation import require_bool, require_non_negative_int
from kompas_core.sketches.models import SketchData, SketchState


def map_sketch(data: JsonObject) -> SketchData:
    """Map one sketch response to typed Core data."""
    sketch_id = _required_string(data, "sketch_id")
    document_id = _required_string(data, "document_id")
    geometry_value = data.get("geometry_count")
    geometry_count = None
    if geometry_value is not None:
        geometry_count = require_non_negative_int(geometry_value, "geometry_count")
    try:
        sketch_handle = SketchHandle(sketch_id)
        document_handle = DocumentHandle(document_id)
    except ValueError as error:
        raise CoreProtocolError("Sketch response contains an invalid handle.") from error
    return SketchData(
        handle=sketch_handle,
        document_handle=document_handle,
        state=SketchState(
            closed=require_bool(data.get("closed"), "closed"),
            geometry_count=geometry_count,
        ),
    )


def _required_string(data: JsonObject, field: str) -> str:
    value = data.get(field)
    if not isinstance(value, str) or not value:
        raise CoreProtocolError(f"{field} must be a nonempty string.")
    return value
