"""Application state and value models."""

from dataclasses import dataclass


@dataclass(frozen=True, slots=True)
class KompasVersion:
    """Installed KOMPAS-3D version reported by the bridge."""

    major: int
    minor: int
    release: int
    build: int


@dataclass(frozen=True, slots=True)
class ApplicationStatus:
    """Current connection state of the KOMPAS application."""

    connected: bool
    visible: bool | None
    kompas_version: KompasVersion | None
