"""Public connection entry points for KOMPAS Core."""

from kompas_bridge_transport import BridgeClient, BridgeClientSettings

from kompas_core._internal.session import CoreSession
from kompas_core.application.application import KompasApplication
from kompas_core.common.types import ConnectionPolicy
from kompas_core.common.validation import validate_connection_policy
from kompas_core.ports.bridge import BridgeClientAdapter


def connect(
    settings: BridgeClientSettings | None = None,
    *,
    bridge_client: BridgeClient | None = None,
    policy: ConnectionPolicy = "attach_only",
) -> KompasApplication:
    """Connect KOMPAS Core through a new or externally managed bridge client."""
    if settings is None and bridge_client is None:
        raise ValueError("settings or bridge_client must be provided.")
    if settings is not None and bridge_client is not None:
        raise ValueError("settings and bridge_client are mutually exclusive.")

    connection_policy = validate_connection_policy(policy)
    owns_client = bridge_client is None
    if bridge_client is None:
        assert settings is not None
        client = BridgeClient(settings)
    else:
        client = bridge_client
    session = CoreSession(
        BridgeClientAdapter(client),
        owns_gateway=owns_client,
    )
    application = KompasApplication(session)
    try:
        application._connect(connection_policy)
    except Exception:
        session.close()
        raise
    return application
