"""Tests for transport-to-Core error translation."""

import pytest

from kompas_bridge_transport import (
    BridgeProcessError,
    BridgeProtocolError,
    BridgeTimeoutError,
    InvalidParamsError,
    ObjectInvalidatedError,
    ObjectNotFoundError,
)
from kompas_core import (
    CoreObjectInvalidatedError,
    CoreObjectNotFoundError,
    CoreProcessError,
    CoreProtocolError,
    CoreRemoteError,
    CoreTimeoutError,
)
from kompas_core._internal.session import CoreSession
from tests.kompas_core.fakes import FakeBridgeGateway


@pytest.mark.parametrize(
    ("bridge_error", "core_error"),
    [
        (BridgeTimeoutError("Timed out."), CoreTimeoutError),
        (BridgeProtocolError("Invalid response."), CoreProtocolError),
        (BridgeProcessError("Could not start."), CoreProcessError),
    ],
)
def test_session_translates_bridge_failures(
    bridge_error: Exception,
    core_error: type[Exception],
) -> None:
    gateway = FakeBridgeGateway()
    gateway.responses.append(bridge_error)
    session = CoreSession(gateway, owns_gateway=False)

    with pytest.raises(core_error):
        session.call("application.status", {})


def test_session_preserves_structured_remote_error() -> None:
    gateway = FakeBridgeGateway()
    gateway.responses.append(
        InvalidParamsError(
            code="invalid_params",
            message="Unknown policy.",
            details={"field": "policy"},
            request_id="req_1",
        )
    )
    session = CoreSession(gateway, owns_gateway=False)

    with pytest.raises(CoreRemoteError) as captured:
        session.call("application.connect", {"policy": "attach_only"})

    assert captured.value.code == "invalid_params"
    assert captured.value.details == {"field": "policy"}
    assert captured.value.request_id == "req_1"


@pytest.mark.parametrize(
    ("bridge_error", "core_error"),
    [
        (
            ObjectNotFoundError("object_not_found", "Missing.", {}),
            CoreObjectNotFoundError,
        ),
        (
            ObjectInvalidatedError("object_invalidated", "Invalidated.", {}),
            CoreObjectInvalidatedError,
        ),
    ],
)
def test_session_translates_object_lifecycle_errors(
    bridge_error: Exception,
    core_error: type[Exception],
) -> None:
    gateway = FakeBridgeGateway()
    gateway.responses.append(bridge_error)
    session = CoreSession(gateway, owns_gateway=False)

    with pytest.raises(core_error):
        session.call("object.get_info", {"handle": "part_1"})
