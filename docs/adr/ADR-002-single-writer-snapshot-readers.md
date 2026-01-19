# ADR-002: Single-Writer with Snapshot Readers Concurrency Model

## Status

Accepted

## Date

2026-01-02

## Context

GTAF is built on an append-only log as its persistence foundation (see ADR-001).  
Given this storage model, a clear concurrency strategy is required to define:

- How writes are serialized
- How reads are served concurrently
- How consistency and determinism are preserved
- How the system evolves toward more advanced concurrency models

The framework must remain:

- Deterministic
- Simple to reason about
- Suitable for embedded and server-side use cases

## Decision

GTAF will adopt a **single-writer, multi-reader concurrency model**, where:

- Exactly one writer is allowed to append to the log at any time
- Readers operate on **immutable snapshots** of in-memory state
- Writers do not block readers
- Readers never observe partial or inconsistent state

## Rationale

### Determinism and Ordering

- A single writer enforces a total, well-defined order of events
- This guarantees deterministic replay and reproducibility
- Eliminates ambiguity in pipeline execution order

### Simplicity

- Avoids complex locking, MVCC, or conflict resolution logic
- Reduces surface area for concurrency bugs
- Keeps the initial implementation small and verifiable

### Alignment with Append-Only Persistence

- Append-only logs naturally favor serialized writes
- Eliminates the need for write-write conflict handling
- Crash recovery is straightforward and predictable

### Snapshot-Friendly Reads

- Readers operate on immutable data structures or versioned state
- Read performance scales with the number of readers
- Long-running reads do not block writes

### Incremental Evolution

This model provides a stable foundation from which to evolve toward:

- Batched writers
- Leader-based multi-writer replication
- Distributed append pipelines

without breaking the core architecture.

## Alternatives Considered

### Multi-Writer with Locking

**Rejected**

Pros:

- Higher theoretical write throughput

Cons:

- Complex locking semantics
- Increased risk of deadlocks and contention
- Harder to guarantee determinism
- Difficult crash recovery

### Optimistic Concurrency (CAS / Compare-and-Swap)

**Rejected**

Pros:

- Reduced blocking

Cons:

- Retry storms under contention
- Still requires conflict resolution
- Adds complexity without immediate benefit

### MVCC (Multi-Version Concurrency Control)

**Deferred**

Pros:

- Excellent read scalability
- Common in mature databases

Cons:

- High implementation complexity
- Requires version tracking, garbage collection
- Overkill for initial GTAF scope

### Distributed Consensus (Raft / Paxos)

**Explicitly Out of Scope**

Pros:

- Strong consistency across nodes

Cons:

- Heavy operational and conceptual cost
- Premature for current goals

## Consequences

### Positive

- Simple and predictable write path
- Strong consistency guarantees
- Easy reasoning about system state
- Clean separation between write and read concerns
- Excellent foundation for replay-based recovery

### Negative

- Write throughput limited to a single writer
- Requires snapshotting for long-lived readers
- Not suitable for high-write distributed workloads (by design)

These limitations are accepted for the initial and mid-term scope of GTAF.

## Implementation Notes

- Writer:
  - Owns the append-only log
  - Applies atoms sequentially
  - Advances the global state version

- Readers:
  - Observe immutable snapshots
  - May lag behind the writer
  - Never block or interfere with writes

- Snapshots:
  - Represent a consistent point-in-time view
  - May be memory-only initially
  - Persistence of snapshots is deferred to a later ADR

## Related Decisions

- ADR-001: Append-Only Log as Persistence Foundation

## References

- Event sourcing architectures
- Log-structured storage engines
- Single-writer database designs (e.g., SQLite)
