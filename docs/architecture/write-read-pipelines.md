# Write and Read Pipelines Architecture

**Status:** Stable  
**Last updated:** 2026-01-18  
**Owner:** GTAF Core  
**Related ADRs:** ADR-001, ADR-002, ADR-004  

---

## 1. Purpose

This document describes the architectural structure of the write and read pipelines
in GTAF, focusing on data flow, responsibilities, and invariants.

The pipelines define how data enters the system, becomes persistent, and is later
observed consistently by readers.

---

## 2. Scope

### In Scope

- Write pipeline stages and responsibilities
- Read pipeline stages and snapshot semantics
- Interaction with append-only storage and reference indexes

### Out of Scope

- Rationale for architectural choices (see ADRs)
- Low-level storage formats
- Query optimization strategies
- Concurrency primitives implementation details

---

## 3. Context

GTAF is built on the following core principles:

- Append-only persistence
- Immutable data structures
- Single-writer, multi-reader access model
- Explicit indexing and relationships

These principles require clearly separated pipelines for writing and reading, with
well-defined boundaries to preserve correctness and historical consistency.

---

## 4. Detailed Description

## 4.1 Write Pipeline Overview

The write pipeline is responsible for transforming incoming data into durable,
append-only records.

Characteristics:

- Single writer
- Deterministic ordering
- No in-place mutation

### 4.1.1 Input Normalization

Incoming data is:

- Validated
- Normalized
- Transformed into candidate atoms

No persistence occurs at this stage.

---

### 4.1.2 Atom Resolution

For each candidate atom:

- Atom identity is computed
- Existing atoms are detected
- New atoms are appended only if necessary

Atom resolution does not mutate existing atoms.

---

### 4.1.3 Reference Creation

For each resolved atom:

- An explicit entity–atom reference is created
- The reference is appended to the reference index

This step guarantees that deduplication does not remove associations.

---

### 4.1.4 Commit Boundary

A write operation is considered complete only after:

- All atoms are resolved
- All references are appended
- All append operations are durably committed

Partial writes are not visible to readers.

---

## 4.2 Read Pipeline Overview

The read pipeline is responsible for reconstructing consistent views of the system
from append-only data.

Characteristics:

- Multi-reader
- Snapshot-based
- Non-blocking with respect to writers

---

### 4.2.1 Snapshot Acquisition

Readers operate on explicit snapshots defined by:

- Atom store high-water mark
- Reference index high-water mark

A snapshot represents a stable, immutable view.

---

### 4.2.2 Data Reconstruction

Within a snapshot:

- Entity–atom references are resolved
- Atoms are materialized as needed
- No write-side state is consulted

Reads are fully deterministic with respect to the snapshot.

---

### 4.2.3 Isolation Guarantees

Readers:

- Never observe partial writes
- Never block the writer
- Never require locks on persisted data

Isolation is achieved structurally, not via locking.

---

## 4.3 Pipeline Separation

Write and read pipelines are strictly separated:

- Writers append
- Readers observe
- No shared mutable state exists between pipelines

This separation is enforced by:

- Append-only persistence
- Snapshot boundaries
- Single-writer constraint

---

## 5. Constraints & Invariants

The following invariants must always hold:

- All persistence is append-only
- Writes are totally ordered
- Readers operate on explicit snapshots
- Partial writes are never observable
- No reader mutates state
- No writer rewrites persisted data

Violating these invariants breaks snapshot consistency.

---

## 6. Trade-offs & Limitations

### Trade-offs

- Snapshot management adds complexity
- Read paths involve indirection through references
- Some real-time visibility is delayed by commit boundaries

### Limitations

- Single-writer limits horizontal write scalability
- Snapshot size grows monotonically
- Readers must handle historical data explicitly

These limitations are accepted to preserve correctness and auditability.

---

## 7. References

- ADR-001: Append-only Persistence
- ADR-002: Single Writer, Snapshot Readers
- ADR-004: Separate Entity–Atom Reference Index
- docs/architecture/entity-deduplication.md
- docs/design/constraints-plan.md
