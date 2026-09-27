"""High-level request client for the low-level kompas_bridge protocol."""

from __future__ import annotations

import threading
import time
from collections.abc import Callable, Mapping
from pathlib import Path
from typing import Protocol

from kompas_bridge_transport._windows.named_pipe import NamedPipeClient
from kompas_bridge_transport.errors import BridgeConnectionError, BridgeTimeoutError
from kompas_bridge_transport.process import BridgeProcessManager
from kompas_bridge_transport.protocol import JsonObject, create_request, parse_response


class BridgeClientSettings(Protocol):
    """Settings required by BridgeClient without coupling it to the MCP package."""

    executable_path: Path
    pipe_name: str
    request_timeout_seconds: float


class BridgeTransport(Protocol):
    """Minimal transport boundary used by BridgeClient and test fakes."""

    def connect(self) -> None: ...

    def send(self, request: Mapping[str, object]) -> JsonObject: ...

    def close(self) -> None: ...


class ProcessManager(Protocol):
    """Lifecycle boundary used by BridgeClient and test fakes."""

    def start(self) -> None: ...

    def stop(self, timeout_seconds: float = 5.0) -> None: ...


TransportFactory = Callable[[str, float], BridgeTransport]
RequestIdFactory = Callable[[], str]


def _named_pipe_factory(pipe_name: str, timeout_seconds: float) -> BridgeTransport:
    return NamedPipeClient(pipe_name=pipe_name, timeout_seconds=timeout_seconds)


class BridgeClient:
    """Manage bridge startup and execute one serialized request at a time."""

    def __init__(
        self,
        settings: BridgeClientSettings,
        *,
        transport_factory: TransportFactory = _named_pipe_factory,
        process_manager: ProcessManager | None = None,
        request_id_factory: RequestIdFactory | None = None,
        start_if_needed: bool = True,
        startup_timeout_seconds: float = 10.0,
        retry_delay_seconds: float = 0.1,
    ) -> None:
        if startup_timeout_seconds <= 0:
            raise ValueError("startup_timeout_seconds must be positive.")
        if retry_delay_seconds <= 0:
            raise ValueError("retry_delay_seconds must be positive.")
        self._settings = settings
        self._transport_factory = transport_factory
        self._process = process_manager or BridgeProcessManager(
            settings.executable_path,
            settings.pipe_name,
        )
        self._request_id_factory = request_id_factory
        self._start_if_needed = start_if_needed
        self._startup_timeout_seconds = startup_timeout_seconds
        self._retry_delay_seconds = retry_delay_seconds
        self._transport: BridgeTransport | None = None
        self._lock = threading.Lock()

    def call(self, method: str, params: Mapping[str, object]) -> JsonObject:
        """Call one bridge method and return its result object."""
        request = create_request(
            method,
            params,
            self._request_id_factory() if self._request_id_factory else None,
        )
        with self._lock:
            transport = self._ensure_connected()
            try:
                response = transport.send(request)
            except (BridgeConnectionError, BridgeTimeoutError):
                self._drop_transport()
                raise
        return parse_response(response, expected_id=str(request["id"]))

    def close(self) -> None:
        """Close the pipe and stop only a bridge process started by this client."""
        with self._lock:
            self._drop_transport()
            self._process.stop()

    def __enter__(self) -> BridgeClient:
        self._ensure_connected()
        return self

    def __exit__(self, *_: object) -> None:
        self.close()

    def _ensure_connected(self) -> BridgeTransport:
        if self._transport is not None:
            return self._transport

        deadline = time.monotonic() + self._startup_timeout_seconds
        bridge_started = False
        last_error: BridgeConnectionError | BridgeTimeoutError | None = None
        while True:
            transport = self._transport_factory(
                self._settings.pipe_name,
                self._settings.request_timeout_seconds,
            )
            try:
                transport.connect()
                self._transport = transport
                return transport
            except (BridgeConnectionError, BridgeTimeoutError) as error:
                transport.close()
                last_error = error
                if self._start_if_needed and not bridge_started:
                    self._process.start()
                    bridge_started = True
                if time.monotonic() >= deadline:
                    raise BridgeConnectionError(
                        "Could not connect to kompas_bridge before the startup timeout."
                    ) from last_error
                time.sleep(self._retry_delay_seconds)

    def _drop_transport(self) -> None:
        if self._transport is not None:
            self._transport.close()
            self._transport = None
