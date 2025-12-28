# GTAF — Generalized Typed Atom Framework

GTAF is a **universal data model and storage framework** designed to represent heterogeneous domains without upfront schema design, while remaining viable for real-world workloads such as OLTP, IoT, and AI systems.

GTAF separates **identity from value**, preserves **logical history**, supports **selective deduplication**, and integrates **graph and vector semantics** — without falling into the common traps of naïve immutable or content-addressed systems.

📄 See `WHITEPAPER.md` for the full technical specification and design rationale.

---

## Core Principles

GTAF is built on five non-negotiable principles:

1. **Identity is separate from value**
2. **Values are strongly typed**
3. **History is preserved logically**
4. **Immutability is a semantic contract, not a physical rule**
5. **Physical storage must optimize for locality and write efficiency**

---

## Core Concepts

### Node
A **Node** represents identity (e.g. Customer, Recipe, Sensor).

Nodes:
- Have stable IDs
- Do not store values directly
- Reference values via properties

---

### Atom (Typed Value)

An **Atom** is a typed unit of value.  
Atoms are classified into **three categories**, each optimized for different workloads:

#### 1. Canonical Atoms
Used for business facts and metadata:
- Names
- Status
- Configuration
- Documents

Properties:
- Immutable
- Content-addressed
- Globally deduplicated

#### 2. Temporal Atoms
Used for high-frequency data:
- IoT readings
- Logs
- Metrics
- Time-series

Properties:
- Append-only
- Chunked for sequential I/O
- Not deduplicated per value
- Immutable at chunk level

#### 3. Mutable Atoms
Used for:
- Counters
- Aggregates
- Derived state

Properties:
- Controlled mutation
- Delta-logged
- Periodically snapshotted

> Logical history is preserved for all atom classes, even when physical mutation is used.

---

### Property
A **Property** is a named pointer from a Node to an Atom.

Node ── property ──▶ Atom


---

### Edge
An **Edge** is an explicit relationship between Nodes.

Edges are used for:
- Graph traversal
- Domain relationships
- Structural modeling

---

## Architecture Overview

### Logical Model
- Nodes
- Atom classes
- Edges
- Version semantics

### Physical Model
- Projection layers (row, column, graph neighborhood)
- Storage engines optimized for locality
- Append-only logs and compaction
- Asynchronous index maintenance

> Logical immutability does not imply physical immutability.

---

## Query Model

GTAF supports **hybrid querying** over materialized projections:

- Structured filters
- Relationship traversal
- Aggregations
- Vector similarity search

Examples:
FIND Customer WHERE status = "active"
TRAVERSE Order FROM Customer:123
VECTOR_SIMILAR("industrial equipment buyer") TOP 10


---

## Vector Search (Corrected Model)

Vector embeddings are **decoupled from atom immutability**.

- Atoms reference a stable vector handle
- Vectors may be updated or versioned
- Index compaction happens asynchronously
- High write rates do not cause index churn

This aligns GTAF with production vector systems (HNSW-based).

---

## What GTAF Is (and Is Not)

**GTAF is:**
- A universal typed data substrate
- Schema-optional but type-safe
- History-preserving
- AI-ready by design

**GTAF is not:**
- A naïve immutable store
- A pure content-addressed filesystem
- A replacement for all physical databases
- A vector database pretending to be transactional

---

## Example Use Cases

- **CRM / ERP**  
  Canonical atoms for identity, temporal atoms for activity, mutable atoms for aggregates

- **Recipe / Content Platforms**  
  Canonical atoms + graph relationships

- **IoT / Telemetry**  
  Chunked temporal atoms + analytical projections

- **AI / RAG Systems**  
  Structured truth + semantic embeddings in one model

---

## Project Status

🚧 **Design-first, early implementation phase**

Current focus:
- Finalizing core spec
- Formalizing atom taxonomy
- Defining write/read pipelines
- Building a minimal reference implementation

---

## License

Apache 2.0 (proposed)

---

## Vision

> GTAF aims to be an **operating system for data** —  
> preserving truth, enabling scale, and remaining honest about physics.

See `WHITEPAPER.md` for the full corrected architecture.
