"""Unit tests for BridgeClient orchestration and retry behavior."""

from __future__ import annotations

from pathlib import Path

import pytest

from kompas_bridge_transport import (
    BridgeClient,
    BridgeTimeoutError,
    InvalidParamsError,
)
from kompas_mcp.config.settings import BridgeSettings
from tests.kompas_bridge_transport.fakes import FakeProcessManager, FakeTransport


def settings() -> BridgeSettings:
    """Return deterministic settings for fake transport tests."""
    return BridgeSettings(
        executable_path=Path("C:/test/kompas_bridge.exe"),
        pipe_name=r"\\.\pipe\kompas-bridge-test",
        request_timeout_seconds=2.0,
    )


def test_call_builds_envelope_and_returns_result() -> None:
    transport = FakeTransport()
    transport.responses.append(
        {
            "protocol_version": 1,
            "id": "req_fixed",
            "ok": True,
            "result": {"connected": True},
        }
    )
    process = FakeProcessManager()
    client = BridgeClient(
        settings(),
        transport_factory=lambda _name, _timeout: transport,
        process_manager=process,
        request_id_factory=lambda: "req_fixed",
    )

    result = client.call("application.status", {})

    assert result == {"connected": True}
    assert transport.requests == [
        {
            "protocol_version": 1,
            "id": "req_fixed",
            "method": "application.status",
            "params": {},
        }
    ]
    assert process.start_calls == 0


def test_call_converts_bridge_error_to_typed_exception() -> None:
    transport = FakeTransport()
    transport.responses.append(
        {
            "protocol_version": 1,
            "id": "req_error",
            "ok": False,
            "error": {
                "code": "invalid_params",
                "message": "Bad parameters.",
                "details": {"field": "document_id"},
            },
        }
    )
    client = BridgeClient(
        settings(),
        transport_factory=lambda _name, _timeout: transport,
        process_manager=FakeProcessManager(),
        request_id_factory=lambda: "req_error",
    )

    with pytest.raises(InvalidParamsError) as captured:
        client.call("document.activate", {})

    assert captured.value.code == "invalid_params"
    assert captured.value.details == {"field": "document_id"}
    assert captured.value.request_id == "req_error"


def test_connection_retry_starts_bridge_once() -> None:
    unavailable = FakeTransport(connect_error=True)
    connected = FakeTransport()
    connected.responses.append(
        {
            "protocol_version": 1,
            "id": "req_retry",
            "ok": True,
            "result": {"documents": []},
        }
    )
    transports = iter([unavailable, connected])
    process = FakeProcessManager()
    client = BridgeClient(
        settings(),
        transport_factory=lambda _name, _timeout: next(transports),
        process_manager=process,
        request_id_factory=lambda: "req_retry",
        startup_timeout_seconds=1.0,
        retry_delay_seconds=0.001,
    )

    result = client.call("document.list", {})

    assert result == {"documents": []}
    assert process.start_calls == 1
    assert unavailable.close_calls == 1
    assert connected.send_calls == 1


def test_request_timeout_is_not_replayed() -> None:
    transport = FakeTransport()
    transport.responses.append(BridgeTimeoutError("Fake response timeout."))
    client = BridgeClient(
        settings(),
        transport_factory=lambda _name, _timeout: transport,
        process_manager=FakeProcessManager(),
        request_id_factory=lambda: "req_timeout",
    )

    with pytest.raises(BridgeTimeoutError):
        client.call("document.create_3d", {"visible": True})

    assert transport.send_calls == 1
    assert transport.close_calls == 1
