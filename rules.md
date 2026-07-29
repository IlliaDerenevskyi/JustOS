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


j_


Example:


j_window_create()
j_window_destroy()
j_workspace_switch()


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

# Rule 24 — Milestone Completion Policy

A milestone is considered complete only if all conditions are met:

- Code implemented
- Project builds successfully
- All tests pass
- Documentation updated

Until then, the milestone remains unfinished.