"""In-memory fake for a future bridge transport."""

from collections import deque
from collections.abc import Mapping
from copy import deepcopy


JsonObject = dict[str, object]


class FakeBridgeTransport:
    """Record requests and return preconfigured JSON responses in FIFO order."""

    def __init__(self) -> None:
        self.requests: list[JsonObject] = []
        self._responses: deque[JsonObject] = deque()

    def queue_response(self, response: Mapping[str, object]) -> None:
        """Add a response that will be returned by the next send call."""
        self._responses.append(dict(response))

    def send(self, request: Mapping[str, object]) -> JsonObject:
        """Record a request and return the next configured response."""
        self.requests.append(deepcopy(dict(request)))
        if not self._responses:
            raise AssertionError("No response was configured for the bridge request.")
        return deepcopy(self._responses.popleft())
