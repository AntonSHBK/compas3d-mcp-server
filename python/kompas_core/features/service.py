"""Feature operations backed by the bridge port."""

from kompas_bridge_transport import DocumentHandle, FeatureHandle, PartHandle, SketchHandle

from kompas_core._internal.session import CoreSession
from kompas_core.common.errors import CoreProtocolError
from kompas_core.features.mapper import map_feature, map_feature_info
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

    def get_parameters(self, handle: FeatureHandle) -> FeatureData:
        return map_feature(
            self._session.call(
                "feature.get_parameters",
                {"feature_id": handle.value},
            )
        )

    def update_extrusion(self, handle: FeatureHandle, distance: float) -> FeatureData:
        parameters = ExtrusionParameters(distance=distance)
        return map_feature(
            self._session.call(
                "feature.update_extrusion",
                {"feature_id": handle.value, "distance": parameters.distance},
            )
        )

    def list_features(self, part_handle: PartHandle) -> list[FeatureData]:
        result = self._session.call("part.list_features", {"part_id": part_handle.value})
        values = result.get("features")
        if not isinstance(values, list):
            raise CoreProtocolError("Bridge did not return a feature list.")
        mapped: list[FeatureData] = []
        for item in values:
            if not isinstance(item, dict):
                raise CoreProtocolError("Feature list item must be an object.")
            mapped.append(map_feature_info(item))
        return mapped

    def get_info(self, handle: FeatureHandle) -> FeatureData:
        return map_feature_info(
            self._session.call("feature.get_info", {"feature_id": handle.value})
        )

    def rebuild(self, part_handle: PartHandle) -> None:
        result = self._session.call("model.rebuild", {"part_id": part_handle.value})
        if result.get("rebuilt") is not True:
            raise CoreProtocolError("Bridge did not confirm model rebuild.")
