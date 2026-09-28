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
    FeatureInfo,
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


def map_feature_info(data: JsonObject) -> FeatureData:
    """Map a feature inspection response without requiring operation parameters."""
    feature_id = _required_string(data, "feature_id")
    document_id = _required_string(data, "document_id")
    feature_type = _required_string(data, "feature_type")
    name = _required_string(data, "name")
    excluded = data.get("excluded")
    valid = data.get("valid")
    owner = data.get("owner_feature_id")
    update_stamp = data.get("update_stamp", 0)
    if not isinstance(excluded, bool) or not isinstance(valid, bool):
        raise CoreProtocolError("Feature flags must be boolean values.")
    if owner is not None and (not isinstance(owner, str) or not owner):
        raise CoreProtocolError("owner_feature_id must be null or a nonempty string.")
    if not isinstance(update_stamp, int) or isinstance(update_stamp, bool) or update_stamp < 0:
        raise CoreProtocolError("update_stamp must be a nonnegative integer.")
    try:
        kind = FeatureKind(feature_type)
    except ValueError:
        kind = FeatureKind.UNKNOWN
    try:
        handle = FeatureHandle(feature_id)
        document_handle = DocumentHandle(document_id)
    except ValueError as error:
        raise CoreProtocolError("Feature response contains an invalid handle.") from error
    return FeatureData(
        handle=handle,
        document_handle=document_handle,
        state=FeatureState(kind=kind),
        info=FeatureInfo(
            name=name,
            feature_type=feature_type,
            excluded=excluded,
            valid=valid,
            owner_feature_id=owner,
            update_stamp=update_stamp,
        ),
    )


def _required_string(data: JsonObject, field: str) -> str:
    value = data.get(field)
    if not isinstance(value, str) or not value:
        raise CoreProtocolError(f"{field} must be a nonempty string.")
    return value
