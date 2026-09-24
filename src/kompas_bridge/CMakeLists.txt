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

# Bridge is a console process because its JSON protocol is written to stdout.
add_executable(kompas_bridge
  src/main.cpp
  src/app/application.cpp
  src/commands/create_cube_command.cpp
  src/commands/ping_command.cpp
  src/common/bridge_error.cpp
  src/kompas/api5_application_factory.cpp
  src/kompas/kompas_session.cpp
  "${kompas_api5_include_dir}/kapi5.cpp"
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
