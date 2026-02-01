# GTAF Development Guide for AI Agents

This file contains essential information for AI agents working on the GTAF codebase.

---

## Project Overview

GTAF (Generalized Typed Atom Framework) is a universal data model and storage framework that separates identity from value, preserves logical history, and supports selective deduplication. It targets OLTP, IoT, and AI/RAG workloads.

## Build Commands

### CMake Build

```bash
# Configure build (run from project root)
cmake -S src -B build -DGTAF_FULL_VERSION="1.0.0"

# Build all targets
cmake --build build

# Build specific target
cmake --build build --target gtaf_test

# Clean build
rm -rf build && cmake -S src -B build
```

### Testing

```bash
# Run all tests
cd build && ./gtaf_test

# Run tests via CTest
cd build && ctest

# Documentation checks
./scripts/docs-check.sh [base_branch]
```

Custom test framework in `src/test/test_framework.h` using `ASSERT_TRUE`, `ASSERT_FALSE`, `ASSERT_EQ`, `ASSERT_NE` macros.

---

## Code Style Guidelines

### File Organization

- **Headers**: `#pragma once` at top, local includes first, then system includes
- **Sources**: Include corresponding header first, then dependencies
- **File naming**: `snake_case.h` and `snake_case.cpp`
- **Directory structure**: `src/core/` for main classes, `src/types/` for type definitions

### Naming Conventions

- **Types**: `PascalCase` (classes, structs, enums)
- **Functions**: `snake_case` (methods, free functions)
- **Private members**: `m_` prefix (e.g., `m_atoms`, `m_entity_refs`)
- **Constants**: `k_` prefix for static constexpr values
- **Namespaces**: `gtaf::subpackage::` pattern

### C++ Standards and Features

- **Standard**: C++20 required
- **Prefer**: `[[nodiscard]]` for pure functions
- **Use**: `noexcept` for non-throwing functions
- **Classes**: Use `final` keyword unless inheritance is intended
- **Const correctness**: Heavy emphasis on const methods and references

### Documentation Style

- **Public APIs**: Use Doxygen comments with `@brief`, `@param`, `@return` in the header file
- **Detailed explanations**: add detailed explanations (@details) in the .cpp file.
- **Section dividers**: Use `// ---- Section Name ----` in headers
- **Implementation**: Inline comments for complex algorithms
- **File headers**: Single line purpose description

---

## Import Patterns

### Include Organization

```cpp
// Local includes first
#include "atom_store.h"
#include "../types/types.h"

// Standard library includes
#include <vector>
#include <unordered_map>
#include <memory>
```

### Forward Declarations

- Prefer full includes over forward declarations for template parameters
- Use forward declarations only for circular dependencies

---

## Error Handling Patterns

### Return Values

- **Optional returns**: Use `const T*` with `nullptr` for not found
- **Success/failure**: Use `bool` return values for simple operations
- **Exceptions**: Custom exceptions only in test framework

### Assertions

- **Test framework**: Use `ASSERT_TRUE(condition)`, `ASSERT_EQ(a, b)` macros
- **Runtime checks**: Use `[[nodiscard]]` and `noexcept` for compile-time safety

---

## Type System Patterns

### Type Aliases

- **Core types**: Use aliases from `gtaf::types` namespace
- **Common patterns**:

  ```cpp
  using Timestamp = uint64_t;  // Microseconds since epoch
  using Hash = std::string;   // Content-addressed ID
  using NodeID = std::string;  // Unique identity anchor
  ```

### Hash Functions

- **Custom hashes**: Implement for custom types used in unordered containers
- **Pattern**: Use `memcpy` for binary data, combine with XOR for multi-part keys

---

## Memory Management

### Ownership Patterns

- **Prefer**: Value semantics and automatic storage
- **Unique ownership**: Use `std::unique_ptr` for owned resources
- **Shared ownership**: Use `std::shared_ptr` only for graph structures
- **Raw pointers**: Use for non-owning references and optional returns

### Performance Patterns

- **Move semantics**: Use `std::move` for string and container parameters
- **Reserve**: Pre-allocate containers for batch operations
- **Copy avoidance**: Pass by reference for large objects, return by value for small ones

---

## GTAF-Specific Patterns

### Atom Classification

- **Canonical**: Immutable, content-addressed, globally deduplicated
- **Temporal**: Sequential IDs, chunked storage for time-series
- **Mutable**: Sequential IDs with delta logging

### Entity-Centric Design

- **EntityId**: 16-byte stable identifiers
- **AtomReference**: Links entities to atoms with LSN ordering
- **Indexing**: Custom hash functions for efficient entity lookups

### Persistence Model

- **Append-only**: Never modify existing data in-place
- **WAL**: Write-Ahead Logging for durability
- **Chunking**: Temporal data in rotation-based chunks

---

## Testing Guidelines

### Test Structure

- **Test files**: `test/test_*.cpp` naming convention
- **Test framework**: Custom lightweight framework in `test/test_framework.h`
- **Assertions**: Use `ASSERT_*` macros with automatic file/line reporting

### Test Patterns

```cpp
void test_atom_creation() {
    // Arrange
    auto store = AtomStore{};
    auto entity = EntityId{...};
    
    // Act
    auto atom = store.append(entity, "test.tag", value, AtomType::Canonical);
    
    // Assert
    ASSERT_TRUE(!atom.id.is_nil());
    ASSERT_EQ(atom.tag, "test.tag");
}
```

---

## Development Workflow

### Before Making Changes

1. Read relevant ADRs in `docs/adr/`
2. Review similar existing code for patterns
3. Ensure changes respect append-only invariants

### Architecture Changes

- Require new ADRs before implementation
- Must update relevant documentation
- Consider migration and compatibility implications

### Performance Considerations

- Hash distribution critical for entity lookups
- Cache-friendly data layout for atom storage
- Minimize allocations in hot paths

---

## Common Gotchas

### Include Guards

- Use `#pragma once` everywhere (no traditional guards)

### Hash Functions

- Always provide custom hash for custom types used in `unordered_map`
- Use all bytes of binary identifiers for better distribution

### String Handling

- Use `std::move` for string parameters to avoid copies
- Prefer string_view for read-only string parameters

### Template Instantiation

- Template definitions must be in header files
- Use explicit instantiation for large templates to reduce compile time

---

## Build System Notes

### CMake Organization

- Static library: `gtaf_lib`
- Multiple executables for examples and tests
- Version handling via CMake `GTAF_FULL_VERSION`
- Cross-platform support (Linux/Windows)

### Dependencies

- Minimal external dependencies
- STL-only for core functionality
- Optional: benchmarking tools in separate targets

---

## Source Organization

- `src/core/`: Main framework (atom_store, projection_engine, query_index, node, persistence)
- `src/types/`: Type definitions (types.h, hash_utils.h, values_helper.h)
- `src/test/`: Test suite
- `src/example/`: Example programs
- `docs/adr/`: Architecture Decision Records (read before making architecture changes)

---

This guide covers the essential patterns and conventions for working with GTAF. Always prioritize correctness and explicit behavior over clever optimizations.
