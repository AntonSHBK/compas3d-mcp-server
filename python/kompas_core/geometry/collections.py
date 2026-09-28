"""Selectors for bodies and faces."""

from __future__ import annotations

from collections.abc import Iterator, Sequence
from math import dist, isfinite

from kompas_core.common.errors import (
    CoreSelectionAmbiguousError,
    CoreSelectionNotFoundError,
)
from kompas_core.features.feature import Feature
from kompas_core.geometry.body import Body
from kompas_core.geometry.face import Face
from kompas_core.geometry.models import Point3D, SurfaceType


class BodyCollection(Sequence[Body]):
    def __init__(self, values: Sequence[Body]) -> None:
        self._values = tuple(values)

    def __len__(self) -> int:
        return len(self._values)

    def __getitem__(self, index: int | slice) -> Body | tuple[Body, ...]:
        return self._values[index]

    def __iter__(self) -> Iterator[Body]:
        return iter(self._values)

    def all(self) -> list[Body]:
        return list(self._values)

    def first(self) -> Body:
        if not self._values:
            raise CoreSelectionNotFoundError("Body selector matched no objects.")
        return self._values[0]

    def one(self) -> Body:
        if len(self._values) != 1:
            if not self._values:
                raise CoreSelectionNotFoundError("Body selector matched no objects.")
            raise CoreSelectionAmbiguousError(
                f"Body selector matched {len(self._values)} objects."
            )
        return self._values[0]


class FaceCollection(Sequence[Face]):
    def __init__(self, values: Sequence[Face]) -> None:
        self._values = tuple(values)

    def __len__(self) -> int:
        return len(self._values)

    def __getitem__(self, index: int | slice) -> Face | tuple[Face, ...]:
        return self._values[index]

    def __iter__(self) -> Iterator[Face]:
        return iter(self._values)

    def all(self) -> list[Face]:
        return list(self._values)

    def first(self) -> Face:
        if not self._values:
            raise CoreSelectionNotFoundError("Face selector matched no objects.")
        return self._values[0]

    def one(self) -> Face:
        if len(self._values) != 1:
            if not self._values:
                raise CoreSelectionNotFoundError("Face selector matched no objects.")
            raise CoreSelectionAmbiguousError(
                f"Face selector matched {len(self._values)} objects."
            )
        return self._values[0]

    def planar(self) -> FaceCollection:
        return self.of_type(SurfaceType.PLANE)

    def cylindrical(self) -> FaceCollection:
        return self.of_type(SurfaceType.CYLINDER)

    def of_type(self, surface_type: SurfaceType | str) -> FaceCollection:
        try:
            expected = SurfaceType(surface_type)
        except ValueError as error:
            raise ValueError(f"Unsupported surface type: {surface_type!r}.") from error
        return FaceCollection(
            [face for face in self if face.geometry.surface_type is expected]
        )

    def normal(self, direction: str, *, tolerance: float = 1e-6) -> FaceCollection:
        expected = _axis_direction(direction)
        _validate_tolerance(tolerance)
        return FaceCollection(
            [
                face
                for face in self
                if face.geometry.normal is not None
                and _point_close(face.geometry.normal, expected, tolerance)
            ]
        )

    def area_between(self, minimum: float, maximum: float) -> FaceCollection:
        if not isfinite(minimum) or not isfinite(maximum) or minimum < 0:
            raise ValueError("Area limits must be finite and nonnegative.")
        if minimum > maximum:
            raise ValueError("Minimum area cannot exceed maximum area.")
        return FaceCollection(
            [face for face in self if minimum <= face.geometry.area_mm2 <= maximum]
        )

    def by_radius(self, radius: float, *, tolerance: float = 1e-6) -> FaceCollection:
        if not isfinite(radius) or radius <= 0:
            raise ValueError("Radius must be finite and positive.")
        _validate_tolerance(tolerance)
        return FaceCollection(
            [
                face
                for face in self
                if face.geometry.radius_mm is not None
                and abs(face.geometry.radius_mm - radius) <= tolerance
            ]
        )

    def by_owner(self, feature: Feature) -> FaceCollection:
        if not isinstance(feature, Feature):
            raise TypeError("feature must be a Feature object.")
        return FaceCollection(
            [face for face in self if face.geometry.owner_feature_id == feature.id]
        )

    def largest(self) -> Face:
        if not self._values:
            raise CoreSelectionNotFoundError("Face selector matched no objects.")
        return max(self._values, key=lambda face: face.geometry.area_mm2)

    def smallest(self) -> Face:
        if not self._values:
            raise CoreSelectionNotFoundError("Face selector matched no objects.")
        return min(self._values, key=lambda face: face.geometry.area_mm2)

    def nearest_to(self, point: Point3D) -> Face:
        if not isinstance(point, Point3D):
            raise TypeError("point must be a Point3D object.")
        if not self._values:
            raise CoreSelectionNotFoundError("Face selector matched no objects.")
        target = (point.x, point.y, point.z)
        return min(
            self._values,
            key=lambda face: dist(
                target,
                (
                    face.geometry.bounding_box.center.x,
                    face.geometry.bounding_box.center.y,
                    face.geometry.bounding_box.center.z,
                ),
            ),
        )


def _axis_direction(value: str) -> Point3D:
    directions = {
        "+x": Point3D(1.0, 0.0, 0.0),
        "-x": Point3D(-1.0, 0.0, 0.0),
        "+y": Point3D(0.0, 1.0, 0.0),
        "-y": Point3D(0.0, -1.0, 0.0),
        "+z": Point3D(0.0, 0.0, 1.0),
        "-z": Point3D(0.0, 0.0, -1.0),
    }
    try:
        return directions[value.lower()]
    except (AttributeError, KeyError) as error:
        raise ValueError("Direction must be one of +x, -x, +y, -y, +z, -z.") from error


def _validate_tolerance(value: float) -> None:
    if not isfinite(value) or value < 0:
        raise ValueError("Tolerance must be finite and nonnegative.")


def _point_close(actual: Point3D, expected: Point3D, tolerance: float) -> bool:
    return (
        abs(actual.x - expected.x) <= tolerance
        and abs(actual.y - expected.y) <= tolerance
        and abs(actual.z - expected.z) <= tolerance
    )
