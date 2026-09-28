"""Typed measurement results."""

from dataclasses import dataclass

from kompas_core.geometry.models import Point3D


@dataclass(frozen=True, slots=True)
class DistanceMeasurement:
    distance_mm: float
    point1: Point3D
    point2: Point3D
    maximum_distance_mm: float | None
    maximum_point1: Point3D | None
    maximum_point2: Point3D | None
    normal_distance_mm: float | None
    normal_point1: Point3D | None
    normal_point2: Point3D | None


@dataclass(frozen=True, slots=True)
class AngleMeasurement:
    angle_degrees: float
