cmake_minimum_required(VERSION 3.15)
project(GTAF_Framework VERSION 1.1 LANGUAGES CXX)

configure_file(
  ${CMAKE_SOURCE_DIR}/include/version.h.in
  ${CMAKE_BINARY_DIR}/generated/version.h
  @ONLY
)

target_include_directories(gtaf
  PUBLIC
    ${CMAKE_BINARY_DIR}/generated
)

# --- 1. Global Settings ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- 2. Include Directories ---
# This tells the compiler where to find the gtaf headers
include_directories(include)

# --- 3. Library Implementation ---
# We bundle the engine and core logic into a static library
set(LIB_SOURCES
    core/engine.cpp
)

add_library(gtaf_lib STATIC ${LIB_SOURCES})

# --- 4. Executable (The main.cpp example) ---
add_executable(gtaf_example example/main.cpp)

# Link the executable with our library
target_link_libraries(gtaf_example PRIVATE gtaf_lib)

# --- 5. Compiler Warnings & Optimizations ---
if(MSVC)
    target_compile_options(gtaf_lib PRIVATE /W4 /O2)
else()
    target_compile_options(gtaf_lib PRIVATE -Wall -Wextra -O3)
endif()

# --- 6. Success Message ---
message(STATUS "GTAF Framework Build System Configured.")
message(STATUS "GTAF build version: ${GIT_VERSION}")