"""Tests for the public KOMPAS Core connection lifecycle."""

from __future__ import annotations

from typing import cast

import pytest

import kompas_core.connection as connection_module
from kompas_bridge_transport import BridgeClient, BridgeConnectionError
from kompas_core import CoreConnectionError, connect
from kompas_core.common.types import ConnectionPolicy
from kompas_mcp.config.settings import BridgeSettings
from tests.kompas_core.fakes import FakeBridgeClient


def connected_status() -> dict[str, object]:
    """Return a valid application status bridge result."""
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


def test_connect_uses_external_client_without_taking_ownership() -> None:
    client = FakeBridgeClient()
    client.responses.extend([connected_status(), connected_status()])

    with connect(bridge_client=cast(BridgeClient, client)) as application:
        status = application.status()

    assert status.connected is True
    assert status.kompas_version is not None
    assert status.kompas_version.major == 24
    assert client.requests == [
        ("application.connect", {"policy": "attach_only"}),
        ("application.status", {}),
    ]
    assert client.close_calls == 0
    assert application.is_closed is True


def test_connect_closes_internally_created_client(
    monkeypatch: pytest.MonkeyPatch,
    bridge_settings: BridgeSettings,
) -> None:
    client = FakeBridgeClient()
    client.responses.append(connected_status())
    monkeypatch.setattr(connection_module, "BridgeClient", lambda _settings: client)

    application = connect(bridge_settings)
    application.close()
    application.close()

    assert client.close_calls == 1


def test_connect_translates_transport_error_and_cleans_up_owned_client(
    monkeypatch: pytest.MonkeyPatch,
    bridge_settings: BridgeSettings,
) -> None:
    client = FakeBridgeClient()
    client.responses.append(BridgeConnectionError("Bridge is unavailable."))
    monkeypatch.setattr(connection_module, "BridgeClient", lambda _settings: client)

    with pytest.raises(CoreConnectionError) as captured:
        connect(bridge_settings)

    assert str(captured.value) == "Bridge is unavailable."
    assert client.close_calls == 1


def test_connect_rejects_ambiguous_client_configuration(
    bridge_settings: BridgeSettings,
) -> None:
    client = cast(BridgeClient, FakeBridgeClient())

    with pytest.raises(ValueError, match="mutually exclusive"):
        connect(bridge_settings, bridge_client=client)


def test_connect_rejects_unknown_policy() -> None:
    client = cast(BridgeClient, FakeBridgeClient())
    invalid_policy = cast(ConnectionPolicy, "automatic")

    with pytest.raises(ValueError, match="Unknown KOMPAS connection policy"):
        connect(bridge_client=client, policy=invalid_policy)
