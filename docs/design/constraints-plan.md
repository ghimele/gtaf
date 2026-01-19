# Constraints Plan

**Status:** Stable  
**Last updated:** 2026-01-18  
**Owner:** GTAF Core  
**Related ADRs:** ADR-001, ADR-002, ADR-003, ADR-004  

---

## 1. Purpose

This document defines the structural constraints and invariants that govern the
behavior of GTAF.

The purpose of these constraints is to ensure correctness, historical consistency,
and architectural integrity across all components and over time.

---

## 2. Scope

### In Scope
- Global architectural invariants
- Component-level constraints
- Enforcement boundaries and expectations

### Out of Scope
- Rationale for constraints (see ADRs)
- Implementation-specific enforcement mechanisms
- Testing strategies and validation tooling

---

## 3. Context

GTAF is designed as a long-lived, append-only, immutable system.

In such systems:
- Errors compound over time
- Silent violations are difficult to correct
- Historical correctness is a first-class requirement

Explicit constraints are therefore required to:
- Prevent accidental architectural drift
- Enable safe evolution
- Make violations immediately visible

---

## 4. Detailed Description

## 4.1 Global Invariants

The following invariants apply to the entire system:

- All persisted data is append-only
- Persisted data is immutable once written
- Writes are totally ordered
- Reads operate on explicit snapshots
- Historical state is never rewritten

These invariants must hold across all components.

---

## 4.2 Atom Constraints

Atoms must satisfy the following constraints:

- Atoms are immutable
- Atom identity is purely semantic
- Atoms contain no entity-specific information
- Equal atoms must be deduplicated
- Atom normalization is deterministic

Atoms must not:
- Track references
- Encode behavior
- Contain mutable fields

---

## 4.3 Entity Constraints

Entities must satisfy the following constraints:

- Entities do not own atoms
- Entities reference atoms explicitly
- Entity–atom relationships are append-only
- Entity state is reconstructible from references

Entities must not:
- Mutate atoms
- Implicitly assume atom uniqueness
- Encode deduplication logic internally

---

## 4.4 Reference Index Constraints

The entity–atom reference index must satisfy:

- Append-only updates
- No in-place mutation
- Explicit entity → atom mapping
- Deterministic iteration order

The reference index must not:
- Collapse or remove historical references
- Store derived or implicit relationships

---

## 4.5 Write Pipeline Constraints

The write pipeline must enforce:

- Single-writer execution
- Deterministic ordering of writes
- Atomic visibility at commit boundaries
- No partial persistence

The write pipeline must not:
- Expose intermediate state
- Rewrite persisted data
- Perform speculative mutation

---

## 4.6 Read Pipeline Constraints

The read pipeline must enforce:

- Snapshot-based reads
- Isolation from concurrent writes
- Deterministic reconstruction

The read pipeline must not:
- Observe partial writes
- Block the writer
- Mutate persisted state

---

## 4.7 Evolution Constraints

System evolution must satisfy:

- Schema changes are additive
- Breaking changes require new constructs
- Historical data remains readable
- Behavioral changes are explicit

Any change violating existing invariants requires:
- A new ADR
- Explicit migration strategy

---

## 5. Constraints & Invariants Summary

The following principles summarize the constraints plan:

- Immutability over convenience
- Explicitness over inference
- Structure over convention
- Auditability over optimization

These principles are non-negotiable.

---

## 6. Trade-offs & Limitations

### Trade-offs
- Increased upfront complexity
- Additional indirection and metadata
- Reduced flexibility for ad-hoc changes

### Limitations
- Certain optimizations are intentionally disallowed
- Some invariants restrict future design space
- Constraint enforcement requires discipline

These trade-offs are accepted to ensure long-term correctness.

---

## 7. References

- ADR-001: Append-only Persistence
- ADR-002: Single Writer, Snapshot Readers
- ADR-003: Atom Representation
- ADR-004: Separate Entity–Atom Reference Index
- docs/architecture/entity-deduplication.md
- docs/architecture/write-read-pipelines.md