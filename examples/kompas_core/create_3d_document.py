"""Create and save an empty KOMPAS-3D document through KOMPAS Core."""

from __future__ import annotations

import argparse
import os
from pathlib import Path

from kompas_core import connect
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
DEFAULT_OUTPUT_PATH = ROOT_DIR / "artifacts" / "kompas_core" / "core_example.m3d"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--output",
        type=Path,
        default=DEFAULT_OUTPUT_PATH,
        help="Target .m3d file path.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    bridge_path = Path(os.environ.get("KOMPAS_BRIDGE_EXE", DEFAULT_BRIDGE_PATH))
    if not bridge_path.is_file():
        raise FileNotFoundError(f"kompas_bridge executable was not found: {bridge_path}")

    output_path = args.output.resolve()
    output_path.parent.mkdir(parents=True, exist_ok=True)
    settings = BridgeSettings(
        executable_path=bridge_path,
        pipe_name=r"\\.\pipe\kompas-bridge-core-example",
        request_timeout_seconds=10.0,
    )

    with connect(settings) as application:
        status = application.status()
        document = application.create_document_3d(visible=True)
        part = document.top_part()
        document.save_as(output_path, overwrite=True)

        print(f"KOMPAS version: {status.kompas_version}")
        print(f"Document: {document.id}")
        print(f"Top part: {part.id}")
        print(f"Saved: {output_path}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
