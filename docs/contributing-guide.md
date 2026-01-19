# Contributor Guide

**Status:** Stable
**Last updated:** 2026-01-19
**Owner:** Core Team
**Related ADRs:** ADR-005

---

## 1. Purpose

This guide defines the minimum rules for contributing to GTAF.
All contributors (human or AI-assisted) must follow these rules.

---

## 2. Scope

**In Scope:**

- Core contribution principles
- Documentation rules and boundaries
- Architecture change requirements
- Code contribution rules
- AI-assisted contribution guidelines

**Out of Scope:**

- Contribution workflow/mechanics (see [CONTRIBUTING.md](../CONTRIBUTING.md))
- Code style details (see [STYLE.md](STYLE.md))

---

## 3. Core Principles

GTAF is designed as a long-lived, append-only, immutable system.

As a contributor, you must prioritize:

- **Correctness over convenience**
- **Explicit structure over implicit behavior**
- **Auditability over short-term optimization**

If a change compromises these principles, it is not acceptable.

---

## 4. Documentation Rules

Documentation is treated as code.

### Mandatory Reading

Before contributing, you must be familiar with:

- [docs/README.md](README.md)
- [docs/STYLE.md](STYLE.md)
- [docs/specs/gtaf-specification.md](specs/gtaf-specification.md)
- Relevant ADRs in [docs/adr/](adr/)

### Decision vs Description

- **Decisions** go into ADRs
- **Descriptions** go into architecture or design documents
- Do not mix the two

If you are changing *why* something exists, you must write an ADR.

---

## 5. Architecture Changes

An architecture change is any change that:

- Alters invariants
- Changes persistence behavior
- Affects atom, entity, or reference semantics
- Modifies read/write pipeline guarantees

All architecture changes require:

1. A new ADR
2. Explicit documentation updates
3. Clear migration or compatibility implications

No exceptions.

---

## 6. Code Contribution Rules

All code must:

- Respect append-only persistence
- Avoid in-place mutation of persisted data
- Make invariants explicit
- Avoid hidden or implicit state

All code must **not**:

- Encode architectural rationale (decisions belong in ADRs)
- Circumvent constraints for performance
- Introduce silent behavior changes

If a constraint is inconvenient, it must be discussed—not bypassed.

---

## 7. AI-Assisted Contributions

AI tools are treated as contributors.

When using AI:

- Ensure it follows [STYLE.md](STYLE.md)
- Ensure it respects all ADRs
- Review outputs for invariant violations

"The AI generated it" is not an acceptable justification.

---

## 8. When in Doubt

If you are unsure:

- Prefer explicitness
- Prefer safety
- Ask before changing invariants

Uncertainty is acceptable.
Silent architectural drift is not.

---

## 9. Summary

- Read the docs
- Respect the constraints
- Write ADRs for decisions
- Keep the system auditable

GTAF values long-term integrity over short-term speed.
