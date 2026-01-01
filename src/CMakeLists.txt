cmake_minimum_required(VERSION 3.15)
project(GTAF_Framework VERSION 0.0.1 LANGUAGES CXX)

# --- 1. Global Settings ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- 2. Include Directories ---
# This tells the compiler where to find the gtaf headers
include_directories(
    core,
    types
)

# --- 2. Version header generation ---
if(NOT DEFINED GTAF_FULL_VERSION)
  set(GTAF_FULL_VERSION "dev")
endif()

message(STATUS "GTAF build version: ${GTAF_FULL_VERSION}")

configure_file(
  ${CMAKE_SOURCE_DIR}/version.h.in
  ${CMAKE_BINARY_DIR}/generated/version.h
  @ONLY
)

# --- 3. Library Implementation ---
set(LIB_SOURCES
    core/engine.cpp
)

add_library(gtaf_lib SHARED
    core/engine.cpp
)

set_target_properties(gtaf_lib PROPERTIES
  WINDOWS_EXPORT_ALL_SYMBOLS ON
)

# Attach include paths to the library (modern CMake)
target_include_directories(gtaf_lib
  PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/generated
)

target_compile_features(gtaf_lib PUBLIC cxx_std_20)

# --- 4. Executable ---
add_executable(gtaf_example example/main.cpp)
target_link_libraries(gtaf_example PRIVATE gtaf_lib)

# --- 5. Compiler Warnings & Optimizations ---
if(MSVC)
  target_compile_options(gtaf_lib PRIVATE /W4)
else()
  target_compile_options(gtaf_lib PRIVATE -Wall -Wextra)
endif()

# --- 6. Messages ---
message(STATUS "GTAF Framework Build System Configured.")
