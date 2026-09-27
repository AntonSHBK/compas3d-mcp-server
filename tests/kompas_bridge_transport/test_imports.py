"""Compatibility tests for public transport imports."""

from kompas_bridge_transport import NamedPipeClient as PublicNamedPipeClient
from kompas_bridge_transport._windows.named_pipe import NamedPipeClient
from kompas_bridge_transport.named_pipe import NamedPipeClient as LegacyNamedPipeClient


def test_public_named_pipe_client_uses_windows_implementation() -> None:
    assert PublicNamedPipeClient is NamedPipeClient


def test_legacy_named_pipe_module_remains_compatible() -> None:
    assert LegacyNamedPipeClient is NamedPipeClient
