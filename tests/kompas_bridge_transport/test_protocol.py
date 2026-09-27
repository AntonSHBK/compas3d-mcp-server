"""Unit tests for protocol envelopes, errors and opaque handles."""

import pytest

from kompas_bridge_transport import DocumentHandle, ObjectInvalidatedError
from kompas_bridge_transport.errors import BridgeProtocolError
from kompas_bridge_transport.protocol import create_request, parse_response


def test_request_ids_are_unique() -> None:
    first = create_request("document.list", {})
    second = create_request("document.list", {})

    assert first["id"] != second["id"]


def test_response_id_mismatch_is_rejected() -> None:
    response = {
        "protocol_version": 1,
        "id": "another_request",
        "ok": True,
        "result": {},
    }

    with pytest.raises(BridgeProtocolError):
        parse_response(response, expected_id="expected_request")


def test_object_invalidated_error_is_typed() -> None:
    response = {
        "protocol_version": 1,
        "id": "req_invalidated",
        "ok": False,
        "error": {
            "code": "object_invalidated",
            "message": "Handle is no longer valid.",
            "details": {},
        },
    }

    with pytest.raises(ObjectInvalidatedError):
        parse_response(response, expected_id="req_invalidated")


def test_document_handle_rejects_another_kind() -> None:
    with pytest.raises(ValueError):
        DocumentHandle("part_session_1")


def test_document_handle_keeps_opaque_value() -> None:
    handle = DocumentHandle("doc_session_1")

    assert str(handle) == "doc_session_1"
