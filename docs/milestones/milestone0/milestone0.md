# Milestone 0 — Boot Manager

> **Status:** Planned
>
> **Priority:** Critical
>
> **Version:** 1.0

---

# Table of Contents

- [Overview](#overview)
- [Goals](#goals)
- [Dependencies](#dependencies)
- [Deliverables](#deliverables)
- [Not Included](#not-included)

- [Stage 1 — Boot Manager](#stage-1--boot-manager)
- [Stage 2 — Platform Initialization](#stage-2--platform-initialization)
- [Stage 3 — Boot Diagnostics](#stage-3--boot-diagnostics)

- [Security Integration](#Security-Integration)

- [Checklist](#checklist)
- [Progress](#progress)
- [Expected Result](#expected-result)
- [Future Extensions](#future-extensions)

---

# Overview

Milestone 0 establishes the complete boot infrastructure of the Just engineering system.

Its purpose is to initialize the execution environment, load the Just kernel, prepare essential platform information and safely transfer control from firmware to the operating system.

Unlike later milestones, Milestone 0 does not implement desktop functionality, services or user interaction.

Its only responsibility is to start the engineering system in a deterministic, verifiable and reproducible manner.

Every subsequent milestone depends on the successful completion of this milestone.

---

# Goals

Milestone 0 provides the engineering foundation required to execute the Just kernel.

After completing this milestone the system is capable of:

- booting through supported firmware interfaces;
- loading the Just kernel executable;
- initializing early execution state;
- detecting physical memory;
- building the initial memory map;
- initializing framebuffer information;
- collecting boot environment information;
- constructing the BootInfo structure;
- transferring execution to the kernel;
- providing basic diagnostic and recovery capabilities.

The engineering result of this milestone is a fully bootable operating system kernel.

---

# Dependencies

Required:

- ISO C17
- CMake
- POSIX build environment
- LLVM / GCC toolchain

Boot implementation may support multiple firmware interfaces.

Examples include:

- UEFI
- BIOS (optional)
- Virtualized firmware environments

Platform-specific implementations remain isolated from the engineering architecture.

---

# Deliverables

Milestone 0 produces the following engineering components:

- Boot Manager
- Kernel Loader
- BootInfo Generator
- Memory Discovery
- Memory Map Builder
- Framebuffer Discovery
- Early Platform Initialization
- Boot Diagnostics
- Recovery Entry Infrastructure

All components expose documented public interfaces.

No implementation details are shared with later milestones except through BootInfo.

---

# Not Included

Milestone 0 intentionally excludes:

- compositor;
- window management;
- rendering;
- IPC;
- scheduler;
- services;
- package management;
- security services;
- desktop shell;
- launcher;
- search;
- networking;
- storage management.

These responsibilities belong to later milestones.

---
# Stage 1 — Boot Manager

## Goal

Stage 1 establishes the complete boot infrastructure responsible for transferring execution from firmware to the Just kernel.

This stage defines every engineering component participating in system startup, including firmware interaction, kernel loading, BootInfo construction and execution handoff.

After completion, the system is capable of loading the kernel in a deterministic and reproducible manner.

---

## Design Principles

Every subsystem developed during this stage must satisfy the following engineering principles:

- deterministic;
- fail-safe;
- architecture-independent where possible;
- firmware abstraction;
- documentation-driven;
- explicit ownership;
- reproducible boot sequence;
- minimal dynamic allocation;
- complete diagnostic visibility.

---

## Subsystems

### Boot Manager

Responsible for coordinating the complete boot sequence.

Responsibilities:

- firmware initialization;
- boot sequence orchestration;
- boot mode selection;
- kernel loading;
- BootInfo construction;
- execution handoff.

Features:

- deterministic startup;
- explicit boot stages;
- reproducible execution order;
- diagnostic reporting.

Dependencies:

- firmware services;
- Kernel Loader;
- BootInfo Builder.

---

### Kernel Loader

Responsible for loading the Just kernel executable into memory.

Responsibilities:

- executable validation;
- ELF parsing;
- segment loading;
- relocation preparation;
- entry point discovery.

Features:

- ELF validation;
- executable integrity verification;
- executable mapping.

Dependencies:

- Boot Manager;
- Memory Manager.

---

### BootInfo Builder

Creates the complete boot description consumed by the kernel.

Responsibilities:

- memory map creation;
- framebuffer description;
- firmware information;
- processor information;
- boot parameters.

Features:

- immutable BootInfo;
- deterministic layout;
- versioned structure.

Dependencies:

- Memory Discovery;
- Framebuffer Discovery.

---

### Firmware Interface

Provides a unified abstraction over firmware services.

Responsibilities:

- firmware calls;
- memory information;
- graphics initialization;
- boot services;
- runtime information.

Features:

- UEFI abstraction;
- platform isolation.

Dependencies:

- firmware implementation.

---

### Execution Handoff

Transfers execution from firmware to the Just kernel.

Responsibilities:

- disable boot services;
- finalize BootInfo;
- switch execution context;
- transfer kernel entry.

Features:

- one-way transition;
- deterministic ownership transfer.

Dependencies:

- Boot Manager;
- Kernel Loader.

---

## Folder Structure

```text
include/justos/
└── boot/
    ├── boot_manager.h
    ├── boot_info.h
    ├── boot_types.h
    ├── boot_errors.h
    ├── boot_stage.h
    ├── kernel_loader.h
    ├── firmware.h
    ├── framebuffer.h
    ├── memory_map.h
    └── handoff.h

src/
└── boot/
    ├── boot_manager.c
    ├── boot_info.c
    ├── kernel_loader.c
    ├── firmware.c
    ├── framebuffer.c
    ├── memory_map.c
    └── handoff.c
```

---

# Main Files

## Boot Manager

### include/justos/boot/boot_manager.h

Purpose:

Defines the public interface responsible for coordinating the complete boot process.

Structures:

```c
JPBootManager
JPBootContext
```

Enums:

```c
JPBootMode
JPBootStage
JPBootResult
```

Public API:

```c
jp_boot_initialize(...)
jp_boot_execute(...)
jp_boot_shutdown(...)
jp_boot_get_stage(...)
jp_boot_set_mode(...)
```

Dependencies:

- boot_info.h
- kernel_loader.h
- firmware.h

---

### src/boot/boot_manager.c

Purpose:

Implements the deterministic boot sequence and coordinates every boot subsystem.

Internal Functions:

```c
boot_detect_environment(...)
boot_prepare_context(...)
boot_execute_stage(...)
boot_validate_boot(...)
boot_finalize(...)
```

Algorithm:

- initialize firmware interface;
- detect execution environment;
- prepare boot context;
- execute every boot stage;
- construct BootInfo;
- transfer execution.

Dependencies:

- firmware.c
- kernel_loader.c
- boot_info.c

---

## Kernel Loader

### include/justos/boot/kernel_loader.h

Purpose:

Defines the public API for loading the Just kernel executable.

Structures:

```c
JPKernelImage
JPKernelSegment
```

Enums:

```c
JPKernelFormat
JPKernelStatus
```

Public API:

```c
jp_kernel_validate(...)
jp_kernel_load(...)
jp_kernel_map(...)
jp_kernel_entry(...)
```

Dependencies:

- memory_map.h

---

### src/boot/kernel_loader.c

Purpose:

Loads the kernel executable into executable memory.

Internal Functions:

```c
kernel_parse_elf(...)
kernel_map_segments(...)
kernel_validate_header(...)
kernel_find_entry(...)
```

Algorithm:

- validate executable;
- parse ELF headers;
- map executable segments;
- locate entry point;
- return executable context.

Dependencies:

- memory_map.c

---

## BootInfo Builder

### include/justos/boot/boot_info.h

Purpose:

Defines the BootInfo structure transferred to the kernel.

Structures:

```c
JPBootInfo
JPFramebufferInfo
JPMemoryRegion
JPBootModule
```

Enums:

```c
JPMemoryType
JPFramebufferFormat
```

Public API:

```c
jp_bootinfo_initialize(...)
jp_bootinfo_add_region(...)
jp_bootinfo_set_framebuffer(...)
jp_bootinfo_finalize(...)
```

Dependencies:

- framebuffer.h
- memory_map.h

---

### src/boot/boot_info.c

Purpose:

Constructs the complete BootInfo passed to the kernel.

Internal Functions:

```c
bootinfo_collect_memory(...)
bootinfo_collect_video(...)
bootinfo_collect_cpu(...)
bootinfo_validate(...)
```

Algorithm:

- enumerate memory;
- collect framebuffer information;
- collect firmware metadata;
- finalize immutable BootInfo.

Dependencies:

- firmware.c
- framebuffer.c
- memory_map.c

## Folder Structure

```text
include/justos/
└── boot
    ├── bootinfo.h
    ├── framebuffer.h
    ├── memory_map.h
    ├── kernel_loader.h
    ├── handoff.h
    └── boot_log.h

src/
└── boot
    ├── bootinfo.c
    ├── framebuffer.c
    ├── memory_map.c
    ├── kernel_loader.c
    ├── handoff.c
    └── boot_log.c
```

---

# Main Files

## Boot Information

### include/justos/boot/bootinfo.h

Purpose:

Defines the BootInfo structure transferred from the Boot Manager to the Just Kernel.

BootInfo represents the complete execution context prepared during system startup.
It becomes immutable after the kernel handoff and serves as the authoritative source of boot-time information.

Structures:

```c
JPBootInfo
JPBootHeader
JPBootVersion
```

Enums:

```c
JPBootMode
JPFirmwareType
JPBootFlags
```

Public API:

```c
jp_bootinfo_init(...)
jp_bootinfo_validate(...)
jp_bootinfo_finalize(...)
jp_bootinfo_get(...)
```

Dependencies:

- memory_map.h
- framebuffer.h

---

### src/boot/bootinfo.c

Purpose:

Constructs and validates the BootInfo instance before transferring control to the kernel.

Internal Functions:

```c
bootinfo_initialize(...)
bootinfo_collect(...)
bootinfo_verify(...)
bootinfo_finalize(...)
```

Algorithm:

- initialize BootInfo
- collect platform information
- validate mandatory fields
- calculate integrity values
- lock BootInfo
- expose immutable instance

Dependencies:

- memory_map.c
- framebuffer.c

---

## Memory Map

### include/justos/boot/memory_map.h

Purpose:

Defines the canonical representation of physical memory detected during boot.

The memory map provides the kernel with an accurate description of usable and reserved memory regions.

Structures:

```c
JPMemoryMap
JPMemoryRegion
```

Enums:

```c
JPMemoryType
```

Public API:

```c
jp_memory_map_collect(...)
jp_memory_map_validate(...)
jp_memory_map_get(...)
```

Dependencies:

- UEFI Memory Services

---

### src/boot/memory_map.c

Purpose:

Collects firmware memory descriptors and converts them into the internal engineering representation.

Internal Functions:

```c
memory_collect(...)
memory_convert(...)
memory_sort(...)
memory_validate(...)
```

Algorithm:

- retrieve firmware descriptors
- normalize memory regions
- merge adjacent compatible regions
- validate layout
- publish immutable memory map

Dependencies:

- UEFI Boot Services

---

## Framebuffer

### include/justos/boot/framebuffer.h

Purpose:

Defines the framebuffer information transferred to the kernel.

The framebuffer remains valid until the graphics subsystem takes ownership.

Structures:

```c
JPFramebuffer
```

Enums:

```c
JPPixelFormat
```

Public API:

```c
jp_framebuffer_initialize(...)
jp_framebuffer_validate(...)
jp_framebuffer_get(...)
```

Dependencies:

- GOP (Graphics Output Protocol)

---

### src/boot/framebuffer.c

Purpose:

Initializes the framebuffer using the firmware graphics interface.

Internal Functions:

```c
framebuffer_detect(...)
framebuffer_verify(...)
framebuffer_publish(...)
```

Algorithm:

- locate graphics protocol
- retrieve framebuffer parameters
- validate pixel format
- publish framebuffer information

Dependencies:

- UEFI Graphics Output Protocol

---

## Testing

### Unit

- [ ] BootInfo validation
- [ ] Memory map normalization
- [ ] Framebuffer validation

### Integration

- [ ] BootInfo transferred to kernel
- [ ] Memory map accessible after handoff
- [ ] Framebuffer initialized correctly

### Performance

- [ ] BootInfo generation latency
- [ ] Memory map conversion overhead
- [ ] Framebuffer initialization time

---

## Stage Status

Header

- [ ] bootinfo.h
- [ ] memory_map.h
- [ ] framebuffer.h

Source

- [ ] bootinfo.c
- [ ] memory_map.c
- [ ] framebuffer.c

Testing

- [ ] Unit Tests
- [ ] Integration Tests
- [ ] Performance Tests

---

## Expected Result

After completing this part of Stage 1 the Boot Manager is capable of:

- generating a complete BootInfo structure;
- discovering and validating the physical memory layout;
- initializing the framebuffer;
- exposing immutable boot-time information to the kernel.

The execution environment required for kernel initialization is now fully described and ready to be transferred during the Kernel Handoff.

---

## Notes

The BootInfo structure becomes one of the fundamental engineering interfaces of the Just system.

Every subsequent milestone depends on the guarantees established by this interface.

Changes to BootInfo require corresponding updates to every dependent subsystem and must remain backward-compatible whenever possible.

## Kernel Loader

### include/justos/boot/kernel_loader.h

Purpose:

Defines the public interface responsible for locating, validating and loading the Just kernel image into memory.

The Kernel Loader prepares the executable image but does not transfer execution.

Structures:

```c
JPKernelImage
JPKernelHeader
```

Enums:

```c
JPKernelFormat
JPKernelStatus
```

Public API:

```c
jp_kernel_loader_initialize(...)
jp_kernel_loader_load(...)
jp_kernel_loader_verify(...)
jp_kernel_loader_entry(...)
```

Dependencies:

- bootinfo.h
- memory_map.h

---

### src/boot/kernel_loader.c

Purpose:

Implements kernel discovery, ELF validation and image loading.

Internal Functions:

```c
kernel_find(...)
kernel_validate(...)
kernel_allocate(...)
kernel_load_segments(...)
kernel_prepare(...)
```

Algorithm:

- locate kernel image
- validate executable format
- allocate executable memory
- map executable segments
- resolve kernel entry point
- prepare execution context

Dependencies:

- bootinfo.c
- memory_map.c

---

## Kernel Handoff

### include/justos/boot/handoff.h

Purpose:

Defines the final transition from the Boot Manager to the Just Kernel.

The handoff represents the last executable stage of the boot process.

Structures:

```c
JPHandoffContext
```

Enums:

```c
JPHandoffStatus
```

Public API:

```c
jp_handoff_prepare(...)
jp_handoff_execute(...)
```

Dependencies:

- kernel_loader.h
- bootinfo.h

---

### src/boot/handoff.c

Purpose:

Transfers execution to the loaded kernel.

Internal Functions:

```c
handoff_validate(...)
handoff_disable_boot_services(...)
handoff_jump(...)
```

Algorithm:

- verify BootInfo integrity
- verify kernel entry point
- terminate firmware services
- finalize execution context
- transfer control to kernel

Dependencies:

- kernel_loader.c
- bootinfo.c

---

## Boot Logging

### include/justos/boot/boot_log.h

Purpose:

Defines the boot-time diagnostic logging interface.

Boot logging exists exclusively during early system startup and provides engineering diagnostics before the runtime logging infrastructure becomes available.

Structures:

```c
JPBootLogEntry
JPBootLogger
```

Enums:

```c
JPBootLogLevel
```

Public API:

```c
jp_boot_log_initialize(...)
jp_boot_log(...)
jp_boot_log_flush(...)
```

Dependencies:

None

---

### src/boot/boot_log.c

Purpose:

Implements the temporary logging system used during boot.

Internal Functions:

```c
boot_log_write(...)
boot_log_format(...)
boot_log_flush(...)
```

Algorithm:

- initialize log buffer
- append diagnostic messages
- preserve boot events
- flush log before kernel handoff

Dependencies:

None

---

## Testing

### Unit

- [ ] Kernel image validation
- [ ] ELF parser
- [ ] BootInfo verification
- [ ] Kernel entry calculation

### Integration

- [ ] Kernel successfully loaded
- [ ] Kernel Handoff completed
- [ ] BootInfo received by kernel
- [ ] Boot logging preserved

### Performance

- [ ] Kernel loading time
- [ ] ELF mapping overhead
- [ ] Boot handoff latency

---

## Stage Status

Headers

- [ ] boot_manager.h
- [ ] bootinfo.h
- [ ] memory_map.h
- [ ] framebuffer.h
- [ ] kernel_loader.h
- [ ] handoff.h
- [ ] boot_log.h

Sources

- [ ] boot_manager.c
- [ ] bootinfo.c
- [ ] memory_map.c
- [ ] framebuffer.c
- [ ] kernel_loader.c
- [ ] handoff.c
- [ ] boot_log.c

Testing

- [ ] Unit Tests
- [ ] Integration Tests
- [ ] Performance Tests

---

## Expected Result

After completing Stage 1 the Boot Manager is capable of:

- booting through supported firmware;
- discovering platform resources;
- constructing a complete BootInfo structure;
- validating the execution environment;
- loading the Just kernel;
- transferring execution to the kernel;
- providing early boot diagnostics.

Stage 1 establishes a deterministic and reproducible boot process upon which every subsequent milestone depends.

---

## Engineering Guarantees

Stage 1 guarantees that:

- BootInfo is complete, validated and immutable before handoff.
- The kernel image is fully verified before execution.
- Firmware services are terminated in a deterministic manner.
- The kernel always receives a consistent execution environment.
- Boot diagnostics remain available until runtime logging takes ownership.
- Every public interface defined in this stage is stable unless explicitly versioned.

---

## Notes

Stage 1 intentionally performs only platform-independent boot responsibilities.

Kernel initialization, memory management and runtime infrastructure begin in **Stage 2 — Platform Initialization**.
---

# Stage 2 — Platform Initialization

## Goal

Initialize the minimal execution environment required by the Just Kernel immediately after the Boot Manager transfers control.

This stage establishes the core runtime infrastructure by initializing the processor state, memory management, execution environment and kernel address space.

No compositor, services or user-space components are initialized during this stage.

The objective is to create a stable engineering platform upon which every higher-level subsystem will depend.

---

## Design Principles

- deterministic
- platform-independent
- fail-safe
- documentation-driven
- minimal runtime assumptions
- explicit ownership
- immutable boot information
- predictable initialization order

---

## Subsystems

### CPU Initialization

Responsible for configuring the processor immediately after kernel entry.

Responsibilities:

- verify processor capabilities;
- initialize architecture-specific state;
- configure descriptor tables;
- initialize interrupt support;
- prepare execution mode.

Features:

- CPU capability detection;
- descriptor initialization;
- feature validation;
- architecture abstraction.

Dependencies:

- Kernel Handoff
- BootInfo

---

### Early Memory

Responsible for creating the first kernel memory environment.

Responsibilities:

- initialize paging;
- initialize kernel stack;
- initialize early heap;
- validate physical memory;
- reserve kernel regions.

Features:

- early page tables;
- temporary allocator;
- stack initialization;
- memory validation.

Dependencies:

- Memory Map
- BootInfo

---

### ELF Loader

Responsible for preparing executable kernel sections.

Responsibilities:

- validate ELF image;
- map executable segments;
- apply permissions;
- prepare executable memory.

Features:

- executable mapping;
- section validation;
- alignment verification.

Dependencies:

- Early Memory

---

## Folder Structure

```text
include/justos/
└── kernel
    ├── cpu.h
    ├── paging.h
    ├── stack.h
    ├── heap.h
    ├── elf.h
    ├── memory.h
    └── kernel_init.h

src/
└── kernel
    ├── cpu.c
    ├── paging.c
    ├── stack.c
    ├── heap.c
    ├── elf.c
    ├── memory.c
    └── kernel_init.c
```

---

# Main Files

## CPU Initialization

### include/justos/kernel/cpu.h

Purpose:

Defines the public interface responsible for initializing processor-specific functionality.

This subsystem abstracts architecture-dependent initialization behind a stable engineering interface.

Structures:

```c
JPCPUInfo
JPCPUFeatures
```

Enums:

```c
JPCPUVendor
JPCPUState
```

Public API:

```c
jp_cpu_initialize(...)
jp_cpu_detect(...)
jp_cpu_validate(...)
jp_cpu_features(...)
```

Dependencies:

- bootinfo.h

---

### src/kernel/cpu.c

Purpose:

Implements processor initialization.

Internal Functions:

```c
cpu_detect_vendor(...)
cpu_detect_features(...)
cpu_initialize_tables(...)
cpu_validate(...)
```

Algorithm:

- identify processor
- detect supported features
- initialize descriptor tables
- validate required functionality
- publish processor information

Dependencies:

- bootinfo.c

---

## Paging

### include/justos/kernel/paging.h

Purpose:

Defines the paging subsystem responsible for establishing the initial virtual memory environment.

Structures:

```c
JPPageTable
JPVirtualRegion
```

Enums:

```c
JPPageFlags
```

Public API:

```c
jp_paging_initialize(...)
jp_paging_map(...)
jp_paging_unmap(...)
jp_paging_translate(...)
```

Dependencies:

- memory.h

### include/justos/kernel/stack.h

Purpose:

Defines the public interface responsible for initializing and managing the kernel stack during early system startup.

The kernel stack is established before any higher-level subsystem begins execution and remains available throughout kernel initialization.

Structures:

```c
JPKernelStack
```

Enums:

```c
JPStackState
```

Public API:

```c
jp_stack_initialize(...)
jp_stack_validate(...)
jp_stack_top(...)
jp_stack_size(...)
```

Dependencies:

- paging.h

---

### src/kernel/stack.c

Purpose:

Implements initialization and validation of the early kernel stack.

Internal Functions:

```c
stack_allocate(...)
stack_align(...)
stack_guard(...)
stack_validate(...)
```

Algorithm:

- reserve stack memory;
- align stack pointer;
- configure guard region;
- validate boundaries;
- publish stack context.

Dependencies:

- paging.c

---

## Early Heap

### include/justos/kernel/heap.h

Purpose:

Defines the temporary heap allocator used during kernel initialization before the main memory subsystem becomes available.

The early heap provides deterministic allocations required by kernel initialization only.

Structures:

```c
JPEarlyHeap
JPEarlyBlock
```

Enums:

```c
JPHeapState
```

Public API:

```c
jp_heap_initialize(...)
jp_heap_allocate(...)
jp_heap_reset(...)
jp_heap_available(...)
```

Dependencies:

- paging.h
- memory.h

---

### src/kernel/heap.c

Purpose:

Implements the temporary kernel heap allocator.

Internal Functions:

```c
heap_initialize(...)
heap_allocate(...)
heap_expand(...)
heap_validate(...)
```

Algorithm:

- initialize heap region;
- allocate sequential blocks;
- verify boundaries;
- report remaining capacity.

Dependencies:

- paging.c
- memory.c

---

## Physical Memory Validation

### include/justos/kernel/memory.h

Purpose:

Defines the public interface responsible for validating and describing physical memory provided by the Boot Manager.

The subsystem converts firmware memory information into a canonical engineering representation suitable for the kernel.

Structures:

```c
JPMemoryRegion
JPMemoryLayout
```

Enums:

```c
JPMemoryType
JPMemoryState
```

Public API:

```c
jp_memory_initialize(...)
jp_memory_validate(...)
jp_memory_layout(...)
jp_memory_available(...)
```

Dependencies:

- BootInfo

---

### src/kernel/memory.c

Purpose:

Implements validation and normalization of the physical memory map.

Internal Functions:

```c
memory_scan(...)
memory_merge(...)
memory_validate(...)
memory_reserve(...)
```

Algorithm:

- read BootInfo memory map;
- validate entries;
- merge adjacent compatible regions;
- reserve kernel memory;
- publish normalized layout.

Dependencies:

- bootinfo.c

### include/justos/kernel/elf.h

Purpose:

Defines the public interface responsible for loading, validating and mapping ELF64 kernel images into memory.

The subsystem provides a deterministic loader independent from firmware-specific implementations.

Structures:

```c
JPELFHeader
JPELFProgramHeader
JPELFImage
```

Enums:

```c
JPELFResult
JPELFSegmentType
```

Public API:

```c
jp_elf_validate(...)
jp_elf_load(...)
jp_elf_entry(...)
jp_elf_program_headers(...)
```

Dependencies:

- paging.h
- memory.h
- bootinfo.h

---

### src/kernel/elf.c

Purpose:

Implements validation, parsing and mapping of ELF64 executable images.

Internal Functions:

```c
elf_validate_header(...)
elf_validate_program_headers(...)
elf_map_segment(...)
elf_zero_bss(...)
elf_calculate_entry(...)
```

Algorithm:

- validate ELF header;
- validate program headers;
- map loadable segments;
- zero BSS sections;
- calculate kernel entry point;
- return executable image.

Dependencies:

- paging.c
- memory.c

---

## Kernel Initialization

### include/justos/kernel/init.h

Purpose:

Defines the public interface responsible for final kernel initialization immediately before transferring execution to the main kernel runtime.

This subsystem completes the transition from boot environment to the initialized Just kernel.

Structures:

```c
JPKernelContext
```

Enums:

```c
JPKernelState
```

Public API:

```c
jp_kernel_initialize(...)
jp_kernel_validate(...)
jp_kernel_enter(...)
```

Dependencies:

- paging.h
- heap.h
- memory.h
- elf.h

---

### src/kernel/init.c

Purpose:

Coordinates initialization of all early kernel subsystems.

Internal Functions:

```c
kernel_initialize_memory(...)
kernel_initialize_heap(...)
kernel_initialize_runtime(...)
kernel_validate_environment(...)
kernel_enter_main(...)
```

Algorithm:

- validate BootInfo;
- initialize paging;
- initialize stack;
- initialize early heap;
- validate physical memory;
- initialize runtime context;
- transfer execution to the kernel entry point.

Dependencies:

- paging.c
- stack.c
- heap.c
- memory.c
- elf.c

---

## Folder Structure

```text
include/justos/kernel/
├── paging.h
├── stack.h
├── heap.h
├── memory.h
├── elf.h
└── init.h

src/kernel/
├── paging.c
├── stack.c
├── heap.c
├── memory.c
├── elf.c
└── init.c
```

---

## Testing

### Unit

[ ] Page table construction

[ ] Page mapping

[ ] Stack initialization

[ ] Early heap allocation

[ ] Memory layout validation

[ ] ELF header validation

[ ] ELF segment mapping

[ ] Kernel initialization

---

### Integration

[ ] BootInfo consumption

[ ] Identity of memory map

[ ] Kernel entry execution

[ ] Complete initialization sequence

---

### Performance

[ ] Paging initialization latency

[ ] Heap allocation latency

[ ] ELF loading time

[ ] Total kernel initialization time

---

## Stage Status

### Headers

[ ] paging.h

[ ] stack.h

[ ] heap.h

[ ] memory.h

[ ] elf.h

[ ] init.h

---

### Sources

[ ] paging.c

[ ] stack.c

[ ] heap.c

[ ] memory.c

[ ] elf.c

[ ] init.c

---

### Tests

[ ] Unit Tests

[ ] Integration Tests

[ ] Performance Tests

---

## Engineering Guarantees

After completing this stage:

- the execution environment is fully initialized;
- virtual memory is operational;
- the kernel stack is active;
- deterministic early allocations are available;
- physical memory is validated;
- the ELF kernel image is successfully mapped;
- the kernel runtime receives a complete execution context;
- no firmware-specific services remain required after initialization.

All kernel initialization remains deterministic and reproducible.

---

## Expected Result

After completing Stage 2 the engineering system is capable of:

- executing the Just kernel in its own runtime environment;
- managing virtual memory;
- allocating temporary initialization memory;
- validating available physical memory;
- loading executable kernel images;
- transferring execution to the initialized kernel runtime.

The system is fully prepared to leave the boot environment.

---

## Notes

Stage 2 establishes the execution environment required by every subsequent milestone.

No compositor logic, desktop services or user-space components are initialized during this stage.

Only the minimum deterministic runtime required to execute the Just kernel is provided.

---

# Stage 3 — Boot Diagnostics

## Goal

Create a deterministic diagnostics infrastructure responsible for observing, reporting and recovering from failures during the boot process.

This stage introduces standardized boot logging, status reporting, error handling, panic management and recovery entry mechanisms.

Its purpose is to make every stage of system startup observable, reproducible and diagnosable without introducing platform-specific dependencies.

---

## Design Principles

The diagnostics infrastructure must remain:

- deterministic;
- fail-safe;
- modular;
- backend-independent;
- platform-independent;
- reproducible;
- observable;
- documentation-driven;
- zero hidden state.

Every diagnostic event must be explainable and reproducible.

---

## Subsystems

### Boot Logger

Provides centralized logging for the entire boot sequence.

Responsibilities:

- boot event logging;
- warnings;
- errors;
- informational messages;
- timestamp generation;
- diagnostic output.

Features:

- deterministic ordering;
- multiple log levels;
- optional serial output;
- optional framebuffer output;
- boot timeline generation.

Dependencies:

- Boot Status
- Error Reporting

---

### Boot Status

Tracks current boot progress.

Responsibilities:

- initialization state;
- current subsystem;
- completed stages;
- boot progress;
- boot result.

Features:

- deterministic state transitions;
- centralized boot status;
- progress reporting.

Dependencies:

- Boot Logger

---

### Error Reporting

Provides standardized engineering errors.

Responsibilities:

- error creation;
- error propagation;
- error classification;
- diagnostic information.

Features:

- immutable error objects;
- deterministic error codes;
- subsystem identification.

Dependencies:

- Boot Logger

---

### Panic Handler

Terminates boot execution when recovery is impossible.

Responsibilities:

- panic screen;
- diagnostic dump;
- halt;
- recovery recommendation.

Features:

- panic context;
- subsystem identification;
- reproducible panic output.

Dependencies:

- Error Reporting
- Boot Logger

---

### Recovery Entry

Determines whether recovery mode can be entered.

Responsibilities:

- recovery detection;
- recovery validation;
- recovery handoff.

Features:

- deterministic recovery entry;
- future compatibility with Recovery Environment.

Dependencies:

- Panic Handler

---

## Folder Structure

```text
include/justos/boot/
├── boot_log.h
├── boot_status.h
├── error.h
├── panic.h
└── recovery.h

src/boot/
├── boot_log.c
├── boot_status.c
├── error.c
├── panic.c
└── recovery.c
```

---

# Main Files

## Boot Logger

### include/justos/boot/boot_log.h

**Purpose:**

Defines the public interface responsible for deterministic logging during the boot process.

The Boot Logger provides a centralized mechanism for recording initialization progress, warnings, errors and diagnostic information before the kernel runtime becomes fully operational.

---

**Structures:**

```c
JPBootLogEntry
JPBootLogContext
```

---

**Enums:**

```c
JPBootLogLevel
JPBootLogOutput
```

---

**Public API:**

```c
jp_boot_log_initialize(...)
jp_boot_log(...)
jp_boot_log_info(...)
jp_boot_log_warning(...)
jp_boot_log_error(...)
jp_boot_log_shutdown(...)
```

---

**Dependencies:**

- boot_status.h
- error.h

---

### src/boot/boot_log.c

**Purpose:**

Implements the deterministic boot logging subsystem.

Responsible for formatting, buffering and publishing boot log entries.

---

**Internal Functions:**

```c
boot_log_write(...)
boot_log_format(...)
boot_log_timestamp(...)
boot_log_output(...)
boot_log_flush(...)
```

---

**Algorithm:**

- initialize logging context;
- receive diagnostic event;
- generate timestamp;
- classify log level;
- create log entry;
- dispatch output;
- flush buffered entries.

---

**Dependencies:**

- boot_status.c
- error.c

---

## Boot Status

### include/justos/boot/boot_status.h

**Purpose:**

Defines the engineering interface representing the current state of the boot pipeline.

Every subsystem participating in the boot sequence reports its progress through the Boot Status subsystem, providing a single authoritative view of system initialization.

---

**Structures:**

```c
JPBootStatus
JPBootStage
```

---

**Enums:**

```c
JPBootState
JPBootResult
```

---

**Public API:**

```c
jp_boot_status_initialize(...)
jp_boot_status_set(...)
jp_boot_status_current(...)
jp_boot_status_completed(...)
jp_boot_status_result(...)
```

---

**Dependencies:**

- boot_log.h

---

### src/boot/boot_status.c

**Purpose:**

Implements deterministic boot state tracking.

Maintains the current execution stage, validates state transitions and records the final boot result.

---

**Internal Functions:**

```c
boot_status_update(...)
boot_status_validate(...)
boot_status_publish(...)
boot_status_complete(...)
```

---

**Algorithm:**

- initialize boot state;
- validate requested transition;
- update current stage;
- publish status changes;
- notify Boot Logger;
- finalize boot result.

---

**Dependencies:**

- boot_log.c

---

## Error Reporting

### include/justos/boot/error.h

**Purpose:**

Defines the engineering error model used throughout the boot process.

The Error Reporting subsystem provides standardized error objects, deterministic error codes and subsystem identification, allowing every boot component to communicate failures in a consistent and traceable manner.

---

**Structures:**

```c
JPBootError
JPBootErrorContext
```

---

**Enums:**

```c
JPBootErrorCode
JPBootErrorSeverity
JPBootSubsystem
```

---

**Public API:**

```c
jp_error_initialize(...)
jp_error_create(...)
jp_error_report(...)
jp_error_last(...)
jp_error_clear(...)
```

---

**Dependencies:**

- boot_log.h

---

### src/boot/error.c

**Purpose:**

Implements deterministic engineering error reporting.

Responsible for constructing immutable error objects, assigning standardized error codes and forwarding diagnostic information to the Boot Logger.

---

**Internal Functions:**

```c
error_create(...)
error_validate(...)
error_publish(...)
error_store(...)
```

---

**Algorithm:**

- receive failure information;
- classify subsystem;
- assign deterministic error code;
- create immutable error object;
- publish diagnostic event;
- store last reported error.

---

**Dependencies:**

- boot_log.c

---

## Panic Handler

### include/justos/boot/panic.h

**Purpose:**

Defines the public interface responsible for terminating the boot sequence when a non-recoverable engineering error occurs.

The Panic Handler guarantees deterministic system shutdown while preserving diagnostic information required for debugging and recovery.

---

**Structures:**

```c
JPPanicContext
```

---

**Enums:**

```c
JPPanicReason
```

---

**Public API:**

```c
jp_panic(...)
jp_panic_with_code(...)
jp_panic_dump(...)
jp_panic_halt(...)
```

---

**Dependencies:**

- error.h
- boot_log.h

---

### src/boot/panic.c

**Purpose:**

Implements deterministic panic handling during the boot process.

Collects diagnostic information, generates panic output and safely halts execution.

---

**Internal Functions:**

```c
panic_collect_context(...)
panic_dump_registers(...)
panic_print(...)
panic_halt(...)
```

---

**Algorithm:**

- stop boot execution;
- collect execution context;
- gather latest boot status;
- retrieve latest engineering error;
- generate panic diagnostics;
- flush pending logs;
- halt processor.

---

**Dependencies:**

- error.c
- boot_log.c
- boot_status.c

---

## Recovery Entry

### include/justos/boot/recovery.h

**Purpose:**

Defines the public interface responsible for entering recovery mode when the normal boot sequence cannot safely continue.

The Recovery Entry subsystem determines whether recovery is available, validates recovery conditions and prepares the transition to a recovery environment without introducing platform-specific logic.

---

**Structures:**

```c
JPRecoveryContext
JPRecoveryEntry
```

---

**Enums:**

```c
JPRecoveryMode
JPRecoveryReason
JPRecoveryResult
```

---

**Public API:**

```c
jp_recovery_initialize(...)
jp_recovery_available(...)
jp_recovery_enter(...)
jp_recovery_reason(...)
jp_recovery_reset(...)
```

---

**Dependencies:**

- panic.h
- error.h
- boot_status.h

---

### src/boot/recovery.c

**Purpose:**

Implements deterministic recovery entry during the boot process.

Responsible for validating recovery conditions, preserving diagnostic context and transferring execution to the configured recovery target.

---

**Internal Functions:**

```c
recovery_validate(...)
recovery_prepare(...)
recovery_collect_context(...)
recovery_transfer(...)
recovery_reset(...)
```

---

**Algorithm:**

- receive recovery request;
- validate recovery conditions;
- collect diagnostic information;
- preserve boot state;
- prepare recovery context;
- transfer execution to the recovery environment;
- return recovery result.

---

**Dependencies:**

- panic.c
- error.c
- boot_status.c
- boot_log.c

---

## Folder Overview

```text
include/justos/boot/
├── boot_log.h
├── boot_status.h
├── error.h
├── panic.h
└── recovery.h

src/boot/
├── boot_log.c
├── boot_status.c
├── error.c
├── panic.c
└── recovery.c
```

---

# Testing

## Unit

- [ ] Boot Logger initialization
- [ ] Boot Logger output formatting
- [ ] Boot Status transitions
- [ ] Error object creation
- [ ] Error code validation
- [ ] Panic context generation
- [ ] Panic halt sequence
- [ ] Recovery availability detection
- [ ] Recovery context creation
- [ ] Recovery entry validation

---

## Integration

- [ ] Complete boot diagnostics pipeline
- [ ] Boot Logger ↔ Boot Status communication
- [ ] Error Reporting ↔ Panic Handler integration
- [ ] Panic Handler ↔ Recovery Entry transition
- [ ] Recovery Entry ↔ Boot Status synchronization
- [ ] Boot diagnostics under simulated failures
- [ ] Deterministic diagnostics output across multiple boots

---

## Performance

- [ ] Boot diagnostics latency
- [ ] Logging overhead measurement
- [ ] Panic execution latency
- [ ] Recovery entry latency
- [ ] Zero memory leaks
- [ ] Deterministic execution order verification

---

# Stage Status

## Boot Logger

- [ ] Header
- [ ] Source
- [ ] Unit Tests
- [ ] Integration Tests

---

## Boot Status

- [ ] Header
- [ ] Source
- [ ] Unit Tests
- [ ] Integration Tests

---

## Error Reporting

- [ ] Header
- [ ] Source
- [ ] Unit Tests
- [ ] Integration Tests

---

## Panic Handler

- [ ] Header
- [ ] Source
- [ ] Unit Tests
- [ ] Integration Tests

---

## Recovery Entry

- [ ] Header
- [ ] Source
- [ ] Unit Tests
- [ ] Integration Tests

---

# Engineering Guarantees

After completing this stage, the boot diagnostics subsystem guarantees:

- deterministic diagnostic output;
- centralized boot logging;
- reproducible error reporting;
- standardized engineering error objects;
- deterministic panic handling;
- reproducible panic diagnostics;
- deterministic recovery entry;
- observable boot progress;
- explicit subsystem identification;
- traceable engineering failures.

All diagnostic information produced during system startup becomes reproducible and explainable.

Every engineering failure can be traced back to its originating subsystem without relying on hidden implementation details.

---

# Expected Result

After completing **Stage 3**, the engineering system is capable of:

- recording every stage of the boot process;
- tracking deterministic boot progress;
- generating standardized engineering errors;
- producing reproducible panic diagnostics;
- entering recovery mode when boot continuation is unsafe;
- preserving diagnostic context during failures;
- providing a complete engineering trace of system initialization.

Together with **Stage 1** and **Stage 2**, this stage completes the deterministic boot infrastructure required before the kernel begins normal execution.

Milestone 0 now provides a complete boot pipeline consisting of:

```text
Firmware
      │
      ▼
Boot Manager
      │
      ▼
Platform Initialization
      │
      ▼
Boot Diagnostics
      │
      ▼
Kernel Entry
```

---

# Notes

This stage intentionally focuses on engineering diagnostics during the boot process.

It does **not** introduce runtime logging, kernel diagnostics, security auditing, crash reporting or user-space recovery mechanisms.

Those capabilities belong to later milestones, where the operating system is fully initialized.

The diagnostics implemented here are limited to the boot environment and terminate once control is transferred to the kernel.

---
# Security Integration

The following security components become available after completing this milestone.

## Boot Security

Establishes the initial trust boundary before the operating system begins execution.

Provides integrity validation for the early boot process.

---

## Boot Manager

Coordinates the secure startup pipeline and transfers validated execution context to the kernel.

---

## Kernel Verification

Validates the integrity of the kernel image before execution.

---

## Boot Integrity

Ensures that the boot sequence has not been modified before the kernel receives control.

---

## BootInfo Validation

Verifies that the BootInfo structure is complete, internally consistent and safe to consume.

---

## Future Extensions

The following security capabilities are intentionally excluded:

- Secure Boot
- TPM Integration
- Measured Boot

---

# Checklist

## Documentation

- [ ] Milestone specification completed
- [ ] All Stage specifications completed
- [ ] Folder structure defined
- [ ] Public APIs specified
- [ ] Internal APIs specified
- [ ] Engineering algorithms documented
- [ ] Testing strategy documented

---

## Stage Completion

- [ ] Stage 1 — Boot Manager
- [ ] Stage 2 — Platform Initialization
- [ ] Stage 3 — Boot Diagnostics

---

## Engineering Deliverables

- [ ] Boot Manager
- [ ] UEFI Boot Support
- [ ] Kernel Loader
- [ ] BootInfo
- [ ] Memory Map
- [ ] Framebuffer Initialization
- [ ] Kernel Handoff
- [ ] CPU Initialization
- [ ] Early Paging
- [ ] Early Stack
- [ ] Early Heap
- [ ] Memory Validation
- [ ] ELF Loader
- [ ] Boot Logger
- [ ] Boot Status
- [ ] Error Reporting
- [ ] Panic Handler
- [ ] Recovery Entry

---

# Progress

**Status**

Planned

**Completion**

0%

**Engineering Readiness**

Ready for implementation.

All engineering components, subsystem boundaries, public interfaces and implementation responsibilities have been defined.

---

# Expected Result

After completing Milestone 0, the Just engineering system is capable of performing a complete deterministic system startup.

The boot process is able to:

- boot through supported firmware interfaces;
- initialize the execution environment;
- load the Just kernel;
- validate executable images;
- establish virtual memory;
- initialize the early runtime;
- collect hardware information;
- generate BootInfo;
- provide framebuffer information;
- transfer execution to the initialized kernel;
- diagnose initialization failures;
- enter Recovery Mode when required.

At this point the system becomes **bootable**.

No desktop environment, compositor, window management or user-space services are implemented yet.

Milestone 0 establishes only the engineering foundation required for all subsequent milestones.

The engineering startup pipeline is fully defined.

```text
Firmware
      │
      ▼
Boot Manager
      │
      ▼
Platform Initialization
      │
      ▼
Boot Diagnostics
      │
      ▼
Kernel Entry
      │
      ▼
Milestone 1
```

---

# Future Extensions

The following capabilities are intentionally excluded from this milestone and will be introduced in later stages of the project:

- Live Boot support;
- Distribution installer;
- Secure Boot integration;
- TPM integration;
- Measured Boot;
- Remote boot diagnostics;
- Boot performance profiling;
- Kernel crash reporting;
- Boot animation;
- Multi-boot management.

These extensions build upon the deterministic boot infrastructure established by Milestone 0 without modifying its architectural principles.

---

# Milestone Completion

Milestone 0 establishes the first executable component of the Just engineering system.

It provides a complete, deterministic and observable startup pipeline that prepares the execution environment before handing control to the kernel.

Every subsequent milestone assumes the guarantees established by this milestone.

Without Milestone 0, no higher-level engineering component can exist.