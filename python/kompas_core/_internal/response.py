"""Internal bridge response parsing helpers."""

from kompas_core.common.types import JsonObject
from kompas_core.common.validation import require_object


def parse_result(value: object, method: str) -> JsonObject:
    """Validate the object result returned for one bridge method."""
    return require_object(value, f"Result of {method}")
