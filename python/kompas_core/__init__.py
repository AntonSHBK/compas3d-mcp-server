"""High-level Python API for KOMPAS-3D automation."""

from kompas_core.application import ApplicationStatus, KompasApplication, KompasVersion
from kompas_core.common.errors import (
    CoreBridgeError,
    CoreConnectionError,
    CoreError,
    CoreNoActiveDocumentError,
    CoreObjectInvalidatedError,
    CoreObjectNotFoundError,
    CoreProcessError,
    CoreProtocolError,
    CoreRemoteError,
    CoreSessionClosedError,
    CoreTimeoutError,
)
from kompas_core.connection import connect
from kompas_core.documents import Document, DocumentState
from kompas_core.features import (
    BooleanOperation,
    ExtrusionDirection,
    ExtrusionParameters,
    Feature,
    FeatureKind,
    FeatureState,
)
from kompas_core.objects import KompasObject, ObjectInfo
from kompas_core.parts import Part
from kompas_core.sketches import LineSegment, Point2D, Sketch, SketchPlane, SketchState

__all__ = [
    "ApplicationStatus",
    "BooleanOperation",
    "CoreBridgeError",
    "CoreConnectionError",
    "CoreError",
    "CoreNoActiveDocumentError",
    "CoreObjectInvalidatedError",
    "CoreObjectNotFoundError",
    "CoreProcessError",
    "CoreProtocolError",
    "CoreRemoteError",
    "CoreSessionClosedError",
    "CoreTimeoutError",
    "Document",
    "DocumentState",
    "ExtrusionDirection",
    "ExtrusionParameters",
    "Feature",
    "FeatureKind",
    "FeatureState",
    "KompasObject",
    "KompasApplication",
    "KompasVersion",
    "LineSegment",
    "ObjectInfo",
    "Part",
    "Point2D",
    "Sketch",
    "SketchPlane",
    "SketchState",
    "connect",
]
