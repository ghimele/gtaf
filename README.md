# GTAF — Generalized Typed Atom Framework

GTAF is a universal data model and storage framework designed to handle heterogeneous data without schema design.

It combines:
- Strong typing
- Immutable history
- Global deduplication
- Graph relationships
- Native vector search

---

## Core Idea
Node = identity
Atom = immutable typed value
Property = Node → Atom pointer
Edge = Node → Node relationship


Values never mutate. Updates create new Atoms.

---

## Why GTAF?
Traditional systems force you to choose:
- SQL or NoSQL
- Graph or documents
- Search or vector DB

GTAF unifies all of them.

---

## Status
🚧 Early design & prototype phase  
📄 See `WHITEPAPER.md` for full specification

---

## License
Apache 2.0 (proposed)

---

> GTAF aims to become an operating system for data, not just another database.
