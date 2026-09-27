"""Primitive solid recipes composed from public Core operations."""

from math import isfinite
from numbers import Real

from kompas_core.features.feature import Feature
from kompas_core.parts.part import Part


def create_cube(part: Part, size_mm: float) -> Feature:
    """Create a centered square sketch and extrude it into a cube."""
    if isinstance(size_mm, bool) or not isinstance(size_mm, Real):
        raise ValueError("Cube size must be a finite number.")
    if not isfinite(size_mm) or size_mm <= 0:
        raise ValueError("Cube size must be positive and finite.")
    sketch = part.create_sketch("xy")
    sketch.rectangle(float(size_mm), float(size_mm)).close()
    return part.extrude(sketch, distance=float(size_mm))
