# Just — Engineering Context

> This document defines the fundamental engineering context of the Just project.
>
> It explains the principles, terminology, architectural philosophy and engineering conventions that govern the entire operating system.
>
> Every architectural document, specification and implementation within Just must remain consistent with the concepts defined here.

---

# Purpose

The purpose of this document is to establish a shared engineering understanding of the Just project.

It serves as the primary reference for:

- developers;
- architects;
- contributors;
- documentation authors;
- AI assistants.

Rather than describing implementation, this document explains **how the project is designed and how engineering decisions are made**.

---

# Project Vision

Just is designed as an engineering operating system.

Its objective is not merely to provide a desktop environment, but to establish a deterministic, modular and fully documented engineering platform.

Every subsystem is designed to be:

- understandable;
- replaceable;
- independently testable;
- backend-independent whenever possible.

Engineering correctness always takes precedence over implementation convenience.

---

# Engineering Philosophy

The following principles define the engineering philosophy of Just.

## Documentation First

Architecture is designed before implementation.

Documentation is considered part of the engineering process rather than a description written afterwards.

---

## One Responsibility

Every engineering component has exactly one responsibility.

Responsibilities must never overlap.

Cooperation occurs through explicit interfaces rather than shared responsibilities.

---

## Deterministic Behaviour

The system should behave predictably.

Identical inputs should always produce identical outputs unless explicitly documented otherwise.

---
## Milestones vs Stages

Milestones are self-sufficient engineering contracts.

Stages are incremental implementation steps inside a milestone.

A milestone must be understandable without consulting other milestones.

A stage may depend on previous stages within the same milestone.

A stage must never depend on future stages.
---

## Explainability

Engineering decisions should always be explainable.

No subsystem should behave as a "black box".

Every decision should be traceable through the documented architecture.

---

## Modularity

Large systems are decomposed into independent engineering components.

Each component may evolve independently while preserving stable interfaces.

---

## Backend Independence

Whenever practical, engineering components should remain independent from implementation-specific technologies.

Backends implement interfaces rather than defining behaviour.

---

## Fail-Safe Design

Failures should always degrade safely.

Unexpected situations should never silently compromise system integrity.

---

## Engineering Before Optimization

Architecture is optimized before code.

Performance improvements should emerge from engineering design rather than premature micro-optimizations.

---

# Engineering Objects

The fundamental abstraction inside Just is the Engineering Object.

Every meaningful entity managed by the operating system is represented as an Engineering Object.

Examples include:

- files;
- packages;
- binaries;
- windows;
- workspaces;
- plugins;
- services;
- applications;
- themes;
- wallpapers;
- widgets;
- users;
- sessions;
- documentation;
- JSL libraries;
- marketplace assets.

Different object types share a common engineering model.

---

# Object Profile

Every Engineering Object exposes a canonical Object Profile.

```text
Engineering Object
        │
        ▼
Object Profile
 ├── Identity
 ├── Metadata
 ├── Relations
 ├── State
 ├── Security Profile
 └── Capabilities
```

Multiple subsystems consume the Object Profile.

Examples include:

- Search;
- Launcher;
- AI;
- IDE;
- Marketplace;
- Security Engine;
- Index Engine.

No subsystem should maintain an independent description of the same object.

---

# Engines

Within Just, an Engine represents an engineering subsystem responsible for one architectural concern.

Examples include:

- Security Engine;
- Identity Engine;
- Trust Engine;
- Layout Engine;
- Index Engine;
- Rendering Engine.

An Engine defines engineering behaviour.

It does not define implementation.

---

# Services

Services are runtime implementations of engineering engines.

Examples include:

- just-indexer;
- just-search;
- just-session;
- just-notify;
- just-daemon.

An Engine may have zero, one or multiple service implementations.

---

# Documentation Hierarchy

Documentation follows a strict hierarchy.

```text
Vision
    │
    ▼
Philosophy
    │
    ▼
Architecture
    │
    ▼
Engine
    │
    ▼
Specification
    │
    ▼
Implementation
```

Each level refines the previous one.

Lower levels must never contradict higher-level documents.

---

# Repository Philosophy

The repository separates engineering concerns.

```text
docs/
    architecture/
    specifications/
    milestones/

src/
    implementation/

tests/
```

Documentation defines architecture.

Source code implements it.

Tests verify it.

---

# Milestones

Milestones represent engineering phases rather than chronological checkpoints.

Each milestone introduces complete engineering capabilities.

The implementation of a milestone should preserve architectural consistency.

---

# Specifications

Specifications describe how an individual subsystem operates internally.

Specifications:

- refine architecture;
- define algorithms;
- define data structures;
- define protocols;
- define interfaces.

Specifications never redefine architectural responsibilities.

---

# Implementation Rules

Implementation must follow the documented architecture.

Implementation may:

- optimize algorithms;
- improve performance;
- fix bugs;
- extend functionality.

Implementation must never:

- redefine responsibilities;
- bypass architectural boundaries;
- duplicate existing responsibilities.

---

# AI Guidelines

AI assistance should always respect the documented engineering model.

When generating architecture or documentation:

- never invent architectural components;
- never merge independent responsibilities;
- always preserve Engineering Objects;
- follow existing terminology;
- respect the documentation hierarchy;
- prefer explicit engineering descriptions over simplified explanations.

When implementation is requested:

- architecture takes precedence over convenience;
- documentation takes precedence over assumptions.

---

# Current Engineering State

The current project state should always follow this workflow:

```text
Idea
    │
    ▼
Architecture
    │
    ▼
Specification
    │
    ▼
Milestone
    │
    ▼
Implementation
    │
    ▼
Testing
    │
    ▼
Documentation Update
```

Architecture evolves intentionally.

Implementation follows architecture.

---

# Engineering Principles Summary

The entire Just project is governed by the following core principles:

- Everything is an Engineering Object.
- Every component has one responsibility.
- Documentation precedes implementation.
- Architecture precedes optimization.
- Behaviour is deterministic.
- Interfaces are explicit.
- Trust is continuously established.
- Security emerges from architecture.
- Engineering decisions remain explainable.
- Lower-level documents never contradict higher-level architecture.
