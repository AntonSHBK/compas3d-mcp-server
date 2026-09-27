"""Windows-specific transport implementation."""

from kompas_bridge_transport._windows.named_pipe import (
    DEFAULT_PIPE_NAME,
    DEFAULT_TIMEOUT_SECONDS,
    MAX_MESSAGE_BYTES,
    NamedPipeClient,
)

__all__ = [
    "DEFAULT_PIPE_NAME",
    "DEFAULT_TIMEOUT_SECONDS",
    "MAX_MESSAGE_BYTES",
    "NamedPipeClient",
]
