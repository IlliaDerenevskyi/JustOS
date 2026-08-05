# Just — Roadmap

> This document describes the long-term evolution of the Just engineering system.
>
> Each milestone introduces a new engineering capability while preserving the architectural principles established by the Project Vision.
>
> The Roadmap defines **what the engineering system becomes** as it evolves.
> Implementation details, subsystem specifications and development stages are documented separately.

---

# Milestone 0 — Boot Manager

> **Status:** Planned
>
> **Priority:** Critical

---

## Overview

Milestone 0 establishes the minimal infrastructure required to start the Just engineering system.

Its purpose is to provide a deterministic boot process capable of loading the kernel, preparing the execution environment and transferring complete system information before control is handed over to the operating system.

This milestone intentionally focuses only on system startup.

It provides the foundation required by every subsequent milestone.

---

## Goal

Create a complete and reliable boot infrastructure capable of initializing the Just kernel.

---

## Engineering Capabilities

After completing this milestone the engineering system is able to:

- boot through supported firmware interfaces;
- load the Just kernel;
- initialize the execution environment;
- provide memory information;
- provide framebuffer information;
- transfer BootInfo to the kernel;
- support diagnostic and recovery boot modes.

The system becomes bootable.

---

## Dependencies

Requires:

- C17
- CMake
- POSIX (build environment)

Platform-specific boot technologies may evolve independently from the engineering architecture.

---

## Deliverables

- Boot Manager
- Kernel Loader
- BootInfo
- Memory Map
- Framebuffer Initialization
- Boot Configuration
- Recovery entry
- Boot Diagnostics

---

## Related Documentation

See:

- `docs/milestones/milestone0.md`

---

# Milestone 1 — Core

> **Status:** Planned
>
> **Priority:** Critical

---

## Overview

Milestone 1 establishes the engineering core of Just.

Its purpose is to create a complete, deterministic and backend-independent Window Manager architecture that serves as the foundation for every higher-level subsystem.

This milestone defines the core runtime responsible for communication, state management, layout computation, rendering abstraction, input processing and extensibility.

Every subsequent milestone depends on the engineering capabilities introduced here.

---

## Goal

Create a complete backend-independent Window Manager foundation for the Just engineering system.

---

## Engineering Capabilities

After completing this milestone the engineering system is able to:

- manage windows;
- maintain system state;
- calculate window layouts;
- process user input;
- render through abstract backend interfaces;
- communicate through the internal protocol;
- support multiple outputs;
- manage window lifecycles;
- provide plugin infrastructure;
- expose a stable foundation for desktop services.

The system becomes a Window Manager.

---

## Dependencies

Requires:

- Milestone 0
- C17
- CMake
- POSIX
- UNIX Domain Sockets

Platform-specific backend implementations remain isolated from the engineering architecture.

---

## Deliverables

- Communication Infrastructure
- Core State Engine
- Window Model
- Layout Engine
- Backend Abstraction
- Rendering Pipeline
- Input System
- Window Lifecycle
- Plugin Infrastructure

---

## Related Documentation

See:

- `docs/milestones/milestone1.md`

---
# Milestone 2 — Desktop Services

> **Status:** Planned
>
> **Priority:** High

---

## Overview

Milestone 2 introduces the service layer of the Just engineering system.

Its purpose is to build the infrastructure responsible for managing hardware resources, coordinating desktop functionality and exposing reusable system services.

Unlike the previous milestone, which establishes the Window Manager itself, this milestone creates the internal service architecture upon which the future desktop environment will be built.

User interface components are intentionally excluded.

---

## Goal

Create a modular and service-oriented desktop infrastructure independent of graphical user interfaces.

---

## Engineering Capabilities

After completing this milestone the engineering system is able to:

- manage hardware resources through dedicated services;
- coordinate system-wide desktop functionality;
- provide reusable desktop services;
- centralize configuration management;
- manage user sessions;
- coordinate notifications;
- manage software packages;
- provide AI service infrastructure;
- orchestrate system services through a central daemon.

The system gains desktop services.

---

## Dependencies

Requires:

- Milestone 1
- POSIX
- D-Bus (optional)
- Network libraries
- Audio libraries
- Bluetooth libraries

Individual services may depend on platform-specific libraries while remaining isolated behind stable service interfaces.

---

## Deliverables

### Hardware Services

- Network Service
- Bluetooth Service
- Audio Service
- Display Service
- Monitor Service
- Power Management Service
- Storage Service

### Core Desktop Services

- Launcher Service
- Search Service
- Process Manager
- Session Manager
- Settings Service
- Configuration Service
- Notification Service
- Package Manager
- Panel Service
- Rules Service
- AI Service Layer

### Appearance Services

- Theme Service
- Workspace Service
- Window Appearance Service
- Wallpaper Service

### Desktop Orchestrator

- Desktop Daemon

---

## Related Documentation

See:

- `docs/milestones/milestone2.md`

---

# Milestone 3 — JSL Runtime

> **Status:** Planned
>
> **Priority:** High

---

## Overview

Milestone 3 introduces the Just Scripting Language (JSL) runtime.

Its purpose is to provide a programmable execution environment capable of extending and automating the engineering system without modifying its native implementation.

This milestone establishes JSL as a first-class component of the Just architecture while maintaining interoperability with the native runtime.

---

## Goal

Create a complete scripting runtime that enables safe, extensible and deterministic system customization.

---

## Engineering Capabilities

After completing this milestone the engineering system is able to:

- execute JSL programs;
- expose native functionality through JSL;
- create and modify system objects using JSL;
- automate system behavior;
- validate JSL programs;
- provide command-line development tools;
- integrate scripting with native services.

The system becomes programmable.

---

## Dependencies

Requires:

- Milestone 2
- Native Runtime
- Plugin Infrastructure
- IPC Infrastructure

The JSL runtime is designed to integrate with existing engineering components without replacing them.

---

## Deliverables

- JSL Runtime (Native)
- JSL Runtime (Managed)
- Native API Bindings
- Object Creation API
- Object Modification API
- Command Line Utilities
- Validation Framework
- Testing Framework

---

## Related Documentation

See:

- `docs/milestones/milestone3.md`

---

# Milestone 4 — User Applications

> **Status:** Planned
>
> **Priority:** High

---

## Overview

Milestone 4 introduces the user-facing application layer of the Just engineering system.

Its purpose is to transform the engineering infrastructure created by previous milestones into a complete and usable desktop environment.

This milestone focuses on graphical applications, user interaction and the integration of desktop services into a coherent user experience.

Applications are built upon the APIs, services and runtime established by earlier milestones rather than implementing system logic themselves.

---

## Goal

Create a complete desktop application ecosystem built on top of the Just engineering system.

---

## Engineering Capabilities

After completing this milestone the engineering system is able to:

- provide a complete graphical desktop interface;
- allow users to launch and manage applications;
- provide native system applications;
- expose desktop functionality through dedicated user interfaces;
- centralize development, configuration and distribution tools;
- support application discovery and installation;
- provide an integrated user experience.

The system becomes a usable Desktop Environment.

---

## Dependencies

Requires:

- Milestone 3
- Desktop Services
- JSL Runtime
- Rendering Pipeline
- Input System

Applications rely exclusively on the public APIs provided by the engineering system.

---

## Deliverables

### Desktop Interface

- Launcher
- Panel
- Search
- Login Manager

### System Applications

- File Manager
- Terminal
- Marketplace
- Calculator
- Notification Center
- Additional native applications

### Just Hub

- Settings
- IDE
- Creator
- Automation Tools
- Theme Creator
- AI Workspace
- Marketplace
- Asset Library
- Update Manager
- Application Store

---

## Related Documentation

See:

- `docs/milestones/milestone4.md`

---

# Milestone 5 — AI

> **Status:** Planned
>
> **Priority:** Medium

---

## Overview

Milestone 5 introduces artificial intelligence as a native capability of the Just engineering system.

Rather than functioning as an isolated application, AI becomes an engineering component capable of assisting development, automation and user interaction while remaining fully integrated with the existing architecture.

The objective is to enhance the engineering system without replacing deterministic system behavior.

---

## Goal

Integrate artificial intelligence into the engineering system as a reusable platform capability.

---

## Engineering Capabilities

After completing this milestone the engineering system is able to:

- generate JSL code from natural language;
- automate repetitive engineering tasks;
- assist users through an integrated desktop assistant;
- simplify desktop customization;
- accelerate workflow automation;
- extend desktop functionality using AI-assisted services.

The system becomes AI-assisted.

---

## Dependencies

Requires:

- Milestone 4
- JSL Runtime
- Desktop Services
- Plugin Infrastructure

Artificial intelligence extends the engineering system while respecting its deterministic architecture.

---

## Deliverables

### AI Development

- AI-assisted JSL Generation

### Desktop Intelligence

- Desktop Assistant

---

## Related Documentation

See:

- `docs/milestones/milestone5.md`

---

# Milestone 6 — Distribution

> **Status:** Planned
>
> **Priority:** Medium

---

## Overview

Milestone 6 transforms the Just engineering system into a distributable operating system.

Its purpose is to provide everything required to build, install and distribute Just as a complete software product while preserving the engineering principles established throughout previous milestones.

This milestone focuses on packaging, deployment and release infrastructure rather than introducing new system functionality.

---

## Goal

Create a complete distribution infrastructure capable of building, installing and releasing Just.

---

## Engineering Capabilities

After completing this milestone the engineering system is able to:

- boot as a live environment;
- install itself onto supported hardware;
- bootstrap the initial JSL environment;
- generate distributable system images;
- package system components;
- produce versioned releases;
- support automated distribution workflows.

The system becomes distributable.

---

## Dependencies

Requires:

- Milestone 5
- Boot Manager
- Desktop Environment
- JSL Runtime
- Package Management Infrastructure

Distribution infrastructure builds upon the complete engineering system established by previous milestones.

---

## Deliverables

### Live Environment

- Live Boot
- Operating System Installer
- JSL Bootstrap

### Distribution Infrastructure

- ISO Builder
- Package Generation
- Release Pipeline
- Distribution Images

---

## Related Documentation

See:

- `docs/milestones/milestone6.md`

---

# Milestone 7 — JSL Migration

> **Status:** Planned
>
> **Priority:** Long-Term

---

## Overview

Milestone 7 represents the long-term evolution of the Just engineering system.

Its purpose is to progressively migrate suitable native components from C to JSL while preserving performance, stability, maintainability and deterministic behavior.

Migration is performed incrementally and only where it improves the engineering system without compromising its architectural principles.

This milestone does not introduce a fundamentally new capability.

Instead, it evolves the internal implementation of the engineering system itself.

---

## Goal

Evolve the engineering system toward a JSL-native architecture while preserving its engineering philosophy.

---

## Engineering Capabilities

After completing this milestone the engineering system is able to:

- implement appropriate system components in JSL;
- reduce native implementation where beneficial;
- improve maintainability through higher-level abstractions;
- preserve deterministic system behavior;
- maintain compatibility between native and JSL components;
- evolve without compromising architectural consistency.

The engineering system evolves toward a JSL-native implementation.

---

## Dependencies

Requires:

- Milestone 6
- Mature JSL Runtime
- Stable Native APIs
- Complete Technical Specifications

Migration decisions are driven by engineering value rather than language preference.

---

## Deliverables

- Progressive Migration Strategy
- Native-to-JSL Compatibility Layer
- Migrated System Components
- Performance Validation
- Stability Validation
- Long-Term Maintenance Strategy

---

## Related Documentation

See:

- `docs/milestones/milestone7.md`

---

# Roadmap Summary

| Milestone | Engineering Capability |
|------------|------------------------|
| Milestone 0 | System becomes bootable |
| Milestone 1 | System becomes a Window Manager |
| Milestone 2 | System gains desktop services |
| Milestone 3 | System becomes programmable |
| Milestone 4 | System becomes a usable Desktop Environment |
| Milestone 5 | System becomes AI-assisted |
| Milestone 6 | System becomes distributable |
| Milestone 7 | System evolves toward a JSL-native implementation |

---

# Long-Term Evolution

The Just engineering system evolves incrementally through a sequence of engineering milestones.

Each milestone introduces a well-defined engineering capability while preserving the architectural principles established by the Project Vision.

Lower-level implementation details are intentionally documented separately through milestone documents, development stages and technical specifications.

The roadmap defines the long-term evolution of the engineering system rather than the implementation of individual components.