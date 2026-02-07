# ADR-006: Unified CLI with Shared Parser and REPL

## Status

Accepted

## Date

2026-01-31

## Context

The GTAF project requires a command-line interface (CLI) that supports both:

- **Non-interactive execution** (argv-based, suitable for scripting, CI/CD, automation)
- **Interactive execution** (persistent REPL session, similar to the SQLite CLI)

Commands must be implemented **once** and reused unchanged across both modes, avoiding duplication and behavioral drift.

---

## Decision

We adopt a **unified CLI architecture** with:

- A shared parser
- A shared command executor
- Two thin frontends (argv and REPL)
- A persistent session object

All inputs are normalized and executed through the same pipeline.

---

## Consequences

### Positive

- No duplication of command logic
- Identical behavior in REPL and batch modes
- High testability
- Future extensibility

### Negative

- Slight increase in architectural complexity

---

## Future Work

- Persistent REPL history
- Autocompletion
- Config-driven execution
- Transactional command groups

---

## References

- SQLite CLI
- Git and kubectl CLIs
- Unix CLI design principles
