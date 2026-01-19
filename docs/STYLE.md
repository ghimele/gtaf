# GTAF C++ Coding Style

## Member Variables

- All private data members MUST use the `m_` prefix

## Naming

- Types: PascalCase
- Functions: snake_case
- Files: snake_case.h / .cpp

## Ownership

- Prefer value semantics
- Use `std::unique_ptr` for ownership
- Use `std::shared_ptr` only for graph edges and atoms

## Exceptions

- Core code MUST be exception-safe
- No throwing across API boundaries
