# ADR 004: Separate Entity-Atom Reference Index for Content Deduplication

## Status

Proposed

## Date

2026-01-12

## Context

GTAF (Graph Transactional Atom Framework) implements a content-addressable storage system where canonical atoms are deduplicated based on their content hash (derived from type tag and value). The current implementation has a critical bug: when multiple entities reference the same canonical value, only the first entity maintains an association with the deduplicated atom.

### The Problem

**Current Implementation:**

The `Atom` class stores a single `entity_id` field:

```cpp
class Atom {
private:
    types::EntityId m_entity_id;  // Single entity reference
    types::AtomId m_atom_id;      // Content hash
    std::string m_type_tag;
    types::AtomValue m_value;
    // ...
};
```

When deduplication occurs in `AtomStore::append()` (src/core/atom_log.cpp:52-56):

```cpp
if (auto it = m_canonical_dedup_map.find(atom_id); it != m_canonical_dedup_map.end()) {
    ++m_dedup_hits;
    return m_atoms[it->second];  // Returns atom with ORIGINAL entity_id
}
```

**Consequence:** The returned atom contains only the original entity's ID. When `ProjectionEngine::rebuild()` filters atoms by entity_id, it cannot find deduplicated atoms for subsequent entities.

**Example Bug Scenario:**

```cpp
// user1 appends "active" status
auto atom1 = store.append(user1, "user.status", "active", Canonical);
// atom1: entity_id=user1, atom_id=hash("user.status", "active")

// user2 appends same "active" status (deduplication)
auto atom2 = store.append(user2, "user.status", "active", Canonical);
// atom2: entity_id=user1 (WRONG!), atom_id=same hash

// Rebuild user2
auto user2_node = projector.rebuild(user2);
// Result: EMPTY (no atoms have entity_id == user2)
```

### Impact

- **Data Loss Risk:** Entities appear to have no data when they reference shared canonical values
- **Scalability Issue:** More entity sharing → worse the problem
- **Architectural Flaw:** Violates content-addressable storage principles (content should be independent of references)

## Decision

We will implement **Option C: Separate Entity-Atom Reference Index**, which completely decouples content storage from entity references.

### Architecture

```
┌─────────────────────────────────────┐
│  CONTENT LAYER (Deduplicated)       │
│  AtomId → Atom (tag, value, type)   │
│  NO entity_id - pure content         │
└─────────────────────────────────────┘
                 ↑
                 │ referenced by
                 │
┌─────────────────────────────────────┐
│  REFERENCE LAYER (Entity Index)     │
│  EntityId → [(AtomId, LSN)]          │
│  Tracks associations                 │
└─────────────────────────────────────┘
                 ↑
                 │
┌─────────────────────────────────────┐
│  GARBAGE COLLECTION LAYER            │
│  AtomId → RefCount                   │
│  Enables cleanup                     │
└─────────────────────────────────────┘
```

### Core Changes

**1. Remove entity_id from Atom class:**

```cpp
class Atom {
private:
    // REMOVED: types::EntityId m_entity_id;

    types::AtomId m_atom_id;      // Content hash
    std::string m_type_tag;
    types::AtomValue m_value;
    types::AtomType m_atom_type;
    // Note: LSN moved to reference layer
};
```

**2. Add reference tracking to AtomStore:**

```cpp
class AtomStore {
private:
    // Content layer (deduplicated storage)
    std::vector<core::Atom> m_atoms;
    std::unordered_map<types::AtomId, size_t> m_content_index;

    // Reference layer (entity-atom associations)
    struct AtomReference {
        types::AtomId atom_id;
        types::LSN lsn;  // LSN is per-entity, not per-atom
    };
    std::unordered_map<types::EntityId, std::vector<AtomReference>> m_entity_refs;

    // Garbage collection support
    std::unordered_map<types::AtomId, uint32_t> m_refcounts;
};
```

**3. Update append logic:**

```cpp
Atom AtomStore::append(EntityId entity, const string& type_tag,
                     const AtomValue& value, AtomType atom_type) {
    AtomId atom_id = generate_atom_id(type_tag, value, atom_type);

    // Check if content exists
    if (auto it = m_content_index.find(atom_id); it != m_content_index.end()) {
        // Content exists - add reference
        ++m_dedup_hits;
        ++m_refcounts[atom_id];
        m_entity_refs[entity].push_back({atom_id, m_next_lsn++});
        return m_atoms[it->second];
    }

    // New content - create atom and add reference
    Atom new_atom(atom_id, type_tag, value, atom_type);
    m_atoms.push_back(new_atom);
    m_content_index[atom_id] = m_atoms.size() - 1;
    m_refcounts[atom_id] = 1;
    m_entity_refs[entity].push_back({atom_id, m_next_lsn++});

    return new_atom;
}
```

**4. Update rebuild logic:**

```cpp
Node ProjectionEngine::rebuild(EntityId entity) const {
    Node node(entity);

    // Get references for this entity (O(1) lookup)
    const auto& refs = m_log.get_entity_atoms(entity);

    // Sort by LSN (chronological order)
    std::vector<AtomReference> sorted_refs = refs;
    std::sort(sorted_refs.begin(), sorted_refs.end(),
              [](const auto& a, const auto& b) { return a.lsn < b.lsn; });

    // Apply atoms in order
    for (const auto& ref : sorted_refs) {
        const Atom& atom = m_store.get_atom(ref.atom_id);
        node.apply(atom.atom_id(), atom.type_tag(), atom.value(), ref.lsn);
    }

    return node;
}
```

## Rationale

### Why Option C Over Alternatives?

**Option A: Store Multiple entity_ids in Atom**
- ❌ Breaks immutability of Atom
- ❌ Complicates LSN tracking (which LSN per entity?)
- ❌ Dynamic vector resizing impacts performance
- ❌ Violates separation of concerns

**Option B: Duplicate Atom Entries Per Entity**
- ❌ 2x memory usage for shared canonical data
- ❌ Doesn't scale with high deduplication rates
- ❌ Storage bloat in persistence layer
- ❌ Patches bug without fixing architecture

**Option C: Separate Reference Index (CHOSEN)**
- ✅ Industry-proven pattern (Git, Perkeep, enterprise systems)
- ✅ True content deduplication (50%+ memory savings)
- ✅ O(atoms_per_entity) queries vs O(all_atoms) scans
- ✅ Clean separation of content and references
- ✅ Enables garbage collection via reference counting
- ✅ Foundation for distributed storage and replication

### Industry Precedents

This architecture is used by all major content-addressable systems:

**1. Git**
- Blobs store content (no metadata about references)
- Trees reference blobs by hash
- Complete separation enables efficient storage and transfer

**2. Perkeep (formerly Camlistore)**
- Blobs are immutable content-addressed objects
- Permanodes are mutable entities with claims
- Claims track which permanodes reference which blobs

**3. Enterprise Deduplication Systems**
- Content index: `content_hash → storage_location`
- Reference table: `(object_id, content_hash) → metadata`
- Reference counting enables garbage collection

**4. Modern Key-Value Stores (DedupKV 2025)**
- Separate metadata management for entity-content relationships
- Inline deduplication with reference tracking
- Optimized chunk-lookup structures

### Performance Analysis

**Memory Overhead:**

```
Current (buggy Option B):
  Per atom: sizeof(Atom) ≈ 128 bytes
  Total: num_entities × avg_atoms × 128 bytes

Proposed (Option C):
  Per unique atom: sizeof(Atom) ≈ 112 bytes (no entity_id)
  Per reference: sizeof(AtomReference) ≈ 40 bytes
  Total: (unique_atoms × 112) + (references × 40) bytes

Example: 1M entities, 100 atoms each, 80% deduplication
  Current: 1M × 100 × 128 = 12.8 GB
  Proposed: (20M × 112) + (100M × 40) = 6.24 GB
  Savings: 51% memory reduction
```

**Query Performance:**

```
Current rebuild(entity):
  Time: O(total_atoms) - linear scan
  Space: O(1)

Proposed rebuild(entity):
  Time: O(atoms_for_entity × log(atoms_for_entity)) - index lookup + sort
  Space: O(atoms_for_entity)

If entities reference <1% of atoms: ~100x speedup
```

## Consequences

### Positive

1. **Bug Fixed:** Multiple entities can correctly reference shared canonical atoms
2. **Performance Improvement:** O(atoms_per_entity) queries instead of O(all_atoms) scans
3. **Memory Efficiency:** 50%+ reduction for highly deduplicated data
4. **Scalability:** Architecture scales with entity count and deduplication rate
5. **Garbage Collection:** Reference counting enables cleanup of unreferenced atoms
6. **Future Features:** Foundation for distributed storage, replication, audit trails
7. **Clean Architecture:** Separation of concerns (content vs references)

### Negative

1. **Breaking Change:** Serialization format changes (requires migration tool)
2. **Code Changes:** All `atom.entity_id()` calls must be removed/refactored
3. **Memory Overhead:** Additional ~40 bytes per entity-atom reference
4. **Complexity:** Two-layer lookup (entity → references → atoms) vs direct access
5. **Migration Cost:** Existing data must be converted to new format

### Neutral

1. **Testing Required:** Comprehensive tests for multi-entity deduplication scenarios
2. **Documentation Update:** Architecture docs must reflect new design
3. **API Changes:** New methods: `get_entity_atoms()`, `get_atom()`, `remove_entity()`

## Implementation Plan

### Phase 1: Core Data Structure Changes (Week 1)
- Remove `entity_id` from Atom class
- Add reference tracking structures to AtomStore
- Update `append()` logic
- Add query methods: `get_entity_atoms()`, `get_atom()`

### Phase 2: Update Query Mechanisms (Week 1)
- Update `ProjectionEngine::rebuild()`
- Update temporal query methods
- Update statistics tracking

### Phase 3: Persistence & Migration (Week 2)
- Design new serialization format (V2)
- Implement backward-compatible loading (V1 → V2)
- Create migration tool for offline conversion
- Update save/load methods

### Phase 4: Testing & Validation (Week 2)
- Unit tests for multi-entity deduplication
- Integration tests for rebuild correctness
- Performance benchmarks (memory, query speed)
- Persistence round-trip tests

### Phase 5: Garbage Collection (Week 3, Optional)
- Implement `remove_entity()` with refcount decrement
- Implement `collect_garbage()` for zero-refcount atoms
- Add periodic GC scheduling
- GC performance tuning

### Phase 6: Documentation & Release (Week 3)
- Update architecture documentation
- Write migration guide for users
- Create example code for new API
- Release notes with breaking changes highlighted

## Verification

### Unit Tests

```cpp
TEST(AtomStoreTest, MultiEntityDeduplication) {
    AtomStore store;
    EntityId user1 = create_entity(1);
    EntityId user2 = create_entity(2);

    // Both entities append same value
    auto atom1 = store.append(user1, "status", "active", Canonical);
    auto atom2 = store.append(user2, "status", "active", Canonical);

    // Content deduplicated
    EXPECT_EQ(atom1.atom_id(), atom2.atom_id());

    // Both entities can retrieve their atoms
    auto refs1 = log.get_entity_atoms(user1);
    auto refs2 = log.get_entity_atoms(user2);

    EXPECT_EQ(refs1.size(), 1);
    EXPECT_EQ(refs2.size(), 1);
    EXPECT_EQ(refs1[0].atom_id, refs2[0].atom_id);
}

TEST(ProjectionEngineTest, RebuildWithSharedAtoms) {
    AtomStore store;
    ProjectionEngine engine(log);

    EntityId user1 = create_entity(1);
    EntityId user2 = create_entity(2);

    store.append(user1, "status", "active", Canonical);
    store.append(user2, "status", "active", Canonical);

    auto node1 = engine.rebuild(user1);
    auto node2 = engine.rebuild(user2);

    // Both nodes should have 1 atom
    EXPECT_EQ(node1.history().size(), 1);
    EXPECT_EQ(node2.history().size(), 1);  // BUG FIX: was 0 before
}

TEST(AtomStoreTest, ReferenceCountingGC) {
    AtomStore store;
    EntityId user1 = create_entity(1);
    EntityId user2 = create_entity(2);

    auto atom = store.append(user1, "status", "active", Canonical);
    store.append(user2, "status", "active", Canonical);

    // Refcount should be 2
    EXPECT_EQ(log.get_refcount(atom.atom_id()), 2);

    // Remove one entity
    log.remove_entity(user1);
    EXPECT_EQ(log.get_refcount(atom.atom_id()), 1);

    // Remove second entity
    log.remove_entity(user2);
    EXPECT_EQ(log.get_refcount(atom.atom_id()), 0);

    // Garbage collection should remove atom
    size_t collected = log.collect_garbage();
    EXPECT_EQ(collected, 1);
}
```

### Integration Test

```bash
# Run existing demo - should show correct output
./build/src/example/gtaf_example

# Expected output change at line 154:
# Before: "Rebuilt 0 atoms for user2"
# After:  "Rebuilt 1 atoms for user2"
```

### Performance Benchmark

```cpp
void BenchmarkRebuildPerformance() {
    AtomStore store;
    const size_t NUM_ENTITIES = 10000;
    const size_t ATOMS_PER_ENTITY = 100;

    // Create entities with 80% deduplication
    for (size_t i = 0; i < NUM_ENTITIES; ++i) {
        EntityId entity = create_entity(i);
        for (size_t j = 0; j < ATOMS_PER_ENTITY; ++j) {
            // 80% chance of shared value
            string value = (rand() % 100 < 80) ? "shared_value" : unique_value();
            store.append(entity, "tag", value, Canonical);
        }
    }

    // Benchmark rebuild performance
    auto start = std::chrono::high_resolution_clock::now();

    ProjectionEngine engine(log);
    for (size_t i = 0; i < 1000; ++i) {
        EntityId entity = create_entity(rand() % NUM_ENTITIES);
        auto node = engine.rebuild(entity);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Average rebuild time: " << duration.count() / 1000.0 << " μs\n";
}

// Expected improvement: 100x faster for entities with <1% of total atoms
```

## References

1. [Git Internals - Git Objects](https://git-scm.com/book/en/v2/Git-Internals-Git-Objects)
2. [Perkeep Documentation](https://perkeep.org/doc/)
3. [US Patent 9268806B1: Efficient Reference Counting in CAS](https://patents.google.com/patent/US9268806)
4. [DEDUPKV: Space-Efficient Deduplication (ICS 2025)](https://hpcrl.github.io/ICS2025-webpage/program/Proceedings_ICS25/ics25-64.pdf)
5. [Distributed Data Deduplication Survey (ACM)](https://dl.acm.org/doi/10.1145/3735508)

For complete analysis and additional references, see:
- [Entity Deduplication Architecture Document](../entity-deduplication-architecture.md)

## Notes

- This ADR supersedes any previous decisions about entity-atom associations
- The migration tool will be critical for production deployments
- Garbage collection (Phase 5) is optional but recommended for long-running systems
- Consider adding metrics/telemetry for deduplication rates and refcount distribution

---

**Decision Date:** 2026-01-12
**Last Updated:** 2026-01-12
**Supersedes:** None
**Superseded By:** None
