"""Sketch input, state and bridge-mapping models."""

from __future__ import annotations

from dataclasses import dataclass
from enum import StrEnum
from math import isfinite
from numbers import Real

from kompas_bridge_transport import DocumentHandle, SketchHandle


class SketchPlane(StrEnum):
    """Standard reference plane for a new sketch."""

    XY = "XY"
    YZ = "YZ"
    ZX = "ZX"


@dataclass(frozen=True, slots=True)
class Point2D:
    """Finite point in sketch coordinates, measured in millimetres."""

    x: float
    y: float

    def __post_init__(self) -> None:
        if (
            isinstance(self.x, bool)
            or isinstance(self.y, bool)
            or not isinstance(self.x, Real)
            or not isinstance(self.y, Real)
        ):
            raise ValueError("Point coordinates must be numbers.")
        if not isfinite(self.x) or not isfinite(self.y):
            raise ValueError("Point coordinates must be finite.")


@dataclass(frozen=True, slots=True)
class LineSegment:
    """Validated line segment added to a sketch."""

    start: Point2D
    end: Point2D

    def __post_init__(self) -> None:
        if not isinstance(self.start, Point2D) or not isinstance(self.end, Point2D):
            raise ValueError("Line endpoints must be Point2D values.")
        if self.start == self.end:
            raise ValueError("Line segment endpoints must be different.")


@dataclass(frozen=True, slots=True)
class CreateSketchParameters:
    """Parameters for creating a sketch on a standard plane."""

    plane: SketchPlane


@dataclass(frozen=True, slots=True)
class SketchState:
    """User-visible state of a sketch editing session."""

    closed: bool
    geometry_count: int | None = None


@dataclass(frozen=True, slots=True)
class SketchData:
    """Internal mapping result for a sketch bridge response."""

    handle: SketchHandle
    document_handle: DocumentHandle
    state: SketchState


def parse_sketch_plane(value: SketchPlane | str) -> SketchPlane:
    """Normalize a public standard-plane value."""
    if isinstance(value, SketchPlane):
        return value
    if not isinstance(value, str):
        raise ValueError("Sketch plane must be a SketchPlane or string.")
    try:
        return SketchPlane(value.upper())
    except ValueError as error:
        raise ValueError(f"Unsupported sketch plane: {value!r}.") from error
