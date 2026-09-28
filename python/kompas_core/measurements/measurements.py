"""Public high-level measurement facade."""

from kompas_core._internal.session import CoreSession
from kompas_core.geometry.face import Face
from kompas_core.measurements.models import AngleMeasurement, DistanceMeasurement
from kompas_core.measurements.service import MeasurementService


class Measurements:
    def __init__(self, session: CoreSession) -> None:
        self._session = session
        self._service = MeasurementService(session)

    def distance(self, face1: Face, face2: Face) -> DistanceMeasurement:
        """Measure minimum, maximum and normal distances between two faces."""
        self._validate_faces(face1, face2)
        return self._service.distance(face1._handle, face2._handle)

    def angle(self, face1: Face, face2: Face) -> AngleMeasurement:
        """Measure the angle between two faces in degrees."""
        self._validate_faces(face1, face2)
        return self._service.angle(face1._handle, face2._handle)

    def _validate_faces(self, face1: Face, face2: Face) -> None:
        self._session.ensure_same_session(face1, face2)
        face1._ensure_usable()
        face2._ensure_usable()
        if face1.part_id != face2.part_id:
            raise ValueError("Measured faces must belong to the same part.")
