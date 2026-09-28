"""High-level KOMPAS part object."""

from __future__ import annotations

from typing import ClassVar

from kompas_bridge_transport import DocumentHandle, PartHandle

from kompas_core._internal.session import CoreSession
from kompas_core.common.errors import CoreProtocolError
from kompas_core.common.types import ObjectKind
from kompas_core.features.feature import Feature
from kompas_core.features.collection import FeatureCollection
from kompas_core.features.models import (
    BooleanOperation,
    ExtrusionDirection,
    ExtrusionParameters,
)
from kompas_core.features.service import FeatureService
from kompas_core.geometry.body import Body
from kompas_core.geometry.collections import BodyCollection, FaceCollection
from kompas_core.geometry.face import Face
from kompas_core.geometry.models import BoundingBox3D, MassProperties
from kompas_core.geometry.service import InspectionService
from kompas_core.objects import KompasObject
from kompas_core.sketches.models import (
    CreateSketchParameters,
    SketchPlane,
    parse_sketch_plane,
)
from kompas_core.sketches.service import SketchService
from kompas_core.sketches.sketch import Sketch


class Part(KompasObject):
    """High-level facade for one KOMPAS part."""

    object_kind: ClassVar[ObjectKind] = "part"

    def __init__(
        self,
        session: CoreSession,
        handle: PartHandle,
        document_handle: DocumentHandle,
    ) -> None:
        super().__init__(session, handle, document_id=document_handle.value)
        self._document_handle = document_handle
        self._sketches = SketchService(session)
        self._feature_service = FeatureService(session)
        self._inspection_service = InspectionService(session)

    @property
    def document_id(self) -> str:
        return self._document_handle.value

    def create_sketch(self, plane: SketchPlane | str) -> Sketch:
        """Create a sketch facade on a standard reference plane."""
        self._ensure_usable()
        parameters = CreateSketchParameters(plane=parse_sketch_plane(plane))
        data = self._sketches.create(self._handle, self._document_handle, parameters)
        if data.document_handle != self._document_handle:
            raise CoreProtocolError("Created sketch belongs to another document.")
        sketch = self._session.get_or_create_object(
            Sketch,
            data.handle.value,
            lambda: Sketch(self._session, self._sketches, data),
            document_id=self.document_id,
        )
        sketch._update(data)
        return sketch

    def extrude(
        self,
        sketch: Sketch,
        *,
        distance: float,
        direction: ExtrusionDirection = ExtrusionDirection.FORWARD,
        operation: BooleanOperation = BooleanOperation.NEW_BODY,
    ) -> Feature:
        """Create an extrusion feature from a closed sketch."""
        self._ensure_usable()
        self._ensure_same_session(sketch)
        sketch._ensure_usable()
        if sketch.document_id != self.document_id:
            raise ValueError("Sketch and part belong to different documents.")
        if not sketch.state.closed:
            raise ValueError("Sketch must be closed before extrusion.")
        parameters = ExtrusionParameters(
            distance=distance,
            direction=direction,
            operation=operation,
        )
        data = self._feature_service.extrude(
            self._handle,
            self._document_handle,
            sketch._handle,
            parameters,
        )
        if data.document_handle != self._document_handle:
            raise CoreProtocolError("Created feature belongs to another document.")
        feature = self._session.get_or_create_object(
            Feature,
            data.handle.value,
            lambda: Feature(self._session, self._feature_service, data),
            document_id=self.document_id,
        )
        feature._update(data)
        return feature

    def features(self) -> FeatureCollection:
        """Return an inspectable snapshot of this part's feature tree."""
        self._ensure_usable()
        features: list[Feature] = []
        for data in self._feature_service.list_features(self._handle):
            if data.document_handle != self._document_handle:
                raise CoreProtocolError("Feature belongs to another document.")
            feature = self._session.get_or_create_object(
                Feature,
                data.handle.value,
                lambda data=data: Feature(self._session, self._feature_service, data),
                document_id=self.document_id,
            )
            feature._update(data)
            features.append(feature)
        return FeatureCollection(features)

    def bodies(self) -> BodyCollection:
        """Return result bodies from the current model revision."""
        self._ensure_usable()
        bodies: list[Body] = []
        for data in self._inspection_service.list_bodies(self._handle):
            if data.part_handle != self._handle:
                raise CoreProtocolError("Body belongs to another part.")
            body = self._session.get_or_create_object(
                Body,
                data.handle.value,
                lambda data=data: Body(
                    self._session, self._inspection_service, data, self.document_id
                ),
                document_id=self.document_id,
            )
            body._update(data)
            self._session.register_topology(self.id, body.id)
            bodies.append(body)
        return BodyCollection(bodies)

    def faces(self) -> FaceCollection:
        """Return faces from all result bodies in the current model revision."""
        self._ensure_usable()
        faces: list[Face] = []
        for data in self._inspection_service.list_part_faces(self._handle):
            if data.part_handle != self._handle:
                raise CoreProtocolError("Face belongs to another part.")
            face = self._session.get_or_create_object(
                Face,
                data.handle.value,
                lambda data=data: Face(
                    self._session, self._inspection_service, data, self.document_id
                ),
                document_id=self.document_id,
            )
            face._update(data)
            self._session.register_topology(self.id, face.id)
            faces.append(face)
        return FaceCollection(faces)

    def mass_properties(self) -> MassProperties:
        """Calculate mass and inertia properties for this part."""
        self._ensure_usable()
        return self._inspection_service.get_mass_properties(self._handle)

    def bounding_box(self) -> BoundingBox3D:
        """Return the axis-aligned bounds of this part in millimeters."""
        self._ensure_usable()
        return self._inspection_service.get_bounding_box(self._handle)

    def rebuild(self) -> Part:
        """Rebuild this part after feature changes."""
        self._ensure_usable()
        self._feature_service.rebuild(self._handle)
        self._session.invalidate_topology(self.id)
        return self
