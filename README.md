# kompas3d-mcp-server

## KsAPI plugin smoke test

The C++ component is a KsAPI plugin (`.rtw`), loaded by KOMPAS-3D itself. It
is not a standalone executable. On load it receives `ksapi::IApplication`,
registers command handler `1`, and writes lifecycle messages to
`%TEMP%\kompas_mcp_plugin.log`.

Build it with:

```powershell
cmake --preset debug-x64-windows
cmake --build --preset debug-x64-windows
```

The plugin is produced at:

```text
build/debug-x64-windows/src/kompas_bridge/kompas_mcp_bridge.rtw
```

The build also prepares `kompas_mcp_bridge.xml`, a KOMPAS application
manifest with an autostart setting and a visible smoke-test command.

The plugin uses the C++ KsAPI supplied with KOMPAS-3D v24. Python MCP
integration will send commands to this plugin in a later stage.
