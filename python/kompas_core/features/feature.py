"""High-level KOMPAS feature object."""

from __future__ import annotations

from typing import ClassVar

from kompas_core._internal.session import CoreSession
from kompas_core.common.errors import CoreProtocolError
from kompas_core.common.types import ObjectKind
from kompas_core.features.models import FeatureData, FeatureState
from kompas_core.features.service import FeatureService
from kompas_core.objects import KompasObject


class Feature(KompasObject):
    """High-level facade for one KOMPAS feature."""

    object_kind: ClassVar[ObjectKind] = "feature"

    def __init__(
        self,
        session: CoreSession,
        service: FeatureService,
        data: FeatureData,
    ) -> None:
        super().__init__(session, data.handle, document_id=data.document_handle.value)
        self._service = service
        self._state = data.state

    @property
    def state(self) -> FeatureState:
        return self._state

    def _update(self, data: FeatureData) -> None:
        if data.handle != self._handle or data.document_handle.value != self.document_id:
            raise CoreProtocolError("Bridge returned another feature object.")
        self._state = data.state

    def refresh_parameters(self) -> Feature:
        """Reload extrusion parameters from KOMPAS."""
        self._ensure_usable()
        self._update(self._service.get_parameters(self._handle))
        return self

    def set_depth(self, distance: float) -> Feature:
        """Change extrusion depth without recreating the feature."""
        self._ensure_usable()
        self._update(self._service.update_extrusion(self._handle, distance))
        return self
