"""Reusable fakes for bridge transport unit tests."""

from __future__ import annotations

from collections import deque
from collections.abc import Mapping
from copy import deepcopy

from kompas_bridge_transport.errors import BridgeConnectionError


JsonObject = dict[str, object]


class FakeTransport:
    """Record lifecycle and return queued bridge responses."""

    def __init__(self, *, connect_error: bool = False) -> None:
        self.connect_error = connect_error
        self.connect_calls = 0
        self.close_calls = 0
        self.send_calls = 0
        self.requests: list[JsonObject] = []
        self.responses: deque[JsonObject | Exception] = deque()

    def connect(self) -> None:
        self.connect_calls += 1
        if self.connect_error:
            raise BridgeConnectionError("Fake connection failed.")

    def send(self, request: Mapping[str, object]) -> JsonObject:
        self.send_calls += 1
        self.requests.append(deepcopy(dict(request)))
        if not self.responses:
            raise AssertionError("No fake bridge response was queued.")
        response = self.responses.popleft()
        if isinstance(response, Exception):
            raise response
        return deepcopy(response)

    def close(self) -> None:
        self.close_calls += 1


class FakeProcessManager:
    """Record bridge process lifecycle without starting a process."""

    def __init__(self) -> None:
        self.start_calls = 0
        self.stop_calls = 0

    def start(self) -> None:
        self.start_calls += 1

    def stop(self, timeout_seconds: float = 5.0) -> None:
        del timeout_seconds
        self.stop_calls += 1
