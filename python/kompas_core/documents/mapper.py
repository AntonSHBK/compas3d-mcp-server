"""Mapping between bridge responses and document models."""

from pathlib import Path

from kompas_bridge_transport import DocumentHandle, PartHandle

from kompas_core.common.errors import CoreProtocolError
from kompas_core.common.types import JsonObject
from kompas_core.common.validation import (
    require_bool,
    require_non_negative_int,
    require_object,
)
from kompas_core.documents.models import DocumentData, DocumentState


def _required_string(data: JsonObject, field: str) -> str:
    value = data.get(field)
    if not isinstance(value, str) or not value:
        raise CoreProtocolError(f"{field} must be a nonempty string.")
    return value


def _string(data: JsonObject, field: str) -> str:
    value = data.get(field)
    if not isinstance(value, str):
        raise CoreProtocolError(f"{field} must be a string.")
    return value


def _optional_path(data: JsonObject, field: str) -> Path | None:
    value = data.get(field)
    if value is None:
        return None
    if not isinstance(value, str) or not value:
        raise CoreProtocolError(f"{field} must be null or a nonempty string.")
    return Path(value)


def _document_handle(value: str) -> DocumentHandle:
    try:
        return DocumentHandle(value)
    except ValueError as error:
        raise CoreProtocolError("document_id is not a valid document handle.") from error


def _part_handle(value: str) -> PartHandle:
    try:
        return PartHandle(value)
    except ValueError as error:
        raise CoreProtocolError("part_id is not a valid part handle.") from error


def map_document(data: JsonObject) -> DocumentData:
    """Map one document response to validated internal data."""
    part_value = data.get("part_id")
    part_handle = None
    if part_value is not None:
        if not isinstance(part_value, str) or not part_value:
            raise CoreProtocolError("part_id must be a nonempty string when present.")
        part_handle = _part_handle(part_value)

    return DocumentData(
        handle=_document_handle(_required_string(data, "document_id")),
        state=DocumentState(
            name=_string(data, "name"),
            file_path=_optional_path(data, "file_path"),
            document_type=require_non_negative_int(
                data.get("document_type"),
                "document_type",
            ),
            active=require_bool(data.get("active"), "active"),
            changed=require_bool(data.get("changed"), "changed"),
            read_only=require_bool(data.get("read_only"), "read_only"),
        ),
        top_part_handle=part_handle,
    )


def map_document_list(data: JsonObject) -> list[DocumentData]:
    """Map a document.list response."""
    documents = data.get("documents")
    if not isinstance(documents, list):
        raise CoreProtocolError("documents must be a JSON array.")
    return [
        map_document(require_object(item, f"documents[{index}]"))
        for index, item in enumerate(documents)
    ]


def map_top_part(data: JsonObject, document_id: str) -> PartHandle:
    """Map and validate a document.get_top_part response."""
    response_document_id = _required_string(data, "document_id")
    if response_document_id != document_id:
        raise CoreProtocolError("Top part response belongs to another document.")
    return _part_handle(_required_string(data, "part_id"))
