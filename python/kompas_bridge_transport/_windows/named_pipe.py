"""Windows Named Pipe client for the low-level KOMPAS bridge protocol."""

from __future__ import annotations

import ctypes
import json
import struct
import sys
from collections.abc import Mapping
from ctypes import wintypes

from kompas_bridge_transport._windows.api import (
    ERROR_BROKEN_PIPE,
    ERROR_IO_PENDING,
    ERROR_PIPE_BUSY,
    ERROR_SEM_TIMEOUT,
    FILE_FLAG_OVERLAPPED,
    GENERIC_READ,
    GENERIC_WRITE,
    OPEN_EXISTING,
    WAIT_OBJECT_0,
    WAIT_TIMEOUT,
    Overlapped,
    load_kernel32,
)
from kompas_bridge_transport.errors import (
    BridgeConnectionError,
    BridgeProtocolError,
    BridgeTimeoutError,
)


DEFAULT_PIPE_NAME = r"\\.\pipe\kompas-bridge"
DEFAULT_TIMEOUT_SECONDS = 30.0
MAX_MESSAGE_BYTES = 1024 * 1024

JsonObject = dict[str, object]


class NamedPipeClient:
    """Send sequential framed JSON requests to one kompas_bridge process."""

    def __init__(
        self,
        pipe_name: str = DEFAULT_PIPE_NAME,
        timeout_seconds: float = DEFAULT_TIMEOUT_SECONDS,
    ) -> None:
        if sys.platform != "win32":
            raise OSError("KOMPAS Named Pipe transport is available only on Windows.")
        if timeout_seconds <= 0:
            raise ValueError("timeout_seconds must be positive.")
        self._pipe_name = pipe_name
        self._timeout_ms = min(round(timeout_seconds * 1000), 0xFFFFFFFF)
        self._kernel32 = load_kernel32()
        self._handle: int | None = None
        self._event: int | None = None

    def connect(self) -> None:
        """Connect to an existing bridge within the configured timeout."""
        if self._handle is not None:
            return
        if not self._kernel32.WaitNamedPipeW(self._pipe_name, self._timeout_ms):
            error = ctypes.get_last_error()
            if error in {ERROR_SEM_TIMEOUT, ERROR_PIPE_BUSY}:
                raise BridgeTimeoutError(
                    f"Timed out waiting for Named Pipe {self._pipe_name!r}."
                )
            raise BridgeConnectionError(
                f"WaitNamedPipeW failed for {self._pipe_name!r}: Windows error {error}."
            )
        handle = self._kernel32.CreateFileW(
            self._pipe_name,
            GENERIC_READ | GENERIC_WRITE,
            0,
            None,
            OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED,
            None,
        )
        if handle == wintypes.HANDLE(-1).value:
            error = ctypes.get_last_error()
            raise BridgeConnectionError(
                f"CreateFileW failed for {self._pipe_name!r}: Windows error {error}."
            )
        event = self._kernel32.CreateEventW(None, True, False, None)
        if not event:
            error = ctypes.get_last_error()
            self._kernel32.CloseHandle(handle)
            raise BridgeConnectionError(f"CreateEventW failed: Windows error {error}.")
        self._handle = handle
        self._event = event

    def close(self) -> None:
        """Close the client handles without stopping the bridge process."""
        if self._handle is not None:
            self._kernel32.CancelIoEx(self._handle, None)
            self._kernel32.CloseHandle(self._handle)
            self._handle = None
        if self._event is not None:
            self._kernel32.CloseHandle(self._event)
            self._event = None

    def send(self, request: Mapping[str, object]) -> JsonObject:
        """Send one request and return its matching JSON response."""
        self.connect()
        try:
            payload = json.dumps(
                dict(request),
                ensure_ascii=False,
                separators=(",", ":"),
            ).encode("utf-8")
        except (TypeError, ValueError) as error:
            raise BridgeProtocolError("Bridge request is not JSON serializable.") from error
        if not payload or len(payload) > MAX_MESSAGE_BYTES:
            raise ValueError("Bridge request size is outside the protocol limit.")
        self._write_all(struct.pack("<I", len(payload)) + payload)
        (response_size,) = struct.unpack("<I", self._read_exact(4))
        if response_size == 0 or response_size > MAX_MESSAGE_BYTES:
            raise BridgeConnectionError("Bridge returned an invalid frame length.")
        try:
            response = json.loads(self._read_exact(response_size).decode("utf-8"))
        except (UnicodeDecodeError, json.JSONDecodeError) as error:
            raise BridgeProtocolError("Bridge response is not valid UTF-8 JSON.") from error
        if not isinstance(response, dict):
            raise BridgeConnectionError("Bridge response is not a JSON object.")
        if response.get("protocol_version") != request.get("protocol_version"):
            raise BridgeConnectionError("Bridge response protocol version does not match.")
        if response.get("id") != request.get("id"):
            raise BridgeConnectionError("Bridge response id does not match the request.")
        return response

    def __enter__(self) -> NamedPipeClient:
        self.connect()
        return self

    def __exit__(self, *_: object) -> None:
        self.close()

    def _new_operation(self) -> Overlapped:
        if self._handle is None or self._event is None:
            raise BridgeConnectionError("Named Pipe client is not connected.")
        if not self._kernel32.ResetEvent(self._event):
            raise BridgeConnectionError(
                f"ResetEvent failed: Windows error {ctypes.get_last_error()}."
            )
        operation = Overlapped()
        operation.hEvent = self._event
        return operation

    def _wait_for_operation(self, operation: Overlapped) -> int:
        assert self._handle is not None
        assert self._event is not None
        wait_result = self._kernel32.WaitForSingleObject(self._event, self._timeout_ms)
        if wait_result == WAIT_TIMEOUT:
            self._kernel32.CancelIoEx(self._handle, ctypes.byref(operation))
            self._kernel32.WaitForSingleObject(self._event, 0xFFFFFFFF)
            raise BridgeTimeoutError("Timed out waiting for a bridge response.")
        if wait_result != WAIT_OBJECT_0:
            raise BridgeConnectionError(
                f"WaitForSingleObject failed: Windows error {ctypes.get_last_error()}."
            )
        transferred = wintypes.DWORD()
        if not self._kernel32.GetOverlappedResult(
            self._handle,
            ctypes.byref(operation),
            ctypes.byref(transferred),
            False,
        ):
            error = ctypes.get_last_error()
            if error == ERROR_BROKEN_PIPE:
                raise BridgeConnectionError("Bridge disconnected from the Named Pipe.")
            raise BridgeConnectionError(
                f"Named Pipe I/O failed with Windows error {error}."
            )
        return transferred.value

    def _read_exact(self, size: int) -> bytes:
        assert self._handle is not None
        result = bytearray()
        while len(result) < size:
            remaining = size - len(result)
            buffer = ctypes.create_string_buffer(remaining)
            operation = self._new_operation()
            transferred = wintypes.DWORD()
            completed = self._kernel32.ReadFile(
                self._handle,
                buffer,
                remaining,
                ctypes.byref(transferred),
                ctypes.byref(operation),
            )
            if not completed:
                error = ctypes.get_last_error()
                if error != ERROR_IO_PENDING:
                    if error == ERROR_BROKEN_PIPE:
                        raise BridgeConnectionError(
                            "Bridge disconnected from the Named Pipe."
                        )
                    raise BridgeConnectionError(
                        f"ReadFile failed: Windows error {error}."
                    )
                count = self._wait_for_operation(operation)
            else:
                count = transferred.value
            if count == 0:
                raise BridgeConnectionError("Bridge closed the Named Pipe.")
            result.extend(buffer.raw[:count])
        return bytes(result)

    def _write_all(self, data: bytes) -> None:
        assert self._handle is not None
        offset = 0
        while offset < len(data):
            chunk = ctypes.create_string_buffer(data[offset:])
            operation = self._new_operation()
            transferred = wintypes.DWORD()
            completed = self._kernel32.WriteFile(
                self._handle,
                chunk,
                len(data) - offset,
                ctypes.byref(transferred),
                ctypes.byref(operation),
            )
            if not completed:
                error = ctypes.get_last_error()
                if error != ERROR_IO_PENDING:
                    if error == ERROR_BROKEN_PIPE:
                        raise BridgeConnectionError(
                            "Bridge disconnected from the Named Pipe."
                        )
                    raise BridgeConnectionError(
                        f"WriteFile failed: Windows error {error}."
                    )
                count = self._wait_for_operation(operation)
            else:
                count = transferred.value
            if count == 0:
                raise BridgeConnectionError("Bridge closed the Named Pipe.")
            offset += count
