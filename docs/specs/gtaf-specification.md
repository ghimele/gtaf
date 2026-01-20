# GTAF Specification

**Status:** Stable
**Last updated:** 2026-01-19
**Owner:** Core Team
**Related ADRs:** ADR-001, ADR-002, ADR-003, ADR-004

---

## 1. Purpose

This document defines the GTAF (Generalized Typed Atom Framework) specification—a universal data model and storage framework designed to represent heterogeneous domains without upfront schema design.

GTAF provides:

- Identity separation from value
- Strong typing with three atom classes
- Logical history preservation
- Selective global deduplication
- Backend-agnostic storage

This specification describes what GTAF is, what guarantees it provides, and what must be true for any conforming implementation.

---

## 2. Scope

**In Scope:**

- Core conceptual model (Nodes, Atoms, Edges, Properties)
- Atom classification system and semantics
- Storage model and persistence guarantees
- Query model and projection semantics
- API contracts and invariants
- Implementation status and capabilities

**Out of Scope:**

- Architectural decisions and rationale (see ADRs)
- Internal implementation details (see `docs/design/`)
- Operational procedures (see `docs/operations/`)
- Future roadmap speculation

---

## 3. Context

Modern software systems repeatedly rebuild data models for each domain (CRM, ERP, IoT, CMS, knowledge graphs). This leads to schema churn, migrations, duplicated data, fragmented query engines, and poor long-term evolvability.

GTAF addresses these problems through four core principles:

1. **Identity is separate from value** — Nodes represent identity; Atoms represent values
2. **Values are strongly typed** — All values have explicit types enforced at write time
3. **History is preserved logically** — Every change creates a logical version
4. **Storage is backend-agnostic** — Logical model decouples from physical storage

### 3.1 Problem Statement

| Problem | Description |
| ------- | ----------- |
| Schema Rigidity | Relational and document databases require early schema commitment |
| Data Fragmentation | Same facts duplicated across systems with no shared identity |
| Auditability | Most systems overwrite values, losing historical truth |
| Heterogeneous Queries | Applications need structured, graph, full-text, and vector queries |

---

## 4. Detailed Description

### 4.1 Core Conceptual Model

#### 4.1.1 Node (Identity)

A Node represents an identity-bearing entity. Examples:

- Customer
- Recipe
- Sensor
- Order

**Invariants:**

- Nodes have stable, unique identifiers (`EntityId`)
- Nodes do not store values directly
- Nodes reference values via Properties

#### 4.1.2 Atom (Typed Value)

An Atom is a strongly typed unit of value. Atoms are classified into three categories with different guarantees.

##### Canonical Atoms (Immutable, Deduplicated)

**Use cases:** Names, Status, Configuration, Metadata, Business facts

**Properties:**

- Immutable once created
- Content-addressed identity: `hash(type + value)`
- Globally deduplicated across all entities
- Suitable for semantic reuse

**Invariants:**

- Same content always produces same AtomId
- Multiple entities may reference the same Canonical atom
- Cannot be modified after creation

##### Temporal Atoms (Append-Chunked)

**Use cases:** Time-series, IoT readings, Logs, Metrics

**Properties:**

- Append-only within entity scope
- Chunked for sequential I/O (default: 1000 values per chunk)
- Not deduplicated per value
- Immutable at chunk level once sealed

**Invariants:**

- Sequential IDs within each (entity, tag) stream
- Chunks seal automatically at threshold
- Sealed chunks are immutable

##### Mutable Atoms (Controlled Mutation)

**Use cases:** Counters, Aggregates, Derived state

**Properties:**

- Mutable within bounded scope
- Changes logged as deltas
- Periodic snapshots (default: every 10 deltas)

**Invariants:**

- Same AtomId across mutations
- Full delta history preserved
- Snapshots emitted as Canonical atoms

#### 4.1.3 Property

A Property is a named pointer from a Node to an Atom.

**Structure:** `(EntityId, tag) → AtomId`

**Invariants:**

- Tags are strings
- Multiple atoms may exist for same (entity, tag) over time
- Projection shows latest value per tag

#### 4.1.4 Edge

An Edge is an explicit relationship between Nodes via `EdgeValue`.

**Structure:** `EdgeValue { target: EntityId, relation: string }`

**Invariants:**

- Edges are stored as Atoms (typically Canonical)
- Enable graph traversal and domain modeling

### 4.2 Value Types

| Type | Description | Example |
| ---- | ----------- | ------- |
| `bool` | Boolean | `true`, `false` |
| `int64_t` | 64-bit signed integer | `42`, `-1000` |
| `double` | 64-bit floating point | `3.14159` |
| `string` | UTF-8 text | `"hello"` |
| `vector<float>` | Dense float vector | `[0.1, 0.2, 0.3]` |
| `EdgeValue` | Graph relationship | `{target: id, relation: "owns"}` |
| `monostate` | Null/empty | (no value) |

### 4.3 Ordering and Versioning

#### 4.3.1 Log Sequence Number (LSN)

Each entity maintains its own LSN sequence for ordering.

**Invariants:**

- LSN is monotonically increasing per entity
- LSN determines "latest" value for projections
- Multiple atoms for same (entity, tag) ordered by LSN

#### 4.3.2 Timestamps

Each atom records creation timestamp.

**Invariants:**

- Timestamps are monotonically increasing globally
- Used for temporal queries and auditing

### 4.4 Storage Model

#### 4.4.1 Logical Tables

```text
NODE(node_id, created_at)
ATOM(atom_id, class, type, value, created_at)
EDGE(from_node, to_node, relation)
ENTITY_REF(entity_id, atom_id, lsn, timestamp)
```

The `ENTITY_REF` layer enables:

- Per-entity LSN ordering
- Same Canonical atom referenced by multiple entities
- Efficient entity-scoped queries

#### 4.4.2 Deduplication Semantics

| Atom Class | Deduplicated | Rationale |
| ---------- | ------------ | --------- |
| Canonical | Yes | Content-addressed, semantic reuse |
| Temporal | No | Avoids write amplification for streams |
| Mutable | No | Identity-based, changes in place |

### 4.5 Projection Model

Projections materialize entity state from the atom log.

#### 4.5.1 Node Projection

A Node projection provides:

- **Latest value per property:** O(1) lookup via `get(tag)`
- **All properties:** Full tag → value map via `get_all()`
- **History:** All atoms applied, ordered by LSN via `history()`

**Conflict Resolution:** LSN-based latest-value-wins

#### 4.5.2 Projection Operations

| Operation | Description | Complexity |
| --------- | ----------- | ---------- |
| `rebuild(entity)` | Single entity projection | O(atoms for entity) |
| `rebuild_all()` | All entities in one pass | O(total atoms) |
| `rebuild_all_streaming()` | Memory-bounded streaming | O(total atoms), bounded memory |
| `get_all_entities()` | Entity enumeration | O(1) |

### 4.6 Query Model

Queries operate on materialized projections, not raw atom pointers.

#### 4.6.1 Implemented Query Capabilities

| Capability | API | Complexity |
| ---------- | --- | ---------- |
| Property lookup | `Node::get(tag)` | O(1) |
| All properties | `Node::get_all()` | O(properties) |
| Entity history | `Node::history()` | O(history length) |
| Substring search | `QueryIndex::find_contains()` | O(indexed entities) |
| Exact match | `QueryIndex::find_equals()` | Near O(1) |
| Integer predicates | `QueryIndex::find_int_where()` | O(indexed entities) |
| Temporal range | `query_temporal_range()` | O(matching chunks) |

#### 4.6.2 Query Index

QueryIndex provides efficient field-based querying:

```cpp
QueryIndex index(store, engine);
index.build_indexes({"name", "status", "priority"});

auto results = index.find_contains("name", "sensor");
auto active = index.find_equals("status", "active");
auto high = index.find_int_where("priority", [](int64_t v) { return v > 5; });
```

### 4.7 Batch Operations

For high-throughput ingestion:

```cpp
store.reserve(atom_count, entity_count);
size_t count = store.append_batch(batch);
```

**Guarantees:**

- Single timestamp for entire batch
- Bulk entity reference merging
- 2-3x performance improvement over individual appends

### 4.8 Persistence

#### 4.8.1 Binary Format

Current implementation uses binary file format with:

- Magic number validation (`GTAF`)
- Version numbering for forward compatibility
- 16MB buffered I/O

#### 4.8.2 Persisted State

| Component | Preserved |
| --------- | --------- |
| Atom content and metadata | ✓ |
| Entity-atom associations | ✓ |
| Per-entity LSNs | ✓ |
| Temporal chunk structure | ✓ |
| Mutable state deltas | ✓ |
| Deduplication statistics | ✓ |

#### 4.8.3 Load Behavior

On load, indexes are automatically rebuilt via `rebuild_indexes()`.

---

## 5. Constraints & Invariants

### 5.1 Core Invariants

1. **Identity Separation:** Nodes never store values directly
2. **Type Safety:** All values have explicit types enforced by `std::variant`
3. **LSN Monotonicity:** LSN increases monotonically per entity
4. **Timestamp Monotonicity:** Timestamps increase monotonically globally
5. **Canonical Immutability:** Canonical atoms cannot be modified after creation
6. **Deduplication Correctness:** Same content always produces same Canonical AtomId
7. **History Preservation:** All atoms are retained; projections show latest state

### 5.2 Performance Invariants

| Operation | Guarantee |
| --------- | --------- |
| Canonical append | O(1) with hash lookup |
| Temporal append | O(1) to active chunk |
| Mutable update | O(1) delta log |
| Property lookup | O(1) from projection |
| Entity enumeration | O(1) from reference layer |

### 5.3 Consistency Model

- **Logical Layer:** Strong consistency (single-writer per ADR-002)
- **Projection Layer:** Eventually consistent (rebuilt from log)
- **Vector Index:** Eventually consistent (planned)

---

## 6. Trade-offs & Limitations

### 6.1 Design Trade-offs

| Trade-off | Decision | Rationale |
| --------- | -------- | --------- |
| Deduplication scope | Canonical only | Avoids write amplification for streams |
| Conflict resolution | LSN-based latest-wins | Simple, deterministic, fits append-only model |
| Projection rebuild | Full scan | Enables streaming, avoids complex indexes |

### 6.2 Current Limitations

| Limitation | Status | Priority |
| ---------- | ------ | -------- |
| No constraint validation | Not implemented | P0 |
| Vector search | Storage only | P0 |
| Transaction support | Minimal (auto-commit) | P1 |
| Thread safety | Unspecified | P1 |
| Crash recovery (WAL) | Not implemented | P1 |
| Distributed operation | Not implemented | Future |

### 6.3 Implementation Status

**Alpha v0.0.1 — Core Complete:**

- ✓ Three-tier atom classification
- ✓ Content-addressed deduplication
- ✓ Temporal chunking (1000 values)
- ✓ Mutable delta logging with snapshots
- ✓ Binary persistence with versioning
- ✓ Node projections with O(1) access
- ✓ QueryIndex with filtering
- ✓ Batch operations
- ✓ 28 unit tests passing
- ✓ CI/CD (Linux + Windows)

**Next (Alpha v0.1.0):**

- Constraint system (NOT NULL, UNIQUE, FOREIGN KEY)
- Code cleanup

**Beta v0.1.0:**

- SQLite backend
- Advanced query operators
- Graph traversal API

**GA v1.0.0:**

- Vector embedding layer
- Distributed backends
- Multi-language SDKs

---

## 7. References

### 7.1 Architecture Decision Records

- **ADR-001:** Append-only Persistence — Establishes append-only event log as primary persistence
- **ADR-002:** Single-Writer Snapshot Readers — Defines concurrency model
- **ADR-003:** Atom Representation — Defines struct-based atom representation with enums
- **ADR-004:** Separate Entity-Atom Reference Index — Establishes two-layer architecture

### 7.2 Related Documents

- [Atom Taxonomy](../design/atom-taxonomy.md) — Formal atom classification specification
- [Write-Read Pipelines](../architecture/write-read-pipelines.md) — Pipeline architecture
- [Entity Deduplication](../architecture/entity-deduplication.md) — Deduplication architecture
- [Constraints Plan](../design/constraints-plan.md) — Planned constraint system

### 7.3 API Reference

#### AtomStore

```cpp
class AtomStore {
    AtomId append(EntityId, tag, value, AtomClass);
    size_t append_batch(vector<BatchAtom>);
    void reserve(atom_count, entity_count);
    optional<Atom> get(AtomId);
    vector<AtomRef> get_entity_atoms(EntityId);
    TemporalQueryResult query_temporal_range(entity, tag, start, end);
    TemporalQueryResult query_temporal_all(entity, tag);
    void save(path);
    void load(path);
    Stats get_stats();
};
```

#### ProjectionEngine

```cpp
class ProjectionEngine {
    Node rebuild(EntityId);
    unordered_map<EntityId, Node> rebuild_all();
    void rebuild_all_streaming(callback, batch_size);
    vector<EntityId> get_all_entities();
};
```

#### Node

```cpp
class Node {
    optional<AtomValue> get(tag);
    unordered_map<string, AtomValue> get_all();
    vector<Atom> history();
    optional<AtomId> latest_atom(tag);
};
```

#### QueryIndex

```cpp
class QueryIndex {
    size_t build_index(tag);
    size_t build_indexes(vector<string> tags);
    vector<EntityId> find_contains(tag, substring);
    vector<EntityId> find_equals(tag, value);
    vector<EntityId> find_int_where(tag, predicate);
    optional<string> get_string(tag, entity);
    IndexStats get_stats();
};
```
