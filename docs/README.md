# Project Documentation

This directory contains all project documentation and follows a strict
"docs as code" model.

---

## Documentation Principles

1. Documentation is version-controlled and reviewed like code
2. Architecture decisions are recorded separately as ADRs
3. No duplication between ADRs and documentation
4. Every document has a clear owner and status

---

## Document Types

### Architecture Decision Records (docs/adr/)
- Capture **why** a decision was made
- Immutable once accepted
- Superseded via new ADRs

### Architecture (docs/architecture/)
- Describe the current system structure
- Reference ADRs for rationale

### Design (docs/design/)
- Internal design and implementation details
- May evolve over time

### Specifications (docs/specs/)
- Define contracts and expected behavior
- Must be precise and testable

### Operations (docs/operations/)
- Build, deployment, and runtime procedures

---

## Required Document Structure

All non-ADR documents must include:
- Status
- Last updated date
- Owner
- Related ADRs

All documents must follow the standard template.

---

## Naming Conventions

- File names: kebab-case.md
- No dates in filenames (except ADR numbers)
- One topic per document

---

## Cross-Referencing Rules

- Documents may reference ADRs
- ADRs must not reference mutable documents
- Do not restate ADR decisions in documentation

---

## Lifecycle

- Draft: Work in progress
- Stable: Approved and current
- Deprecated: Kept for historical reference only

---

## Enforcement (Recommended)

- Markdown linting in CI
- Required headers validation
- ADR reference checks