"""Fixtures for loading static JSON protocol examples."""

import json
from collections.abc import Callable
from pathlib import Path

import pytest


JsonObject = dict[str, object]
DATA_DIRECTORY = Path(__file__).parent.parent / "data"


@pytest.fixture
def load_json_fixture() -> Callable[[str], JsonObject]:
    """Return a loader for JSON data files relative to the test data directory."""

    def load(relative_path: str) -> JsonObject:
        fixture_path = DATA_DIRECTORY / relative_path
        payload = json.loads(fixture_path.read_text(encoding="utf-8"))
        if not isinstance(payload, dict):
            message = f"Expected a JSON object in {fixture_path}"
            raise ValueError(message)
        return payload

    return load
