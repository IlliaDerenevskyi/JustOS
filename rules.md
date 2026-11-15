# JustOS Development Rules

These rules are mandatory for every contributor.
Code that violates these rules must not be merged.

---

# Rule 1 - Unified Coding Style

- Never use 4 spaces for indentation.
- Use tabs.
- Keep one brace style across the whole project.
- Use one naming convention.
- Use one commenting style.

---

# Rule 2 - Public API Prefix

Every public function must start with:


jp_


Example:


jp_window_create()
jp_window_destroy()
jp_workspace_switch()


Private helpers must be declared static.

---

# Rule 3 - Mirrored Directory Structure

Every public header has exactly one implementation.


include/justos/state/window.h
↓

src/state/window.c


---

# Rule 4 - Single Responsibility

Every module has one responsibility.

Large files must be split.

Example:


workspace_manager.c
workspace_layout.c
workspace_history.c


---

# Rule 5 - One Entity per File

One source file implements one entity.

Never mix:

- Window
- Focus
- Geometry
- Rules

inside the same source file.

---

# Rule 6 - No Cyclic Includes

Headers must not include unnecessary headers.

Prefer forward declarations.

---

# Rule 7 - Forward Declarations First

Instead of


#include "workspace.h"


prefer


typedef struct JPWorkspace JPWorkspace;


whenever possible.

---

# Rule 8 - No God Objects

Structures larger than roughly 10–15 fields should be reconsidered.

Split responsibilities.

---

# Rule 9 - Module Isolation

Modules only know what they need.

Example:

GeometryManager must not know:

- Focus
- Layout
- Rules
- Renderer

---

# Rule 10 - Kernel Context Access

Subsystems are accessed only through managers.

Correct:


ctx->services.window_mgr


Incorrect:


ctx->window


---

# Rule 11 - No Global Mutable State

Allowed globals:

- const
- enums
- lookup tables

Everything else lives inside context.

---

# Rule 12 - Memory Allocation

Only use:


just_alloc()
just_free()
just_realloc()


Never use malloc/free directly.

---

# Rule 13 - No Magic Numbers

Incorrect:


width = 15;


Correct:


#define JP_DEFAULT_BORDER_WIDTH 15


or


enum { JP_DEFAULT_BORDER_WIDTH = 15 };


---

# Rule 14 - Include Guards

Every header must use unique include guards.

Example:


JUSTOS_STATE_WINDOW_MANAGER_H


---

# Rule 15 - Public Before Private

Source files should follow:

1. Public API
2. Private helpers
3. Implementation

---

# Rule 16 - Build Frequently

Compile after finishing every module.

Never wait until the end of the milestone.

---

# Rule 17 - Every Module Has Tests

No module is considered complete without tests.

---

# Rule 18 - No Warnings

Main branch must compile without warnings.

Warnings are treated as bugs.

---

# Rule 19 - Git Workflow

main must always build and contains only completed milestones and
documentation-only states.

Branches:

- main: completed milestones and documentation-only states. Every one of
  them is tagged (Rule 24).
- develop: the working branch. It holds the work in progress of the
  current stage. Every push is checked by CI (Rule 42).
- feature/milestone<N> (for example feature/milestone0): the branch of one
  milestone. It receives only stages that passed review (Rule 26). It is
  created from main when the milestone starts, after the milestone has
  been created on GitHub (Rule 41).

Flow:

1. Work happens in develop.
2. When a stage is ready, the diff between feature/milestone<N> and develop
   and the Stage Report go to the architecture review (Rule 26).
3. After the review the stage enters feature/milestone<N> as one commit
   (squash merge). CI must pass.
4. develop is then brought up to date with feature/milestone<N>.
5. When the milestone is complete (Rule 25), feature/milestone<N> is merged
   into main, the tag is created, and the release is published. develop is
   then brought up to date with main.

Documentation-only states (tags v0.0.x, Rule 24) go from develop directly
to main.

Merge only after successful build and tests.

---

# Rule 20 - Documentation Is Part of the Code

Every module must document:

- Purpose
- Responsibilities
- Limitations
- Public API

---

# Rule 21 - No TODOs in Main

TODO comments are forbidden in the main branch.

Future work belongs in Issues or the Roadmap.

---

# Rule 22 - Explicit Dependencies

Every module documents:


Depends on:
...

Must not depend on:
...


---

# Rule 23 - Layered Architecture

Allowed dependency direction:


Kernel
↓

State
↓

Scene
↓

Layout
↓

Policy
↓

Backend
↓

Renderer


Lower layers must never call higher layers.

---
# Rule 24. Every Stable Milestone Must Be Tagged

Every significant project state must be marked with an annotated Git tag.

Tags represent immutable checkpoints in the project's history and allow any contributor to restore a known-good version at any time.

Tag naming convention:

- v0.0.x - Documentation / Repository bootstrap
- v0.x.0 - Completed milestones
- v1.0.0 - First public release
- Semantic Versioning afterwards

Every tag must include:

- Version number
- Short description
- Summary of completed work

Example:

v0.0.2

Initial project documentation

- README
- Rules
- Overview
- Architecture skeleton

Whenever possible, every tag should also have a corresponding GitHub Release containing the same information.

Tags must never be moved or rewritten after publication.
---
# Rule 25 - Milestone Completion Policy

A milestone is considered complete only if all conditions are met:

- Code implemented
- Project builds successfully
- All tests pass
- Documentation updated
- Git tag created
- GitHub Release published (if applicable)

Until then, the milestone remains unfinished.

---
# Rule 26 - Stage Verification Workflow

A development stage is **not considered complete** until it has passed architectural review.

The required workflow is:

1. Complete the implementation of the current Stage.
2. Generate:
   - a `git diff` containing all changes made during the Stage (the
     difference between `feature/milestone<N>` and `develop`);
   - a short **Stage Report** describing:
     - completed tasks;
     - modified modules;
     - known limitations (if any).
3. Submit the following to the Architecture Review chat:
   - `git diff`;
   - Stage Report;
   - the corresponding milestone documentation.
4. The Architecture Review must verify:
   - compliance with the milestone documentation;
   - implementation completeness;
   - architectural consistency;
   - coding standards;
   - unnecessary or redundant code;
   - missing modules or functionality.
5. Address all review comments.
6. Only after all issues have been resolved may the developer put the
   Stage into the milestone branch as one commit:
   ```bash
   git switch feature/milestone<N>
   git merge --squash develop
   git commit -m "<message>"
   ```
7. A Stage is considered finished **only after the commit has been created in the milestone branch following a successful architectural review**.

**Important:**

- Never commit unfinished or unreviewed stages to a milestone branch or to main. Working commits in develop are allowed.
- `git diff` must represent the complete set of changes made during the current Stage.
- Documentation is the single source of truth; implementation must always follow it.
---
# Rule 27 - Documentation Is the Source of Truth

Engineering documentation is the authoritative source of truth.

Architecture, interfaces, algorithms, data structures and engineering decisions are defined in documentation before implementation begins.

Source code exists to realize those specifications.

Implementation must remain consistent with the documented engineering design.

Whenever code and documentation diverge, the inconsistency is treated as an engineering defect rather than a documentation issue.
---
# Rule 28 - Every engineering artifact must be fully reconstructible from its specification.

Every subsystem specification shall contain sufficient engineering information to reconstruct its public interface, internal architecture and implementation without relying on the original source code.

Engineering documentation exists to preserve architectural knowledge independently of implementation.

Loss of source code must never imply loss of engineering knowledge.
---
# Rule 29 - Dependency Completeness

Every dependency introduced by a milestone shall already exist as both engineering documentation and source code before the dependent milestone may begin implementation.

A milestone may only depend on components that are fully defined and implemented.

For every dependency:

the corresponding milestone specification shall exist;
the component shall be completely documented;
all public interfaces shall be specified;
the source code shall exist;
the implementation shall satisfy the documented engineering specification.

Undocumented or unimplemented dependencies are prohibited.

This guarantees that every referenced file, subsystem or component has a single authoritative engineering definition and an existing implementation.

No milestone may reference engineering components that do not yet exist.
---
# Rule 30 - No Fake State

The operating system must never report fabricated hardware,
runtime state or successful initialization of a subsystem.

Every reported status must originate from an actual completed
operation.

If information is unavailable, the system must explicitly report:

UNKNOWN

NOT DETECTED

NOT INITIALIZED

FAILED

instead of fabricated placeholder values.
---
# Rule 31 - Milestone Self-Sufficiency

Every milestone must be self-sufficient.

A developer working on a milestone must be able to understand, implement, test, review and complete that milestone without consulting documentation from other milestones.

Requirements

Each milestone*.md must fully describe:

- engineering purpose;
- engineering goals;
- subsystem decomposition;
- folder structure;
- file structure;
- public interfaces;
- internal responsibilities;
- algorithms;
- dependencies;
- testing strategy;
- engineering guarantees;
- expected result.
- Scope

A milestone may reference:

Engineering Context;
Development Rules;
Engineering Breakdown;
global Architecture documents.

It must never require reading another milestone to understand its own implementation.

Independence

Milestones are engineering contracts.

Every milestone must define:

What exists.

Why it exists.

Where it exists.

How it works.

How it communicates.

How it is tested.

When it is considered complete.
Cross-Milestone Dependencies

A milestone may depend on completed milestones.

However, it may only depend on their public engineering contracts, never on undocumented implementation details.

Verification

A new developer should be able to:

Read
engineering_context.md
rules.md
engineering_breakdown.md
milestoneX.md
Understand the milestone.
Implement the milestone.
Test the milestone.
Produce a valid Stage Report.

without opening any other milestone*.md.

Engineering Guarantee

If implementation cannot begin after reading the milestone documentation, the milestone documentation is considered incomplete.
---
Rule 32 - Stage Continuity

Every stage must define a complete engineering objective, but it is not required to be independently implementable.

Rationale

Stages represent incremental engineering steps inside a milestone.

Unlike milestones, stages intentionally build upon the results of previous stages.

Requirements

Each stage must clearly define:

Goal;
Subsystems affected;
Components modified;
Engineering guarantees introduced;
Expected result after completion.
Dependencies

A stage may depend on:

previous stages of the same milestone;
public APIs introduced earlier in the milestone.

A stage must never depend on future stages.

Engineering Guarantee

After completing a stage:

the repository must remain internally consistent;
no partially implemented architecture may be introduced;
all engineering guarantees of completed stages remain valid.
Verification

A completed stage must satisfy:

Previous stages
        ↓
Current stage
        ↓
Repository still builds
        ↓
Tests pass
        ↓
Stage Report
        ↓
Architecture Review
        ↓
Commit
---
# Rule 33 - Engineering Contract Stability

Public engineering contracts introduced by a completed milestone shall remain stable.

Changes to public APIs, public structures, engineering guarantees or documented behaviour are prohibited unless performed as an explicit engineering refactoring.

Every such change shall update:

- architecture documentation;
- milestone documentation;
- implementation;
- tests.

Silent breaking changes are prohibited.

Engineering Guarantee:

Every completed milestone remains a stable engineering foundation for all subsequent milestones.
---
# Rule 34 - Aggregation Layer
Purpose

Aggregation components provide a unified engineering view over multiple independent subsystems.

They do not own, discover or modify the underlying information.

Requirements

An aggregation component may exist before all of its data providers have been implemented.

It shall never:

fabricate information;
infer unavailable subsystem state;
substitute missing data with placeholder values;
modify data produced by other components.

An aggregation component shall only:

collect information from completed components;
aggregate independent engineering data;
format information for presentation;
expose a unified interface.

For every unavailable provider, the aggregation layer shall present the exported unavailable state exactly as reported by the provider or by the engineering contract.

Supported unavailable states include:

- UNKNOWN
- NOT DETECTED
- NOT INITIALIZED
- FAILED
- Engineering Guarantee

Aggregation layers remain deterministic and truthful regardless of implementation progress.

Incomplete subsystems never result in fabricated operating system state.
Rule 34 applies to every aggregation component in JustOS
---
# Rule 35 - Architecture vs Implementation

Architecture defines contracts.

Implementation fulfills contracts.

Architecture never contains executable behavior.

Implementation never changes architecture.
---
# Rule 36 - Implementation Stage

Once an interface is approved, the next stage shall implement it.

Repeating architectural placeholders in implementation stages is prohibited unless explicitly justified by a missing dependency.
---
# Rule 37 - Architectural Authority

The implementation shall never rename, replace, redesign, extend, or reinterpret any architectural component, API, enum, type, state, lifecycle, or terminology unless the architecture or stage specification explicitly requires that change.

When implementation conflicts with the existing architecture, the implementation is considered incorrect.

Architecture always has priority over generated code.

The implementation engine may report inconsistencies but shall never resolve them autonomously.
---
# Rule 38 - Specification Lock

During implementation, the milestone specification is immutable.

No function names, file names, enums, structures, APIs, terminology, directory layout, lifecycle, contracts, or architecture may be changed by the implementation engine.

If an implementation requires a specification change, the implementation must stop and report the conflict instead of continuing.

The specification must always be modified before the implementation.
---
# Rule 39 - Specification Lock 

A **Milestone**, and therefore every **Stage** within that Milestone, is a **Specification Lock**.

This means that before implementation begins, the Milestone and its Stages must define the complete technical contract required for their implementation.

A Specification Lock must define, where applicable:

* architecture and platform targets;
* module and file structure;
* responsibilities and boundaries of every module;
* public interfaces and APIs;
* data structures;
* enums and constants;
* ownership and lifetime rules;
* state machines and valid state transitions;
* algorithms and required invariants;
* input and output contracts;
* error and failure semantics;
* resource allocation and cleanup rules;
* ABI and calling conventions;
* inter-stage contracts;
* platform-specific contracts where required;
* completion criteria;
* explicit non-goals and boundaries with other Milestones or Stages.

The Specification Lock must be sufficiently precise that an implementation coder can implement the defined scope **without inventing missing architectural behavior or making undocumented design decisions**.

The Specification Lock does **not** prescribe implementation details that are intentionally left as implementation choices. It defines the required behavior, interfaces, invariants, constraints, and observable results.

Therefore:

> **Architecture is decided in the Specification Lock. Implementation is performed by the coder.**

A Stage must not be considered Specification Locked merely because its files, functions, or high-level architecture are listed. All behavior that affects compatibility, correctness, resource ownership, ABI, platform interaction, memory layout, state transitions, or inter-stage communication must also be explicitly defined.

If a required architectural or behavioral decision is discovered to be missing during implementation, implementation must stop for that affected area and the Specification Lock must be updated before proceeding.

No coder may silently introduce a new architectural contract that is not defined by the Specification Lock.

## Architecture Targets

Architecture support is defined **per Milestone/Stage target**, not implicitly for the entire project.

For example, if a Milestone is locked for:

```text
Target Architecture: x86_64
```

then all architecture-dependent contracts within that Milestone are specified for x86_64.

Support for another architecture, such as ARM64/AArch64, requires a separate explicitly defined architecture target and corresponding contracts.

Adding ARM64/AArch64 support must not silently alter or weaken the existing x86_64 Specification Lock.

Architecture-independent code must remain architecture-independent where the specification explicitly defines such a boundary.

## Lock Status

A Milestone or Stage may be considered **Specification Locked** only when:

1. its scope is completely defined;
2. its architectural boundaries are defined;
3. its public and inter-module contracts are defined;
4. its required behavior and invariants are defined;
5. its platform and architecture assumptions are explicit;
6. its dependencies on other Stages are defined;
7. its completion criteria are defined;
8. no unresolved architectural decisions remain inside the locked scope.

Until these conditions are satisfied, the Milestone/Stage remains:

```text
SPECIFICATION INCOMPLETE
```

and implementation is not considered contractually stable.

---
# Rule 40 - Documentation drives implementation
Implementation never comes first.

For every module:

1. Update documentation.
2. Define public API.
3. Write tests.
4. Implement.
5. Refactor.

If the documentation is not updated, the code must not be written.

# Rule 41 - GitHub Milestones

Priveously write the code of Milestone - create the milestone on Github

# Rule 42 - GitHub Actions

After push - CI is taking place on GitHub Actions
