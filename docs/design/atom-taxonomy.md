# Atom Taxonomy

**Status:** Stable  
**Last updated:** 2026-01-18  
**Owner:** GTAF Core  
**Related ADRs:** ADR-003  

---

## 1. Purpose

This document defines the taxonomy of atoms used in GTAF, including their
classification, structure, and semantic roles.

The goal is to provide a precise and shared understanding of what an atom is,
how atoms are categorized, and how they are interpreted by the system.

---

## 2. Scope

### In Scope
- Definition of atoms and their core properties
- Atom classification and taxonomy
- Semantic rules governing atom identity and usage

### Out of Scope
- Rationale for atom representation choices (see ADR-003)
- Persistence layout and storage encoding
- Entity–atom reference mechanics
- Query and execution semantics

---

## 3. Context

GTAF represents information using **atoms** as the smallest immutable units
of meaning.

Atoms are:
- Typed
- Immutable
- Semantically meaningful
- Independently reusable across entities

A clear taxonomy is required to:
- Ensure consistent interpretation
- Enable safe deduplication
- Preserve correctness across snapshots

---

## 4. Detailed Description

## 4.1 Atom Definition

An atom represents a single, typed, immutable value.

An atom is defined by:
- A **tag** (semantic label)
- A **type**
- A **normalized value**

Atoms have no identity beyond their semantic content.

---

## 4.2 Atom Identity

Atom identity is derived exclusively from semantic fields:

```
(tag, type, normalized_value)
```

If two atoms share the same identity tuple, they are considered identical and
must be represented by the same atom instance.

Atoms:
- Do not contain entity identifiers
- Do not contain reference counts
- Do not encode usage context

---

## 4.3 Atom Categories

Atoms are classified by their semantic role and value domain.

### 4.3.1 Scalar Atoms

Scalar atoms represent indivisible primitive values.

Examples:
- Numeric values
- Strings
- Booleans
- Timestamps

Properties:
- Single normalized value
- No internal structure

---

### 4.3.2 Structured Atoms

Structured atoms represent values with internal composition that is treated
as a single semantic unit.

Examples:
- Composite identifiers
- Encoded ranges
- Structured literals

Properties:
- Internally structured
- Semantically atomic at the system level

---

### 4.3.3 Reference Atoms

Reference atoms represent identifiers or keys that refer to external or logical
concepts.

Examples:
- External IDs
- Logical keys
- Namespace-qualified identifiers

Properties:
- Value interpreted as a reference
- No dereferencing at the atom level

---

## 4.4 Normalization Rules

All atoms undergo normalization before identity computation.

Normalization ensures:
- Canonical representation
- Deterministic equality
- Stable deduplication

Examples of normalization:
- String trimming and casing rules
- Numeric canonicalization
- Timestamp normalization

Normalization rules are type-specific and deterministic.

---

## 4.5 Atom Lifecycle

Atoms follow a simple lifecycle:

1. Candidate atom is created during write processing
2. Identity is computed after normalization
3. Atom is either reused or appended
4. Atom remains immutable forever

Atoms are never updated, deleted, or invalidated.

---

## 4.6 Atom Semantics and Usage

Atoms:
- Are passive data carriers
- Do not encode behavior
- Do not track relationships

All relationships involving atoms are externalized and explicit.

---

## 5. Constraints & Invariants

The following invariants must always hold:

- Atoms are immutable
- Atom identity is purely semantic
- Equal atoms must be deduplicated
- Atoms contain no entity-specific state
- Normalization is deterministic
- Atom definitions are stable over time

Violating these invariants breaks deduplication and snapshot correctness.

---

## 6. Trade-offs & Limitations

### Trade-offs
- Strict immutability increases indirection
- Normalization adds upfront processing cost
- Atom schema evolution requires care

### Limitations
- Atom types must be defined ahead of time
- Changes to normalization rules are backward-sensitive
- Complex semantics must be modeled outside atoms

These limitations are accepted to preserve consistency and auditability.

---

## 7. References

- ADR-003: Atom Representation (Struct vs Enum)
- docs/architecture/entity-deduplication.md
- docs/design/constraints-plan.md
