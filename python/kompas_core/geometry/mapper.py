"""Strict bridge-response mapping for model inspection."""

from kompas_bridge_transport import BodyHandle, FaceHandle, PartHandle

from kompas_core.common.errors import CoreProtocolError
from kompas_core.common.types import JsonObject
from kompas_core.geometry.models import (
    BodyData,
    BodyInfo,
    BoundingBox3D,
    FaceData,
    FaceGeometry,
    MassProperties,
    MomentsOfInertia,
    Point3D,
    SurfaceType,
)


def map_body(data: JsonObject) -> BodyData:
    try:
        return BodyData(
            handle=BodyHandle(_string(data, "body_id")),
            part_handle=PartHandle(_string(data, "part_id")),
            info=BodyInfo(
                solid=_boolean(data, "solid"),
                bounding_box=_box(_object(data, "bounding_box")),
                owner_feature_id=_optional_string(data, "owner_feature_id"),
            ),
        )
    except ValueError as error:
        raise CoreProtocolError("Body response contains an invalid handle.") from error


def map_face(data: JsonObject) -> FaceData:
    try:
        return FaceData(
            handle=FaceHandle(_string(data, "face_id")),
            part_handle=PartHandle(_string(data, "part_id")),
            geometry=FaceGeometry(
                surface_type=SurfaceType(_string(data, "surface_type")),
                area_mm2=_number(data, "area_mm2"),
                normal=_optional_point(data.get("normal")),
                bounding_box=_box(_object(data, "bounding_box")),
                radius_mm=_optional_number(data, "radius_mm"),
                owner_feature_id=_optional_string(data, "owner_feature_id"),
                body_id=_optional_string(data, "body_id"),
            ),
        )
    except ValueError as error:
        raise CoreProtocolError("Face response contains an invalid value.") from error


def map_mass_properties(data: JsonObject) -> MassProperties:
    moments = _object(data, "moments_of_inertia")
    return MassProperties(
        mass_kg=_number(data, "mass_kg"),
        volume_mm3=_number(data, "volume_mm3"),
        area_mm2=_number(data, "area_mm2"),
        density_kg_m3=_number(data, "density_kg_m3"),
        center_of_mass=_point(_object(data, "center_of_mass")),
        moments_of_inertia=MomentsOfInertia(
            jx=_number(moments, "jx"),
            jy=_number(moments, "jy"),
            jz=_number(moments, "jz"),
            jxy=_number(moments, "jxy"),
            jxz=_number(moments, "jxz"),
            jyz=_number(moments, "jyz"),
        ),
    )


def map_bounding_box(data: JsonObject) -> BoundingBox3D:
    return _box(data)


def _object(data: JsonObject, key: str) -> JsonObject:
    value = data.get(key)
    if not isinstance(value, dict):
        raise CoreProtocolError(f"{key} must be an object.")
    return value


def _string(data: JsonObject, key: str) -> str:
    value = data.get(key)
    if not isinstance(value, str) or not value:
        raise CoreProtocolError(f"{key} must be a nonempty string.")
    return value


def _optional_string(data: JsonObject, key: str) -> str | None:
    value = data.get(key)
    if value is not None and (not isinstance(value, str) or not value):
        raise CoreProtocolError(f"{key} must be null or a nonempty string.")
    return value


def _number(data: JsonObject, key: str) -> float:
    value = data.get(key)
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise CoreProtocolError(f"{key} must be numeric.")
    return float(value)


def _optional_number(data: JsonObject, key: str) -> float | None:
    return None if data.get(key) is None else _number(data, key)


def _boolean(data: JsonObject, key: str) -> bool:
    value = data.get(key)
    if not isinstance(value, bool):
        raise CoreProtocolError(f"{key} must be boolean.")
    return value


def _point(data: JsonObject) -> Point3D:
    return Point3D(_number(data, "x"), _number(data, "y"), _number(data, "z"))


def _optional_point(value: object) -> Point3D | None:
    if value is None:
        return None
    if not isinstance(value, dict):
        raise CoreProtocolError("normal must be null or an object.")
    return _point(value)


def _box(data: JsonObject) -> BoundingBox3D:
    return BoundingBox3D(_point(_object(data, "min")), _point(_object(data, "max")))
