"""High-level KOMPAS feature object."""

from typing import ClassVar

from kompas_core._internal.session import CoreSession
from kompas_core.common.errors import CoreProtocolError
from kompas_core.common.types import ObjectKind
from kompas_core.features.models import FeatureData, FeatureState
from kompas_core.objects import KompasObject


class Feature(KompasObject):
    """High-level facade for one KOMPAS feature."""

    object_kind: ClassVar[ObjectKind] = "feature"

    def __init__(
        self,
        session: CoreSession,
        data: FeatureData,
    ) -> None:
        super().__init__(session, data.handle, document_id=data.document_handle.value)
        self._state = data.state

    @property
    def state(self) -> FeatureState:
        return self._state

    def _update(self, data: FeatureData) -> None:
        if data.handle != self._handle or data.document_handle.value != self.document_id:
            raise CoreProtocolError("Bridge returned another feature object.")
        self._state = data.state
