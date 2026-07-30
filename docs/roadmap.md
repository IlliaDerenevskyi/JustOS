# JustOS — Roadmap

> This document describes the long-term development roadmap of JustOS.
>
> It intentionally contains only high-level goals.
> Technical implementation details belong to the architecture documentation.

---

# Philosophy

Development is milestone-driven.

Each milestone produces a usable and testable result.

A milestone is considered complete only when:

- functionality is implemented;
- the project builds successfully;
- all tests pass;
- documentation is updated.

No milestone is skipped.

---

# Milestone 1 — Core

Goal:

Create a fully functional compositor and window manager.

Result:

A minimal but fully working desktop core.

Main objectives:

- boot process
- backend abstraction
- X11 backend
- Wayland backend
- IPC
- renderer
- layout engine
- input
- scene graph
- state engine
- window rules
- shell integration
- plugin infrastructure

---

# Milestone 2 — Desktop

Goal:

Transform the compositor into a usable desktop environment.

Result:

A complete desktop without custom applications.

Main objectives:

- settings
- themes
- launcher
- notifications
- wallpaper
- clipboard
- workspace daemon
- power management
- status panel
- desktop search
- widgets

---

# Milestone 3 — JSL

Goal:

Create the Just Scripting Language.

Result:

Self-hosted JSL runtime.

Main objectives:

- lexer
- parser
- runtime
- virtual machine
- module system
- standard library
- self-hosted compiler/interpreter

---

# Milestone 4 — Applications

Goal:

Move the ecosystem to JSL.

Result:

Core applications written in JSL.

Examples:

- launcher
- settings
- file manager
- terminal
- widgets
- SDK

---

# Milestone 5 — Intelligence

Goal:

Build an AI-assisted desktop.

Result:

Local AI deeply integrated into the operating system.

Main objectives:

- desktop assistant
- semantic search
- desktop memory
- indexing
- automation
- local models

---

# Milestone 6 — Release

Goal:

Prepare JustOS for public usage.

Result:

Official JustOS Release Candidate.

Main objectives:

- installer
- bootloader integration
- live image
- ISO
- package repositories
- update system
- documentation

---

# Future

Possible long-term directions:

- Just Marketplace
- Just Hub
- Just Sync
- Enterprise edition
- Cloud synchronization
- AI ecosystem
- Mobile integration
- Cross-device workflows

---

# Notes

This roadmap describes **what** should be built.

Architecture documents describe **how** it should be built.

Brain Dump contains evolving ideas that may later influence this roadmap.

---

# Roadmap Policy

The roadmap defines **what** the project should become.

It does **not** describe implementation details.

Implementation belongs to the architecture documentation.

The roadmap may evolve over time, but it must always reflect the current long-term direction of the project.

---

# Documentation Hierarchy

The project documentation follows a strict hierarchy:

```
Brain Dump
        ↓
Random Thoughts
        ↓
Vision
        ↓
Roadmap
        ↓
Architecture
        ↓
Implementation
        ↓
Source Code
```

Each level answers a different question.

| Document | Purpose |
|----------|---------|
| Brain Dump | Raw ideas, experiments and notes. |
| Random Thoughts | Unsorted concepts that may or may not become features. |
| Vision | Why JustOS exists. |
| Roadmap | What should be built. |
| Architecture | How the system should be designed. |
| Implementation | How individual modules work. |
| Source Code | The final implementation. |

Higher levels should never depend on lower levels.

Lower levels always follow the decisions made above them.