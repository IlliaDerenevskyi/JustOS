# Just — Kernel Architecture

> **Status:** Draft. The decisions that are not made yet are listed in "Open Decisions" at the end.
>
> This document defines how the Linux kernel and the Just kernel divide the work, and how they talk to each other.
>
> It describes the architecture, not the implementation. Every engineering component is specified separately in its own milestone document.

---

# Table of Contents

- [Overview](#overview)
- [Purpose](#purpose)
- [Two Kernels](#two-kernels)
- [What Each Kernel Does](#what-each-kernel-does)
- [Where the Just Kernel Sits](#where-the-just-kernel-sits)
- [Communication Between the Kernels](#communication-between-the-kernels)
- [Relationship to Boot](#relationship-to-boot)
- [Engineering Principles](#engineering-principles)
- [How the Milestones Build the Just Kernel](#how-the-milestones-build-the-just-kernel)
- [Relationship to Other Documents](#relationship-to-other-documents)
- [Open Decisions](#open-decisions)

---

# Overview

JustOS is a Linux distribution. A running JustOS therefore has two kernels, and each one does its own work:

- the **Linux kernel**, which runs the hardware and the processes;
- the **Just kernel**, the kernel of the distribution, which runs everything the user sees and uses.

The two kernels never do each other's work. They exchange information and commands through a defined channel (see "Communication Between the Kernels").

The word "kernel" in the name "Just kernel" is a project term, not the operating-system term. In this document "the Linux kernel" always means the kernel of the Linux operating system, and "the Just kernel" always means the kernel of the distribution.

The boot loader (the Just Boot Manager, described in `boot.md`) is not counted as either of them. It runs before both. It hands control to the Linux kernel and leaves the flow for good. The Just kernel does not start from the boot loader. It starts later, from the running Linux kernel, as a process of Linux (see "Relationship to Boot").

---

# Purpose

The purpose of this document is to fix, once, three things that every later document depends on:

1. which work belongs to the Linux kernel and which work belongs to the Just kernel;
2. in which order the parts of the system are stacked on each other;
3. how the two kernels exchange information and commands.

Without this document each milestone would have to guess where its component lives. The milestone documents (`milestone0.md`, and the ones that follow) describe how a part is built. This document says what the part is for and where it stands.

---

# Two Kernels

```text
   ┌── Just kernel (the kernel of the distribution) ──┐
   │   Applications (JSL and others)                  │
   │             │                                    │
   │   Services                                       │
   │             │                                    │
   │   Shell                                          │
   │             │                                    │
   │   Kernel layer · State · Scene · Layout ·        │
   │   Policy · Backend · Renderer                    │
   │             │                                    │
   │   DRM / Wayland / X11                            │
   └─────────────┬─────────────────▲──────────────────┘
      information │       ipc       │ commands
                  ▼                 │
              Linux kernel
                  │
               Hardware
```

The picture shows the order (from the high-level work at the top to the low level at the bottom) and the direction of the exchange. The work of the applications is high-level. The drawing, the graphics system, and the Linux kernel are low-level. The two kernels do not reach into each other's data. They talk only through the channels described in "Communication Between the Kernels".

Why two kernels and not one: the Linux kernel already solves the problems of hardware, drivers, processes, memory, files, and the network, and solves them well. The Just kernel does not repeat that work. Its job is the part that makes the system "Just": how windows, drawing, programs, and the user's work are organized, in the deterministic and documented way that the project vision requires.

---

# What Each Kernel Does

| Question | Linux kernel | Just kernel |
| --- | --- | --- |
| Talks to the hardware and its drivers | Yes | No (it asks the Linux kernel) |
| Runs and schedules processes, owns the memory and the file systems | Yes | No |
| Owns the state of windows, workspaces, and screens | No | Yes |
| Decides the layout of windows | No | Yes |
| Draws, through the graphics backend (Wayland or X11) | No (it only provides the graphics device) | Yes |
| Handles input as the user sees it (keys, pointer, focus) | Delivers the raw events | Interprets them |
| Runs the Just programming language (JSL) and the applications | Runs the processes | Defines the runtime they run in |
| Provides the shell and the system services | No | Yes |

The rule behind the table: if the Linux kernel already provides a function, the Just kernel uses it and does not build a second copy of it. The Just kernel may complement it, that is, add what the Linux function does not give, on top of the Linux function. If a function is about how the distribution presents and organizes the system, the Linux kernel is not asked to know about it.

---

# Where the Just Kernel Sits

The Just kernel is built in layers. Each layer depends only on the layers below it, and a lower layer never calls a higher one (Rule 23 in `rules.md`, which governs the order). From the top to the bottom:

```text
Applications (JSL and others)
    ↓
Services
    ↓
Shell
    ↓
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
    ↓
DRM / Wayland / X11
    ↓
Linux kernel
```

The Shell and the Services stand above the Kernel layer, and the applications above them (decided with the project owner on 2026-11-22, shown in `layers.md`). The layer named Kernel is the core of the Just kernel. It is not the same thing as the Linux kernel at the bottom.

What the layers mean. This is a first reading from the engineering breakdown (`engineering_breakdown.md`); the exact responsibility of each layer is fixed in the Milestone 1 specification:

- **Kernel:** the communication core of the Just kernel (the internal message bus, the router that sends each message to its handler, and the transport between processes).
- **State:** the authoritative record of windows, workspaces, outputs, and monitors.
- **Scene:** the tree of what is shown.
- **Layout:** the calculation of where each window goes.
- **Policy:** the rules of the system, for example the window rules.
- **Backend:** the abstract interface to a graphics system, with the Wayland and X11 implementations behind it.
- **Renderer:** the drawing itself, including the tracking of which part of the screen changed.

Nobody but the layer that owns a piece of information may change it. Another layer asks the owner through the Kernel layer.

---

# Communication Between the Kernels

The two kernels exchange two kinds of things:

- **information**, going up: what the Linux kernel knows and the Just kernel needs, for example that a screen was connected, that a key was pressed, that a device appeared or went away;
- **commands**, going down: what the Just kernel decides and the Linux kernel must carry out, for example to open a device or to change a screen mode.

The exchange follows these rules:

1. **Explicit.** Every kind of message is defined in a specification before it is used. No message travels through hidden state.
2. **Both ways are named.** For every message it is written who sends it, who receives it, and what a failure means.
3. **Truthful.** If the Linux kernel cannot answer, the Just kernel shows that the answer is unavailable (`UNKNOWN`, `NOT DETECTED`, `NOT INITIALIZED`, or `FAILED`, Rule 30). It never invents an answer.
4. **One owner.** A fact has one owner. The Just kernel keeps a copy only as a record of what the owner last reported, and says so.

**How the exchange is carried** (decided with the project owner on 2026-11-22: both ways are used).

In Linux, IPC (Inter-Process Communication) is the set of kernel mechanisms and system interfaces that let isolated processes exchange data and synchronize: pipes, UNIX domain sockets, shared memory, message queues, signals, and similar. The exchange of the Just kernel uses two paths:

1. **The standard Linux interfaces.** The Linux kernel is not a process, so a Just process reaches it through system calls and the interfaces that the kernel offers to programs (for example device files, the graphics and input interfaces, and the notifications about devices). This is how the information goes up and the commands go down.
2. **The project's own protocol on top of IPC.** Between the Just processes, and between them and the applications, the messages follow the project protocol (JRP) and travel through the Kernel Bus, the Router, and IPC over UNIX domain sockets (Milestone 1).

The exact list of messages and interfaces is specified in Milestone 1.

---

# Relationship to Boot

Everything described so far is the running system. Before it there is the boot, which is described in `boot.md` and specified in `milestone0.md`.

The Boot Manager discovers the machine, loads a kernel image, prepares its first execution environment, and hands control over. From that moment the Boot Manager is gone: it never resumes.

**The boot hands control to the Linux kernel** (decided with the project owner on 2026-11-22). The Just kernel does not receive control from the boot. It receives control from the running Linux kernel, as a process that Linux has adopted, and from then on it is one of the processes of the Linux system.

Milestone 0 builds and proves the boot machinery: the firmware layers (UEFI and BIOS), the memory map, the framebuffer, loading and checking a kernel image, the boot menu and its configuration, the diagnostics, and the hand-over of control. It proves the hand-over with a test kernel, in the format that Milestone 0 defines (`JPBootContext` together with `JPHandoffInfo`). The hand-over to the Linux kernel itself, in the way the Linux kernel expects to be started, is not part of Milestone 0 (see "Open Decisions").

---

# Engineering Principles

The Just kernel follows the axioms of `project_vision.md`:

- **Deterministic:** the same input in the same state gives the same result.
- **Documentation-driven:** the specification comes before the code.
- **Zero hidden state:** every state lives in a known place with a known owner.
- **Backend independent:** the graphics system (Wayland, X11) is behind an interface, and no layer above it knows which one is in use.

It also follows the security documents in `docs/security/`: the security layers apply to the Just kernel, and a check of structure is never presented as a proof of origin.

---

# How the Milestones Build the Just Kernel

This is the map of where each part comes from (from `roadmap.md` and `engineering_breakdown.md`):

| Milestone | What it adds to the Just kernel |
| --- | --- |
| 0, Boot Manager | The boot and the hand-over (not a part of either kernel) |
| 1, Core | The internal protocol (JRP), the Kernel Bus, the Router, IPC transport; the State engine; the Scene engine; the Layout engine; the Input engine; the Backend abstraction (Wayland and X11); the Renderer; the Shell integration; the plugin infrastructure |
| 2, Desktop Services | The services of the distribution (hardware, session, notifications, settings, packages, and others) and the daemon that runs them |
| 3, JSL Runtime | The runtime of the Just programming language |
| 4, User Applications | The applications and the Just Hub |
| 5, AI | The AI services |
| 6, Distribution | The live environment, the installer, and the images |
| 7, JSL Migration | Moving parts of the system to JSL where it gives value |

---

# Relationship to Other Documents

```text
Project Vision
        │
        ▼
Roadmap
        │
        ▼
Architecture (this document, layers.md, overview.md)
        │
        ▼
Milestone documents
        │
        ▼
Source Code
```

Related documents: `boot.md` (the boot architecture), `layers.md` (the diagram of the layers) and `overview.md` (the same order as a list), `docs/security/` (the security architecture), `roadmap.md`, and `engineering_breakdown.md`.

---

# Open Decisions

These points are not decided. Each one needs the project owner's decision before the document is locked, and before the milestone that depends on it is specified.

1. **The hand-over to the Linux kernel.** The boot hands control to the Linux kernel (decided), but Milestone 0 hands control to a kernel in its own format: an ELF image, an entry that receives `JPBootContext`, a processor state and page tables prepared by Stage 1, and a runtime prepared by Stage 2 (paging, stack, heap) for a freestanding kernel without a C library. The Linux kernel is started in another way: its own image format and boot protocol, its command line, and often an initial RAM disk. It must be decided which milestone specifies and builds the hand-over to Linux, which parts of Milestone 0 it reuses (for example the memory map, the framebuffer, and the boot configuration, whose kernel parameters could become the Linux command line), and what the runtime of Stage 2 and `kernel_main` are for once Linux is the kernel that starts.
2. **How Linux starts the Just kernel.** The Just kernel starts as a process of the running Linux system. Which process it is: the first process of the system (the init process), a service started by the init system, or something else.
3. **One part or two.** Whether the Just kernel runs as one part, or as two cooperating parts, and if two, whether they are separate processes or separate threads of one process. The project owner would like the second kernel to be hidden from the user and leans toward two parts in different threads, but has not decided. Things to weigh: threads of one process share memory and are cheap to connect, while separate processes isolate a failure and fit the layered security of `docs/security/`. The engineering breakdown also names a daemon in Milestone 2.
