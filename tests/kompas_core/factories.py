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


def feature_info_result(
    *,
    feature_id: str = "feat_1",
    document_id: str = "doc_1",
    part_id: str = "part_1",
    name: str = "Extrusion1",
    feature_type: str = "extrusion",
    excluded: bool = False,
    valid: bool = True,
    owner_feature_id: str | None = None,
    update_stamp: int = 1,
) -> dict[str, object]:
    return {
        "feature_id": feature_id,
        "document_id": document_id,
        "part_id": part_id,
        "name": name,
        "feature_type": feature_type,
        "excluded": excluded,
        "valid": valid,
        "owner_feature_id": owner_feature_id,
        "update_stamp": update_stamp,
    }


def face_result(
    *,
    face_id: str = "face_1",
    body_id: str | None = "body_1",
    surface_type: str = "plane",
    area_mm2: float = 2500.0,
    normal: dict[str, float] | None = None,
    radius_mm: float | None = None,
    owner_feature_id: str | None = "feat_1",
    z: float = 50.0,
) -> dict[str, object]:
    return {
        "face_id": face_id,
        "part_id": "part_1",
        "body_id": body_id,
        "surface_type": surface_type,
        "area_mm2": area_mm2,
        "normal": normal,
        "radius_mm": radius_mm,
        "owner_feature_id": owner_feature_id,
        "bounding_box": {
            "min": {"x": -25.0, "y": -25.0, "z": z},
            "max": {"x": 25.0, "y": 25.0, "z": z},
        },
    }


def body_result(body_id: str = "body_1") -> dict[str, object]:
    return {
        "body_id": body_id,
        "part_id": "part_1",
        "solid": True,
        "owner_feature_id": "feat_1",
        "bounding_box": {
            "min": {"x": -25.0, "y": -25.0, "z": 0.0},
            "max": {"x": 25.0, "y": 25.0, "z": 50.0},
        },
    }


def mass_properties_result() -> dict[str, object]:
    return {
        "mass_kg": 1.56,
        "volume_mm3": 200000.0,
        "area_mm2": 28000.0,
        "density_kg_m3": 7800.0,
        "center_of_mass": {"x": 0.0, "y": 0.0, "z": 40.0},
        "moments_of_inertia": {
            "jx": 1.0,
            "jy": 2.0,
            "jz": 3.0,
            "jxy": 0.0,
            "jxz": 0.0,
            "jyz": 0.0,
        },
    }


def bounding_box_result() -> dict[str, object]:
    return {
        "min": {"x": -25.0, "y": -25.0, "z": 0.0},
        "max": {"x": 25.0, "y": 25.0, "z": 50.0},
    }


def distance_result() -> dict[str, object]:
    return {
        "distance_mm": 50.0,
        "point1": {"x": 0.0, "y": 0.0, "z": 0.0},
        "point2": {"x": 0.0, "y": 0.0, "z": 50.0},
        "maximum_distance_mm": 70.71,
        "maximum_point1": {"x": -25.0, "y": -25.0, "z": 0.0},
        "maximum_point2": {"x": 25.0, "y": 25.0, "z": 50.0},
        "normal_distance_mm": 50.0,
        "normal_point1": {"x": 0.0, "y": 0.0, "z": 0.0},
        "normal_point2": {"x": 0.0, "y": 0.0, "z": 50.0},
    }


def angle_result() -> dict[str, object]:
    return {"angle_degrees": 90.0}


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
