# GTAF Atom Taxonomy
## Formal Specification
### Version 1.0

---

## 1. Purpose of This Document

This document defines the **official Atom taxonomy** used by GTAF (Generalized Typed Atom Framework).

Its goals are to:
- Prevent write amplification
- Preserve performance under high-frequency workloads
- Maintain logical immutability without imposing physical immutability
- Clearly separate semantic guarantees from storage mechanics
- Enable production-grade implementations

This taxonomy is **normative**. Any implementation claiming GTAF compatibility MUST conform to the rules defined here.

---

## 2. Definition of an Atom

An **Atom** is the smallest typed unit of value in GTAF.

All Atoms:
- Are strongly typed
- Are referenced by Nodes via properties
- Participate in logical history
- Are never implicitly overwritten

However, **not all Atoms are physically immutable**.

---

## 3. Atom Classes (Normative)

GTAF defines **three Atom classes**.  
Each Atom MUST belong to exactly one class.

Atom
├─ CanonicalAtom
├─ TemporalAtom
└─ MutableAtom

---

## 4. Canonical Atoms

### 4.1 Purpose

Canonical Atoms represent **semantic facts** that are:
- Low-frequency in change
- High in semantic reuse
- Stable in meaning over time

Examples:
- Names
- Status values
- Titles
- Descriptions
- Configuration flags
- Business metadata

---

### 4.2 Properties

| Property | Requirement |
|--------|------------|
| Physical mutability | Forbidden |
| Logical mutability | Allowed via versioning |
| Identity | Content-addressed |
| Deduplication | Global |
| Hashing | Mandatory |
| Storage layout | Packed / segment-based |
| Vector embedding | Optional |

---

### 4.3 Identity & Deduplication

Canonical Atoms are identified by:
hash = HASH(type + canonical_value)

If an Atom with the same hash already exists, it MUST be reused.

Global deduplication applies across:
- Nodes
- Domains
- Storage backends

---

### 4.4 Update Semantics

Updating a Canonical Atom:
1. Create a new Atom
2. Repoint the Node property
3. Preserve the old Atom indefinitely (subject to GC policy)

Canonical Atoms are never mutated in place.

---

### 4.5 Allowed Types

- String
- Int
- Float
- Bool
- Date / Time
- Enum
- Object (structured, immutable)
- List (immutable)

---

## 5. Temporal Atoms

### 5.1 Purpose

Temporal Atoms represent **high-frequency, append-only data**.

Examples:
- IoT sensor readings
- Logs
- Metrics
- Time-series measurements
- Event streams

---

### 5.2 Properties

| Property | Requirement |
|--------|------------|
| Physical mutability | Append-only |
| Deduplication | Forbidden |
| Hashing | Chunk-level only |
| Storage layout | Sequential, chunked |
| Write pattern | High throughput |
| Read pattern | Range-based |
| Vector embedding | Optional (batch-derived) |

---

### 5.3 Chunking Model

Temporal Atoms MUST be stored as **chunks**:

TemporalAtom
├─ Chunk_001 (values[])
├─ Chunk_002 (values[])
└─ Chunk_003 (values[])


Rules:
- Chunks are immutable once sealed
- Writes append to the active chunk
- Chunk size is implementation-defined
- History is preserved via chunks, not per-value atoms

---

### 5.4 Update Semantics

Appending a value:
- Writes sequentially
- MUST NOT perform deduplication
- MUST NOT update the global atom index

This is mandatory to avoid write amplification.

---

### 5.5 Allowed Types

- Int
- Float
- Bool
- Timestamped structs
- Binary payloads

---

## 6. Mutable Atoms

### 6.1 Purpose

Mutable Atoms represent **derived or aggregated state** where:
- Updates are frequent
- Per-update immutability is not required
- Logical history must still be reconstructible

Examples:
- Counters
- Running totals
- Cached aggregates
- Materialized metrics

---

### 6.2 Properties

| Property | Requirement |
|--------|------------|
| Physical mutability | Allowed (controlled) |
| Deduplication | Optional / local only |
| Hashing | Not content-addressed |
| History | Delta-logged |
| Snapshotting | Mandatory |
| Vector embedding | Snapshot-based only |

---

### 6.3 Update Semantics

Updating a Mutable Atom:
1. Mutate value in place
2. Append a delta event to the log
3. Periodically emit a snapshot Atom

Snapshots MAY be represented as Canonical Atoms.

---

### 6.4 History Guarantees

Despite physical mutation:
- All deltas are logged
- Historical reconstruction MUST be possible
- Logical immutability is preserved

---

### 6.5 Allowed Types

- Int
- Float
- Composite numeric structs

---

## 7. Vector Embedding Compatibility

Vector embeddings follow special rules.

| Atom Class | Vector Strategy |
|----------|-----------------|
| Canonical | Stable, versioned |
| Temporal | Optional, batch-derived |
| Mutable | Snapshot-based |

Vectors MUST NOT be tightly coupled to Atom identity.

Instead, Atoms reference a **Vector Handle**:
- Vectors may be updated or versioned
- Old vectors may be tombstoned
- Index compaction happens asynchronously

---

## 8. Forbidden Usage Patterns (Normative)

The following are **explicitly forbidden**:

- Using Canonical Atoms for IoT or high-frequency streams
- Deduplicating Temporal Atom values
- Treating Mutable Atoms as immutable facts
- Binding vector index nodes directly to atom hashes
- Performing per-value hashing for high-frequency data

Implementations violating these rules are **non-compliant**.

---

## 9. Garbage Collection Rules

- Canonical Atoms MAY be GC’d only when unreferenced and policy allows
- Temporal Atom chunks MAY be expired by retention policies
- Mutable Atom deltas MAY be compacted after snapshot emission

GC MUST preserve logical history guarantees.

---

## 10. Design Rationale (Non-Normative)

This taxonomy exists to:
- Avoid write amplification
- Preserve cache locality
- Enable time-series workloads
- Prevent vector index churn
- Maintain conceptual clarity

> **Immutability without classification is dogma.  
> GTAF treats immutability as a tool, not a religion.**

---

## 11. Compliance Statement

A system claiming GTAF compatibility MUST:
- Implement all three Atom classes
- Enforce class-specific semantics
- Reject forbidden usage patterns
- Preserve logical history guarantees

---

## 12. Versioning

This document is versioned independently of the core GTAF specification.

Future versions MAY introduce additional Atom classes, but MUST NOT weaken the guarantees of existing ones.

---

