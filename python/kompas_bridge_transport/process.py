"""Lifecycle management for an optional child kompas_bridge process."""

from __future__ import annotations

import subprocess
from pathlib import Path

from kompas_bridge_transport.errors import BridgeProcessError


class BridgeProcessManager:
    """Start and stop only the bridge process owned by this Python client."""

    def __init__(self, executable_path: Path, pipe_name: str) -> None:
        self._executable_path = Path(executable_path)
        self._pipe_name = pipe_name
        self._process: subprocess.Popen[bytes] | None = None

    @property
    def is_running(self) -> bool:
        """Return whether the owned child process is still alive."""
        return self._process is not None and self._process.poll() is None

    def start(self) -> None:
        """Start bridge in Named Pipe mode if this manager owns no live child."""
        if self.is_running:
            return
        if not self._executable_path.is_file():
            raise BridgeProcessError(
                f"Bridge executable was not found: {self._executable_path}"
            )
        creation_flags = getattr(subprocess, "CREATE_NO_WINDOW", 0)
        try:
            self._process = subprocess.Popen(
                [str(self._executable_path), "--pipe", self._pipe_name],
                stdin=subprocess.DEVNULL,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                creationflags=creation_flags,
            )
        except OSError as error:
            raise BridgeProcessError(
                f"Failed to start bridge executable {self._executable_path}: {error}"
            ) from error

    def stop(self, timeout_seconds: float = 5.0) -> None:
        """Stop the owned bridge child; never terminate an externally started bridge."""
        process = self._process
        self._process = None
        if process is None or process.poll() is not None:
            return
        process.terminate()
        try:
            process.wait(timeout=timeout_seconds)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait(timeout=timeout_seconds)

    def __enter__(self) -> BridgeProcessManager:
        self.start()
        return self

    def __exit__(self, *_: object) -> None:
        self.stop()
