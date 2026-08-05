# Milestone 1 — Core

> **Status:** Planned  
> **Version:** 1.0  
> **Priority:** Critical

---

# Overview

Milestone 1 is the foundation of the entire JustOS project.

Its primary objective is to build a complete, stable and backend-independent Window Manager core that will become the basis for every higher-level subsystem of the operating system.

This milestone focuses exclusively on the compositor kernel and its internal architecture.

By the end of Milestone 1 the system must already be capable of:

- creating and managing windows;
- tracking keyboard and mouse focus;
- maintaining workspaces;
- calculating window layouts;
- communicating through the internal IPC system;
- managing multiple outputs;
- interacting with X11 or Wayland through an abstract backend layer.

---

# Goals

The primary goals of Milestone 1 are:

- build a backend-independent compositor architecture;
- separate system logic from rendering;
- implement an event-driven execution model;
- minimize runtime memory allocations;
- guarantee deterministic execution order;
- provide an extensible IPC architecture;
- build reusable low-level libraries;
- prepare the foundation for Desktop Services (Milestone 2).

---

# Dependencies

Milestone 1 depends only on:

- C17
- CMake
- POSIX
- UNIX Domain Sockets

Optional backend libraries:

## X11

- xcb
- xcb-util
- xkbcommon

## Wayland

- wlroots
- wayland-server

No desktop libraries are required.

---

# Deliverables

By the end of this milestone JustOS must provide:

- backend-independent compositor core;
- internal event system;
- JRP communication protocol;
- Kernel Bus;
- Router;
- IPC subsystem;
- complete window model;
- scene graph;
- layout engine;
- backend abstraction;
- renderer abstraction;
- input subsystem;
- plugin infrastructure.

---

# Not Included

This milestone intentionally contains **no desktop environment features**.

There is:

- no panel;
- no launcher;
- no wallpaper;
- no clipboard;
- no widgets;
- no desktop services;
- no Marketplace;
- no AI;
- no Timeline;
- no Search.

Only the Window Manager itself.

---

# Stage 1 — Foundation & Transport Layer

## Goal

Build the communication layer of JustOS.

Every subsystem inside the operating system communicates through this layer.

The communication infrastructure must be:

- fast;
- deterministic;
- memory-safe;
- backend-independent;
- secure;
- extensible.

---

## Design Principles

The transport layer follows several strict rules:

- binary-first communication;
- zero unnecessary allocations;
- deterministic packet processing;
- O(1) command dispatch;
- asynchronous communication;
- reusable low-level utilities;
- authentication before execution.

---

# Subsystems

## JRP Protocol

The JustOS Runtime Protocol is the binary communication format used by every subsystem.

Responsibilities:

- binary packet format;
- serialization;
- deserialization;
- packet validation;
- protocol versioning.

Features:

- TLV packet structure;
- CRC32 verification;
- transaction identifiers;
- sequence numbers;
- timestamps;
- protocol versioning.

---

## Kernel Bus

The Kernel Bus is the internal communication backbone of the compositor.

Responsibilities:

- event distribution;
- command scheduling;
- message prioritization;
- queue management;
- latency tracking.

Features:

- Multi-Ring Buffer;
- priority queues;
- overflow protection;
- deterministic execution.

---

## Router

The Router converts incoming packets into executable actions.

Responsibilities:

- handler lookup;
- command dispatch;
- module separation.

Execution model:

```
(ModuleID, CommandID)
            ↓
         Handler
```

Target complexity:

**O(1)**

---

## IPC Transport

IPC provides communication between JustOS and external applications.

Transport:

- UNIX Domain Socket

Responsibilities:

- asynchronous communication;
- packet transmission;
- buffering;
- timeout handling;
- connection management.

---

## Security Layer

Provides IPC authentication.

Responsibilities:

- Session Tokens;
- replay protection;
- packet validation;
- session expiration.

Future extensions:

- payload encryption;
- capability permissions;
- plugin authentication.

---

## Low-Level Base

Reusable utilities shared across the whole project.

Includes:

- memory management;
- frame allocator;
- string helpers;
- CRC utilities;
- logging;
- geometry primitives.

---

# Folder Structure

```text
include/justos/
├── common/
├── protocol/
│   ├── jrp/
│   ├── router/
│   └── transport/
├── core/
│   └── bus/
├── geometry/
└── math/

src/
├── common/
├── protocol/
│   ├── jrp/
│   ├── router/
│   └── transport/
├── core/
│   └── bus/
├── geometry/
└── math/
```

---

# Main Files

# JRP Protocol

## include/justos/protocol/jrp/jrp.h

### Purpose

Public API of the JustOS Runtime Protocol.

Responsible for packet structures and serialization interface.

### Structures

```c
JRPPacket
JRPHeader
JRPTLV
```

### Enums

```c
JRPVersion
JRPFlags
JRPError
```

### Public API

```c
jrp_encode(...)
jrp_decode(...)
jrp_validate(...)
jrp_crc(...)
```

### Dependencies

- common
- math

---

## src/protocol/jrp/jrp.c

### Purpose

Implements the complete binary protocol.

### Internal Functions

```c
jrp_encode(...)
jrp_decode(...)
jrp_validate(...)
jrp_crc(...)
jrp_parse_header(...)
jrp_parse_tlv(...)
```

### Responsibilities

- packet serialization;
- packet parsing;
- CRC verification;
- protocol validation.

---

# Kernel Bus

## include/justos/core/bus/bus.h

### Purpose

Public interface of the internal Kernel Bus.

### Structures

```c
JPKernelBus
JPMessage
```

### Enums

```c
JPBusPriority
JPBusResult
```

### Public API

```c
bus_init(...)
bus_shutdown(...)
bus_publish(...)
bus_dispatch(...)
```

### Dependencies

- common

---

## src/core/bus/bus.c

### Purpose

Implements the internal message bus.

### Internal Functions

```c
bus_init(...)
bus_shutdown(...)
bus_publish(...)
bus_dispatch(...)
bus_collect_stats(...)
bus_ring_push(...)
bus_ring_pop(...)
```

### Responsibilities

- Multi-Ring Buffer;
- priority scheduling;
- eviction policy;
- latency statistics.

---

# Router

## include/justos/protocol/router/router.h

### Purpose

Public routing interface.

### Structures

```c
JPRouter
JPHandler
```

### Public API

```c
router_register(...)
router_unregister(...)
router_dispatch(...)
```

### Dependencies

- protocol/jrp

---

## src/protocol/router/router.c

### Purpose

Implements O(1) command routing.

### Internal Functions

```c
router_register(...)
router_unregister(...)
router_dispatch(...)
router_find(...)
```

### Responsibilities

- command lookup;
- handler execution;
- dispatch optimization.

---

# IPC Transport

## include/justos/protocol/transport/transport.h

### Purpose

Public IPC interface.

### Structures

```c
JPIPCServer
JPIPCClient
JPSession
```

### Public API

```c
ipc_start(...)
ipc_stop(...)
ipc_send(...)
ipc_receive(...)
ipc_disconnect(...)
```

### Dependencies

- protocol/jrp

---

## src/protocol/transport/transport.c

### Purpose

Implements UNIX Socket communication.

### Internal Functions

```c
ipc_start(...)
ipc_stop(...)
ipc_send(...)
ipc_receive(...)
ipc_disconnect(...)
ipc_poll(...)
ipc_accept(...)
```

### Responsibilities

- socket communication;
- buffering;
- polling;
- timeout handling.

---

# Security Layer

## include/justos/protocol/transport/security.h

### Purpose

Authentication layer for IPC.

### Structures

```c
JPSessionToken
JPSecurityContext
```

### Public API

```c
security_generate_token(...)
security_validate_token(...)
security_validate_packet(...)
security_expire_token(...)
```

---

## src/protocol/transport/security.c

### Purpose

Implements authenticated IPC communication.

### Internal Functions

```c
security_generate_token(...)
security_validate_token(...)
security_validate_packet(...)
security_expire_token(...)
security_check_sequence(...)
```

### Responsibilities

- session generation;
- replay protection;
- packet authentication.

---

# Common

## include/justos/common/

### Purpose

Contains reusable low-level utilities shared across the whole project.

---

### memory.h

#### Purpose

Memory allocation wrappers.

Functions

```c
just_alloc(...)
just_realloc(...)
just_free(...)
```

---

### string_utils.h

#### Purpose

String manipulation helpers.

Functions

```c
jp_string_copy(...)
jp_string_compare(...)
jp_string_trim(...)
```

---

### logging.h

#### Purpose

Logging interface.

Functions

```c
jp_log_info(...)
jp_log_warn(...)
jp_log_error(...)
```

---

### crc32.h

#### Purpose

CRC32 helper.

Functions

```c
jp_crc32(...)
```

---

### frame_allocator.h

#### Purpose

Temporary per-frame arena allocator.

Functions

```c
jp_frame_begin(...)
jp_frame_alloc(...)
jp_frame_reset(...)
```

---

## src/common/

### memory.c

Implements

```c
just_alloc(...)
just_realloc(...)
just_free(...)
```

---

### string_utils.c

Implements

```c
jp_string_copy(...)
jp_string_compare(...)
jp_string_trim(...)
```

---

### logging.c

Implements

```c
jp_log_info(...)
jp_log_warn(...)
jp_log_error(...)
```

---

### crc32.c

Implements

```c
jp_crc32(...)
```

---

### frame_allocator.c

Implements

```c
jp_frame_begin(...)
jp_frame_alloc(...)
jp_frame_reset(...)
```

---

# Geometry

## include/justos/geometry/

### Purpose

Standalone geometry subsystem used throughout the compositor.

Used by:

- Layout Engine
- Renderer
- Scene Graph
- Input
- Decorations

---

### rect.h

Structures

```c
JPRect
```

Functions

```c
jp_rect_make(...)
jp_rect_intersects(...)
jp_rect_union(...)
jp_rect_clip(...)
```

---

### point.h

Structures

```c
JPPoint
```

Functions

```c
jp_point_make(...)
```

---

### region.h

Structures

```c
JPRegion
```

Functions

```c
jp_region_add(...)
jp_region_subtract(...)
jp_region_clear(...)
```

---

## src/geometry/

### rect.c

Implements

```c
jp_rect_make(...)
jp_rect_intersects(...)
jp_rect_union(...)
jp_rect_clip(...)
```

---

### point.c

Implements

```c
jp_point_make(...)
```

---

### region.c

Implements

```c
jp_region_add(...)
jp_region_subtract(...)
jp_region_clear(...)
```

---

# Testing

### Unit

- [ ]  JRP
- [ ]  Kernel Bus
- [ ]  Router
- [ ] IPC
- [ ]  Security
- [ ] Common
- [ ]  Geometry

### Integration

- [ ]  Internal communication
- [ ]  IPC transport
- [ ]  Router dispatch

### Performance

- [ ]  Bus latency
- [ ]  Packet throughput

---

# Stage Status

## Progress

### Header files

- [ ] JRP
- [ ] Bus
- [ ] Router
- [ ] IPC
- [ ] Security
- [ ] Common
- [ ] Geometry

### Source files

- [ ] JRP
- [ ] Bus
- [ ] Router
- [ ] IPC
- [ ] Security
- [ ] Common
- [ ] Geometry

### Tests

- [ ] Unit
- [ ] Integration
- [ ] Performance
---

## Expected Result

After Stage 1 JustOS must have:

- complete binary protocol;
- authenticated IPC communication;
- working Kernel Bus;
- deterministic routing;
- reusable low-level libraries;
- reusable geometry subsystem.

Every later stage depends on this foundation.

---

# Stage 2 — Core State Engine / Compositor Kernel

## Goal

Create a complete in-memory representation of the entire desktop state.

This stage builds a backend-independent compositor kernel that maintains the logical state of the entire desktop environment without interacting with X11, Wayland, OpenGL, Vulkan or any rendering backend.

At the end of this stage, the compositor should be capable of simulating a complete desktop entirely in RAM.

---

## Subsystems

### Model Layer

Responsible for representing every logical object inside JustOS.

Core objects:

- Window
- Workspace
- Output
- Monitor
- Surface Reference
- Geometry State

The model layer stores only runtime data.

It performs no rendering and never communicates directly with any backend.

---

### Registry Layer

Provides fast lookup and lifecycle management of every runtime object.

Responsibilities:

- object allocation
- object destruction
- generation counters
- O(1) lookup
- free stack management

Every runtime object receives a unique identifier.

Destroyed objects never leave dangling references.

---

### Scheduler & Kernel

Acts as the execution heart of the compositor.

Responsibilities:

- frame lifecycle
- subsystem execution order
- dirty flag processing
- synchronization
- frame orchestration

The scheduler guarantees deterministic execution.

---

### Event System

Responsible for asynchronous communication between internal subsystems.

Examples:

- window created
- window destroyed
- workspace changed
- monitor connected
- monitor disconnected
- focus changed

Events never execute logic directly.

They only transport information.

---

### Hook System

Provides extension points for future plugins.

Supports:

- PRE hooks
- POST hooks

Example:

Window Created

↓

PRE Hook

↓

Window Manager

↓

POST Hook

---

### Scene Graph

Represents spatial relationships between windows.

Responsibilities:

- hierarchy
- stacking order
- traversal
- clipping hierarchy

The Scene Graph exists independently of any rendering backend.

---

### Focus Engine

Maintains user interaction state.

Responsible for:

- focused window
- previous focus
- MRU stack
- workspace-local history

Each workspace maintains an independent focus stack.

---

### Constraint System

Defines limitations for every window.

Examples:

- minimum size
- maximum size
- aspect ratio
- monitor boundaries
- workspace restrictions

Constraints validate geometry.

They never modify it directly.

---

### Geometry Cache

Caches calculated window geometry.

Responsibilities:

- compare previous geometry
- detect changes
- reduce unnecessary backend commits

Only modified geometry proceeds further.

---

## Folders in use

```text
include/justos/
├── model/
├── registry/
├── scheduler/
├── kernel/
├── events/
├── hooks/
├── scene/
├── state/
├── constraints/
└── layout/

src/
├── model/
├── registry/
├── scheduler/
├── kernel/
├── events/
├── hooks/
├── scene/
├── state/
├── constraints/
└── layout/
```

---

## Main files

### include/justos/model/window.h

Declares:

- JPWindow
- window flags
- window states
- geometry state

Functions:
```c
- window_create()
- window_destroy()
- window_update()
- window_move()
- window_resize()
```
---

### src/model/window.c

Implements:

- creation
- destruction
- state updates
- geometry updates

---

### include/justos/model/workspace.h

Declares:

- JPWorkspace
- workspace information
- active layout

Functions:

- workspace_create()
- workspace_destroy()
- workspace_activate()
- workspace_set_layout()

---

### src/model/workspace.c

Implements:

- workspace lifecycle
- active workspace management
- layout assignment

---

### include/justos/model/output.h

Declares:

- JPOutput
- monitor information
- output geometry

Functions:

- output_create()
- output_destroy()
- output_update()

---

### src/model/output.c

Implements:

- output creation
- output removal
- monitor state updates

---

### include/justos/registry/window_registry.h

Declares:

- JPWindowRegistry

Functions:

- registry_init()
- registry_allocate()
- registry_release()
- registry_lookup()

---

### src/registry/window_registry.c

Implements:

- O(1) registry
- generation validation
- free stack allocator

---

### include/justos/registry/backend_registry.h

Declares backend object mapping.

Functions:
```c
- backend_register_surface()
- backend_unregister_surface()
- backend_lookup()
```
---

### src/registry/backend_registry.c

Implements:

- backend lookup
- surface mapping
- registry synchronization

---

### include/justos/kernel/kernel.h

Declares compositor lifecycle.

Functions:
```c
- jp_kernel_init()
- jp_kernel_shutdown()
- jp_kernel_begin_frame()
- jp_kernel_dispatch()
- jp_kernel_end_frame()
```
---

### src/kernel/kernel.c

Implements:

- frame orchestration
- subsystem synchronization
- dirty flag processing

---

### include/justos/scheduler/scheduler.h

Declares scheduler state.

Functions:
```c
- scheduler_init()
- scheduler_dispatch()
- scheduler_shutdown()
```
---

### src/scheduler/scheduler.c

Implements:

- subsystem execution
- deterministic scheduling
- frame ordering

---

### include/justos/events/event_queue.h

Declares:

- JPEvent
- JPEventQueue

Functions:
```c
- event_push()
- event_pop()
- event_clear()
```
---

### src/events/event_queue.c

Implements:

- asynchronous queue
- event dispatch
- queue management

---

### include/justos/hooks/hook_manager.h

Declares:

- PRE hooks
- POST hooks

Functions:
```c
- hook_register()
- hook_unregister()
- hook_execute()
```
---

### src/hooks/hook_manager.c

Implements:

- callback execution
- hook ordering
- plugin extension points

---

### include/justos/scene/scene_tree.h

Declares:

- JPSceneTree
- JPSceneNode

Functions:
```c
- scene_insert()
- scene_remove()
- scene_traverse()
```
---

### src/scene/scene_tree.c

Implements:

- hierarchy management
- sibling traversal
- scene updates

---

### include/justos/state/focus_stack.h

Declares:

- JPFocusStack

Functions:
```c
- focus_push()
- focus_pop()
- focus_current()
- focus_set()
```
---

### src/state/focus_stack.c

Implements:

- MRU stack
- workspace-local focus
- focus switching

---

### include/justos/constraints/constraints.h

Declares:

- JPWindowConstraints
- JPMonitorConstraints

Functions:
```c
- constraints_validate()
- constraints_apply()
```
---

### src/constraints/constraints.c

Implements:

- validation
- boundary checks
- size restrictions

---

### include/justos/layout/geometry_cache.h

Declares:

- JPGeometryCache

Functions:
```c
- cache_store()
- cache_compare()
- cache_invalidate()
```
---

### src/layout/geometry_cache.c

Implements:

- geometry comparison
- cache invalidation
- dirty detection

---

## Expected Result

After Stage 2 the compositor must have:

- complete runtime desktop model;
- O(1) object registries;
- deterministic scheduler;
- asynchronous event system;
- plugin hook infrastructure;
- fully functional Scene Graph;
- workspace-local MRU focus stack;
- validated constraint engine;
- geometry cache capable of detecting changes.

---

## Completion Criteria

- All modules compile successfully.
- Unit tests pass.
- Integration tests pass.
- No memory leaks are detected.
- Public API is documented.
- The compositor can simulate a complete desktop state entirely in RAM without any graphical backend.

---

# Stage 3 — Window Algorithms & Rules Engine

## Goal

Implement the complete window management logic of JustOS.

At the end of this stage, the compositor must be able to automatically arrange windows, switch between layout algorithms, manage floating windows, apply window rules, and generate optimized geometry updates.

This stage is entirely backend-independent.

No rendering is performed.

No Wayland or X11 calls are allowed.

The only output of this stage is updated window geometry.

---

# Overview

The Layout Engine operates entirely on the internal system state.

```
Workspace
        │
        ▼
Layout Manager
        │
        ▼
Layout Engine
        │
        ▼
Geometry Engine
        │
        ▼
Geometry Cache
        │
        ▼
Commit Queue
```

Each layout engine receives a list of windows and produces new rectangles.

It never:

- draws windows;
- communicates with wlroots;
- communicates with X11;
- modifies rendering state.

The Layout subsystem is purely mathematical.

---

# Goals

- implement the complete tiling subsystem;
- support multiple layout engines;
- implement runtime layout switching;
- implement floating window management;
- implement automatic window rules;
- minimize unnecessary geometry updates;
- prepare geometry for backend commit.

---

# Stage Components

## Layout Manager

The Layout Manager is responsible for selecting and executing the active layout algorithm.

Responsibilities:

- register layout engines;
- unregister layout engines;
- switch active layouts;
- dispatch layout calculations;
- maintain per-workspace layouts.

---

## Layout Engines

### Master Layout

Classic master-stack algorithm.

Features:

- one master area;
- stack area;
- configurable master ratio;
- dynamic resizing;
- window swapping;
- rotation support.

---

### Grid Layout

Places windows inside an automatically generated grid.

Features:

- automatic row calculation;
- automatic column calculation;
- balanced distribution;
- adaptive resizing.

---

### Monocle Layout

Every tiled window occupies the full workspace.

Only the focused window is visible.

---

### Floating Layout

Floating window management.

Supports:

- free movement;
- resize;
- snapping;
- restoring;
- constraints.

---

## Window Rules Engine

Automatically applies rules when a window appears.

Rules may define:

- workspace assignment;
- floating mode;
- default geometry;
- opacity;
- border mode;
- focus behaviour.

Matching can be performed using:

- WM_CLASS
- App ID
- Window Title
- Role
- Output
- Workspace

Example:

```
WM_CLASS = Steam
        ↓
Workspace = 4
        ↓
Floating = true
        ↓
Opacity = 95%
```

---

## Geometry Engine

Responsible for calculating final window geometry.

Calculates:

- window rectangles;
- gaps;
- borders;
- monitor boundaries;
- constraints;
- usable workspace area.

---

## Geometry Cache

Avoids unnecessary commits.

Compares:

Previous Geometry

↓

Current Geometry

↓

Dirty?

↓

Commit only if changed.

---

# Folders in use

```text
include/justos/
├── layout/
│   ├── layout_manager.h
│   ├── geometry_cache.h
│   ├── layout_context.h
│   └── engines/
│       ├── master.h
│       ├── grid.h
│       ├── monocle.h
│       └── floating.h
│
├── rules/
│   ├── rules.h
│   ├── matcher.h
│   └── filters.h
│
├── geometry/
│   ├── rect.h
│   ├── gaps.h
│   └── constraints.h

src/
├── layout/
│   ├── layout_manager.c
│   ├── geometry_cache.c
│   ├── layout_context.c
│   └── engines/
│       ├── master.c
│       ├── grid.c
│       ├── monocle.c
│       └── floating.c
│
├── rules/
│   ├── rules.c
│   ├── matcher.c
│   └── filters.c
│
└── geometry/
```

---

# Main Files

## include/justos/layout/layout_manager.h

Declares:

- `JPLayoutManager`
- layout registry
- active layout interface

Main functions:

```c
jp_layout_register()
jp_layout_unregister()
jp_layout_set()
jp_layout_get()
jp_layout_apply()
```

---

## src/layout/layout_manager.c

Implements:

- layout registration;
- layout switching;
- workspace dispatch;
- layout execution.

---

## include/justos/layout/layout_context.h

Declares:

- `JPLayoutContext`

Contains:

- workspace geometry;
- monitor geometry;
- window list;
- constraints.

Provides an isolated execution context for every layout engine.

---

## src/layout/layout_context.c

Implements:

- context initialization;
- workspace extraction;
- geometry preparation.

---

## include/justos/layout/geometry_cache.h

Declares:

- `JPGeometryCache`

Functions:

```c
jp_geometry_compare()
jp_geometry_update()
jp_geometry_reset()
```

---

## src/layout/geometry_cache.c

Implements:

- geometry comparison;
- dirty detection;
- cache synchronization.

---

## include/justos/layout/engines/master.h

Functions:

```c
jp_master_layout()
jp_master_resize()
jp_master_swap()
jp_master_rotate()
```

---

## src/layout/engines/master.c

Implements:

- Master-Stack algorithm;
- ratio calculations;
- stack balancing.

---

## include/justos/layout/engines/grid.h

Functions:

```c
jp_grid_layout()
```

---

## src/layout/engines/grid.c

Implements:

- grid generation;
- row calculation;
- column calculation;
- adaptive placement.

---

## include/justos/layout/engines/monocle.h

Functions:

```c
jp_monocle_layout()
```

---

## src/layout/engines/monocle.c

Implements:

- fullscreen stacking;
- focus selection.

---

## include/justos/layout/engines/floating.h

Functions:

```c
jp_floating_layout()
jp_floating_move()
jp_floating_resize()
jp_floating_snap()
```

---

## src/layout/engines/floating.c

Implements:

- floating placement;
- drag;
- resize;
- snapping;
- restoring.

---

## include/justos/rules/rules.h

Declares:

- `JPRule`
- rule registry

Functions:

```c
jp_rules_load()
jp_rules_reload()
jp_rules_apply()
```

---

## src/rules/rules.c

Implements:

- rule execution;
- priority resolution;
- workspace assignment;
- floating assignment.

---

## include/justos/rules/matcher.h

Functions:

```c
jp_rule_match_class()
jp_rule_match_title()
jp_rule_match_role()
jp_rule_match_output()
```

---

## src/rules/matcher.c

Implements:

- window matching;
- wildcard support;
- comparison logic.

---

## include/justos/rules/filters.h

Functions:

```c
jp_rule_filter_workspace()
jp_rule_filter_geometry()
jp_rule_filter_opacity()
```

---

## src/rules/filters.c

Implements:

- property filtering;
- rule application helpers.

---

## include/justos/geometry/

Contains:

- `rect.h`
- `constraints.h`
- `gaps.h`

Provides all geometry primitives used by the Layout Engine.

---

# Expected Result

After completing Stage 3, the core must provide:

- a fully functional Layout Manager;
- multiple interchangeable layout engines;
- automatic geometry calculation;
- floating window support;
- rule-based window placement;
- optimized geometry caching;
- backend-independent window arrangement.

The compositor is now capable of managing windows entirely in memory and producing deterministic geometry ready for backend synchronization.