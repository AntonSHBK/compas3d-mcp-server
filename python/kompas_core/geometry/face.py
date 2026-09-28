"""High-level KOMPAS face facade."""

from typing import ClassVar

from kompas_core._internal.session import CoreSession
from kompas_core.common.errors import CoreProtocolError
from kompas_core.common.types import ObjectKind
from kompas_core.geometry.models import FaceData, FaceGeometry
from kompas_core.geometry.service import InspectionService
from kompas_core.objects import KompasObject


class Face(KompasObject):
    object_kind: ClassVar[ObjectKind] = "face"

    def __init__(
        self,
        session: CoreSession,
        service: InspectionService,
        data: FaceData,
        document_id: str,
    ) -> None:
        super().__init__(session, data.handle, document_id=document_id)
        self._service = service
        self._part_handle = data.part_handle
        self._geometry = data.geometry

    @property
    def geometry(self) -> FaceGeometry:
        self._ensure_usable()
        return self._geometry

    @property
    def part_id(self) -> str:
        """Return the owning part handle for diagnostics."""
        return self._part_handle.value

    def _update(self, data: FaceData) -> None:
        if data.handle != self._handle or data.part_handle != self._part_handle:
            raise CoreProtocolError("Bridge returned another face object.")
        self._geometry = data.geometry

    def refresh(self) -> Face:
        self._ensure_usable()
        self._update(self._service.get_face_geometry(self._handle))
        return self
