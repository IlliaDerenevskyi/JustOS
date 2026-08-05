# JustOS Development Rules

These rules are mandatory for every contributor.
Code that violates these rules must not be merged.

---

# Rule 1 — Unified Coding Style

- Use 4 spaces for indentation.
- Never use tabs.
- Keep one brace style across the whole project.
- Use one naming convention.
- Use one commenting style.

---

# Rule 2 — Public API Prefix

Every public function must start with:


jp_


Example:


jp_window_create()
jp_window_destroy()
jp_workspace_switch()


Private helpers must be declared static.

---

# Rule 3 — Mirrored Directory Structure

Every public header has exactly one implementation.


include/justos/state/window.h
↓

src/state/window.c


---

# Rule 4 — Single Responsibility

Every module has one responsibility.

Large files must be split.

Example:


workspace_manager.c
workspace_layout.c
workspace_history.c


---

# Rule 5 — One Entity per File

One source file implements one entity.

Never mix:

- Window
- Focus
- Geometry
- Rules

inside the same source file.

---

# Rule 6 — No Cyclic Includes

Headers must not include unnecessary headers.

Prefer forward declarations.

---

# Rule 7 — Forward Declarations First

Instead of


#include "workspace.h"


prefer


typedef struct JPWorkspace JPWorkspace;


whenever possible.

---

# Rule 8 — No God Objects

Structures larger than roughly 10–15 fields should be reconsidered.

Split responsibilities.

---

# Rule 9 — Module Isolation

Modules only know what they need.

Example:

GeometryManager must not know:

- Focus
- Layout
- Rules
- Renderer

---

# Rule 10 — Kernel Context Access

Subsystems are accessed only through managers.

Correct:


ctx->services.window_mgr


Incorrect:


ctx->window


---

# Rule 11 — No Global Mutable State

Allowed globals:

- const
- enums
- lookup tables

Everything else lives inside context.

---

# Rule 12 — Memory Allocation

Only use:


just_alloc()
just_free()
just_realloc()


Never use malloc/free directly.

---

# Rule 13 — No Magic Numbers

Incorrect:


width = 15;


Correct:


#define JP_DEFAULT_BORDER_WIDTH 15


or


enum { JP_DEFAULT_BORDER_WIDTH = 15 };


---

# Rule 14 — Include Guards

Every header must use unique include guards.

Example:


JUSTOS_STATE_WINDOW_MANAGER_H


---

# Rule 15 — Public Before Private

Source files should follow:

1. Public API
2. Private helpers
3. Implementation

---

# Rule 16 — Build Frequently

Compile after finishing every module.

Never wait until the end of the milestone.

---

# Rule 17 — Every Module Has Tests

No module is considered complete without tests.

---

# Rule 18 — No Warnings

Main branch must compile without warnings.

Warnings are treated as bugs.

---

# Rule 19 — Git Workflow

main must always build.

Feature development happens in dedicated branches.

Example:


feature/window-manager


Merge only after successful build and tests.

---

# Rule 20 — Documentation Is Part of the Code

Every module must document:

- Purpose
- Responsibilities
- Limitations
- Public API

---

# Rule 21 — No TODOs in Main

TODO comments are forbidden in the main branch.

Future work belongs in Issues or the Roadmap.

---

# Rule 22 — Explicit Dependencies

Every module documents:


Depends on:
...

Must not depend on:
...


---

# Rule 23 — Layered Architecture

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

- v0.0.x — Documentation / Repository bootstrap
- v0.x.0 — Completed milestones
- v1.0.0 — First public release
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
# Rule 25 — Milestone Completion Policy

A milestone is considered complete only if all conditions are met:

- Code implemented
- Project builds successfully
- All tests pass
- Documentation updated
- Git tag created
- GitHub Release published (if applicable)

Until then, the milestone remains unfinished.

---
# Rule 26 — Stage Verification Workflow

A development stage is **not considered complete** until it has passed architectural review.

The required workflow is:

1. Complete the implementation of the current Stage.
2. Generate:
   - a `git diff` containing all changes made during the Stage;
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
6. Only after all issues have been resolved may the developer execute:
   ```bash
   git add .
   git commit -m "<message>"
   ```
7. A Stage is considered finished **only after the commit has been created following a successful architectural review**.

**Important:**

- Never commit unfinished or unreviewed stages.
- `git diff` must represent the complete set of changes made during the current Stage.
- Documentation is the single source of truth; implementation must always follow it.
---
# Rule 27 — Documentation Is the Source of Truth

Engineering documentation is the authoritative source of truth.

Architecture, interfaces, algorithms, data structures and engineering decisions are defined in documentation before implementation begins.

Source code exists to realize those specifications.

Implementation must remain consistent with the documented engineering design.

Whenever code and documentation diverge, the inconsistency is treated as an engineering defect rather than a documentation issue.
---
# Rule 28 — Every engineering artifact must be fully reconstructible from its specification.

Every subsystem specification shall contain sufficient engineering information to reconstruct its public interface, internal architecture and implementation without relying on the original source code.

Engineering documentation exists to preserve architectural knowledge independently of implementation.

Loss of source code must never imply loss of engineering knowledge.
---
# Rule 29 — Dependency Completeness

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
# Rule 30 — Documentation drives implementation
Implementation never comes first.

For every module:

1. Update documentation.
2. Define public API.
3. Write tests.
4. Implement.
5. Refactor.

If the documentation is not updated, the code must not be written.

