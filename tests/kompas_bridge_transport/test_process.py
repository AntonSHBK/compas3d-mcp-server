"""Unit tests for bridge child-process ownership."""

from __future__ import annotations

import subprocess
from pathlib import Path

import pytest

from kompas_bridge_transport import BridgeProcessError, BridgeProcessManager


class StubProcess:
    """Minimal live subprocess replacement."""

    def __init__(self) -> None:
        self.running = True
        self.terminate_calls = 0

    def poll(self) -> int | None:
        return None if self.running else 0

    def terminate(self) -> None:
        self.terminate_calls += 1
        self.running = False

    def wait(self, timeout: float) -> int:
        del timeout
        self.running = False
        return 0

    def kill(self) -> None:
        self.running = False


def test_start_passes_configured_pipe_name(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    executable = tmp_path / "kompas_bridge.exe"
    executable.write_bytes(b"test")
    process = StubProcess()
    commands: list[list[str]] = []

    def fake_popen(command: list[str], **_kwargs: object) -> StubProcess:
        commands.append(command)
        return process

    monkeypatch.setattr(subprocess, "Popen", fake_popen)
    manager = BridgeProcessManager(executable, r"\\.\pipe\isolated-test")

    manager.start()
    manager.start()
    manager.stop()

    assert commands == [
        [str(executable), "--pipe", r"\\.\pipe\isolated-test"],
    ]
    assert process.terminate_calls == 1


def test_start_rejects_missing_executable(tmp_path: Path) -> None:
    manager = BridgeProcessManager(
        tmp_path / "missing.exe",
        r"\\.\pipe\isolated-test",
    )

    with pytest.raises(BridgeProcessError):
        manager.start()
