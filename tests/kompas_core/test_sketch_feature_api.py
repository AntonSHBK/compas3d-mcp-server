"""Tests for extensible Sketch and Feature Core boundaries."""

from typing import cast

import pytest

from kompas_bridge_transport import BridgeClient
from kompas_core import (
    BooleanOperation,
    ExtrusionDirection,
    ExtrusionParameters,
    Feature,
    FeatureKind,
    Point2D,
    Sketch,
    SketchPlane,
    connect,
    create_cube,
)
from kompas_core.ports import BridgeGateway
from tests.kompas_core.factories import (
    application_status_result,
    document_result,
    feature_result,
    sketch_result,
)
from tests.kompas_core.fakes import FakeBridgeClient, FakeBridgeGateway


def test_fake_gateway_implements_core_protocol() -> None:
    assert isinstance(FakeBridgeGateway(), BridgeGateway)


def test_sketch_and_extrusion_flow_uses_typed_public_objects() -> None:
    client = FakeBridgeClient()
    client.responses.extend(
        [
            application_status_result(),
            document_result(),
            {"document_id": "doc_1", "part_id": "part_1"},
            sketch_result(closed=True),
            sketch_result(),
            sketch_result(geometry_count=1),
            sketch_result(closed=True, geometry_count=1),
            feature_result(),
        ]
    )
    application = connect(bridge_client=cast(BridgeClient, client))
    part = application.active_document().top_part()

    sketch = part.create_sketch(plane="xy")
    sketch.add_line(Point2D(0.0, 0.0), Point2D(50.0, 0.0))
    sketch.close()
    feature = part.extrude(sketch, distance=50.0)

    assert isinstance(sketch, Sketch)
    assert sketch.state.closed is True
    assert sketch.state.geometry_count == 1
    assert isinstance(feature, Feature)
    assert feature.state.kind is FeatureKind.EXTRUSION
    assert client.requests[-1] == (
        "feature.extrude",
        {
            "part_id": "part_1",
            "document_id": "doc_1",
            "sketch_id": "sketch_1",
            "distance": 50.0,
            "direction": "forward",
            "operation": "new_body",
        },
    )


@pytest.mark.parametrize("distance", [0.0, -1.0, float("inf"), float("nan")])
def test_extrusion_parameters_reject_invalid_distance(distance: float) -> None:
    with pytest.raises(ValueError):
        ExtrusionParameters(distance=distance)


def test_line_rejects_equal_endpoints_before_bridge_call() -> None:
    client = FakeBridgeClient()
    client.responses.extend(
        [
            application_status_result(),
            document_result(),
            {"document_id": "doc_1", "part_id": "part_1"},
            sketch_result(closed=True),
            sketch_result(),
        ]
    )
    application = connect(bridge_client=cast(BridgeClient, client))
    sketch = application.active_document().top_part().create_sketch(SketchPlane.XY)
    request_count = len(client.requests)

    with pytest.raises(ValueError, match="endpoints"):
        sketch.add_line(Point2D(1.0, 1.0), Point2D(1.0, 1.0))

    assert len(client.requests) == request_count


def test_extrusion_requires_closed_sketch() -> None:
    client = FakeBridgeClient()
    client.responses.extend(
        [
            application_status_result(),
            document_result(),
            {"document_id": "doc_1", "part_id": "part_1"},
            sketch_result(closed=True),
            sketch_result(),
        ]
    )
    application = connect(bridge_client=cast(BridgeClient, client))
    part = application.active_document().top_part()
    sketch = part.create_sketch(SketchPlane.XY)

    with pytest.raises(ValueError, match="closed"):
        part.extrude(
            sketch,
            distance=10.0,
            direction=ExtrusionDirection.REVERSE,
            operation=BooleanOperation.CUT,
        )


def test_cube_recipe_updates_depth_and_rebuilds_without_recreating_document() -> None:
    client = FakeBridgeClient()
    client.responses.extend(
        [
            application_status_result(),
            document_result(),
            {"document_id": "doc_1", "part_id": "part_1"},
            sketch_result(closed=True),
            sketch_result(),
            sketch_result(geometry_count=1),
            sketch_result(geometry_count=2),
            sketch_result(geometry_count=3),
            sketch_result(geometry_count=4),
            sketch_result(closed=True, geometry_count=4),
            feature_result(),
            feature_result(distance=80.0),
            {"part_id": "part_1", "rebuilt": True},
        ]
    )
    application = connect(bridge_client=cast(BridgeClient, client))
    part = application.active_document().top_part()

    feature = create_cube(part, 50.0)
    feature.set_depth(80.0)
    part.rebuild()

    assert feature.state.extrusion is not None
    assert feature.state.extrusion.distance == 80.0
    methods = [method for method, _ in client.requests]
    assert methods.count("document.get_active") == 1
    assert methods.count("sketch.add_line") == 4
    assert methods[-2:] == ["feature.update_extrusion", "model.rebuild"]
