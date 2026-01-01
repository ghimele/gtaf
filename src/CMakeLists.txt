cmake_minimum_required(VERSION 3.15)
project(GTAF_Framework VERSION 0.0.1 LANGUAGES CXX)

# --- 1. Global Settings ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(DEFINED GTAF_BUILD_NUMBER)
  set(GTAF_FULL_VERSION
      "${PROJECT_VERSION}.${GTAF_BUILD_NUMBER}")
else()
  set(GTAF_FULL_VERSION "${PROJECT_VERSION}")
endif()

message(STATUS "GTAF build version: ${GTAF_FULL_VERSION}")

# --- 2. Version header generation ---
configure_file(
  ${CMAKE_SOURCE_DIR}/include/version.h.in
  ${CMAKE_BINARY_DIR}/generated/version.h
  @ONLY
)

# --- 3. Library Implementation ---
set(LIB_SOURCES
    core/engine.cpp
)

add_library(gtaf_lib STATIC ${LIB_SOURCES})

# Attach include paths to the library (modern CMake)
target_include_directories(gtaf_lib
  PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/generated
)

# --- 4. Executable ---
add_executable(gtaf_example example/main.cpp)
target_link_libraries(gtaf_example PRIVATE gtaf_lib)

# --- 5. Compiler Warnings & Optimizations ---
if(MSVC)
    target_compile_options(gtaf_lib PRIVATE /W4 /O2)
else()
    target_compile_options(gtaf_lib PRIVATE -Wall -Wextra -O3)
endif()

# --- 6. Messages ---
message(STATUS "GTAF Framework Build System Configured.")
