"""Low-level typed model-inspection calls."""

from kompas_bridge_transport import BodyHandle, FaceHandle, PartHandle

from kompas_core._internal.session import CoreSession
from kompas_core.common.errors import CoreProtocolError
from kompas_core.geometry.mapper import (
    map_body,
    map_bounding_box,
    map_face,
    map_mass_properties,
)
from kompas_core.geometry.models import BoundingBox3D, BodyData, FaceData, MassProperties


class InspectionService:
    def __init__(self, session: CoreSession) -> None:
        self._session = session

    def list_bodies(self, part: PartHandle) -> list[BodyData]:
        result = self._session.call("part.list_bodies", {"part_id": part.value})
        return [map_body(value) for value in _object_list(result, "bodies")]

    def get_body_info(self, body: BodyHandle) -> BodyData:
        return map_body(self._session.call("body.get_info", {"body_id": body.value}))

    def list_body_faces(self, body: BodyHandle) -> list[FaceData]:
        result = self._session.call("body.list_faces", {"body_id": body.value})
        return [map_face(value) for value in _object_list(result, "faces")]

    def list_part_faces(self, part: PartHandle) -> list[FaceData]:
        result = self._session.call("part.list_faces", {"part_id": part.value})
        return [map_face(value) for value in _object_list(result, "faces")]

    def get_face_geometry(self, face: FaceHandle) -> FaceData:
        return map_face(
            self._session.call("face.get_geometry", {"face_id": face.value})
        )

    def get_mass_properties(self, part: PartHandle) -> MassProperties:
        return map_mass_properties(
            self._session.call("part.get_mass_properties", {"part_id": part.value})
        )

    def get_bounding_box(self, part: PartHandle) -> BoundingBox3D:
        return map_bounding_box(
            self._session.call("part.get_bounding_box", {"part_id": part.value})
        )


def _object_list(result: dict[str, object], key: str) -> list[dict[str, object]]:
    values = result.get(key)
    if not isinstance(values, list) or any(not isinstance(value, dict) for value in values):
        raise CoreProtocolError(f"{key} must be a list of objects.")
    return values
