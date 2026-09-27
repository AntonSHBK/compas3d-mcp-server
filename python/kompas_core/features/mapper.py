"""Mapping between bridge responses and feature models."""

from kompas_bridge_transport import DocumentHandle, FeatureHandle

from kompas_core.common.errors import CoreProtocolError
from kompas_core.common.types import JsonObject
from kompas_core.features.models import (
    BooleanOperation,
    ExtrusionDirection,
    ExtrusionParameters,
    FeatureData,
    FeatureKind,
    FeatureState,
)


def map_feature(data: JsonObject) -> FeatureData:
    """Map one feature response to typed Core data."""
    feature_id = _required_string(data, "feature_id")
    document_id = _required_string(data, "document_id")
    kind_value = _required_string(data, "kind")
    try:
        feature_handle = FeatureHandle(feature_id)
        document_handle = DocumentHandle(document_id)
        kind = FeatureKind(kind_value)
        extrusion = None
        if kind is FeatureKind.EXTRUSION:
            distance = data.get("distance")
            direction = data.get("direction")
            operation = data.get("operation")
            if not isinstance(distance, (int, float)) or isinstance(distance, bool):
                raise ValueError("distance must be numeric")
            extrusion = ExtrusionParameters(
                distance=float(distance),
                direction=ExtrusionDirection(direction),
                operation=BooleanOperation(operation),
            )
    except ValueError as error:
        raise CoreProtocolError("Feature response contains an invalid value.") from error
    return FeatureData(
        handle=feature_handle,
        document_handle=document_handle,
        state=FeatureState(kind=kind, extrusion=extrusion),
    )


def _required_string(data: JsonObject, field: str) -> str:
    value = data.get(field)
    if not isinstance(value, str) or not value:
        raise CoreProtocolError(f"{field} must be a nonempty string.")
    return value
