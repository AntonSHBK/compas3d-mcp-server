"""Shared type aliases and value types."""

from typing import Literal


JsonObject = dict[str, object]
ConnectionPolicy = Literal["attach_only", "attach_or_start", "start_new"]
ObjectKind = Literal["document", "part", "sketch", "feature", "body", "face", "edge"]
