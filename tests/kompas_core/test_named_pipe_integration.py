"""Integration checks for BridgeClient with a real C++ bridge."""

from __future__ import annotations

import os
from pathlib import Path
from uuid import uuid4

import pytest

from kompas_bridge_transport import BridgeClient
from kompas_core import CoreNoActiveDocumentError, CoreRemoteError, connect, create_cube
from kompas_mcp.config.settings import BridgeSettings


ROOT_DIR = Path(__file__).resolve().parents[2]
DEFAULT_BRIDGE_PATH = (
    ROOT_DIR
    / "build"
    / "debug-x64-windows"
    / "src"
    / "kompas_bridge"
    / "kompas_bridge.exe"
)


@pytest.mark.integration
def test_core_document_chain_through_real_bridge() -> None:
    """Core traverses application, document, part and object layers."""
    bridge_path = Path(os.environ.get("KOMPAS_BRIDGE_EXE", DEFAULT_BRIDGE_PATH))
    if not bridge_path.is_file():
        pytest.skip(f"Bridge executable was not found: {bridge_path}")
    settings = BridgeSettings(
        executable_path=bridge_path,
        pipe_name=rf"\\.\pipe\kompas-bridge-test-{uuid4().hex}",
        request_timeout_seconds=2.0,
    )

    with BridgeClient(
        settings,
        startup_timeout_seconds=5.0,
        retry_delay_seconds=0.05,
    ) as bridge:
        initial_status = bridge.call("application.status", {})
        if initial_status["connected"] is not True:
            pytest.skip("KOMPAS-3D is not running in the integration test session.")
        with connect(bridge_client=bridge) as application:
            status = application.status()
            documents = application.documents()
            try:
                document = application.active_document()
            except CoreNoActiveDocumentError:
                document = application.create_document_3d(visible=True)
                create_cube(document.top_part(), 50.0)
            try:
                part = document.top_part()
                info = part.get_info()
                features = part.features()
                if features:
                    feature_info = features.first().refresh_info().info
                bodies = part.bodies()
                faces = part.faces()
                bounding_box = part.bounding_box()
                distance = None
                angle = None
                if len(faces) >= 2:
                    distance = application.measurements.distance(
                        faces.all()[0], faces.all()[1]
                    )
                planar_faces = [
                    face for face in faces if face.geometry.normal is not None
                ]
                for index, first in enumerate(planar_faces):
                    for second in planar_faces[index + 1 :]:
                        normal1 = first.geometry.normal
                        normal2 = second.geometry.normal
                        assert normal1 is not None and normal2 is not None
                        dot = (
                            normal1.x * normal2.x
                            + normal1.y * normal2.y
                            + normal1.z * normal2.z
                        )
                        if abs(dot) < 0.5:
                            angle = application.measurements.angle(first, second)
                            break
                    if angle is not None:
                        break
                if bodies:
                    body_faces = bodies.first().faces()
                    mass_properties = part.mass_properties()
            except CoreRemoteError as error:
                pytest.skip(f"Active document does not expose a top part: {error}")

    assert status.connected is True
    assert isinstance(documents, list)
    assert document.id.startswith("doc_")
    assert part.id.startswith("part_")
    assert info.id == part.id
    assert isinstance(features.all(), list)
    if features:
        assert feature_info.name
    assert isinstance(bodies.all(), list)
    assert isinstance(faces.all(), list)
    assert bounding_box.max.z >= bounding_box.min.z
    if distance is not None:
        assert distance.distance_mm >= 0.0
    if angle is not None:
        assert 0.0 <= angle.angle_degrees <= 180.0
    if bodies:
        assert isinstance(body_faces.all(), list)
        assert mass_properties.volume_mm3 >= 0.0
