"""High-level KOMPAS body facade."""

from typing import TYPE_CHECKING, ClassVar

from kompas_core._internal.session import CoreSession
from kompas_core.common.errors import CoreProtocolError
from kompas_core.common.types import ObjectKind
from kompas_core.geometry.models import BodyData, BodyInfo
from kompas_core.geometry.service import InspectionService
from kompas_core.objects import KompasObject

if TYPE_CHECKING:
    from kompas_core.geometry.collections import FaceCollection


class Body(KompasObject):
    object_kind: ClassVar[ObjectKind] = "body"

    def __init__(
        self,
        session: CoreSession,
        service: InspectionService,
        data: BodyData,
        document_id: str,
    ) -> None:
        super().__init__(session, data.handle, document_id=document_id)
        self._service = service
        self._part_handle = data.part_handle
        self._info = data.info

    @property
    def info(self) -> BodyInfo:
        self._ensure_usable()
        return self._info

    def _update(self, data: BodyData) -> None:
        if data.handle != self._handle or data.part_handle != self._part_handle:
            raise CoreProtocolError("Bridge returned another body object.")
        self._info = data.info

    def refresh(self) -> Body:
        self._ensure_usable()
        self._update(self._service.get_body_info(self._handle))
        return self

    def faces(self) -> FaceCollection:
        from kompas_core.geometry.collections import FaceCollection
        from kompas_core.geometry.face import Face

        self._ensure_usable()
        values: list[Face] = []
        for data in self._service.list_body_faces(self._handle):
            face = self._session.get_or_create_object(
                Face,
                data.handle.value,
                lambda data=data: Face(
                    self._session, self._service, data, self.document_id or ""
                ),
                document_id=self.document_id,
            )
            face._update(data)
            self._session.register_topology(self._part_handle.value, face.id)
            values.append(face)
        return FaceCollection(values)
