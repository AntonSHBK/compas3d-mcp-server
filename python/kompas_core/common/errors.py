"""Exceptions exposed by KOMPAS Core."""

from __future__ import annotations

from dataclasses import dataclass

from kompas_bridge_transport import (
    BridgeConnectionError,
    BridgeError,
    BridgeProcessError,
    BridgeProtocolError,
    BridgeRemoteError,
    BridgeTimeoutError,
)

from kompas_core.common.types import JsonObject


class CoreError(Exception):
    """Base class for failures exposed by KOMPAS Core."""


class CoreBridgeError(CoreError):
    """Base class for communication failures at the bridge boundary."""


class CoreConnectionError(CoreBridgeError, ConnectionError):
    """KOMPAS Core could not connect to or communicate with the bridge."""


class CoreTimeoutError(CoreBridgeError, TimeoutError):
    """A bridge operation exceeded its timeout."""


class CoreProtocolError(CoreBridgeError):
    """Bridge data does not satisfy the Core contract."""


class CoreProcessError(CoreBridgeError):
    """The local bridge process could not be managed."""


class CoreSessionClosedError(CoreError):
    """An operation was attempted through a closed Core session."""


class CoreNoActiveDocumentError(CoreError):
    """KOMPAS-3D has no active document."""


class CoreObjectNotFoundError(CoreError):
    """A CAD object is not available in the current Core session."""


class CoreObjectInvalidatedError(CoreError):
    """A previously available CAD object is no longer valid."""


@dataclass(eq=False)
class CoreRemoteError(CoreError):
    """The bridge rejected a valid Core request."""

    code: str
    message: str
    details: JsonObject
    request_id: str | None = None

    def __str__(self) -> str:
        return f"{self.code}: {self.message}"


def translate_bridge_error(error: BridgeError) -> CoreError:
    """Translate a transport-layer exception into a stable Core exception."""
    if isinstance(error, BridgeTimeoutError):
        return CoreTimeoutError(str(error))
    if isinstance(error, BridgeConnectionError):
        return CoreConnectionError(str(error))
    if isinstance(error, BridgeProtocolError):
        return CoreProtocolError(str(error))
    if isinstance(error, BridgeProcessError):
        return CoreProcessError(str(error))
    if isinstance(error, BridgeRemoteError):
        if error.code == "no_active_document":
            return CoreNoActiveDocumentError(error.message)
        if error.code == "object_not_found":
            return CoreObjectNotFoundError(error.message)
        if error.code == "object_invalidated":
            return CoreObjectInvalidatedError(error.message)
        return CoreRemoteError(
            code=error.code,
            message=error.message,
            details=dict(error.details),
            request_id=error.request_id,
        )
    return CoreBridgeError(str(error))
