"""Application entry point."""

from kompas_mcp.config.logging import configure_logging
from kompas_mcp.server import run_server


def main() -> None:
    """Configure the application and start the MCP server layer."""
    configure_logging()
    run_server()
