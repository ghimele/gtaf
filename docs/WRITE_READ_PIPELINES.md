# GTAF Write & Read Pipelines
## Formal Specification
### Version 1.0

---

## 1. Purpose of This Document

This document specifies the **write and read pipelines** for GTAF (Generalized Typed Atom Framework).

Its goals are to:
- Eliminate ambiguity between logical and physical behavior
- Prevent write amplification and random I/O
- Define where immutability applies and where it does not
- Make performance characteristics explicit
- Enable independent, interoperable implementations

This document is **normative**.  
A compliant GTAF implementation MUST follow the pipelines defined here.

---

## 2. Guiding Principles

1. **Logical immutability ≠ physical immutability**
2. **Write paths are optimized for throughput**
3. **Read paths are optimized for locality**
4. **Derived state is allowed to mutate**
5. **History is preserved at the semantic layer**

---

## 3. High-Level Architecture

       ┌─────────────┐
       │ Application  │
       └──────┬───────┘
              │
      ┌───────▼────────┐
      │ Write Pipeline │
      └───────┬────────┘
              │
      ┌───────▼────────┐
      │ Logical Store  │
      │ (Nodes/Atoms)  │
      └───────┬────────┘
              │
  ┌───────────▼────────────┐
  │ Projection & Indexing  │
  └───────────┬────────────┘
              │
      ┌───────▼────────┐
      │ Read Pipeline  │
      └────────────────┘

---

## 4. Write Pipeline Overview

All writes flow through the following stages:

1. **Classification**
2. **Validation**
3. **Class-Specific Write Path**
4. **Logical Event Logging**
5. **Projection Update (Async)**

---

## 5. Atom Classification (Mandatory)

Every write MUST classify the Atom before storage:

CanonicalAtom | TemporalAtom | MutableAtom

Classification determines:
- Storage layout
- Deduplication behavior
- Index participation
- Vector lifecycle
- Compaction strategy

Misclassification is a fatal error.

---

## 6. Write Pipeline: Canonical Atoms

### 6.1 Intended Use
Low-frequency, high-semantic-value data.

Examples:
- Names
- Status
- Configuration
- Metadata
- Documents

---

### 6.2 Write Path

Input Value
↓
Type Validation
↓
Canonical Hash Computation
↓
Global Deduplication Check
↓
┌───────────────┐
│ Exists?       │
├─────┬─────────┤
│ Yes │ No      │
│     │         │
│Reuse│ Store   │
└─────┴─────────┘
↓
Node Property Repoint
↓
Append Logical Event

---

### 6.3 Guarantees

- Physical immutability
- Global deduplication
- Referential stability
- Full logical history

---

### 6.4 Forbidden Operations

- In-place mutation
- Per-value updates in hot loops
- Use for time-series data

---

## 7. Write Pipeline: Temporal Atoms

### 7.1 Intended Use
High-frequency, append-only data.

Examples:
- IoT sensors
- Logs
- Metrics
- Event streams

---

### 7.2 Write Path

Input Value
↓
Type Validation
↓
Append to Active Chunk
↓
If Chunk Full?
↓
Seal Chunk → Create New Chunk
↓
Append Logical Event (Chunk-Level)

---

### 7.3 Guarantees

- Sequential I/O
- No per-value hashing
- No deduplication
- Chunk-level immutability
- Logical history via chunk lineage

---

### 7.4 Explicit Prohibitions

- Per-value hashing
- Global deduplication
- Random write patterns

---

## 8. Write Pipeline: Mutable Atoms

### 8.1 Intended Use
Derived or aggregated state.

Examples:
- Counters
- Running totals
- Cached aggregates

---

### 8.2 Write Path

Input Delta
↓
Type Validation
↓
In-Place Mutation
↓
Append Delta Event
↓
If Snapshot Threshold?
↓
Emit Snapshot Atom

---

### 8.3 Guarantees

- High write throughput
- Logical history via deltas
- Recoverable state
- Snapshot-based immutability

---

### 8.4 Constraints

- Mutation MUST be logged
- Snapshots MUST be periodic
- Direct exposure of mutable state is forbidden

---

## 9. Logical Event Log (All Writes)

Every write operation MUST append a logical event:

Examples:
- NODE_CREATED
- ATOM_ATTACHED
- TEMPORAL_CHUNK_SEALED
- MUTABLE_DELTA_APPLIED
- SNAPSHOT_EMITTED

The event log is:
- Append-only
- Totally ordered per Node
- Replayable

---

## 10. Projection Layer (Critical)

### 10.1 Purpose

Projections convert the logical model into **read-optimized physical layouts**.

They are:
- Derived
- Mutable
- Rebuildable
- Disposable

---

### 10.2 Projection Types

| Projection | Purpose |
|----------|---------|
| Row | Fast entity reads |
| Column | Analytics |
| Graph Neighborhood | Traversal |
| Hot Set | Caching |
| Vector | Semantic search |

---

### 10.3 Update Semantics

- Projections update **asynchronously**
- Eventual consistency is acceptable
- Projections MUST be rebuildable from the log

---

## 11. Read Pipeline Overview

Reads NEVER traverse raw atom storage directly.

Query
↓
Projection Selection
↓
Locality-Optimized Read
↓
Optional Join / Traverse
↓
Result

---

## 12. Read Pipeline Examples

### 12.1 Entity Read

GET Node(Customer:123)

Resolved via:
- Row projection
- Cached neighborhood

No random atom fetches allowed.

---

### 12.2 Time-Series Read

GET Sensor:77.temperature

Resolved via:
- Temporal chunk scans
- Range reads only

---

### 12.3 Vector Search

VECTOR_SIMILAR(“industrial buyer”) TOP 10

Resolved via:
- Vector projection
- Stable vector handles
- No atom hash coupling

---

## 13. Vector Index Pipeline

### 13.1 Write Behavior

- Vectors reference **Vector Handles**
- Atom updates DO NOT force index deletion
- Old vectors are tombstoned

---

### 13.2 Maintenance

- Lazy pruning
- Offline rebuild
- Atomic index swap

---

## 14. Compaction & GC

### 14.1 Canonical Atoms
- GC only when unreferenced
- Subject to policy

### 14.2 Temporal Atoms
- Chunk expiration by retention
- Sequential compaction

### 14.3 Mutable Atoms
- Delta compaction after snapshot

---

## 15. Consistency Model

| Layer | Consistency |
|----|------------|
| Logical model | Strong |
| Event log | Strong |
| Projections | Eventual |
| Vector index | Eventual |

This separation is intentional and mandatory.

---

## 16. Failure Recovery

Recovery is performed by:
1. Replaying the event log
2. Rebuilding projections
3. Restoring vector indexes

No write-time rollback is required.

---

## 17. Compliance Checklist

A GTAF-compliant implementation MUST:

- Implement all three write pipelines
- Reject invalid atom classification
- Never read raw atom storage directly
- Provide rebuildable projections
- Preserve logical history

---

## 18. Final Principle

> **Writes optimize for truth.  
> Reads optimize for locality.  
> History optimizes for meaning.**

---

## 19. Versioning

This document is versioned independently of the core GTAF spec.

Future versions MAY extend pipelines, but MUST NOT weaken existing guarantees.

---
