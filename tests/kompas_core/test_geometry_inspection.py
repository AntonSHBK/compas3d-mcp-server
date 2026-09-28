"""Body, face selector and mass-property tests."""

from typing import cast

import pytest

from kompas_bridge_transport import BridgeClient
from kompas_core import CoreObjectInvalidatedError, Point3D, connect
from tests.kompas_core.factories import (
    application_status_result,
    angle_result,
    body_result,
    bounding_box_result,
    distance_result,
    document_result,
    face_result,
    mass_properties_result,
)
from tests.kompas_core.fakes import FakeBridgeClient


def make_part(client: FakeBridgeClient):
    client.responses.extend(
        [
            application_status_result(),
            document_result(),
            {"document_id": "doc_1", "part_id": "part_1"},
        ]
    )
    return connect(bridge_client=cast(BridgeClient, client)).active_document().top_part()


def test_face_selectors_filter_geometry_without_extra_bridge_calls() -> None:
    client = FakeBridgeClient()
    part = make_part(client)
    client.responses.append(
        {
            "part_id": "part_1",
            "faces": [
                face_result(
                    face_id="face_top",
                    normal={"x": 0.0, "y": 0.0, "z": 1.0},
                    area_mm2=2500.0,
                    z=50.0,
                ),
                face_result(
                    face_id="face_bottom",
                    normal={"x": 0.0, "y": 0.0, "z": -1.0},
                    area_mm2=2500.0,
                    z=0.0,
                ),
                face_result(
                    face_id="face_side",
                    surface_type="cylinder",
                    area_mm2=5000.0,
                    radius_mm=25.0,
                ),
            ],
        }
    )

    faces = part.faces()
    request_count = len(client.requests)

    assert faces.planar().normal("+z").largest().id == "face_top"
    assert faces.cylindrical().by_radius(25.0, tolerance=0.01).one().id == "face_side"
    assert faces.area_between(2400.0, 2600.0).nearest_to(Point3D(0, 0, 49)).id == "face_top"
    assert len(client.requests) == request_count


def test_bodies_faces_and_mass_properties_use_typed_results() -> None:
    client = FakeBridgeClient()
    part = make_part(client)
    client.responses.extend(
        [
            {"part_id": "part_1", "bodies": [body_result()]},
            {"body_id": "body_1", "faces": [face_result()]},
            mass_properties_result(),
            bounding_box_result(),
        ]
    )

    body = part.bodies().one()
    face = body.faces().one()
    properties = part.mass_properties()
    box = part.bounding_box()

    assert body.info.solid is True
    assert face.geometry.area_mm2 == 2500.0
    assert properties.mass_kg == 1.56
    assert properties.center_of_mass == Point3D(0.0, 0.0, 40.0)
    assert box.center == Point3D(0.0, 0.0, 25.0)


def test_application_measures_distance_and_angle_between_faces() -> None:
    client = FakeBridgeClient()
    client.responses.extend(
        [
            application_status_result(),
            document_result(),
            {"document_id": "doc_1", "part_id": "part_1"},
            {
                "part_id": "part_1",
                "faces": [
                    face_result(face_id="face_1"),
                    face_result(face_id="face_2", z=0.0),
                ],
            },
            distance_result(),
            angle_result(),
        ]
    )
    app = connect(bridge_client=cast(BridgeClient, client))
    face1, face2 = app.active_document().top_part().faces().all()

    distance = app.measurements.distance(face1, face2)
    angle = app.measurements.angle(face1, face2)

    assert distance.distance_mm == 50.0
    assert distance.normal_point2 == Point3D(0.0, 0.0, 50.0)
    assert angle.angle_degrees == 90.0


def test_rebuild_invalidates_cached_topology_facades() -> None:
    client = FakeBridgeClient()
    part = make_part(client)
    client.responses.extend(
        [
            {"part_id": "part_1", "faces": [face_result()]},
            {"part_id": "part_1", "rebuilt": True, "revision": 2},
        ]
    )
    face = part.faces().one()

    part.rebuild()

    with pytest.raises(CoreObjectInvalidatedError):
        _ = face.geometry
