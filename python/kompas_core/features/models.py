"""Feature input, state and bridge-mapping models."""

from dataclasses import dataclass
from enum import StrEnum
from math import isfinite
from numbers import Real

from kompas_bridge_transport import DocumentHandle, FeatureHandle


class ExtrusionDirection(StrEnum):
    """Direction used by an extrusion operation."""

    FORWARD = "forward"
    REVERSE = "reverse"
    BOTH = "both"


class BooleanOperation(StrEnum):
    """Boolean operation applied by a generated feature."""

    NEW_BODY = "new_body"
    JOIN = "join"
    CUT = "cut"
    INTERSECT = "intersect"


class FeatureKind(StrEnum):
    """Kinds of feature currently represented by Core."""

    EXTRUSION = "extrusion"
    SKETCH = "sketch"
    UNKNOWN = "unknown"


@dataclass(frozen=True, slots=True)
class FeatureInfo:
    """Inspection data for one item in the part feature tree."""

    name: str
    feature_type: str
    excluded: bool
    valid: bool
    owner_feature_id: str | None = None
    update_stamp: int = 0


@dataclass(frozen=True, slots=True)
class ExtrusionParameters:
    """Validated parameters for extruding a closed sketch."""

    distance: float
    direction: ExtrusionDirection = ExtrusionDirection.FORWARD
    operation: BooleanOperation = BooleanOperation.NEW_BODY

    def __post_init__(self) -> None:
        if isinstance(self.distance, bool) or not isinstance(self.distance, Real):
            raise ValueError("Extrusion distance must be a finite number.")
        if not isfinite(self.distance):
            raise ValueError("Extrusion distance must be a finite number.")
        if self.distance <= 0:
            raise ValueError("Extrusion distance must be positive.")
        if not isinstance(self.direction, ExtrusionDirection):
            raise ValueError("direction must be an ExtrusionDirection.")
        if not isinstance(self.operation, BooleanOperation):
            raise ValueError("operation must be a BooleanOperation.")


@dataclass(frozen=True, slots=True)
class FeatureState:
    """User-visible state of a generated feature."""

    kind: FeatureKind
    extrusion: ExtrusionParameters | None = None


@dataclass(frozen=True, slots=True)
class FeatureData:
    """Internal mapping result for a feature bridge response."""

    handle: FeatureHandle
    document_handle: DocumentHandle
    state: FeatureState
    info: FeatureInfo | None = None
