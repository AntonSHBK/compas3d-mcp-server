"""Application settings shared by the Python MCP layer."""

from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True, slots=True)
class BridgeSettings:
    """Connection settings for the local kompas_bridge process."""

    executable_path: Path
    pipe_name: str
    request_timeout_seconds: float = 30.0
