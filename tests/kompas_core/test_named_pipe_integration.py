"""Integration checks for BridgeClient with a real C++ bridge."""

from __future__ import annotations

import os
from pathlib import Path
from uuid import uuid4

import pytest

from kompas_bridge_transport import BridgeClient
from kompas_core import CoreNoActiveDocumentError, CoreRemoteError, connect
from kompas_mcp.config.settings import BridgeSettings


ROOT_DIR = Path(__file__).resolve().parents[2]
DEFAULT_BRIDGE_PATH = (
    ROOT_DIR
    / "build"
    / "debug-x64-windows"
    / "src"
    / "kompas_bridge"
    / "kompas_bridge.exe"
)


@pytest.mark.integration
def test_core_document_chain_through_real_bridge() -> None:
    """Core traverses application, document, part and object layers."""
    bridge_path = Path(os.environ.get("KOMPAS_BRIDGE_EXE", DEFAULT_BRIDGE_PATH))
    if not bridge_path.is_file():
        pytest.skip(f"Bridge executable was not found: {bridge_path}")
    settings = BridgeSettings(
        executable_path=bridge_path,
        pipe_name=rf"\\.\pipe\kompas-bridge-test-{uuid4().hex}",
        request_timeout_seconds=2.0,
    )

    with BridgeClient(
        settings,
        startup_timeout_seconds=5.0,
        retry_delay_seconds=0.05,
    ) as bridge:
        initial_status = bridge.call("application.status", {})
        if initial_status["connected"] is not True:
            pytest.skip("KOMPAS-3D is not running in the integration test session.")
        with connect(bridge_client=bridge) as application:
            status = application.status()
            documents = application.documents()
            try:
                document = application.active_document()
                part = document.top_part()
                info = part.get_info()
            except CoreNoActiveDocumentError:
                pytest.skip("KOMPAS-3D has no active document.")
            except CoreRemoteError as error:
                pytest.skip(f"Active document does not expose a top part: {error}")

    assert status.connected is True
    assert isinstance(documents, list)
    assert document.id.startswith("doc_")
    assert part.id.startswith("part_")
    assert info.id == part.id
