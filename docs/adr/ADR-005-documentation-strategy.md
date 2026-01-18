# ADR-005: Documentation Strategy

**Status:** Accepted  
**Date:** 2026-01-18  

---

## Context

As the GTAF project evolves, the amount and variety of documentation has increased, including:
- Architecture Decision Records (ADRs)
- Architectural descriptions
- Internal design documents
- Specifications and contracts
- Operational and contributor guidance

Previously, documentation existed in a flat structure with mixed intent, leading to:
- Blurred boundaries between decisions and descriptions
- Risk of duplication between ADRs and technical documents
- Inconsistent structure and discoverability
- Difficulty enforcing documentation quality and governance

Given the long-term, architecture-driven nature of GTAF, documentation must be:
- Auditable
- Scalable
- Consistent
- Clearly separated by intent

---

## Decision

The project adopts a **structured, “docs as code” documentation strategy** with the following principles:

1. **Architecture Decision Records (ADRs) are the sole source of truth for decisions**
   - ADRs capture *why* decisions were made
   - ADRs are immutable once accepted
   - Decisions are superseded only by new ADRs

2. **All other documentation is descriptive, not decisional**
   - Architecture documents describe *what the system is*
   - Design documents describe *how it works internally*
   - Specifications describe *what must be true*
   - Operations documents describe *how to build, deploy, and run the system*

3. **Documentation is organized by intent, not chronology**
   - Dedicated folders are used for each document type
   - Each folder contains a README defining its purpose and rules

4. **A single canonical document template is enforced**
   - All non-ADR technical documents must include:
     - Status
     - Last updated date
     - Owner
     - Related ADRs
   - A standard section structure is used across documents

5. **Cross-referencing rules are enforced**
   - Documents may reference ADRs
   - ADRs must not reference mutable documents
   - ADR content must not be duplicated elsewhere

6. **Documentation is version-controlled and reviewed like code**
   - Changes follow normal code review practices
   - Documentation quality is considered part of system quality

---

## Consequences

### Positive

- Clear separation between *decision history* and *current system description*
- Improved readability and discoverability of documentation
- Reduced risk of architectural drift or undocumented changes
- Stronger governance suitable for a long-lived framework
- Documentation becomes consumable by both humans and automated tools (e.g. AI assistants)

### Negative / Trade-offs

- Increased upfront discipline when writing documentation
- Additional structure may feel heavy for small or experimental documents
- Contributors must understand and respect documentation boundaries

These trade-offs are considered acceptable given the architectural and longevity goals of the project.

---

## Resulting Structure (Informative)

```
docs/
├── adr/            # Why decisions were made (immutable)
├── architecture/   # System structure (current state)
├── design/         # Internal mechanisms
├── specs/          # Formal contracts and invariants
├── operations/     # Build, deploy, operate
├── STYLE.md        # Coding and architectural rules
└── README.md       # Documentation governance
```
---

## Related

- ADR-001: Append-only Persistence
- ADR-004: Separate Entity–Atom Reference Index
- docs/README.md
- docs/STYLE.md