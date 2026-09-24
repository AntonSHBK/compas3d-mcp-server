"""Fixtures for bridge connection settings."""

from pathlib import Path

import pytest

from kompas_mcp.config.settings import BridgeSettings


@pytest.fixture
def bridge_settings(tmp_path: Path) -> BridgeSettings:
    """Return isolated connection settings for a test bridge process."""
    return BridgeSettings(
        executable_path=tmp_path / "kompas_bridge.exe",
        pipe_name=r"\\.\pipe\kompas-bridge-test",
    )
