"""Reusable bridge-result factories for KOMPAS Core tests."""


def application_status_result() -> dict[str, object]:
    return {
        "connected": True,
        "visible": True,
        "kompas_version": {
            "major": 24,
            "minor": 0,
            "release": 7,
            "build": 0,
        },
    }


def document_result(
    document_id: str = "doc_1",
    *,
    name: str = "Detail",
    active: bool = True,
    part_id: str | None = None,
) -> dict[str, object]:
    result: dict[str, object] = {
        "document_id": document_id,
        "document_type": 5,
        "name": name,
        "file_path": None,
        "active": active,
        "changed": False,
        "read_only": False,
    }
    if part_id is not None:
        result["part_id"] = part_id
    return result


def sketch_result(
    *,
    sketch_id: str = "sketch_1",
    document_id: str = "doc_1",
    closed: bool = False,
    geometry_count: int = 0,
) -> dict[str, object]:
    return {
        "sketch_id": sketch_id,
        "document_id": document_id,
        "closed": closed,
        "geometry_count": geometry_count,
    }


def feature_result(
    *,
    feature_id: str = "feat_1",
    document_id: str = "doc_1",
    kind: str = "extrusion",
    distance: float = 50.0,
    direction: str = "forward",
    operation: str = "new_body",
) -> dict[str, object]:
    return {
        "feature_id": feature_id,
        "document_id": document_id,
        "kind": kind,
        "distance": distance,
        "direction": direction,
        "operation": operation,
    }


def object_info_result(
    handle: str,
    kind: str,
    document_id: str | None,
) -> dict[str, object]:
    return {
        "handle": handle,
        "kind": kind,
        "document_id": document_id,
    }
