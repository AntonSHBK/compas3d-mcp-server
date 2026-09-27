"""Document state and value models."""

from dataclasses import dataclass
from pathlib import Path

from kompas_bridge_transport import DocumentHandle, PartHandle


@dataclass(frozen=True, slots=True)
class DocumentState:
    """User-visible state of an open KOMPAS document."""

    name: str
    file_path: Path | None
    document_type: int
    active: bool
    changed: bool
    read_only: bool


@dataclass(frozen=True, slots=True)
class DocumentData:
    """Internal document mapping result with validated bridge handles."""

    handle: DocumentHandle
    state: DocumentState
    top_part_handle: PartHandle | None = None
