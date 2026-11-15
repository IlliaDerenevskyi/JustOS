# Just — Project Vision

> This document defines **why** Just exists.
>
> It establishes the engineering philosophy that guides the entire project.
> Implementation details, architecture and development processes are intentionally documented elsewhere.

---

# Why Just Exists

Modern software systems often become increasingly complex as they evolve.

Over time, implementation tends to dominate architecture, documentation becomes outdated, and engineering decisions become difficult to understand or reproduce.

Just exists to explore a different engineering philosophy.

The project is founded on the belief that software should be engineered as a coherent system where architecture, documentation, methodology and implementation evolve together throughout the entire lifetime of the project.

Just is not simply another desktop environment.

It is an engineering system designed to produce a consistent, understandable and maintainable software ecosystem.

---

# Vision

The vision of Just is to establish an engineering system whose principles remain consistent regardless of how the project evolves.

Every architectural decision should be intentional.

Every subsystem should have a clearly defined responsibility.

Every interface should be explicit.

Every implementation should faithfully realize documented engineering decisions.

As the project grows, new capabilities should be introduced without compromising architectural consistency or long-term maintainability.

---

# Philosophy

Just is engineered as a unified engineering system where architecture, documentation, methodology and implementation evolve together.

```text
Engineering System
│
├── Architecture
├── Documentation
├── Methodology
└── Implementation
```

None of these elements exists independently.

Each continuously influences and evolves alongside the others.

Software is not the definition of the project.

Software is the result of the engineering system.

---

# Fundamental Principles

The following principles are considered immutable engineering axioms.

## Deterministic

The same input should always produce the same observable behavior.

System behavior must remain predictable, reproducible and easy to reason about.

---

## Documentation-Driven

Documentation precedes implementation.

Engineering decisions are documented before they are implemented.

Documentation is treated as an essential part of the engineering system rather than supplementary material.

---

## Zero Hidden State

All system state must remain explicit.

Hidden dependencies, implicit ownership and undocumented side effects are intentionally avoided.

Every observable behavior should have a clear and understandable origin.

---

## Backend Independent

The engineering system must remain independent of platform-specific implementations.

Technologies may evolve.

Engineering principles must not.

---

# Long-Term Direction

Just is designed to evolve continuously as a unified engineering system.

Each development milestone introduces new engineering capabilities while preserving the architectural philosophy established by this document.

Individual technologies, programming languages and implementations may change over time.

The engineering philosophy that guides the project should remain constant.

---

# Project Values

The project values:

- simplicity over unnecessary complexity;
- explicit design over implicit behavior;
- modularity over monolithic solutions;
- consistency over shortcuts;
- maintainability over temporary optimizations;
- long-term evolution over rapid feature growth.

These values guide every engineering decision throughout the lifetime of the project.

---

# Scope

Just is not defined by a specific implementation, programming language, platform or technology.

Instead, it is defined by the engineering philosophy that governs how every part of the system is designed, documented, implemented and evolved.

The engineering system is expected to evolve through successive milestones, each introducing new capabilities while preserving architectural consistency.

The resulting software ecosystem is an outcome of this engineering process.

---

# Relationship to Other Documents

Project Vision is the foundation of the project's documentation hierarchy.

Every document below it refines the ideas established here without redefining them.

```text
Project Vision
        ↓
Roadmap
        ↓
Architecture
        ↓
Development Rules
        ↓
Milestones
        ↓
Stages
        ↓
Technical Specifications
        ↓
Implementation
        ↓
Source Code
```

Each document answers a specific engineering question.

| Document | Purpose |
|----------|---------|
| Project Vision | Why does the project exist? |
| Roadmap | What engineering capabilities will be created? |
| Architecture | How is the engineering system organized? |
| Development Rules | How should the engineering system be developed? |
| Milestones | What engineering objective must be achieved in each phase? |
| Stages | What engineering contract must be satisfied before continuing? |
| Technical Specifications | How does a subsystem work? |
| Implementation | How is the specification realized? |
| Source Code | The implementation itself. |

Higher-level documents define engineering intent.

Lower-level documents progressively transform that intent into a working system.

No lower-level document may contradict or redefine decisions established by documents above it.