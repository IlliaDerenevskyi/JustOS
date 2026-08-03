# Just — Engineering Breakdown

> This document describes the engineering decomposition of the Just engineering system.
>
> Unlike the Roadmap, which defines long-term project evolution, this document explains how each milestone is progressively decomposed into engineering subsystems.
>
> It represents the engineering work breakdown structure of the project.

---

# Milestone 0 — Boot Infrastructure

## Boot Manager

### Boot Manager Core

- Bootloader
- Boot Configuration
- Boot Menu
- Recovery Entry

### Kernel Loading

- ELF Loader
- Kernel Validation
- Boot Parameters
- Boot Handoff

### Boot Information

- BootInfo
- Memory Map
- Framebuffer
- Video Mode

### Platform Initialization

- CPU Initialization
- Paging
- Early Stack
- Early Heap

### Boot Diagnostics

- Boot Logging
- Error Reporting
- Recovery Support

---

# Milestone 1 — Core

## Foundation & Transport Layer

### JRP Protocol

- TLV Encoding
- Serialization
- Deserialization
- CRC32 Validation
- Transaction ID
- Sequence Numbers
- Timestamp
- Versioning

### Kernel Bus

- Multi-Ring Buffer
- Priority Queues
- Eviction Policy
- Message Scheduling
- Latency Statistics

### Router

- Module Registry
- Command Registry
- O(1) Dispatch
- Handler Resolution

### IPC Transport

- UNIX Domain Socket
- Non-Blocking Communication
- Connection Management
- Session Transport

### Low-Level Base

- Memory
- Frame Arena
- Geometry
- String Utilities
- Logging
- CRC Utilities

---

## Core State Engine

### Model

- Windows
- Workspaces
- Outputs
- Monitors

### Registries

- Window Registry
- Workspace Registry
- Output Registry
- Backend Registry

### Scheduler

- Dirty Flags
- Frame Scheduler
- Dispatch Pipeline

### Event System

- Event Queue
- Priorities
- Hooks
- Event Dispatch

### Scene Engine

- Scene Tree
- Scene Nodes
- Focus Stack

### State Engine

- Global State
- Context
- State Synchronization

---

## Layout Engine

### Layout Infrastructure

- Layout Registry
- Layout Context
- Geometry Cache
- Constraints

### Layout Algorithms

- Master Stack
- Grid
- Monocle
- Floating

### Window Rules

- Floating Rules
- Workspace Rules
- Size Constraints
- Placement Rules

---

## Input Engine

### Keyboard

- Key Dispatcher
- Hotkeys
- Command Mapping

### Pointer

- Cursor Tracking
- Focus Follows Mouse
- Drag & Drop
- Gestures

---

## Backend Abstraction

### Backend Layer

- Backend Interface
- Backend Registry
- Surface Mapping

### Wayland Backend

- wlroots Integration

### X11 Backend

- XCB Integration

---

## Rendering Pipeline

### Renderer

- Render Graph
- Render Queue
- Commit Pipeline

### Damage Tracking

- Damage Regions
- Partial Rendering
- Geometry Cache

---

## Shell Integration

### justctl

- CLI
- IPC Commands

### Shell Protocol

- Launcher Protocol
- Panel Protocol
- Widget Protocol

### Theme Engine

- Themes
- Decorations
- Colors
- Borders
- Gaps

---

## Plugin Infrastructure

### Plugin API

- Commands
- Events
- Hooks

### Snapshot Engine

- State Snapshot
- Undo
- Redo
- Session Restore
- Hot Reload

### AI Infrastructure

- AI API
- Context API
- Extension Interfaces

---

# Milestone 2 — Desktop Services

## Hardware Services

### Device Integration

- BlueZ
- UDisks2
- PipeWire
- NetworkManager
- Power Management
- Display Management

---

## Core Desktop Services

### Session Services

- Session Manager
- Process Manager
- Configuration
- Notifications

### Desktop Logic

- Launcher
- Search
- Rules
- Settings

### Index Engine

- Object Discovery
- Metadata Extraction
- Relations
- Search Index
- Update Pipeline

---

## Appearance Services

### Desktop Appearance

- Theme Service
- Wallpaper Service
- Workspace Service
- Window Appearance

---

## Desktop Daemon

### Service Orchestration

- Service Lifecycle
- Dependency Management
- Event Coordination

---

# Milestone 3 — JSL Runtime

## Runtime

### Core Runtime

- Parser
- Runtime
- Memory
- Execution Engine

---

## Development API

### Creation API

- Projects
- Components
- Resources

### Modification API

- Updates
- Refactoring
- Validation

---

## Toolchain

### CLI

- Build
- Run
- Package

### Testing

- Validation
- Runtime Tests

---

## Native Runtime

### JSL Runtime

- Interpreter
- Runtime Libraries
- Execution Pipeline

---

# Milestone 4 — User Applications

## Desktop Interface

### User Interface

- Launcher
- Panel
- Login
- Search

---

## System Applications

### Core Applications

- Files
- Terminal
- Marketplace
- Calculator
- Notifications

---

## Just Hub

### Development Platform

- Settings
- IDE
- Creator
- Automation

### Asset Management

- Themes
- Widgets
- Wallpapers
- Marketplace

---

# Milestone 5 — Artificial Intelligence

## Knowledge Integration

### Index Engine

- Object Index
- Metadata
- Relations

### Documentation

- Engineering Documents
- API References
- Specifications

### Search

- Semantic Search
- Symbol Search
- Documentation Search

### Context Retrieval

- Engineering Context
- Project Context
- Runtime Context

---

## AI Assistant

### Desktop Assistant

- Desktop Automation
- Command Execution
- Workflow Assistance

### AI Services

- JSL Assistance
- Engineering Assistance
- Documentation Assistance

---

# Milestone 6 — Distribution

## Distribution Infrastructure

### Live Environment

- Live Boot
- Installer
- Bootstrap

---

### Distribution Builder

- ISO Builder
- Packaging
- Release Pipeline

---

# Milestone 7 — JSL Migration

## Migration Strategy

### Core Components

- Core Libraries
- Runtime Infrastructure

### Desktop Services

- just-* Services
- Daemons
- Background Components

### Applications

- Desktop Applications
- System Utilities

### Toolchain

- Build System
- Development Tools
- Packaging

### Performance Validation

- Benchmarks
- Memory Validation
- Runtime Optimization

### Compatibility Validation

- API Compatibility
- Behaviour Consistency
- Regression Testing