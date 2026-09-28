"""Strict bridge-response mapping for measurements."""

from kompas_core.common.errors import CoreProtocolError
from kompas_core.common.types import JsonObject
from kompas_core.geometry.models import Point3D
from kompas_core.measurements.models import AngleMeasurement, DistanceMeasurement


def map_distance(data: JsonObject) -> DistanceMeasurement:
    return DistanceMeasurement(
        distance_mm=_number(data, "distance_mm"),
        point1=_point(_object(data, "point1")),
        point2=_point(_object(data, "point2")),
        maximum_distance_mm=_optional_number(data, "maximum_distance_mm"),
        maximum_point1=_optional_point(data.get("maximum_point1")),
        maximum_point2=_optional_point(data.get("maximum_point2")),
        normal_distance_mm=_optional_number(data, "normal_distance_mm"),
        normal_point1=_optional_point(data.get("normal_point1")),
        normal_point2=_optional_point(data.get("normal_point2")),
    )


def map_angle(data: JsonObject) -> AngleMeasurement:
    return AngleMeasurement(angle_degrees=_number(data, "angle_degrees"))


def _object(data: JsonObject, key: str) -> JsonObject:
    value = data.get(key)
    if not isinstance(value, dict):
        raise CoreProtocolError(f"{key} must be an object.")
    return value


def _number(data: JsonObject, key: str) -> float:
    value = data.get(key)
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise CoreProtocolError(f"{key} must be numeric.")
    return float(value)


def _optional_number(data: JsonObject, key: str) -> float | None:
    return None if data.get(key) is None else _number(data, key)


def _point(data: JsonObject) -> Point3D:
    return Point3D(_number(data, "x"), _number(data, "y"), _number(data, "z"))


def _optional_point(value: object) -> Point3D | None:
    if value is None:
        return None
    if not isinstance(value, dict):
        raise CoreProtocolError("Measurement point must be null or an object.")
    return _point(value)
