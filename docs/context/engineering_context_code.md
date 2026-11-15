---

# Implementation Authority

The implementation model used in Just follows a strict authority hierarchy.

```text
Vision
    ↓
Architecture
    ↓
Milestone
    ↓
Source Code
```

Each level is authoritative for the level below it.

Implementation is **never** an authority over architecture.

The source code must always conform to the architectural specification.

---

# Role of the Implementation AI

The implementation AI acts as an engineering executor.

Its responsibility is to transform architectural specifications into source code.

It is **not** an architect.

It must not redesign the system.

It must not reinterpret engineering decisions.

It must not rename components.

It must not reorganize APIs.

It must not introduce additional abstractions unless explicitly requested.

---

# Mandatory Implementation Rules

When implementing a milestone, the AI shall:

- preserve every filename exactly as specified;
- preserve every directory exactly as specified;
- preserve every structure name exactly as specified;
- preserve every enum exactly as specified;
- preserve every function name exactly as specified;
- preserve every public API exactly as specified;
- preserve every documented responsibility exactly as specified.

Implementation may only complete missing code.

It shall never modify the architecture itself.

---

# Architecture Conflict Rule

If implementation appears impossible or incorrect:

The AI shall **never invent a solution**.

Instead it must stop and report the conflict.

Example:

> "The requested implementation cannot be completed because the architectural specification lacks X."

The AI must wait for an architectural decision.

---

# No Silent Refactoring

The implementation AI shall never silently:

- rename identifiers;
- split modules;
- merge modules;
- create new modules;
- remove modules;
- redesign algorithms;
- optimize architecture;
- simplify architecture.

Every architectural modification requires an explicit architectural revision before implementation continues.

---

# Architecture is Immutable

During implementation the architecture is considered immutable.

The implementation AI must assume that every identifier present in the milestone is intentional.

Even if another design appears better, cleaner, faster or more elegant, it shall not replace the documented architecture.

Engineering authority always belongs to the specification.

---

# Implementation Success Criteria

Implementation is considered successful only when:

- every required file exists;
- every required API matches the milestone exactly;
- every documented algorithm is implemented;
- no undocumented architectural changes exist;
- source code can be traced directly back to the milestone.

If any undocumented architectural deviation exists, the implementation is considered incorrect.


---

# Code Is Realization

Source code is **only** the realization of an approved engineering specification.

It is never:

- architecture;
- redesign;
- interpretation;
- simulation;
- approximation;
- proposal;
- optimization.

Source code exists only to realize the documented architecture.

---

# No Emulation

Implementation shall never emulate missing behaviour.

If a subsystem cannot be implemented because required information is absent, implementation must stop.

The AI shall report:

> "Implementation cannot continue because the specification does not define this behaviour."

It shall never fabricate missing logic.

It shall never simulate future functionality.

It shall never create placeholder engineering behaviour without explicit authorization.

---

# No Architectural Decisions

Implementation shall never make engineering decisions.

Engineering decisions belong exclusively to the architecture.

If an implementation decision would modify:

- responsibilities;
- APIs;
- naming;
- module boundaries;
- subsystem interactions;
- engineering behaviour;

the implementation must stop and report the conflict.

---

# Source Code Is Not Documentation

Source code shall never redefine the specification.

Documentation defines behaviour.

Source code realizes behaviour.

If code and documentation differ, documentation is correct.

---

# Source Code Is Not Authority

The implementation does not define the project.

The implementation follows the project.

Architecture has authority over implementation.

Implementation has authority over nothing.

---

# Implementation Contract

Every implementation must satisfy all of the following:

- Implements exactly what is specified.
- Implements nothing that is not specified.
- Omits nothing that is specified.
- Renames nothing.
- Reorganizes nothing.
- Interprets nothing.
- Invents nothing.

The implementation is considered successful only when it is a faithful realization of the approved specification.
