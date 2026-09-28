"""Composable selectors over a snapshot of part features."""

from __future__ import annotations

from collections.abc import Iterator, Sequence

from kompas_core.common.errors import (
    CoreSelectionAmbiguousError,
    CoreSelectionNotFoundError,
)
from kompas_core.features.feature import Feature
from kompas_core.features.models import FeatureKind


class FeatureCollection(Sequence[Feature]):
    """Immutable feature snapshot with explicit result cardinality methods."""

    def __init__(self, values: Sequence[Feature]) -> None:
        self._values = tuple(values)

    def __len__(self) -> int:
        return len(self._values)

    def __getitem__(self, index: int | slice) -> Feature | tuple[Feature, ...]:
        return self._values[index]

    def __iter__(self) -> Iterator[Feature]:
        return iter(self._values)

    def by_name(self, name: str) -> FeatureCollection:
        if not name:
            raise ValueError("Feature name cannot be empty.")
        return FeatureCollection([item for item in self if item.info.name == name])

    def of_type(self, feature_type: FeatureKind | str) -> FeatureCollection:
        value = feature_type.value if isinstance(feature_type, FeatureKind) else feature_type
        if not value:
            raise ValueError("Feature type cannot be empty.")
        return FeatureCollection(
            [item for item in self if item.info.feature_type == value]
        )

    def invalid(self) -> FeatureCollection:
        return FeatureCollection([item for item in self if not item.info.valid])

    def all(self) -> list[Feature]:
        return list(self._values)

    def first(self) -> Feature:
        if not self._values:
            raise CoreSelectionNotFoundError("Feature selector matched no objects.")
        return self._values[0]

    def one(self) -> Feature:
        if not self._values:
            raise CoreSelectionNotFoundError("Feature selector matched no objects.")
        if len(self._values) > 1:
            raise CoreSelectionAmbiguousError(
                f"Feature selector matched {len(self._values)} objects."
            )
        return self._values[0]
