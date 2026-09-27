"""Bridge gateway contract used by Core services."""

from __future__ import annotations

from collections.abc import Mapping
from typing import Protocol, runtime_checkable

from kompas_bridge_transport import BridgeClient

from kompas_core.common.types import JsonObject


@runtime_checkable
class BridgeGateway(Protocol):
    """Internal boundary between Core and the low-level bridge client."""

    def call(self, method: str, params: Mapping[str, object]) -> JsonObject: ...

    def close(self) -> None: ...


class BridgeClientAdapter:
    """Adapt BridgeClient to the internal Core gateway contract."""

    def __init__(self, client: BridgeClient) -> None:
        self._client = client

    def call(self, method: str, params: Mapping[str, object]) -> JsonObject:
        return self._client.call(method, params)

    def close(self) -> None:
        self._client.close()
