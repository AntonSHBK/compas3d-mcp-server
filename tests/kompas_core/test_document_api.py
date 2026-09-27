"""Tests for the public Application and Document object API."""

from pathlib import Path
from typing import cast

import pytest

from kompas_bridge_transport import BridgeClient
from kompas_bridge_transport import NoActiveDocumentError as BridgeNoActiveDocumentError
from kompas_core import CoreNoActiveDocumentError, Document, KompasApplication, connect
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


def document_data(
    document_id: str,
    name: str,
    *,
    active: bool,
    file_path: str | None = None,
    part_id: str | None = None,
    changed: bool = False,
) -> dict[str, object]:
    result: dict[str, object] = {
        "document_id": document_id,
        "document_type": 5,
        "name": name,
        "file_path": file_path,
        "active": active,
        "changed": changed,
        "read_only": False,
    }
    if part_id is not None:
        result["part_id"] = part_id
    return result


def connected_application(client: FakeBridgeClient) -> KompasApplication:
    client.responses.append(application_status())
    return connect(bridge_client=cast(BridgeClient, client))


def test_application_lists_documents_as_facades() -> None:
    client = FakeBridgeClient()
    application = connected_application(client)
    client.responses.append(
        {
            "documents": [
                document_data("doc_1", "First", active=True),
                document_data("doc_2", "Second", active=False),
            ]
        }
    )

    documents = application.documents()

    assert [document.id for document in documents] == ["doc_1", "doc_2"]
    assert [document.name for document in documents] == ["First", "Second"]
    assert all(isinstance(document, Document) for document in documents)


def test_active_document_returns_its_top_part() -> None:
    client = FakeBridgeClient()
    application = connected_application(client)
    client.responses.extend(
        [
            document_data("doc_1", "Active", active=True),
            {"document_id": "doc_1", "part_id": "part_1"},
        ]
    )

    document = application.active_document()
    part = document.top_part()

    assert document.id == "doc_1"
    assert part.id == "part_1"
    assert part.document_id == "doc_1"


def test_create_document_reuses_part_returned_by_bridge() -> None:
    client = FakeBridgeClient()
    application = connected_application(client)
    client.responses.append(
        document_data(
            "doc_3",
            "New detail",
            active=True,
            part_id="part_3",
        )
    )

    document = application.create_document_3d(visible=True)
    part = document.top_part()

    assert part.id == "part_3"
    assert client.requests[-1] == ("document.create_3d", {"visible": True})


def test_create_document_accepts_empty_unsaved_name() -> None:
    client = FakeBridgeClient()
    application = connected_application(client)
    client.responses.append(
        document_data(
            "doc_3",
            "",
            active=True,
            part_id="part_3",
        )
    )

    document = application.create_document_3d()

    assert document.name == ""


def test_open_second_document_and_return_to_first() -> None:
    client = FakeBridgeClient()
    application = connected_application(client)
    client.responses.extend(
        [
            document_data("doc_1", "First", active=True),
            document_data(
                "doc_2",
                "Second",
                active=True,
                file_path="D:/models/second.m3d",
                part_id="part_2",
            ),
            document_data("doc_1", "First", active=True),
        ]
    )

    first = application.active_document()
    second = application.open_document(Path("D:/models/second.m3d"))
    first.activate()

    assert second.file_path == Path("D:/models/second.m3d")
    assert first.state.active is True
    assert client.requests[-1] == (
        "document.activate",
        {"document_id": "doc_1"},
    )


def test_document_save_refresh_and_close_update_facade() -> None:
    client = FakeBridgeClient()
    application = connected_application(client)
    client.responses.extend(
        [
            document_data("doc_1", "Detail", active=True, changed=True),
            document_data("doc_1", "Detail", active=True, changed=False),
            document_data(
                "doc_1",
                "Detail",
                active=True,
                file_path="D:/models/detail.m3d",
            ),
            {
                "documents": [
                    document_data(
                        "doc_1",
                        "Renamed detail",
                        active=True,
                        file_path="D:/models/detail.m3d",
                    )
                ]
            },
            {"document_id": "doc_1", "closed": True},
        ]
    )

    document = application.active_document()
    document.save()
    document.save_as("D:/models/detail.m3d", overwrite=True)
    document.refresh()
    document.close(discard_changes=False)

    assert document.name == "Renamed detail"
    assert document.state.changed is False
    assert document.is_closed is True


def test_active_document_translates_missing_document_error() -> None:
    client = FakeBridgeClient()
    application = connected_application(client)
    client.responses.append(
        BridgeNoActiveDocumentError(
            code="no_active_document",
            message="No active document is available.",
            details={},
            request_id="req_active",
        )
    )

    with pytest.raises(CoreNoActiveDocumentError, match="No active document"):
        application.active_document()
