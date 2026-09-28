"""Opaque bridge handles without CAD selection or business semantics."""

from __future__ import annotations

from dataclasses import dataclass
from typing import ClassVar


@dataclass(frozen=True, slots=True)
class BridgeHandle:
    """A validated opaque identifier owned by one bridge session."""

    value: str
    prefix: ClassVar[str] = ""

    def __post_init__(self) -> None:
        if not isinstance(self.value, str) or not self.value:
            raise ValueError("Bridge handle must be a nonempty string.")
        if self.prefix and not self.value.startswith(f"{self.prefix}_"):
            raise ValueError(f"Expected a {self.prefix!r} bridge handle.")

    def __str__(self) -> str:
        return self.value


@dataclass(frozen=True, slots=True)
class DocumentHandle(BridgeHandle):
    """Opaque document handle."""

    prefix: ClassVar[str] = "doc"


@dataclass(frozen=True, slots=True)
class PartHandle(BridgeHandle):
    """Opaque part handle."""

    prefix: ClassVar[str] = "part"


@dataclass(frozen=True, slots=True)
class SketchHandle(BridgeHandle):
    """Opaque sketch handle."""

    prefix: ClassVar[str] = "sketch"


@dataclass(frozen=True, slots=True)
class FeatureHandle(BridgeHandle):
    """Opaque feature handle."""

    prefix: ClassVar[str] = "feat"


@dataclass(frozen=True, slots=True)
class BodyHandle(BridgeHandle):
    """Opaque result-body handle."""

    prefix: ClassVar[str] = "body"


@dataclass(frozen=True, slots=True)
class FaceHandle(BridgeHandle):
    """Opaque model-face handle tied to a model revision."""

    prefix: ClassVar[str] = "face"


@dataclass(frozen=True, slots=True)
class EdgeHandle(BridgeHandle):
    """Opaque model-edge handle tied to a model revision."""

    prefix: ClassVar[str] = "edge"
