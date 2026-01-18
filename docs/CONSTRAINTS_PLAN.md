# GTAF Constraint System - Implementation Plan

## Executive Summary

This document outlines the design and implementation plan for adding database constraints (NOT NULL, UNIQUE, CHECK, FOREIGN KEY) to GTAF.

**Key Findings:**
- ✅ Constraints are essential for production-ready data integrity
- ✅ Performance impact is minimal (<5% overhead with optimizations)
- ✅ GTAF's architecture is well-suited for constraint validation
- ✅ Foreign Key constraints are critical for validating EdgeValue references

**Status:** Planned for Alpha v0.1.0

---

## Table of Contents

1. [EdgeValue vs Foreign Key Constraint](#edgevalue-vs-foreign-key-constraint)
2. [Current State Analysis](#current-state-analysis)
3. [Design Approach](#design-approach)
4. [Implementation Phases](#implementation-phases)
5. [Performance Analysis](#performance-analysis)
6. [Testing Strategy](#testing-strategy)
7. [API Design](#api-design)
8. [Open Questions](#open-questions)

---

## EdgeValue vs Foreign Key Constraint

### The Problem

GTAF currently supports **EdgeValue** as a data type for representing relationships:

```cpp
struct EdgeValue {
    EntityId target;      // The entity being referenced
    std::string label;    // Relationship type ("friend", "customer", etc.)
    double weight;        // Relationship strength/weight
};
```

**However**, EdgeValue provides **no validation** - you can create edges pointing to entities that don't exist:

```cpp
AtomStore store;
EntityId order = make_entity(1);
EntityId customer999 = make_entity(999); // Never created!

EdgeValue edge{customer999, "customer", 1.0};
store.append(order, "customer", edge, Canonical); // ✓ Succeeds silently

// Later when querying: Data corruption - edge points to nothing!
```

### The Solution: Foreign Key Constraints

Foreign Key constraints validate that referenced entities actually exist in the log:

```cpp
AtomStore store;

// Add FK constraint
log.add_constraint(
    ForeignKey("order.customer", "customer_must_exist")
);

EntityId order = make_entity(1);
EntityId customer999 = make_entity(999); // Not in log

EdgeValue edge{customer999, "customer", 1.0};
store.append(order, "customer", edge, Canonical); // ✗ THROWS!
// Error: "Constraint violation: customer_must_exist - target entity 999 does not exist"
```

### Why Both Are Needed

| Feature | EdgeValue | Foreign Key Constraint |
|---------|-----------|----------------------|
| **Type** | Data type | Validation rule |
| **Purpose** | Express relationships | Ensure relationships are valid |
| **Validation** | None | Validates target exists |
| **Error Detection** | At query time (silent) | At write time (exception) |
| **Performance** | Zero overhead | <3% overhead (with cache) |

**Together they provide:** Type-safe, integrity-checked graph capabilities

---

## Current State Analysis

### Existing Validation ✓

- Type safety via `std::variant<AtomValue>`
- Immutability enforcement (sealed chunks, canonical deduplication)
- LSN monotonicity validation
- Binary format validation (magic number, version)
- Temporal chunk state validation
- Content-addressed uniqueness (canonical atoms)
- EdgeValue support (relationships without validation)

### Missing Constraints ✗

- NOT NULL enforcement
- UNIQUE per tag/property
- FOREIGN KEY / referential integrity
- CHECK constraints (value ranges, patterns)
- DEFAULT values
- Cross-field validation
- Schema-level constraint metadata

---

## Design Approach

### Layered Constraint Architecture

```
┌─────────────────────────────────────┐
│   Application Layer                 │
│   (Define Constraints)              │
└─────────────┬───────────────────────┘
              │
┌─────────────▼───────────────────────┐
│   Schema & Constraint Metadata      │
│   (Stored as Special Atoms)         │
└─────────────┬───────────────────────┘
              │
┌─────────────▼───────────────────────┐
│   ConstraintValidator                │
│   (Pre-append validation hook)       │
│   - Entity cache (O(1) FK lookup)   │
│   - UNIQUE indexes (O(1) lookup)    │
└─────────────┬───────────────────────┘
              │
┌─────────────▼───────────────────────┐
│   AtomStore::append()                  │
│   (Validates before LSN assignment)  │
└──────────────────────────────────────┘
```

### Core Principles

1. **Optional by Default** - Constraints are opt-in, zero overhead when unused
2. **Fail Fast** - Validation at append time, before LSN assignment
3. **Schema as Atoms** - Constraints stored as special atoms (durable)
4. **Classification-Aware** - Different rules for Canonical/Temporal/Mutable
5. **Performance First** - Fast path optimization when no constraints defined
6. **Backward Compatible** - Existing code continues to work

---

## Implementation Phases

### Phase 1: Core Infrastructure (Week 1-2)

#### Files to Create

1. **`src/types/constraints.h`** - Constraint type definitions
2. **`src/types/constraints.cpp`** - Constraint implementations
3. **`src/core/constraint_validator.h`** - Validator class
4. **`src/core/constraint_validator.cpp`** - Validator implementation

#### Files to Modify

5. **`src/core/atom_log.h`** - Add constraint management methods
6. **`src/core/atom_log.cpp`** - Integrate validation in append()

#### Key Components

**Constraint Base Class:**
```cpp
struct Constraint {
    ConstraintType type;
    std::string tag;         // Which property this applies to
    std::string name;        // Constraint name for error messages

    virtual bool validate(
        const AtomValue& value,
        const EntityId& entity,
        const AtomStore& store
    ) const = 0;

    virtual std::string error_message() const = 0;
};
```

**NOT NULL Constraint:**
```cpp
struct NotNullConstraint : Constraint {
    bool validate(const AtomValue& value, ...) const override {
        return !std::holds_alternative<std::monostate>(value);
    }
};
```

**UNIQUE Constraint (with index):**
```cpp
struct UniqueConstraint : Constraint {
    bool validate(const AtomValue& value, ...) const override {
        // O(1) lookup in ConstraintValidator's UNIQUE index
        return !validator.value_exists_for_other_entity(tag, value, entity);
    }
};
```

**CHECK Constraint:**
```cpp
struct CheckConstraint : Constraint {
    using ValidatorFn = std::function<bool(const AtomValue&)>;
    ValidatorFn validator;
    std::string expression;

    bool validate(const AtomValue& value, ...) const override {
        return validator(value);
    }
};
```

**FOREIGN KEY Constraint (OPTIMIZED):**
```cpp
struct ForeignKeyConstraint : Constraint {
    bool validate(
        const AtomValue& value,
        const EntityId& entity,
        const AtomStore& store,
        const ConstraintValidator& validator
    ) const override {
        if (auto* edge = std::get_if<types::EdgeValue>(&value)) {
            // O(1) lookup using validator's entity cache
            return validator.entity_exists(edge->target);
        }
        return false; // Wrong type for FK
    }
};
```

**ConstraintValidator with Caching:**
```cpp
class ConstraintValidator {
public:
    void add_constraint(std::unique_ptr<types::Constraint> constraint);

    struct ValidationResult {
        bool valid;
        std::vector<std::string> errors;
    };

    ValidationResult validate(
        types::EntityId entity,
        const std::string& tag,
        const types::AtomValue& value,
        types::AtomType classification,
        const AtomStore& store
    ) const;

    bool has_constraints(const std::string& tag) const;

    // Entity cache for FK validation (O(1))
    void notify_entity_created(types::EntityId entity);
    bool entity_exists(types::EntityId entity) const;
    void rebuild_entity_cache(const AtomStore& store);

private:
    std::unordered_map<std::string, std::vector<std::unique_ptr<Constraint>>> m_constraints;
    bool m_has_any_constraints = false;

    // O(1) FK validation cache
    mutable std::unordered_set<types::EntityId, EntityIdHash> m_entity_cache;

    // O(1) UNIQUE validation indexes
    mutable std::unordered_map<std::string,
        std::unordered_map<AtomValue, EntityId, AtomValueHash>> m_unique_indexes;
};
```

**AtomStore Integration:**
```cpp
Atom AtomStore::append(
    types::EntityId entity,
    std::string tag,
    types::AtomValue value,
    types::AtomType classification
) {
    // FAST PATH: Skip validation if no constraints for this tag
    if (m_validator.has_constraints(tag)) {
        auto result = m_validator.validate(entity, tag, value, classification, *this);
        if (!result.valid) {
            // Throw with detailed error messages
            std::ostringstream oss;
            oss << "Constraint violation for " << tag << ":\n";
            for (const auto& err : result.errors) {
                oss << "  - " << err << "\n";
            }
            throw std::runtime_error(oss.str());
        }
    }

    // Update entity cache for FK validation
    m_validator.notify_entity_created(entity);

    // Existing append logic...
}

void AtomStore::load(const std::string& filepath) {
    // Existing load logic...

    // Rebuild constraint caches
    m_validator.rebuild_entity_cache(*this);
}
```

### Phase 2: Advanced Features (Week 3)

- Schema persistence (constraints as special atoms)
- Constraint rebuilding on load
- RANGE constraint implementation
- Multi-constraint per tag support

### Phase 3: Testing & Polish (Week 4)

- 30+ unit tests
- Performance benchmarks
- Documentation
- Example code

---

## Performance Analysis

### Expected Overhead

| Constraint Type | Complexity | Overhead | Notes |
|----------------|------------|----------|-------|
| Fast path (no constraints) | O(1) | <1% | Single bool check |
| NOT NULL | O(1) | ~2-3% | Variant type check |
| UNIQUE | O(1) avg | ~5-10% | Hash lookup + index update |
| CHECK | O(1) | ~3-5% | Lambda execution |
| FOREIGN KEY | O(1) avg | ~2-3% | Cached entity set lookup |
| RANGE | O(1) | ~2-3% | Numeric comparison |

### Optimization Strategies

1. **Fast Path** - Zero overhead when no constraints defined
2. **Entity Cache** - O(1) FK validation instead of O(n) entity scan
3. **UNIQUE Indexes** - O(1) uniqueness checks
4. **Lazy Evaluation** - Only validate tags with constraints
5. **Incremental Updates** - Update caches on append, not rebuild every time

### Memory Cost

**Entity Cache:**
- EntityId: 16 bytes
- Hash overhead: ~16 bytes
- Total: ~32 bytes per entity
- 1M entities = ~32MB (acceptable)

**UNIQUE Indexes:**
- Per tag: HashMap<AtomValue, EntityId>
- Varies by value type (strings > numbers)
- Example: 1M unique emails = ~64MB

---

## Testing Strategy

### Unit Tests (30+ tests)

**NOT NULL:**
- ✓ Accept non-null values
- ✓ Reject std::monostate
- ✓ Error message validation

**UNIQUE:**
- ✓ First insert succeeds
- ✓ Duplicate value fails
- ✓ Different entities, same value fails
- ✓ Different values succeed
- ✓ Cross-entity uniqueness

**CHECK:**
- ✓ Range validation (age 0-150)
- ✓ Enum validation (status in ['active', 'inactive'])
- ✓ Custom lambda validation
- ✓ Complex expressions

**FOREIGN KEY + EdgeValue:**
- ✓ Valid edge reference succeeds
- ✓ Edge to non-existent entity fails
- ✓ Edge created before target fails
- ✓ Edge created after target succeeds
- ✓ Multiple edges to same target succeed
- ✓ FK validation with entity cache (O(1))
- ✓ FK validation after load (cache rebuilt)

**Performance:**
- ✓ Fast path has zero overhead
- ✓ NOT NULL <5% overhead
- ✓ UNIQUE <10% overhead
- ✓ FK <3% overhead with cache

### Example Test: FK + EdgeValue

```cpp
TEST(Constraints, ForeignKeyWithEdgeValue) {
    core::AtomStore store;

    // Add FK constraint
    log.add_constraint(
        std::make_unique<types::ForeignKeyConstraint>(
            "order.customer",
            "customer_must_exist"
        )
    );

    auto customer1 = make_entity(1);
    auto order1 = make_entity(100);

    // Create customer first
    store.append(customer1, "name", std::string("Alice"), Canonical);

    // Valid edge - customer exists
    types::EdgeValue valid{customer1, "customer", 1.0};
    ASSERT_NO_THROW(
        store.append(order1, "order.customer", valid, Canonical)
    );

    // Invalid edge - customer999 doesn't exist
    types::EdgeValue invalid{make_entity(999), "customer", 1.0};
    ASSERT_THROWS(
        store.append(order1, "order.customer", invalid, Canonical),
        std::runtime_error
    );
}
```

---

## API Design

### Fluent Constraint Builder

```cpp
AtomStore store;

// NOT NULL
log.add_constraint(
    NotNull("user.email", "email_required")
);

// UNIQUE
log.add_constraint(
    Unique("user.email", "email_unique")
);

// CHECK with lambda
log.add_constraint(
    Check("user.age", "age_valid")
        .range(0, 150)
);

// FOREIGN KEY for edges
log.add_constraint(
    ForeignKey("order.customer", "customer_exists")
);

// Composite constraints
log.add_constraint(
    Composite("user.email")
        .not_null()
        .unique()
);
```

### Error Handling

```cpp
try {
    store.append(user, "user.age", static_cast<int64_t>(-5), Canonical);
} catch (const std::runtime_error& e) {
    std::cout << "Constraint violation: " << e.what() << "\n";
    // Output:
    // Constraint violation for user.age:
    //   - age_valid: value -5 is outside range [0, 150]
}
```

---

## Open Questions

1. **Priority:** P0 for Alpha v0.1.0 or defer to Beta?
2. **Constraint Types:** Focus on NOT NULL, UNIQUE, FK first?
3. **Performance Trade-off:** Is 5-15% write overhead acceptable?
4. **API Style:** Fluent builder vs individual constraint objects?
5. **Schema Storage:** Persist constraints to disk or in-memory only?
6. **Validation Timing:** Always validate or optional "unsafe" fast mode?

---

## Benefits vs. Costs

### Benefits ✅

- **Data Integrity** - Catch bugs at write time
- **Production Readiness** - Meet enterprise standards
- **Developer Experience** - Clear error messages
- **EdgeValue Validation** - Prevent orphaned edges
- **Compliance** - Meet regulatory requirements
- **Debugging** - Easier to trace violations

### Costs ⚠️

- **Development Time** - 3-4 weeks for full implementation
- **Performance** - 5-15% write overhead when enabled
- **Complexity** - More code to maintain
- **API Surface** - More concepts to learn

### Recommendation

**✅ IMPLEMENT CONSTRAINTS**

1. Essential for production use
2. Minimal performance impact with optimizations
3. Strong architectural fit with GTAF
4. Critical for EdgeValue integrity
5. Competitive necessity (all databases have this)

---

## Next Steps

1. ✓ Review and approve this plan
2. Implement Phase 1 (core infrastructure)
3. Add comprehensive tests
4. Benchmark performance
5. Document API and examples
6. Release in Alpha v0.1.0

---

**Document Version:** 1.0
**Last Updated:** 2026-01-09
**Status:** Awaiting approval
