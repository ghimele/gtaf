cmake_minimum_required(VERSION 3.15)

project(GTAF_Framework
  LANGUAGES CXX
)

# ------------------------------------------------------------
# 1. Global settings
# ------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ------------------------------------------------------------
# 2. Version handling (single source of truth)
# ------------------------------------------------------------
if(NOT DEFINED GTAF_FULL_VERSION)
  set(GTAF_FULL_VERSION "99.99.99.99")
endif()

message(STATUS "GTAF build version: ${GTAF_FULL_VERSION}")

# Generate version header
configure_file(
  ${CMAKE_SOURCE_DIR}/version.h.in
  ${CMAKE_BINARY_DIR}/generated/version.h
  @ONLY
)

# ------------------------------------------------------------
# 3. Library target (now with implementation files)
# ------------------------------------------------------------
add_library(gtaf_lib STATIC
  core/atom_log.cpp
  core/node.cpp
  core/projection_engine.cpp
  core/temporal_chunk.cpp
  # Add more .cpp files here as they are created
)

target_include_directories(gtaf_lib
  PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/generated
)

target_compile_features(gtaf_lib PUBLIC cxx_std_20)

# ------------------------------------------------------------
# 4. Executable target
# ------------------------------------------------------------
add_executable(gtaf_example
  example/main.cpp
)

target_link_libraries(gtaf_example PRIVATE gtaf_lib)

# Embed version in the executable (Linux + Windows)
target_compile_definitions(gtaf_example PRIVATE
  GTAF_VERSION="${GTAF_FULL_VERSION}"
)

# ------------------------------------------------------------
# 5. Windows VERSIONINFO resource
# ------------------------------------------------------------
if(WIN32 AND MSVC)
  # Convert 0.0.1 -> 0,0,1,0
  string(REPLACE "." "," GTAF_VERSION_COMMA "${GTAF_FULL_VERSION}")

  configure_file(
    ${CMAKE_SOURCE_DIR}/windows/gtaf_version.rc.in
    ${CMAKE_BINARY_DIR}/gtaf_version.rc
    @ONLY
  )

  target_sources(gtaf_example PRIVATE
    ${CMAKE_BINARY_DIR}/gtaf_version.rc
  )
endif()

# ------------------------------------------------------------
# 6. Final message
# ------------------------------------------------------------
message(STATUS "GTAF Framework Build System Configured.")
