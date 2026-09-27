set(kompas_sdk_dir "${PROJECT_SOURCE_DIR}/other/SDK" CACHE PATH
    "Path to the KOMPAS-3D SDK")
set(kompas_api5_include_dir "${kompas_sdk_dir}/Include")

if(NOT WIN32)
  message(FATAL_ERROR "kompas_bridge can be built only on Windows.")
endif()
if(NOT MSVC)
  message(FATAL_ERROR "kompas_bridge requires MSVC because API5 uses MFC wrappers.")
endif()
if(NOT EXISTS "${kompas_api5_include_dir}/kapi5.h")
  message(FATAL_ERROR "API5 headers were not found: ${kompas_api5_include_dir}")
endif()

add_library(kompas_bridge_protocol STATIC
  src/protocol/codec.cpp
  src/protocol/error_code.cpp
  src/protocol/request.cpp
  src/protocol/response.cpp
)
target_include_directories(kompas_bridge_protocol PUBLIC
  include
  "${PROJECT_SOURCE_DIR}/third_party/nlohmann"
)
target_compile_features(kompas_bridge_protocol PUBLIC cxx_std_20)

add_library(kompas_bridge_runtime STATIC
  src/app/request_dispatcher.cpp
  src/app/request_loop.cpp
  src/app/routes/application_routes.cpp
  src/app/routes/document_routes.cpp
  src/app/routes/feature_routes.cpp
  src/app/routes/object_routes.cpp
  src/app/routes/sketch_routes.cpp
  src/handlers/application_handler.cpp
  src/handlers/object_handler.cpp
  src/handlers/document_handler.cpp
  src/handlers/feature_handler.cpp
  src/handlers/sketch_handler.cpp
  src/kompas/object_registry.cpp
  src/transport/stdio_transport.cpp
)
target_link_libraries(kompas_bridge_runtime PUBLIC kompas_bridge_protocol)
target_link_libraries(kompas_bridge_runtime PRIVATE ole32)
target_compile_features(kompas_bridge_runtime PUBLIC cxx_std_20)

# Bridge is a console process because its JSON protocol is written to stdout.
add_executable(kompas_bridge
  src/main.cpp
  src/app/bridge_application.cpp
  src/transport/transport.cpp
  src/transport/named_pipe_server.cpp
  src/kompas/api5_application_factory.cpp
  src/kompas/kompas_application_service.cpp
  src/kompas/kompas_session.cpp
  src/kompas/kompas_document_service.cpp
  src/kompas/sketch_service.cpp
  src/kompas/feature_service.cpp
  "${kompas_api5_include_dir}/kapi5.cpp"
  "${kompas_api5_include_dir}/kapi7.cpp"
)

target_link_libraries(kompas_bridge PRIVATE
  kompas_bridge_runtime
  advapi32
)

target_include_directories(kompas_bridge PRIVATE
  include
  src
  src/kompas
  "${kompas_api5_include_dir}"
)

# API5 is activated through the registered external COM server. The generated
# kapi5.cpp wrapper needs API5 headers and MFC, but not kAPI5.lib.
target_compile_definitions(kompas_bridge PRIVATE UNICODE _UNICODE _AFXDLL)
target_compile_features(kompas_bridge PRIVATE cxx_std_20)
set_target_properties(kompas_bridge PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED YES
  CXX_EXTENSIONS NO
)
