# Just — Boot Architecture

> This document defines the architecture of the Just Boot System.
>
> It describes how the engineering system starts, initializes the execution environment and transfers control to the kernel.
>
> This document intentionally avoids implementation details.
> Every engineering component is specified separately in its own engineering specification.

---

# Table of Contents

- [Overview](#overview)
- [Purpose](#purpose)
- [Boot Philosophy](#boot-philosophy)
- [Diagrams](#diagrams)
- [Design Goals](#design-goals)
- [Boot Architecture](#boot-architecture)
- [Boot Flow](#boot-flow)
- [Boot Manager](#boot-manager)
- [Boot Manager Interface](#boot-manager-interface)
- [Boot Configuration](#boot-configuration)
- [Kernel Loading](#kernel-loading)
- [Memory Discovery](#memory-discovery)
- [Framebuffer Initialization](#framebuffer-initialization)
- [BootInfo](#bootinfo)
- [Kernel Handoff](#kernel-handoff)
- [Recovery Mode](#recovery-mode)
- [Boot Diagnostics](#boot-diagnostics)
- [Relationship to Other Documents](#relationship-to-other-documents)
- [Engineering Guarantees](#Engineering-Guarantees)
- [Future Extensions](#future-extensions)


---

# Overview

The Just Boot System is responsible for establishing the initial execution environment of the engineering system.

Its responsibility begins when the firmware transfers execution to the Boot Manager and ends when control is transferred to the kernel.

The boot system performs orchestration rather than operating system functionality.

Its purpose is to collect the information required by the kernel, validate the boot environment and prepare a deterministic execution state.

The boot process must remain predictable, reproducible and architecture-independent whenever possible.

**Which kernel.** In this document "the kernel" means the kernel image that the Boot Manager hands control to. In the finished distribution this is the Linux kernel. The Just kernel (the kernel of the distribution) is not started by the Boot Manager: it starts later, from the running Linux kernel, as one of its processes (see `kernel.md`). Milestone 0 builds and proves the boot machinery and proves the hand-over with a test kernel, in the format that Milestone 0 defines (`JPBootContext` together with `JPHandoffInfo`). The hand-over to the Linux kernel in the way Linux expects to be started is not part of Milestone 0 (see "Open Decisions" in `kernel.md`).

---

# Purpose

The purpose of the Boot System is to create a reliable and deterministic transition between firmware and the kernel.

During startup it discovers the execution environment, initializes essential boot services, validates the selected kernel image and prepares the normalized `JPBootContext` and `JPHandoffInfo` transfer contract passed to Stage 2.

The boot system never performs responsibilities belonging to the kernel.

Once the kernel receives control, the Boot Manager permanently leaves the execution flow.

---

# Boot Philosophy

The Just Boot System follows the same engineering philosophy as every other subsystem within the operating system.

Boot is treated as an engineering pipeline rather than a collection of independent initialization routines.

Every component owns a single responsibility.

Every transition is deterministic.

Every decision is reproducible.

The boot system never performs unnecessary work.

Only the minimum execution environment required by the kernel is prepared.

The kernel is responsible for operating system initialization.

The Boot Manager is responsible only for preparing the conditions required for the kernel to begin execution safely.

This separation allows the boot infrastructure and kernel to evolve independently while maintaining stable engineering interfaces.

---
# Diagrams

UEFI
 │
 ▼
Boot Manager
 │
 ├── Configuration
 │
 ├── Diagnostics
 │
 ├── Security Checks
 │
 ├── Boot Security
 │
 └── Kernel Loader
        │
        ▼
Kernel Entry
---

# Design Goals

The boot architecture is designed to be:

- deterministic;
- reproducible;
- modular;
- architecture-independent;
- firmware-independent whenever possible;
- fail-safe;
- observable;
- documentation-driven;
- easily diagnosable;
- extensible.

The boot system should always produce identical results when executed under identical conditions.

Unexpected behaviour must never become part of the startup process.

---

# Boot Architecture

The boot architecture consists of several engineering components cooperating through a linear startup pipeline.

Each component performs one well-defined responsibility before transferring execution to the next stage.

```text
Firmware
      │
      ▼
Boot Manager
      │
      ▼
Boot Configuration
      │
      ▼
Kernel Loader
      │
      ▼
Memory Discovery
      │
      ▼
Framebuffer Initialization
      │
      ▼
Boot Security and Handoff State
      │
      ▼
Kernel Handoff
      │
      ▼
Kernel
```

The Boot Manager coordinates this entire pipeline.

Individual components never bypass intermediate stages or communicate through hidden state.

All information required by the kernel is collected before execution is transferred.

---

# Boot Flow

The startup sequence follows a deterministic execution order.

```text
Power Button
      │
      ▼
Firmware Initialization
      │
      ▼
Firmware loads Just Boot Manager
      │
      ▼
Boot Manager starts
      │
      ▼
Boot configuration loaded
      │
      ▼
User configuration (optional)
      │
      ▼
Kernel selected
      │
      ▼
Kernel image validated
      │
      ▼
Memory discovered
      │
      ▼
Framebuffer initialized
      │
      ▼
Normalized handoff state prepared
      │
      ▼
CPU state prepared
      │
      ▼
Kernel handoff
      │
      ▼
Kernel execution begins
```

Each stage completes before the next stage begins.

No stage executes concurrently.

The startup sequence is intentionally linear to simplify diagnostics, verification and future maintenance.

Every transition between stages is explicit and observable through the boot diagnostics subsystem.

---

# Boot Manager

The Boot Manager is the central orchestrator of the Just Boot System.

Its responsibility is to coordinate every stage of the startup process before execution is transferred to the kernel.

Unlike a traditional bootloader, the Boot Manager does not implement operating system functionality.

Instead, it prepares the complete execution environment required by the kernel.

Its primary responsibilities include:

- loading boot configuration;
- selecting the kernel;
- validating the kernel image;
- discovering system memory;
- initializing the framebuffer;
- collecting firmware information;
- constructing BootInfo;
- coordinating diagnostics;
- transferring execution to the kernel.

The Boot Manager owns the startup sequence.

It never owns the operating system.

Once control has been transferred to the kernel, the Boot Manager permanently leaves the execution flow.

---

# Boot Manager Interface

The Boot Manager provides a minimal user interface allowing limited configuration before the operating system starts.

The interface exists solely to configure the startup environment.

It is not intended to become a pre-operating-system shell.

Example layout:

```text
+------------------------------------------------------------+
|                  Just Boot Manager                         |
+------------------------------------------------------------+

 [ General ] [ Kernel ] [ Theme ] [ AI ] [ Recovery ] [info]

--------------------------------------------------------

 Kernel Backend

 (•) Wayland
 ( ) X11
 ( ) Experimental

--------------------------------------------------------

 Logging

 [x] Enable Boot Log
 [ ] Verbose Output

--------------------------------------------------------

 Safe Mode

 [ ] Enable Safe Mode

--------------------------------------------------------

 Recovery

 [ ] Recovery Boot

--------------------------------------------------------

 Framebuffer

 Resolution:
 1920 × 1080

--------------------------------------------------------

                  [F10] Boot

                  [ESC] Exit
```

The interface is intentionally simple.

Navigation is performed using:

- ↑
- ↓
- ←
- →
- Enter
- ESC
- F10

Every option modifies only the boot configuration.

No permanent system configuration is changed until the operating system has started.

---

## General

The General tab contains common startup options.

Typical settings include:

- Kernel Backend
- Boot Delay
- Boot Logging
- Verbose Output
- Safe Mode
- Recovery Mode

These options influence the startup sequence but never modify the operating system itself.

---

## Kernel

The Kernel tab selects the execution backend and kernel startup parameters.

Typical options include:

- Wayland
- X11
- Experimental Backend
- Kernel Parameters

Only one execution backend may be selected.

---

## Theme

The Theme tab controls the visual appearance of the boot interface.

Example themes:

- Dark
- Light
- Developer
- Minimal

Themes affect only the Boot Manager interface.

The operating system loads its own theme independently after startup.

---

## AI

The AI tab controls optional AI-related startup functionality.

Possible options include:

- Enable AI
- Debug Context
- Disable AI

These options determine whether AI services should be initialized after the operating system has started.

The Boot Manager itself performs no AI processing.

---

## Recovery

The Recovery tab provides access to recovery-oriented startup modes.

Available actions may include:

- Recovery Boot
- Diagnostics
- Repair
- Boot Log Viewer

Recovery capabilities remain intentionally minimal.

Complex recovery procedures belong to the operating system rather than the boot environment.

---

# Boot Configuration

The Boot Configuration defines how the engineering system should be initialized.

It represents the complete startup configuration consumed by the Boot Manager.

Typical configuration categories include:

```text
Boot Configuration

├── Selected Kernel
├── Kernel Parameters
├── Boot Backend
├── Framebuffer Settings
├── Boot Theme
├── Safe Mode
├── Recovery Mode
├── Logging
├── AI Options
└── Boot Flags
```

The configuration is loaded before the kernel image is selected.

Users may modify supported options through the Boot Manager interface before startup continues.

After the kernel receives control, the boot configuration becomes read-only.

The kernel consumes the configuration through the normalized Stage 1 to Stage 2 handoff contract rather than interacting with the Boot Manager directly. `JPBootInfo` is not part of Stage 1 or Stage 2.

---

# Kernel Loading

Kernel loading is responsible for locating, validating and preparing the selected kernel image.

The Boot Manager never executes a kernel that has not successfully completed validation.

The loading process follows a deterministic sequence.

```text
Kernel Selected
        │
        ▼
Locate Image
        │
        ▼
Read ELF Image
        │
        ▼
Validate ELF
        │
        ▼
Validate Architecture
        │
        ▼
Validate Entry Point
        │
        ▼
Map Executable Sections
        │
        ▼
Prepare Kernel Image
```

The Boot Manager verifies that the selected image is compatible with the current execution environment.

Only after successful validation is the kernel prepared for execution.

Any validation failure immediately stops the startup sequence and transfers control to the diagnostics or recovery subsystem.

The kernel loader performs no operating system initialization.

Its responsibility ends once the executable image has been successfully prepared for kernel handoff.

---

# Memory Discovery

Before the kernel can begin execution, the Boot Manager must discover the physical memory available to the system.

The purpose of memory discovery is not to manage memory, but to describe the execution environment accurately.

Memory management belongs entirely to the kernel.

The Boot Manager only observes and records the memory layout provided by the firmware.

The discovery process follows a deterministic sequence.

```text
Firmware Memory Information
           │
           ▼
Read Memory Map
           │
           ▼
Validate Entries
           │
           ▼
Normalize Regions
           │
           ▼
Classify Memory Types
           │
           ▼
Create Memory Map
           │
           ▼
Store in normalized handoff state
```

The resulting memory map describes every known physical memory region.

Typical region types include:

- usable memory;
- reserved memory;
- firmware memory;
- ACPI regions;
- MMIO regions;
- framebuffer memory;
- boot services memory;
- runtime services memory.

The Boot Manager never allocates or manages physical memory beyond what is required for its own execution.

The kernel becomes the sole owner of memory management after kernel handoff.

---

# Framebuffer Initialization

The framebuffer provides the initial graphical output available before the kernel graphics subsystem is initialized.

The Boot Manager discovers the framebuffer provided by the firmware and records its characteristics.

Initialization follows a deterministic process.

```text
Firmware Graphics Interface
            │
            ▼
Locate Framebuffer
            │
            ▼
Validate Video Mode
            │
            ▼
Read Framebuffer Information
            │
            ▼
Store Framebuffer Descriptor
            │
            ▼
BootInfo
```

Typical framebuffer information includes:

- framebuffer address;
- width;
- height;
- pixel format;
- color depth;
- pixels per scanline.

The Boot Manager never performs rendering beyond its own interface.

Window management, compositing and graphical services are responsibilities of the operating system.

The framebuffer remains available to the kernel immediately after startup, allowing early graphical output before higher-level rendering systems become available.

---

# BootInfo

The generic term `BootInfo` must not be used ambiguously. The authoritative Stage 1 to Stage 2 contract is:

```text
JPBootContext
      +
JPHandoffInfo
      +
explicit ownership-transfer state
```

`JPBootInfo` is not part of Stage 1 or Stage 2. A later kernel-facing `BootInfo` object may be introduced only as a separate structure with an explicitly defined producer, consumer, ABI, ownership, lifetime, conversion source, and stage boundary.

The normalized handoff contract represents the execution environment prepared during startup.

The Boot Manager and platform provider construct the normalized handoff state according to their Stage 1 responsibilities.

After kernel handoff, `JPBootContext` and `JPHandoffInfo` remain authoritative for the Stage 2 initialization lifetime defined by Milestone 0.

A typical BootInfo structure contains information such as:

```text
Normalized Handoff State

├── Firmware Information
├── Boot Configuration
├── Boot Flags
├── Memory Map
├── Framebuffer
├── CPU Information
├── Video Mode
├── Kernel Parameters
├── Boot Timestamp
└── Architecture Information
```

Each field represents a distinct engineering responsibility.

No subsystem maintains duplicate copies of the same information.

The kernel consumes the normalized handoff state during Stage 2 initialization and distributes relevant information to the appropriate kernel subsystems.

The handoff state exists only to transfer startup information.

It never becomes a long-term system database.

---

# Kernel Handoff

Kernel handoff is the final responsibility of the Boot Manager.

At this point every required startup task has been completed.

No further boot initialization is performed after execution is transferred.

The handoff sequence follows a strict order.

```text
Boot Manager
      │
      ▼
Finalize Diagnostics
      │
      ▼
Finalize JPBootContext and JPHandoffInfo
      │
      ▼
Release Boot Resources
      │
      ▼
Prepare CPU State
      │
      ▼
Prepare Initial Stack
      │
      ▼
Transfer normalized handoff state
      │
      ▼
Jump to Kernel Entry Point
      │
      ▼
Kernel
```

Before transferring execution, the Boot Manager ensures that:

- the kernel image has been validated;
- the normalized handoff state has been completed;
- the execution environment is consistent;
- the processor state is prepared;
- the initial execution stack is available.

Once the kernel entry point is invoked, the Boot Manager permanently relinquishes control.

The kernel becomes fully responsible for the engineering system.

The Boot Manager never resumes execution after a successful kernel handoff.

---

# Recovery Mode

Recovery Mode provides a controlled startup environment when the normal boot process cannot safely continue.

Its purpose is to assist diagnostics and system recovery without loading the complete operating system.

Recovery Mode is entered only when explicitly requested by the user or when the Boot Manager determines that normal startup cannot continue safely.

Typical entry conditions include:

- invalid kernel image;
- corrupted boot configuration;
- boot validation failure;
- repeated startup failures;
- explicit user request;
- firmware recovery request.

The recovery workflow follows a deterministic sequence.

```text
Boot Failure
      │
      ▼
Recovery Decision
      │
      ▼
Recovery Environment
      │
      ▼
Diagnostics
      │
      ▼
Repair (optional)
      │
      ▼
Retry Boot
      │
      ▼
Normal Startup
```

Recovery Mode intentionally provides only a limited execution environment.

Its responsibilities include:

- viewing boot diagnostics;
- inspecting boot configuration;
- selecting an alternative kernel;
- performing basic repair operations;
- restarting the boot sequence.

Operating system functionality is intentionally unavailable while running in Recovery Mode.

---

# Boot Diagnostics

Boot Diagnostics provides complete visibility into the startup process.

Every significant event occurring during boot is observable.

The diagnostic subsystem exists to simplify troubleshooting, validation and engineering verification.

Typical diagnostic events include:

- firmware initialization;
- configuration loading;
- kernel selection;
- kernel validation;
- memory discovery;
- framebuffer initialization;
- normalized handoff-state creation;
- kernel handoff;
- recovery entry;
- fatal errors.

The diagnostic flow can be represented as:

```text
Boot Event
      │
      ▼
Diagnostics Engine
      │
      ▼
Boot Log
      │
      ▼
Status Information
      │
      ▼
Error Reporting
      │
      ▼
Recovery Decision
```

Diagnostics never alter the startup sequence.

They only observe, record and report boot events.

This separation ensures that diagnostics remain deterministic and do not influence system behaviour.

---

# Relationship to Other Documents

The Boot Architecture document describes the behaviour of the boot system.

It intentionally omits implementation details.

Implementation is specified separately through engineering specifications.

The documentation hierarchy is:

```text
Project Vision
        │
        ▼
Roadmap
        │
        ▼
Engineering Breakdown
        │
        ▼
Boot Architecture
        │
        ▼
Milestone 0 — Boot Infrastructure
        │
        ▼
Source Code
```

Related engineering documents include:

- `kernel.md` (the architecture of the two kernels)
- `security_system.md`
- `layers.md`
- `overview.md`
- `milestone0.md`

Each document describes a different level of the engineering system.

No document duplicates responsibilities belonging to another.

---
# Engineering Guarantees

After Boot Manager finishes:

- kernel integrity has been verified;
- normalized handoff state is complete;
- memory map is validated;
- framebuffer is initialized;
- execution context is deterministic;
- control is transferred to the kernel.
---

# Future Extensions

The Boot Architecture is intentionally designed to evolve without changing its core engineering principles.

Future extensions may include:

- Secure Boot integration;
- cryptographic kernel verification;
- signed boot configurations;
- multiple boot profiles;
- automatic boot fallback;
- boot performance profiling;
- graphical boot themes;
- hardware compatibility diagnostics;
- remote diagnostics;
- encrypted boot configuration;
- AI-assisted boot diagnostics;
- boot telemetry;
- multi-platform firmware support;
- live boot support.

Future capabilities should extend the existing architecture rather than replace it.

The fundamental engineering pipeline remains unchanged:

```text
Firmware
      │
      ▼
Boot Manager
      │
      ▼
Configuration
      │
      ▼
Kernel Loading
      │
      ▼
Environment Discovery
      │
      ▼
BootInfo
      │
      ▼
Kernel Handoff
      │
      ▼
Kernel
```

Regardless of future improvements, the boot system continues to follow the same engineering philosophy:

- deterministic execution;
- explicit responsibilities;
- observable behaviour;
- reproducible startup;
- architecture-driven design;
- documentation-driven implementation.