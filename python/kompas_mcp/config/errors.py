"""Application-level errors returned by the MCP layer."""


class BridgeError(RuntimeError):
    """Raised when kompas_bridge cannot complete a request."""
