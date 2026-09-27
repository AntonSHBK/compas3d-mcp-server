"""Typed errors exposed by the Python bridge transport layer."""

from __future__ import annotations

from collections.abc import Mapping
from dataclasses import dataclass


JsonObject = dict[str, object]


class BridgeError(Exception):
    """Base class for all bridge transport failures."""


class BridgeConnectionError(BridgeError, ConnectionError):
    """Raised when the local bridge cannot be connected or disconnects."""


class BridgeTimeoutError(BridgeError, TimeoutError):
    """Raised when a pipe connection or request exceeds its timeout."""


class BridgeProtocolError(BridgeError):
    """Raised when bridge data violates the versioned JSON protocol."""


class BridgeProcessError(BridgeError):
    """Raised when the managed C++ bridge process cannot be started."""


@dataclass(eq=False)
class BridgeRemoteError(BridgeError):
    """Base class for a structured error returned by the C++ bridge."""

    code: str
    message: str
    details: JsonObject
    request_id: str | None = None

    def __str__(self) -> str:
        return f"{self.code}: {self.message}"


class InvalidRequestError(BridgeRemoteError):
    """The request envelope is malformed."""


class UnsupportedProtocolVersionError(BridgeRemoteError):
    """The bridge does not support the requested protocol version."""


class UnknownMethodError(BridgeRemoteError):
    """The requested low-level method is not implemented."""


class InvalidParamsError(BridgeRemoteError):
    """Method parameters are invalid."""


class KompasNotRunningError(BridgeRemoteError):
    """The bridge is not connected to KOMPAS-3D."""


class KompasApiError(BridgeRemoteError):
    """A COM, API5 or API7 operation failed."""


class NoActiveDocumentError(BridgeRemoteError):
    """KOMPAS-3D has no active document."""


class ObjectNotFoundError(BridgeRemoteError):
    """An opaque handle does not exist in this bridge session."""


class ObjectInvalidatedError(BridgeRemoteError):
    """An opaque handle existed but is no longer valid."""


class BridgeInternalError(BridgeRemoteError):
    """The bridge failed while processing an otherwise valid request."""


ERROR_TYPES: dict[str, type[BridgeRemoteError]] = {
    "invalid_request": InvalidRequestError,
    "unsupported_protocol_version": UnsupportedProtocolVersionError,
    "unknown_method": UnknownMethodError,
    "invalid_params": InvalidParamsError,
    "kompas_not_running": KompasNotRunningError,
    "kompas_api_error": KompasApiError,
    "no_active_document": NoActiveDocumentError,
    "object_not_found": ObjectNotFoundError,
    "object_invalidated": ObjectInvalidatedError,
    "internal_error": BridgeInternalError,
}


def remote_error_from_payload(
    payload: Mapping[str, object],
    request_id: str | None,
) -> BridgeRemoteError:
    """Convert one bridge error object to its stable Python exception type."""
    code = payload.get("code")
    message = payload.get("message")
    details = payload.get("details", {})
    if not isinstance(code, str) or not code:
        raise BridgeProtocolError("Bridge error code is missing or invalid.")
    if not isinstance(message, str) or not message:
        raise BridgeProtocolError("Bridge error message is missing or invalid.")
    if not isinstance(details, dict):
        raise BridgeProtocolError("Bridge error details must be an object.")
    error_type = ERROR_TYPES.get(code, BridgeRemoteError)
    return error_type(
        code=code,
        message=message,
        details=dict(details),
        request_id=request_id,
    )
