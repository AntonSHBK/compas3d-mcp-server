"""Feature operations backed by the bridge port."""

from kompas_bridge_transport import DocumentHandle, PartHandle, SketchHandle

from kompas_core._internal.session import CoreSession
from kompas_core.features.mapper import map_feature
from kompas_core.features.models import ExtrusionParameters, FeatureData


class FeatureService:
    """Translate typed feature operations into bridge calls."""

    def __init__(self, session: CoreSession) -> None:
        self._session = session

    def extrude(
        self,
        part_handle: PartHandle,
        document_handle: DocumentHandle,
        sketch_handle: SketchHandle,
        parameters: ExtrusionParameters,
    ) -> FeatureData:
        result = self._session.call(
            "feature.extrude",
            {
                "part_id": part_handle.value,
                "document_id": document_handle.value,
                "sketch_id": sketch_handle.value,
                "distance": parameters.distance,
                "direction": parameters.direction.value,
                "operation": parameters.operation.value,
            },
        )
        return map_feature(result)
