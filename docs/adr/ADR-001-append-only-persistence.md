# ADR-001: Adopt Append-Only Event Log for Persistence

## Status
Accepted

## Date
2025-12-31

## Context

GTAF requires a persistence model that supports multiple deployment modes:

- Embedded / portable usage (SQLite-like)
- Integration with external databases (e.g. PostgreSQL)
- Standalone server deployment (including containerized and Kubernetes environments)

The persistence layer must preserve GTAF’s core semantics (Atoms, Edges, Pipelines, Projections) while enabling:

- Strong durability guarantees
- Deterministic replay and recovery
- Projection rebuilds
- Future replication and distributed operation

Two primary storage models were evaluated:

- Page-based, mutable storage with Write-Ahead Logging (WAL)
- Append-only, event-log–based storage

## Decision

GTAF will use an **append-only, logical event log** as its primary persistence mechanism.

All state changes are recorded as immutable, ordered events.  
The event log is the **source of truth**.  
Current state is derived via replay and/or snapshots.

No page-based mutable storage is used as the authoritative data store.

## Rationale

### Alignment with GTAF semantics

- GTAF operations are inherently logical (e.g. `PutAtom`, `LinkAtom`, `RunPipeline`)
- Logical operations map naturally to events
- Replaying events reconstructs the same semantic state deterministically

### Simplified durability model

- The append-only log **is the WAL**
- Durability is achieved by fsyncing appended events
- Crash recovery is performed by replaying the log

This removes the need for:
- Page-level WAL
- Complex checkpoint ordering
- Torn-page protection logic

### Support for multiple deployment modes

The same persistence model supports:

- Embedded usage (local log file)
- Server mode (log + network access)
- Replication (streaming the log)

External databases (e.g. PostgreSQL) act as alternative storage backends and do **not** use the GTAF log.

### Future extensibility

An append-only log enables:

- Logical replication
- Time-travel queries
- Auditing and traceability
- Projection rebuilds and schema evolution
- Deterministic testing and simulation

## Consequences

### Positive

- Sequential writes with high throughput
- Single, unified durability mechanism
- Natural replication primitive
- Clear separation between history and derived state
- Storage engine remains semantic, not byte-oriented

### Negative

- Reads depend on derived state (snapshots, indexes)
- Log grows indefinitely without compaction
- Snapshot and compaction mechanisms are mandatory
- Read path is more complex than page-based storage

These trade-offs are accepted and addressed via snapshots and background compaction.

## Architectural Implications

### Event Log

- Append-only
- Totally ordered
- Transaction-bounded
- Immutable once written

Conceptual example:

```text
TX_BEGIN
PutAtom(id=123, class=Canonical, payload=…)
LinkAtom(from=123, to=456, type=DependsOn)
TX_COMMIT