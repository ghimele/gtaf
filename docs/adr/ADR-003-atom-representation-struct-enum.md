# ADR-003: Atom Representation as Plain Struct with Enum Classification

## Status

Accepted

## Date

2026-01-02

## Context

Atoms are the fundamental data unit in GTAF.  
They represent all persisted facts and all inputs to pipelines and projections.

Early design exploration considered a class hierarchy approach, where different
atom types (e.g., canonical, derived) would be represented via inheritance and
virtual dispatch.

However, GTAF places strong emphasis on:

- Deterministic replay
- Cache-friendly memory layouts
- Simple persistence and serialization
- Low-overhead iteration over large collections of atoms

Given these goals, the representation of atoms has a significant impact on
performance, simplicity, and long-term maintainability.

## Decision

All atoms in GTAF will be represented as a **plain data structure (`struct`)**
with explicit classification via an **enum**, rather than through class
inheritance or virtual dispatch.

There will be:

- A single `Atom` struct
- An `AtomType` (or equivalent) enum to distinguish atom categories
- No virtual methods
- No subclassing

Behavioral differences between atom classes are handled externally by registries,
pipelines, or dispatch logic.

## Rationale

### Memory Locality and Performance

- All atoms share a uniform, contiguous memory layout
- Enables cache-efficient iteration over large atom sets
- Avoids vtable indirection and pointer chasing

### Deterministic Serialization

- Plain structs map cleanly to binary serialization
- No hidden fields or compiler-dependent layout changes
- Simplifies persistence and log replay

### Simplicity

- Eliminates inheritance hierarchies and polymorphic complexity
- Removes the need for `dynamic_cast` or RTTI
- Makes atom handling explicit and predictable

### Separation of Data and Behavior

- Atoms represent **facts**, not behavior
- Behavior belongs in:
  - Pipelines
  - Registries
  - Projections
- This enforces a clean data-flow architecture

### Alignment with Append-Only and Replay Model

- Uniform atom representation simplifies:
  - Log writing
  - Log reading
  - Replay
  - Snapshotting
- Enables straightforward versioning and evolution

## Alternatives Considered

### Class Hierarchy with Virtual Methods

**Rejected**

Pros:

- Encapsulation of behavior
- Familiar object-oriented design

Cons:

- Vtable overhead
- Fragmented memory layout
- Harder serialization
- Increased complexity during replay
- Difficult to reason about performance at scale

### Tagged Union with Separate Types

**Rejected**

Pros:

- Strong compile-time type separation

Cons:

- Increases complexity of dispatch
- Complicates storage and generic handling
- Less flexible for future extensions

### Fully Dynamic / Schema-Based Atoms

**Deferred**

Pros:

- Highly flexible
- Runtime schema evolution

Cons:

- Significant runtime overhead
- Complicates validation and optimization
- Premature for current scope

## Consequences

### Positive

- High-performance iteration and replay
- Simple, stable binary layout
- Easier debugging and inspection
- Clear ownership of behavior outside data
- Predictable memory usage

### Negative

- Behavior must be dispatched externally
- Adding new atom classes requires enum extension
- Less idiomatic for pure OOP designs

These trade-offs are accepted in favor of performance, determinism, and simplicity.

## Implementation Notes

- `Atom` should be a trivially copyable struct where possible
- Example fields (non-exhaustive):
  - `AtomId`
  - `AtomType`
  - Payload (e.g., variant or tagged union)
  - Metadata (timestamps, version, flags)

- Atom interpretation logic belongs in:
  - Pipeline execution
  - Projection builders
  - Registries

## Related Decisions

- ADR-001: Append-Only Log as Persistence Foundation
- ADR-002: Single-Writer with Snapshot Readers Concurrency Model

## References

- Data-oriented design principles
- ECS (Entity Component System) architectures
- Log-structured and event-sourced systems
