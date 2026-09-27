"""Reusable fakes for KOMPAS Core unit tests."""

from __future__ import annotations

from collections import deque
from collections.abc import Mapping
from copy import deepcopy
from typing import cast

from kompas_core.common.types import JsonObject


class FakeBridgeGateway:
    """Record Core calls and return queued bridge results."""

    def __init__(self) -> None:
        self.requests: list[tuple[str, JsonObject]] = []
        self.responses: deque[object] = deque()
        self.close_calls = 0

    def call(self, method: str, params: Mapping[str, object]) -> JsonObject:
        self.requests.append((method, deepcopy(dict(params))))
        if not self.responses:
            raise AssertionError("No fake bridge result was queued.")
        response = self.responses.popleft()
        if isinstance(response, Exception):
            raise response
        return deepcopy(cast(JsonObject, response))

    def close(self) -> None:
        self.close_calls += 1


class FakeBridgeClient(FakeBridgeGateway):
    """BridgeClient-compatible fake used at the public connect boundary."""
