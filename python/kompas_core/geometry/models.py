"""Typed values returned by model inspection operations."""

from dataclasses import dataclass
from enum import StrEnum

from kompas_bridge_transport import BodyHandle, FaceHandle, PartHandle


@dataclass(frozen=True, slots=True)
class Point3D:
    x: float
    y: float
    z: float


@dataclass(frozen=True, slots=True)
class BoundingBox3D:
    min: Point3D
    max: Point3D

    @property
    def center(self) -> Point3D:
        return Point3D(
            (self.min.x + self.max.x) / 2.0,
            (self.min.y + self.max.y) / 2.0,
            (self.min.z + self.max.z) / 2.0,
        )


class SurfaceType(StrEnum):
    PLANE = "plane"
    CYLINDER = "cylinder"
    CONE = "cone"
    SPHERE = "sphere"
    TORUS = "torus"
    NURBS = "nurbs"
    REVOLVED = "revolved"
    SWEPT = "swept"
    UNKNOWN = "unknown"


@dataclass(frozen=True, slots=True)
class BodyInfo:
    solid: bool
    bounding_box: BoundingBox3D
    owner_feature_id: str | None


@dataclass(frozen=True, slots=True)
class FaceGeometry:
    surface_type: SurfaceType
    area_mm2: float
    normal: Point3D | None
    bounding_box: BoundingBox3D
    radius_mm: float | None
    owner_feature_id: str | None
    body_id: str | None


@dataclass(frozen=True, slots=True)
class BodyData:
    handle: BodyHandle
    part_handle: PartHandle
    info: BodyInfo


@dataclass(frozen=True, slots=True)
class FaceData:
    handle: FaceHandle
    part_handle: PartHandle
    geometry: FaceGeometry


@dataclass(frozen=True, slots=True)
class MomentsOfInertia:
    jx: float
    jy: float
    jz: float
    jxy: float
    jxz: float
    jyz: float


@dataclass(frozen=True, slots=True)
class MassProperties:
    mass_kg: float
    volume_mm3: float
    area_mm2: float
    density_kg_m3: float
    center_of_mass: Point3D
    moments_of_inertia: MomentsOfInertia
