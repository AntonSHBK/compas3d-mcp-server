"""Validation helpers for public Core inputs and bridge responses."""

from collections.abc import Mapping
from os import PathLike
from pathlib import Path
from typing import cast

from kompas_core.common.errors import CoreProtocolError
from kompas_core.common.types import ConnectionPolicy, JsonObject


CONNECTION_POLICIES: frozenset[str] = frozenset(
    {"attach_only", "attach_or_start", "start_new"}
)


def validate_connection_policy(value: str) -> ConnectionPolicy:
    """Validate and narrow a public connection policy value."""
    if value not in CONNECTION_POLICIES:
        raise ValueError(f"Unknown KOMPAS connection policy: {value!r}.")
    return cast(ConnectionPolicy, value)


def validate_file_path(value: str | PathLike[str]) -> Path:
    """Return a nonempty filesystem path without accessing the filesystem."""
    if isinstance(value, str) and not value.strip():
        raise ValueError("Document file path cannot be empty.")
    path = Path(value)
    if not str(path).strip():
        raise ValueError("Document file path cannot be empty.")
    return path


def require_object(value: object, field: str) -> JsonObject:
    """Return a JSON object or raise a Core protocol error."""
    if not isinstance(value, Mapping):
        raise CoreProtocolError(f"{field} must be a JSON object.")
    if not all(isinstance(key, str) for key in value):
        raise CoreProtocolError(f"{field} contains a non-string key.")
    return dict(value)


def require_bool(value: object, field: str) -> bool:
    """Return a strict boolean value."""
    if not isinstance(value, bool):
        raise CoreProtocolError(f"{field} must be a boolean.")
    return value


def require_non_negative_int(value: object, field: str) -> int:
    """Return a non-negative integer that is not a boolean."""
    if isinstance(value, bool) or not isinstance(value, int) or value < 0:
        raise CoreProtocolError(f"{field} must be a non-negative integer.")
    return value
