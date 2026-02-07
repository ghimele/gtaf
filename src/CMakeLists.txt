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
  core/atom_store.cpp
  core/node.cpp
  core/projection_engine.cpp
  core/query_index.cpp
  core/temporal_chunk.cpp
  core/mutable_state.cpp
  core/persistence.cpp
  # Add more .cpp files here as they are created
)

target_include_directories(gtaf_lib
  PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/generated
)

target_compile_features(gtaf_lib PUBLIC cxx_std_20)

# ------------------------------------------------------------
# 4. Executable targets
# ------------------------------------------------------------
add_executable(gtaf_example
  example/main.cpp
)

target_link_libraries(gtaf_example PRIVATE gtaf_lib)

add_executable(gtaf_history_example
  example/history_example.cpp
)

target_link_libraries(gtaf_history_example PRIVATE gtaf_lib)

add_executable(gtaf_import_workrequest
  example/import_workrequest.cpp
)

target_link_libraries(gtaf_import_workrequest PRIVATE gtaf_lib)

add_executable(gtaf_query_workrequest
  example/query_workrequest.cpp
)

target_link_libraries(gtaf_query_workrequest PRIVATE gtaf_lib)

add_executable(gtaf_cli
  example/gtaf_cli.cpp
)

target_link_libraries(gtaf_cli PRIVATE gtaf_lib)

add_executable(gtaf_query_workrequest_indexed
  example/query_workrequest_indexed.cpp
)

target_link_libraries(gtaf_query_workrequest_indexed PRIVATE gtaf_lib)

add_executable(gtaf-cli
  cli/main.cpp
  cli/types.h
  cli/parser/parser.h
  cli/parser/parser.cpp
  cli/session/session.h
  cli/executor/executor.h
  cli/executor/executor.cpp
  cli/frontends/argv.h
  cli/frontends/argv.cpp
  cli/frontends/repl.h
  cli/frontends/repl.cpp
  cli/commands/command_base.h
  cli/commands/stats_command.h
  cli/commands/stats_command.cpp
  cli/commands/load_command.h
  cli/commands/load_command.cpp
  cli/commands/save_command.h
  cli/commands/save_command.cpp
)

target_link_libraries(gtaf-cli PRIVATE gtaf_lib)

# ------------------------------------------------------------
# TPC-H Benchmark Tools
# ------------------------------------------------------------
add_executable(gtaf_tpch_import
  test/tpch/tpch_import.cpp
)

target_link_libraries(gtaf_tpch_import PRIVATE gtaf_lib)

add_executable(gtaf_tpch_import_fast
  test/tpch/tpch_import_fast.cpp
)

target_link_libraries(gtaf_tpch_import_fast PRIVATE gtaf_lib)

add_executable(gtaf_tpch_query
  test/tpch/tpch_query.cpp
)

target_link_libraries(gtaf_tpch_query PRIVATE gtaf_lib)

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
# 6. Test executable
# ------------------------------------------------------------
add_executable(gtaf_test
  test/test_main.cpp
  test/test_atom_store.cpp
  test/test_persistence.cpp
  test/test_node.cpp
  test/test_cli_parser.cpp
  cli/parser/parser.cpp
)

target_link_libraries(gtaf_test PRIVATE gtaf_lib)

target_compile_definitions(gtaf_test PRIVATE
  GTAF_VERSION="${GTAF_FULL_VERSION}"
)

# ------------------------------------------------------------
# 7. Enable CTest and add tests
# ------------------------------------------------------------
enable_testing()

add_test(NAME gtaf_tests COMMAND gtaf_test)

# ------------------------------------------------------------
# 8. Final message
# ------------------------------------------------------------
message(STATUS "GTAF Framework Build System Configured.")
