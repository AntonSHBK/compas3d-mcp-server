"""Mapping between bridge responses and application models."""

from kompas_core.application.models import ApplicationStatus, KompasVersion
from kompas_core.common.types import JsonObject
from kompas_core.common.validation import (
    require_bool,
    require_non_negative_int,
    require_object,
)


def map_application_status(data: JsonObject) -> ApplicationStatus:
    """Map an application status response to a typed Core model."""
    connected = require_bool(data.get("connected"), "connected")

    visible_value = data.get("visible")
    visible = None if visible_value is None else require_bool(visible_value, "visible")

    version_value = data.get("kompas_version")
    version = None
    if version_value is not None:
        version_data = require_object(version_value, "kompas_version")
        version = KompasVersion(
            major=require_non_negative_int(version_data.get("major"), "kompas_version.major"),
            minor=require_non_negative_int(version_data.get("minor"), "kompas_version.minor"),
            release=require_non_negative_int(
                version_data.get("release"),
                "kompas_version.release",
            ),
            build=require_non_negative_int(version_data.get("build"), "kompas_version.build"),
        )

    return ApplicationStatus(
        connected=connected,
        visible=visible,
        kompas_version=version,
    )
