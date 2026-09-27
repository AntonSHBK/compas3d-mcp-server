"""Tests for the internal Core session boundary."""

import pytest

from kompas_core import CoreProtocolError, CoreSessionClosedError
from kompas_core._internal.session import CoreSession
from tests.kompas_core.fakes import FakeBridgeGateway


def test_session_calls_fake_gateway() -> None:
    gateway = FakeBridgeGateway()
    gateway.responses.append({"connected": True})
    session = CoreSession(gateway, owns_gateway=False)

    result = session.call("application.status", {})

    assert result == {"connected": True}
    assert gateway.requests == [("application.status", {})]


def test_session_rejects_non_object_result() -> None:
    gateway = FakeBridgeGateway()
    gateway.responses.append([])
    session = CoreSession(gateway, owns_gateway=False)

    with pytest.raises(CoreProtocolError, match="must be a JSON object"):
        session.call("application.status", {})


def test_closed_session_rejects_calls() -> None:
    gateway = FakeBridgeGateway()
    session = CoreSession(gateway, owns_gateway=True)
    session.close()

    with pytest.raises(CoreSessionClosedError):
        session.call("application.status", {})

    assert gateway.close_calls == 1
