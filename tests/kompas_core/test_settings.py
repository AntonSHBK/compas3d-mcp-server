"""Tests for Python MCP settings."""

from kompas_mcp.config.settings import BridgeSettings


def test_bridge_settings_use_isolated_bridge_path(bridge_settings: BridgeSettings) -> None:
    """The shared settings fixture must not point to a real bridge executable."""
    assert bridge_settings.executable_path.name == "kompas_bridge.exe"
    assert bridge_settings.executable_path.parent.name.startswith("test_")
    assert bridge_settings.pipe_name == r"\\.\pipe\kompas-bridge-test"
