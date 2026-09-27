"""Versioned request and response envelopes for kompas_bridge."""

from __future__ import annotations

from collections.abc import Mapping
from uuid import uuid4

from kompas_bridge_transport.errors import BridgeProtocolError, remote_error_from_payload


PROTOCOL_VERSION = 1
MAX_ID_LENGTH = 128
JsonObject = dict[str, object]


def new_request_id() -> str:
    """Create a process-independent request identifier."""
    return f"req_{uuid4().hex}"


def create_request(
    method: str,
    params: Mapping[str, object],
    request_id: str | None = None,
) -> JsonObject:
    """Build and validate one bridge request envelope."""
    if not isinstance(method, str) or not method:
        raise ValueError("Bridge method must be a nonempty string.")
    resolved_id = request_id or new_request_id()
    if not isinstance(resolved_id, str) or not resolved_id or len(resolved_id) > MAX_ID_LENGTH:
        raise ValueError("Bridge request id is invalid.")
    return {
        "protocol_version": PROTOCOL_VERSION,
        "id": resolved_id,
        "method": method,
        "params": dict(params),
    }


def parse_response(response: Mapping[str, object], expected_id: str) -> JsonObject:
    """Validate one response and return its result or raise a typed error."""
    if response.get("protocol_version") != PROTOCOL_VERSION:
        raise BridgeProtocolError("Bridge response protocol version does not match.")
    response_id = response.get("id")
    if response_id != expected_id:
        raise BridgeProtocolError("Bridge response id does not match the request.")
    ok = response.get("ok")
    if not isinstance(ok, bool):
        raise BridgeProtocolError("Bridge response ok field must be boolean.")
    if ok:
        result = response.get("result")
        if not isinstance(result, dict):
            raise BridgeProtocolError("Successful bridge response has no result object.")
        if "error" in response:
            raise BridgeProtocolError("Successful bridge response contains an error.")
        return dict(result)
    error = response.get("error")
    if not isinstance(error, dict):
        raise BridgeProtocolError("Failed bridge response has no error object.")
    raise remote_error_from_payload(error, expected_id)
