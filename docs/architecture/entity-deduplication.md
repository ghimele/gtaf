# Entity Deduplication Architecture

**Status:** Stable  
**Last updated:** 2026-01-18  
**Owner:** GTAF Core  
**Related ADRs:** ADR-001, ADR-004  

---

## 1. Purpose

This document describes the architecture used by GTAF to deduplicate entities
while preserving append-only persistence, immutability, and historical auditability.

The goal is to ensure that identical logical values are stored once, while allowing
multiple entities to reference them without loss of association or correctness.

---

## 2. Scope

### In Scope

- Deduplication of atom values across entities
- Relationship between entities, atoms, and reference indexes
- Structural guarantees required to preserve correctness

### Out of Scope

- Rationale for choosing deduplication strategies (see ADRs)
- Performance optimizations and indexing internals
- Query execution details

---

## 3. Context

GTAF models information using:

- **Entities** as logical aggregations
- **Atoms** as immutable, typed values
- **Append-only persistence** as a hard invariant

Multiple entities may produce atoms with identical semantic value
(e.g. same tag + value). To avoid duplication while preserving correctness,
the system must:

- Reuse identical atoms
- Preserve all entity-to-atom relationships
- Avoid hidden or implicit coupling

---

## 4. Detailed Description

### 4.1 Atom Identity

Atoms are uniquely identified by their semantic content:

```text
(tag, normalized_value, type)
```

If an atom with identical identity already exists, it is reused.
Atoms themselves are immutable and unaware of which entities reference them.

---

### 4.2 Entity–Atom Relationship

Entities do **not** own atoms.

Instead:

- Entities reference atoms
- References are explicit
- References are append-only

This prevents accidental loss of relationships when deduplication occurs.

---

### 4.3 Reference Index

A dedicated **Entity–Atom Reference Index** is maintained as a first-class structure.

Properties:

- Append-only
- Explicit entity → atom associations
- No back-references inside atoms

Each new association appends a new record:

```text
(entity_id, atom_id)
```

Multiple entities may reference the same atom.
An entity may reference multiple atoms.

---

### 4.4 Deduplication Flow

1. An entity emits a candidate atom
2. The atom identity is computed
3. If the atom already exists:
   - The existing atom_id is reused
4. A new entity–atom reference is appended
5. No existing data is mutated

This ensures that deduplication never removes information.

#### Deduplication Flow Diagram

```mermaid
sequenceDiagram
    participant Entity
    participant AtomStore
    participant ContentIndex
    participant RefIndex as Reference Index

    Entity->>AtomStore: emit(tag, value, type)
    AtomStore->>AtomStore: Compute identity hash(tag, value, type)
    AtomStore->>ContentIndex: Lookup by identity

    alt Atom exists
        ContentIndex-->>AtomStore: Return existing atom_id
    else New atom
        AtomStore->>AtomStore: Create atom with new atom_id
        AtomStore->>ContentIndex: Store (identity → atom_id)
    end

    AtomStore->>RefIndex: Append (entity_id, atom_id)
    Note over RefIndex: Reference always appended<br/>regardless of dedup result
    AtomStore-->>Entity: Return atom_id
```

#### Multiple Entities Sharing Atoms

```mermaid
flowchart LR
    subgraph Entities["Entities"]
        E1[("entity_1")]
        E2[("entity_2")]
        E3[("entity_3")]
    end

    subgraph ContentStore["Content Store (Deduplicated)"]
        A1["Atom A<br/>(tag: status, value: 'active')"]
        A2["Atom B<br/>(tag: status, value: 'inactive')"]
        A3["Atom C<br/>(tag: name, value: 'Alice')"]
    end

    subgraph RefIndex["Reference Index (Append-Only)"]
        R1["(entity_1, Atom A)"]
        R2["(entity_2, Atom A)"]
        R3["(entity_3, Atom B)"]
        R4["(entity_1, Atom C)"]
    end

    E1 --> R1
    E2 --> R2
    E3 --> R3
    E1 --> R4

    R1 --> A1
    R2 --> A1
    R3 --> A2
    R4 --> A3

    style A1 fill:#4A7A4A
```

> In this example, entity_1 and entity_2 both reference Atom A (status: 'active'). The atom is stored once, but both entity relationships are preserved in the Reference Index.

---

### 4.5 Snapshot and Read Semantics

Because both atoms and references are append-only:

- Historical snapshots remain consistent
- Readers observe a stable view
- Deduplication does not affect past reads

Snapshots are defined over:

- Atom store state
- Reference index state

---

## 5. Constraints & Invariants

The following invariants must always hold:

- Atoms are immutable
- Persistence is append-only
- Deduplication must not remove associations
- Entity–atom relationships are explicit
- No atom contains entity-specific state
- No in-place mutation of references is allowed

Violating any of these breaks historical correctness.

---

## 6. Trade-offs & Limitations

### Trade-offs

- Additional storage for reference indexes
- Slightly more complex read paths
- Indirection between entities and atoms

### Limitations

- Reference growth is unbounded by design
- Cleanup or compaction must be modeled explicitly
- Deduplication granularity is limited to atom identity rules

These trade-offs are accepted to preserve auditability and correctness.

---

## 7. References

- [ADR-001: Append-Only Atom Log](../adr/001-append-only-atom-log.md)
- [ADR-004: Entity-Atom Reference Index](../adr/004-entity-atom-reference-index.md)
- [Design: Atom-Node Relationship](../design/atom-node-design.md)
- [Design: Atom Taxonomy](../design/atom-taxonomy.md)
- [Design: Constraints Plan](../design/constraints-plan.md)
