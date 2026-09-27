cmake_minimum_required(VERSION 3.24)

set(CMAKE_MFC_FLAG 2)

project(kompas3d_mcp_server LANGUAGES CXX)

include(CTest)

add_subdirectory(src/kompas_bridge)
if(BUILD_TESTING)
  add_subdirectory(tests/kompas_bridge)
endif()
