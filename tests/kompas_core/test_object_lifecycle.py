"""Tests for shared CAD object identity and lifecycle rules."""

import gc
import weakref
from typing import cast

import pytest

from kompas_bridge_transport import BridgeClient
from kompas_core import (
    CoreObjectInvalidatedError,
    Document,
    KompasApplication,
    KompasObject,
    ObjectInfo,
    Part,
    connect,
)
from tests.kompas_core.fakes import FakeBridgeClient


def application_status() -> dict[str, object]:
    return {
        "connected": True,
        "visible": True,
        "kompas_version": {
            "major": 24,
            "minor": 0,
            "release": 7,
            "build": 0,
        },
    }


def document_data(document_id: str = "doc_1") -> dict[str, object]:
    return {
        "document_id": document_id,
        "document_type": 5,
        "name": "Detail",
        "file_path": None,
        "active": True,
        "changed": False,
        "read_only": False,
    }


def connected_application(client: FakeBridgeClient) -> KompasApplication:
    client.responses.append(application_status())
    return connect(bridge_client=cast(BridgeClient, client))


def active_document_with_part(
    client: FakeBridgeClient,
) -> tuple[Document, Part]:
    application = connected_application(client)
    client.responses.extend(
        [
            document_data(),
            {"document_id": "doc_1", "part_id": "part_1"},
        ]
    )
    document = application.active_document()
    return document, document.top_part()


def test_repeated_top_part_returns_cached_facade() -> None:
    client = FakeBridgeClient()
    document, first = active_document_with_part(client)

    second = document.top_part()

    assert first is second
    assert first == second
    assert isinstance(first, KompasObject)
    assert sum(method == "document.get_top_part" for method, _ in client.requests) == 1


def test_closing_document_invalidates_child_facades() -> None:
    client = FakeBridgeClient()
    document, part = active_document_with_part(client)
    client.responses.append({"document_id": "doc_1", "closed": True})

    document.close(discard_changes=True)

    with pytest.raises(CoreObjectInvalidatedError):
        part.refresh()

    assert client.requests[-1] == (
        "document.close",
        {"document_id": "doc_1", "discard_changes": True},
    )


def test_release_is_idempotent_and_prevents_further_use() -> None:
    client = FakeBridgeClient()
    _document, part = active_document_with_part(client)
    client.responses.append({"released": True})

    part.release()
    part.release()

    with pytest.raises(CoreObjectInvalidatedError):
        part.refresh()
    assert sum(method == "object.release" for method, _ in client.requests) == 1


def test_refresh_exposes_typed_object_information() -> None:
    client = FakeBridgeClient()
    _document, part = active_document_with_part(client)
    client.responses.append(
        {
            "handle": "part_1",
            "kind": "part",
            "document_id": "doc_1",
        }
    )

    part.refresh()

    assert part.info == ObjectInfo(
        id="part_1",
        kind="part",
        document_id="doc_1",
    )
    assert not isinstance(part.info, dict)


def test_objects_from_different_sessions_are_not_interchangeable() -> None:
    first_client = FakeBridgeClient()
    second_client = FakeBridgeClient()
    first_app = connected_application(first_client)
    second_app = connected_application(second_client)
    first_client.responses.append(document_data())
    second_client.responses.append(document_data())
    first = first_app.active_document()
    second = second_app.active_document()

    assert first != second
    with pytest.raises(ValueError, match="different Core sessions"):
        first._ensure_same_session(second)


def test_object_cache_does_not_keep_facade_alive() -> None:
    client = FakeBridgeClient()
    document, part = active_document_with_part(client)
    reference = weakref.ref(part)
    del part
    gc.collect()

    replacement = document.top_part()

    assert reference() is None
    assert replacement.id == "part_1"
