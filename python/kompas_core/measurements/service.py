"""Internal typed bridge calls for measurements."""

from kompas_bridge_transport import FaceHandle

from kompas_core._internal.session import CoreSession
from kompas_core.measurements.mapper import map_angle, map_distance
from kompas_core.measurements.models import AngleMeasurement, DistanceMeasurement


class MeasurementService:
    def __init__(self, session: CoreSession) -> None:
        self._session = session

    def distance(self, face1: FaceHandle, face2: FaceHandle) -> DistanceMeasurement:
        return map_distance(
            self._session.call(
                "measurement.distance",
                {"object1_id": face1.value, "object2_id": face2.value},
            )
        )

    def angle(self, face1: FaceHandle, face2: FaceHandle) -> AngleMeasurement:
        return map_angle(
            self._session.call(
                "measurement.angle",
                {"object1_id": face1.value, "object2_id": face2.value},
            )
        )
