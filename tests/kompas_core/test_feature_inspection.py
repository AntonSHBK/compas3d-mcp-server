"""Feature inspection and selector behavior."""

from typing import cast

import pytest

from kompas_bridge_transport import BridgeClient
from kompas_core import (
    CoreSelectionAmbiguousError,
    CoreSelectionNotFoundError,
    FeatureKind,
    connect,
)
from tests.kompas_core.factories import (
    application_status_result,
    document_result,
    feature_info_result,
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


def test_feature_selectors_are_typed_and_explicit_about_cardinality() -> None:
    client = FakeBridgeClient()
    part = make_part(client)
    client.responses.append(
        {
            "part_id": "part_1",
            "features": [
                feature_info_result(feature_id="feat_1", name="Extrusion1"),
                feature_info_result(feature_id="feat_2", name="Extrusion1", valid=False),
                feature_info_result(
                    feature_id="feat_3", name="Sketch1", feature_type="sketch"
                ),
            ],
        }
    )

    features = part.features()

    assert len(features.of_type(FeatureKind.EXTRUSION).all()) == 2
    assert features.invalid().one().id == "feat_2"
    assert features.by_name("Sketch1").first().info.feature_type == "sketch"
    with pytest.raises(CoreSelectionAmbiguousError):
        features.by_name("Extrusion1").one()
    with pytest.raises(CoreSelectionNotFoundError):
        features.by_name("Missing").one()
    assert client.requests[-1] == ("part.list_features", {"part_id": "part_1"})


def test_feature_info_refresh_updates_cached_facade() -> None:
    client = FakeBridgeClient()
    part = make_part(client)
    client.responses.extend(
        [
            {"part_id": "part_1", "features": [feature_info_result()]},
            feature_info_result(name="Renamed", update_stamp=2),
        ]
    )

    feature = part.features().one()
    same = feature.refresh_info()

    assert same is feature
    assert feature.info.name == "Renamed"
    assert client.requests[-1] == ("feature.get_info", {"feature_id": "feat_1"})
