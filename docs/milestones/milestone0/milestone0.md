# Milestone 0 — Boot Infrastructure

> **Status:** Planned
> **Priority:** Critical
> **Version:** 1.0

---

# Table of Contents

- [Overview](#overview)
- [Goals](#goals)
- [Dependencies](#dependencies)
- [Deliverables](#deliverables)
- [Not Included](#not-included)
- [Stage 1 — Boot Pipeline Foundation](#stage-1--boot-pipeline-foundation)
  - [Stage Definition](#1-stage-definition)
  - [Specification Lock](#1-specification-lock)
  - [Architecture Target](#1-architecture-target)
  - [Platform Target](#1-platform-target)
  - [Stage 1 Responsibilities](#stage-1-responsibilities)
  - [Stage 1 Non-Goals](#stage-1-non-goals)
  - [Folder Structure](#folder-structure)
  - [File Specifications](#file-specifications)
  - [Stage 1 Execution Order](#stage-1-execution-order)
  - [Stage 1 Architectural Rules](#stage-1-architectural-rules)
  - [Stage 1 → Stage 2 Boundary](#stage-1--stage-2-boundary-1)
    - [BootInfo Lifecycle](#bootinfo-lifecycle)
    - [Minimal Boot Security Scope](#minimal-boot-security-scope)
  - [Specification Lock Criteria](#1-specification-lock-criteria)
  - [Specification Traceability](#1-specification-traceability)
  - [Specification Completeness Verification](#1-specification-completeness-verification)
  - [Specification Lock Decision](#1-specification-lock-decision)
  - [Implementation Acceptance](#1-implementation-acceptance)
  - [Final Lock Statement](#1-final-lock-statement)

- [Stage 2 — Platform Initialization](#2-stage-2--platform-initialization)
  - [Stage Definition](#2-stage-2-definition)
  - [Specification Lock](#2-specification-lock)
  - [Architecture Target](#2-architecture-target)
    - [Architecture-Specific Stage 2 Contract](#architecture-specific-stage-2-contract)
    - [Stage 2 Architecture Contract Matrix](#stage-2-architecture-contract-matrix)
  - [Platform Target](#2-platform-target)
  - [Stage 2 Responsibilities](#2-stage-2-responsibilities)
    - [Stage 2 Physical-Memory Ownership Boundary](#stage-2-physical-memory-ownership-boundary)
  - [Stage 2 Non-Goals](#2-stage-2-non-goals)
  - [Folder Structure](#2-folder-structure)
  - [File Specifications](#file-specifications-1)
  - [Stage 2 Execution Order](#stage-2-execution-order)
  - [Stage 2 Architectural Rules](#stage-2-architectural-rules)
  - [Stage 2 → Stage 3 Boundary](#stage-2--stage-3-boundary)
  - [Specification Lock Criteria](#2-specification-lock-criteria)
  - [Specification Traceability](#2-specification-traceability)
  - [Specification Completeness Verification](#2-specification-completeness-verification)
  - [Specification Lock Decision](#2-specification-lock-decision)
  - [Implementation Acceptance](#2-implementation-acceptance)
  - [Final Lock Statement](#2-final-lock-statement)

- [Stage 3 — Boot Diagnostics & Recovery](#stage-3--boot-diagnostics--recovery)
  - [Stage 3 Definition](#stage-3-definition)
  - [Specification Lock](#3-specification-lock)
  - [Execution Context](#3-execution-context)
  - [Diagnostic Architecture](#3-diagnostic-architecture)
  - [Boot Attempt and Event Identity](#3-boot-attempt-and-event-identity)
  - [Boot Status Contract](#3-boot-status-contract)
  - [Boot Log Contract](#3-boot-log-contract)
  - [Error Classification Contract](#3-error-classification-contract)
  - [Diagnostic Output Contract](#3-diagnostic-output-contract)
  - [Panic Lifecycle](#3-panic-lifecycle)
  - [Retry Contract](#3-retry-contract)
  - [Architecture Target](#3-architecture-target)
  - [Stage 3 Responsibilities](#3-stage-3-responsibilities)
  - [Stage 3 Non-Goals](#3-stage-3-non-goals)
  - [Folder Structure](#3-folder-structure)
  - [Stage 3 Common Contract](#3-stage-3-common-contract)
  - [Stage 3 Field and Value Semantics](#3-stage-3-field-and-value-semantics)
  - [Stage 3 Algorithms and State Rules](#3-stage-3-algorithms-and-state-rules)
  - [Retry Reset and Preservation Contract](#3-retry-reset-and-preservation-contract)
  - [Architecture Verification Matrix](#3-architecture-verification-matrix)
  - [Stage 3 Verification Matrix](#3-stage-3-verification-matrix)
  - [Boot Event Contract](#3-boot-event-contract)
  - [Error Contract](#3-error-contract)
  - [Panic and Diagnostic Presentation Contract](#3-panic-and-diagnostic-presentation-contract)
  - [Recovery Contract](#3-recovery-contract)
  - [Recovery Transition Boundary](#3-recovery-transition-boundary)
  - [Ownership and Lifetime](#3-ownership-and-lifetime)
  - [Stage 3 Execution Flow](#3-stage-3-execution-flow)
  - [Boot Report Contract](#3-boot-report-contract)
  - [Diagnostic Finalization Contract](#3-diagnostic-finalization-contract)
  - [File Specifications](#3-file-specifications)
  - [Stage 3 → Stage 4 Boundary](#3-stage-3--stage-4-boundary)
  - [Specification Lock Criteria](#3-specification-lock-criteria)
  - [Specification Traceability](#3-specification-traceability)
  - [Specification Completeness Verification](#3-specification-completeness-verification)
  - [Specification Lock Decision](#3-specification-lock-decision)
  - [Implementation Acceptance](#3-implementation-acceptance)
  - [Final Lock Statement](#3-final-lock-statement)

- [Stage 4 — Boot manager & UI](#stage-4--boot-manager--ui)
  - [Stage 4 Definition](#stage-4-definition)
  - [Specification Lock](#4-specification-lock)
  - [Architecture Target](#4-architecture-target)
  - [Execution Context](#4-execution-context)
  - [Stage 4 Responsibilities](#4-stage-4-responsibilities)
  - [Stage 4 Non-Goals](#4-stage-4-non-goals)
  - [Folder Structure](#4-folder-structure)
  - [Stage 4 Common Contract](#4-stage-4-common-contract)
  - [Stage 4 Area of JPBootContext](#4-stage-4-area-of-jpbootcontext)
  - [Boot Configuration Contract](#4-boot-configuration-contract)
  - [Selection Outcome and Manager State](#4-selection-outcome-and-manager-state)
  - [Selection Policy](#4-selection-policy)
  - [Boot UI Contract](#4-boot-ui-contract)
  - [Boot Information Aggregation Contract](#4-boot-information-aggregation-contract)
  - [Boot Manager Decision Contract](#4-boot-manager-decision-contract)
  - [Recovery Transition Owner Contract](#4-recovery-transition-owner-contract)
  - [Freeze and Kernel Handoff Contract](#4-freeze-and-kernel-handoff-contract)
  - [Stage 4 Algorithms and State Rules](#4-stage-4-algorithms-and-state-rules)
  - [Ownership and Lifetime](#4-ownership-and-lifetime)
  - [Stage 4 Execution Flow](#4-stage-4-execution-flow)
  - [Stage 4 → Kernel Boundary](#4-stage-4--kernel-boundary)
  - [Stage 4 Verification Matrix](#4-stage-4-verification-matrix)
  - [File Specifications](#4-file-specifications)
  - [Specification Lock Criteria](#4-specification-lock-criteria)
  - [Specification Traceability](#4-specification-traceability)
  - [Specification Completeness Verification](#4-specification-completeness-verification)
  - [Specification Lock Decision](#4-specification-lock-decision)
  - [Implementation Acceptance](#4-implementation-acceptance)
  - [Final Lock Statement](#4-final-lock-statement)


- [Test Kernel](#test-kernel)
- [Security Integration](#security-integration)
- [Checklist](#checklist)
- [Progress](#progress)
- [Expected Result](#expected-result)
- [Future Extensions](#future-extensions)

---

# Overview

Milestone 0 establishes the complete boot infrastructure of the Just engineering system.

Its purpose is to create a deterministic execution environment, discover and validate the platform, acquire and validate the Just kernel, prepare the minimum kernel execution environment, and establish the contract required to transfer control from the boot infrastructure into the kernel.

Milestone 0 is strictly limited to boot infrastructure.

It does not implement the desktop environment, desktop services, JSL, applications, or higher-level operating-system functionality.

The milestone establishes the first executable foundation upon which all later milestones depend.

The complete Milestone 0 pipeline is conceptually:

```text
Power On
    │
    ▼
Firmware / Boot Environment
    │
    ▼
Boot Entry
    │
    ▼
Stage 1
Boot Pipeline Foundation
    │
    ▼
Later Milestone 0 Stages
    │
    ▼
Kernel Runtime
```

Stage 1 is the first normative execution boundary of Milestone 0.

Stage 1 is responsible for transforming the active platform environment into a validated and prepared kernel execution environment.

Later Milestone 0 stages may consume the contracts established by Stage 1, but they must not redefine Stage 1 behavior or silently repair unspecified Stage 1 state.

---

# Goals

Milestone 0 shall provide:

- firmware entry;
- firmware abstraction;
- boot-environment detection;
- hardware discovery;
- CPU discovery;
- boot-device discovery;
- storage discovery required for boot;
- firmware information;
- physical memory discovery;
- memory-map validation;
- framebuffer discovery;
- framebuffer initialization;
- kernel image discovery;
- ELF kernel validation;
- kernel segment loading;
- kernel execution preparation;
- early CPU state required for kernel entry;
- early paging required for kernel entry;
- early kernel stack required for kernel entry;
- early memory state required for boot-to-kernel transition where explicitly specified;
- boot diagnostics;
- boot status handling;
- standardized boot errors;
- panic handling;
- recovery entry;
- canonical boot-information infrastructure;
- Boot Manager orchestration;
- Boot Manager UI;
- Boot Log;
- kernel handoff;
- kernel entry stub;
- final Boot Report.

Milestone-level goals are not automatically Stage 1 responsibilities.

Each goal must be assigned to the appropriate Stage by the normative Stage definitions.

In particular:

- Stage 1 owns only the early CPU state required by its handoff contract;
- Stage 1 owns only the paging state required for valid kernel entry;
- Stage 1 owns only the kernel stack required for valid kernel entry;
- Stage 1 does not own the complete kernel CPU subsystem;
- Stage 1 does not own the complete kernel virtual-memory subsystem;
- Stage 1 does not own the kernel runtime stack subsystem;
- Stage 1 does not own the kernel runtime heap;
- Stage 1 does not define the final `JPBootInfo` contract.

Every component must be defined by engineering documentation before implementation.

Every dependency must exist as both documented engineering specification and source implementation before the dependent component begins implementation.

---

# Dependencies

Milestone 0 depends only on the platform and firmware interfaces required to enter and initialize the Just execution environment.

External dependencies include:

- ISO C17;
- CMake;
- GCC and/or LLVM toolchain;
- x86_64 execution environment;
- AArch64 execution environment;
- UEFI firmware;
- UEFI boot services;
- UEFI memory-map services;
- UEFI Graphics Output Protocol;
- executable kernel image;
- boot device required to locate the kernel;
- BIOS platform interfaces required by the BIOS provider.

The Milestone 0 target architectures are:

```text
x86_64:  REQUIRED
AArch64: REQUIRED
```

Both architectures are part of the Milestone 0 contract and implementation acceptance. A future architecture extension is required only for architectures other than x86_64 and AArch64.

Supported Stage 1 platform targets are:

```text
x86_64 / UEFI:  SUPPORTED
x86_64 / BIOS:  SUPPORTED
AArch64 / UEFI: SUPPORTED
AArch64 / BIOS:  NOT SUPPORTED
```

AArch64 must never enter a BIOS execution path. Every supported architecture/provider combination requires a complete normative contract and functional implementation.

A supported provider must never be represented by a placeholder, simulation, or fake successful state.

Milestone 0 must not depend on:

- Milestone 1 implementation;
- compositor;
- desktop services;
- JSL runtime;
- applications;
- desktop security services;
- AI services;
- package management;
- distribution infrastructure.

---

# Deliverables

Milestone 0 shall deliver:

1. Firmware abstraction.
2. UEFI provider.
3. BIOS provider.
4. Hardware discovery.
5. Memory discovery and validation.
6. Framebuffer discovery and initialization.
7. Kernel image discovery.
8. ELF validation.
9. Kernel loading.
10. Segment mapping.
11. Early CPU initialization.
12. Early virtual-memory initialization.
13. Kernel stack initialization.
14. Early heap or boot-time allocation support where explicitly assigned.
15. Boot diagnostics.
16. Boot status handling.
17. Boot error handling.
18. Panic handling.
19. Recovery infrastructure.
20. Boot-information infrastructure.
21. Boot Manager.
22. Boot Manager UI.
23. Boot Log.
24. Kernel handoff.
25. Kernel entry stub.
26. Boot Report.

All deliverables must have documented interfaces and implementation responsibilities.

---

# Not Included

Milestone 0 explicitly excludes:

- compositor;
- window manager;
- desktop shell;
- desktop services;
- networking services;
- Bluetooth services;
- audio services;
- storage services beyond boot acquisition;
- display services beyond boot framebuffer requirements;
- JSL runtime;
- application runtime;
- desktop security UI;
- AI services;
- package management;
- distribution infrastructure.

The Boot Manager UI is not a desktop environment.

It exists only for boot configuration, boot information, boot diagnostics, and boot-related interaction.

---

# Stage 1 — Boot Pipeline Foundation

## 1 Stage Definition

Stage 1 establishes the low-level boot pipeline required to transform the active boot environment into a validated and prepared kernel execution environment.

Stage 1 is the normative boundary between platform-specific boot mechanisms and the prepared kernel execution environment.

Stage 1 does not attempt to implement the complete kernel runtime.

---

## Goal

Stage 1 defines and implements the complete low-level boot pipeline required to prepare JustOS for transition into the kernel runtime environment.

Stage 1 is responsible for:

- identifying the active boot environment;
- initializing the selected platform provider;
- discovering required hardware information;
- acquiring and normalizing the system memory map;
- validating normalized memory information;
- acquiring and validating the framebuffer;
- acquiring the kernel image;
- parsing and validating the ELF64 kernel image;
- discovering and validating `PT_LOAD` segments;
- validating the kernel entry point;
- allocating or reserving memory required by the kernel execution environment;
- loading kernel segment contents;
- zero-initializing the uninitialized portion of segments;
- establishing required kernel memory mappings;
- establishing required segment permissions;
- establishing the minimum paging state required for kernel entry;
- establishing the kernel stack required for kernel entry;
- establishing the CPU execution state required by the handoff contract;
- preparing the platform handoff;
- validating all handoff prerequisites;
- establishing the Stage 1 → Stage 2 execution contract.

Stage 1 does **not** define `JPBootInfo`.

`JPBootInfo` is outside the Stage 1 contract and must not be required by Stage 1 implementation.

Stage 1 must not report successful states unless the corresponding operation has actually been performed successfully.

---

# 1 Specification Lock

Stage 1 may be considered **Specification Locked** only when all requirements below are explicitly defined, internally consistent, and independently verifiable.

---

## 1 Architecture Target

- The supported architectures are **x86_64** and **AArch64**.
- `EM_X86_64` is required for x86_64 images.
- `EM_AARCH64` is required for AArch64 images.
- ELF64 is required for both architectures.
- The selected ELF machine type must match the detected Stage 1 architecture.
- Architectures other than x86_64 and AArch64 require a separate approved architecture extension.

---

## 1 Platform Target

Stage 1 explicitly defines:

```text
x86_64 / UEFI:  SUPPORTED
x86_64 / BIOS:  SUPPORTED
AArch64 / UEFI: SUPPORTED
AArch64 / BIOS:  NOT SUPPORTED
```

Supported combinations are Stage 1 implementation targets, and both architecture implementations are required for Milestone 0 completion.

No supported provider may be represented by a placeholder.

Every supported provider must expose the normalized Stage 1 contract required by generic boot logic.

Provider-specific mechanisms remain provider-owned.

---

## Provider Contract

For every Stage 1 provider, the specification defines:

- initialization behavior;
- required inputs;
- produced outputs;
- provider-owned context;
- context allocation;
- context ownership;
- context lifetime;
- context validity;
- invalidation rules;
- destruction point;
- failure behavior;
- interaction with normalized Stage 1 state;
- handoff responsibilities.

Provider-specific structures must not leak into the generic boot contract.

`platform_context` remains opaque to generic Stage 1 logic.

---

## Boot Environment

The boot environment contract defines:

- environment type;
- detection state;
- provider context;
- provider context ownership;
- provider context lifetime;
- initialization state;
- validity conditions;
- selected provider;
- supported provider transitions.

The generic boot layer must not depend on provider-private internal representations.

---

## Hardware Discovery

Stage 1 explicitly defines ownership for:

- CPU discovery;
- firmware discovery;
- boot-device discovery;
- hardware normalization.

CPU discovery must not be implemented as an undocumented second firmware-detection mechanism.

Firmware discovery identifies the active boot environment.

CPU discovery obtains architecture-relevant CPU information through the defined architecture contract.

Boot-device discovery obtains only the device information required by the Stage 1 kernel-acquisition contract.

---

## CPU Entry State

The Stage 1 → Stage 2 contract explicitly defines the CPU execution state required at kernel entry.

For the supported architectures:

```text
x86_64:
    long mode
    paging enabled
    CR3 defined by handoff contract
    IF = 0
    valid active stack
    validated kernel entry

AArch64:
    required exception level defined
    MMU state enabled as defined by handoff contract
    TTBR state defined by handoff contract
    PSTATE.DAIF = 1111
    valid active stack
    validated kernel entry
```

Stage 1 does not prescribe unnecessary instruction-level CPU initialization.

Only CPU state required to establish a valid kernel entry environment is part of Stage 1.

Long-lived CPU initialization belongs outside the Stage 1 execution environment.

---

## Kernel Entry Contract

At the Stage 1 → Stage 2 boundary, the following state is explicitly defined:

```text
x86_64:
    RIP = validated kernel entry address
    RSP = valid initial kernel stack pointer
    CR3 = required page-table root
    CPU mode = long mode
    paging = enabled
    IF = 0

AArch64:
    PC = validated kernel entry address
    SP = valid initial kernel stack pointer
    TTBR1_EL1 = required kernel page-table root
    TTBR0_EL1 = unused by Stage 2
    exception level = EL1
    MMU = enabled
    translation granule = 4 KiB
    virtual-address regime = 48-bit
    TCR_EL1 and MAIR_EL1 = valid for selected mappings
    TLBI/DSB/ISB synchronization = complete
    PSTATE.DAIF = 1111

Both architectures:
    required mappings established
    required permissions established
    provider state defined
    transferred ownership defined
```

The exact register setup sequence remains implementation-defined unless it affects the observable architecture-significant entry state.

---

## `JPBootContext` Contract

`JPBootContext` is the central Stage 1 runtime context.

It is not an unstructured container for arbitrary state.

Each Stage 1 context component has:

- defined semantic meaning;
- defined owner;
- defined lifetime;
- defined validity condition;
- defined producer;
- defined consumer;
- defined Stage boundary.

The context conceptually contains:

```text
JPBootContext
    ├── boot environment
    ├── hardware discovery result
    ├── memory-map result
    ├── framebuffer result
    ├── kernel image and normalized kernel state
    ├── kernel segment state
    ├── paging state
    ├── kernel stack state
    ├── handoff state
    ├── interaction state (hook registration)
    ├── stage2_result (owned by Stage 2)
    ├── reserved stage3 and stage4 areas (opaque to Stage 1)
    └── provider context reference
```

No context member may acquire architecture-significant meaning merely because an implementation stores data there.

No Stage 2 component may be required to infer hidden state from undocumented `JPBootContext` contents.

---

## Memory Map Contract

Normalized memory regions define:

- base address;
- length;
- type;
- validity;
- ownership semantics.

The following expression must never overflow:

```text
base_address + length
```

Unless explicitly permitted by the normalized contract:

```text
normalized regions must not overlap
```

The implementation may not silently merge, reorder, discard, or reinterpret overlapping regions.

Any normalization rule affecting ownership or architecture-significant memory interpretation must be defined by Stage 1.

---

## Framebuffer Contract

For every supported framebuffer format, the contract defines:

- framebuffer base address;
- width;
- height;
- pitch;
- channel ordering;
- bits per pixel;
- bytes per pixel;
- unused channel bits;
- byte ordering where architecture-significant;
- interpretation of framebuffer memory.

`pitch` means:

```text
number of bytes between the start of two consecutive scanlines
```

The normalized framebuffer representation must be sufficient for a later consumer to interpret framebuffer memory without provider-specific assumptions.

---

## ELF Contract

Stage 1 explicitly defines:

- ELF64 support;
- x86_64 machine type for x86_64 boot;
- AArch64 machine type for AArch64 boot;
- supported ELF object types;
- required ELF header fields;
- program-header bounds;
- program-header entry size;
- program-header count;
- `PT_LOAD` selection;
- segment file ranges;
- segment memory ranges;
- alignment requirements;
- zero-fill behavior;
- entry-point validation;
- malformed-data failure behavior.

`ET_DYN` is supported only when its load-bias and entry-point semantics are explicitly satisfied by the loader contract.

If the implementation cannot satisfy those semantics, `ET_DYN` must not be silently interpreted as `ET_EXEC`.

---

## Kernel Segment Lifecycle

Every loadable segment has a lifecycle distinguishable at the Stage 1 boundary:

```text
UNINITIALIZED
    ↓
VALIDATED
    ↓
ALLOCATED
    ↓
LOADED
    ↓
MAPPED
    ↓
PERMISSIONED
```

Failure states are distinct from successful states.

The specification does not require a particular internal state representation.

A state may be represented by:

- an enum;
- independent flags;
- equivalent state fields;
- another internal representation;

provided the required lifecycle remains independently verifiable.

---

## Segment Address Semantics

The specification explicitly defines:

- `virtual_address`;
- `physical_address`;
- `file_offset`;
- `file_size`;
- `memory_size`;
- `flags`.

`physical_address` represents the physical base of the destination memory allocated or reserved for the segment.

`virtual_address` represents the virtual address at which the segment is required to execute.

`file_offset` identifies the source range inside the kernel image.

`file_size` defines the file-backed portion.

`memory_size` defines the complete runtime memory range.

---

## Segment Permissions

The mapping from ELF flags to execution permissions is:

```text
PF_R → readable
PF_W → writable
PF_X → executable
```

The specification explicitly defines the policy for:

- read-only segments;
- writable segments;
- executable segments;
- writable + executable segments;
- unsupported flag combinations.

The implementation must not invent executable/writable policy.

---

## Kernel Physical-Memory Handoff

Stage 1 transfers the ownership facts required for kernel entry: kernel image memory, boot page tables, kernel stack, framebuffer, firmware-reserved regions, and other permanent boot resources. Stage 2 adopts and classifies these facts through its authoritative memory subsystem. Stage 2 must not infer ownership from provider allocation internals.

---

## Boot-Time Paging

Stage 1 establishes only the paging state required for valid kernel entry.

The contract defines:

- paging mode;
- page-table root;
- required kernel mappings;
- stack mapping;
- mapping granularity;
- mapping permissions;
- page-table ownership;
- page-table lifetime;
- transition responsibility.

Stage 1 does not implement the complete kernel virtual-memory subsystem.

---

## Kernel Stack

The kernel stack contract defines:

- physical address;
- virtual address;
- size;
- alignment;
- mapping state;
- permissions;
- ownership;
- lifetime;
- initial stack pointer.

The stack must be valid before kernel entry.

---

## Handoff Contract

The handoff contract explicitly distinguishes:

```text
handoff preparation
        ↓
handoff prerequisite validation
        ↓
pre-transition interaction (optional)
        ↓
final platform state acquisition
        ↓
final memory-map acquisition where required
        ↓
transition-ready state
        ↓
actual platform transition
        ↓
kernel execution transfer
```

The generic boot layer and provider must not independently perform the same platform transition.

For UEFI, the specification explicitly defines:

- final memory-map acquisition;
- final memory-map key;
- stale-key invalidation;
- `ExitBootServices()` readiness;
- actual transition responsibility;
- post-transition provider state;
- kernel execution transfer.

The final key must correspond to the final memory-map retrieval used for the transition.

A stale key is invalid.

For BIOS, the specification defines the equivalent provider-specific transition without weakening the normalized Stage 1 contract.

---

## Pre-Transition Interaction Point

Stage 1 exposes one optional interaction point while provider services, including input, are still valid. It is the only point in Stage 1 where an external component may interact with the user.

Position: after all handoff prerequisites are validated (`PREREQUISITES_VALID`) and before platform handoff preparation, which includes final platform state acquisition and final memory-map acquisition. No memory-map key is held across the interaction point, so provider services used during interaction cannot invalidate a key that is later used for the transition.

Contract:

- a caller registers at most one hook through `jp_boot_register_interaction()` before `jp_boot_prepare_handoff()`;
- `jp_boot_prepare_handoff()` invokes the hook exactly once per handoff attempt, only at the position above;
- with no hook registered the step is skipped and the interaction state remains `NOT_REGISTERED`; a skipped step is never reported as completed;
- the hook may use `jp_boot_environment_read_input()` and may read the validated framebuffer, including writing pixels to the validated framebuffer region for presentation; it must not modify any other `JPBootContext` state, ownership, mappings, paging, stack, or provider state;
- if the hook returns anything other than `JP_BOOT_SUCCESS`, the interaction state becomes `FAILED`, handoff becomes `FAILED`, and the result is propagated without conversion;
- Stage 1 does not name, implement, or depend on the hook's content or on Stage 4; the hook is an inverted dependency supplied by the caller;
- from the start of final platform state acquisition onward (and therefore after the platform transition) no input is possible and `jp_boot_environment_read_input()` reports `JP_BOOT_NOT_INITIALIZED`.
- after handoff the hook registration is historical state; Stage 2 and later stages never invoke the hook.

Provider input is normalized as `JPBootInputEvent` and is non-blocking: it returns `JP_BOOT_SUCCESS` with `JP_BOOT_INPUT_NONE` when nothing is pending, `JP_BOOT_NOT_DETECTED` when no input device exists, `JP_BOOT_NOT_INITIALIZED` when the provider is not initialized or its services are no longer valid, and `JP_BOOT_FAILED` on provider error. Events are never fabricated.

---

## Handoff State

The normalized handoff state includes at minimum:

```text
UNINITIALIZED
PREREQUISITES_VALID
PLATFORM_PREPARED
READY
FAILED
```

`READY` may only represent an execution environment that actually satisfies the Stage 1 → Stage 2 entry contract.

`READY` must never mean merely:

```text
"all checks appear to have passed"
```

---

## Boot Result Semantics

`JPBootResult` is intentionally coarse-grained.

It represents the public result of a Stage 1 operation.

Detailed diagnostic classification is outside Stage 1 unless explicitly promoted into the Stage 1 contract.

Multiple internal failures may therefore resolve to:

```text
JP_BOOT_FAILED
```

provided that observable Stage 1 semantics remain deterministic.

---

## Stage 1 → Stage 2 Boundary

Stage 1 transfers:

- detected boot environment;
- normalized hardware state;
- normalized memory information;
- validated framebuffer state;
- validated kernel information;
- loaded kernel segments;
- kernel physical-memory ownership information;
- established kernel mappings;
- required permissions;
- boot-time page-table state;
- valid kernel stack;
- defined CPU entry state;
- handoff state;
- provider-service availability state.

Stage 2 is responsible for transforming this prepared environment into its own runtime kernel infrastructure.

Stage 2 must not be required to:

- rediscover kernel segment ownership;
- reconstruct initial mappings;
- parse ELF again for Stage 1 correctness;
- reconstruct the initial stack;
- infer CPU entry state;
- depend on provider-private allocation state.

---

# Stage 1 Responsibilities

Stage 1 owns:

1. Boot environment initialization.
2. UEFI provider.
3. BIOS provider.
4. Hardware discovery.
5. CPU discovery.
6. Firmware discovery.
7. Boot-device discovery.
8. Memory-map acquisition.
9. Memory-map normalization.
10. Memory-map validation.
11. Framebuffer acquisition.
12. Framebuffer validation.
13. Kernel image acquisition.
14. ELF validation.
15. `PT_LOAD` discovery.
16. Segment validation.
17. Kernel entry validation.
18. Kernel physical-memory allocation/reservation required for entry.
19. Kernel segment loading.
20. Segment zero-fill.
21. Kernel segment mapping.
22. Segment permission establishment.
23. Boot-time paging required for entry.
24. Kernel stack required for entry.
25. CPU execution state required for entry.
26. Handoff preparation.
27. Final readiness validation.
28. Stage 1 → Stage 2 ownership transfer.

---

# Stage 1 Non-Goals

Stage 1 does not own:

- `JPBootInfo`;
- Boot Manager UI;
- boot diagnostics framework;
- generalized logging infrastructure;
- generalized panic infrastructure;
- recovery policy;
- scheduler;
- kernel runtime heap;
- complete physical memory manager;
- complete virtual-memory subsystem;
- generalized device management;
- desktop services;
- application infrastructure.

The pre-transition interaction point exposes only normalized provider input and a caller-registered hook. The content of the interaction (Boot UI, configuration selection) is not a Stage 1 responsibility.

Stage 1 must not silently implement later-stage functionality merely because such functionality could simplify implementation.

---

# Folder Structure

```text
include/justos/boot/
├── boot_types.h
├── boot_environment.h
├── hardware.h
├── memory_map.h
├── framebuffer.h
├── kernel_image.h
├── kernel_loader.h
├── handoff.h
└── architecture/
    ├── x86_64.h
    └── aarch64.h

include/justos/boot/platform/
├── uefi.h
├── bios.h
└── bios_block.h

include/justos/boot/
└── boot_pipeline.h

src/boot/
├── boot_pipeline.c
├── boot_environment.c
├── hardware.c
├── memory_map.c
├── framebuffer.c
├── kernel_image.c
├── kernel_loader.c
├── handoff.c
└── architecture/
    ├── x86_64.c
    ├── x86_64_enter.S
    ├── aarch64.c
    └── aarch64_enter.S

src/boot/platform/
├── uefi.c
├── uefi_defs.h
├── uefi_entry.c
├── bios.c
├── bios_boot_sector.S
├── bios_loader.S
└── bios_entry.c
```

---

# File Specifications

The Stage 1 file specifications define shared boot responsibilities. Whenever a Stage 1 file describes x86_64 mechanisms, the same file must also define the corresponding AArch64 mechanism, state, ownership, failure semantics, and verification. A file is incomplete until both architecture paths are specified.

Each Stage 1 file is specified using the following normative structure:

```text
File
├── Purpose
├── Responsibilities
├── Types / Structures
├── Constants / Enums
├── Public API
├── Internal API
├── Algorithm
├── State / Lifecycle
├── Ownership / Lifetime
├── Invariants
├── Dependencies
├── Boundary Rules
├── Failure Semantics
├── Implementation Freedom
└── Verification
```

---

# 1. `include/justos/boot/boot_types.h`

## File

Central normalized Stage 1 type and constant contract.

## Purpose

`boot_types.h` is the central shared data contract of Stage 1.

It defines normalized structures, state enumerations, and architecture-independent contract constants required by generic Stage 1 modules.

The file contains declarations and contract definitions only.

It contains no runtime logic.

It contains no provider-specific firmware structures.

## Responsibilities

- Define normalized Stage 1 data structures.
- Define Stage 1 state enumerations.
- Define public Stage 1 constants.
- Define architecture-significant normalized state where required.
- Provide a common contract for generic Stage 1 modules.
- Prevent generic modules from redefining protocol constants independently.
- Keep platform-native structures outside the generic contract.
- Define normalized boot input and pre-transition interaction types.

## provider support matrix

| Architecture |     UEFI |         BIOS |
| ------------ | -------: | -----------: |
| x86_64       | REQUIRED |     REQUIRED |
| AArch64      | REQUIRED | NOT REQUIRED |

BIOS support is an x86_64-only provider.
AArch64 Stage 1 must not contain a BIOS execution path.

## abstraction

```text
UEFI provider
      ↓
normalized memory map
normalized framebuffer
normalized kernel image
      ↓
generic Stage 1
      ↓
architecture-specific handoff
```

## Types / Structures

### `JPCpuInfo`

Represents discovered CPU information.

```c
JPDetectionState state;
uint32_t logical_cores;
```

### `JPFirmwareInfo`

Represents normalized firmware information.

```c
JPDetectionState state;
JPBootEnvironmentType type;
```

### `JPBootDeviceInfo`

Represents normalized boot-device discovery state.

```c
JPDetectionState state;
```

Stage 1 does not require provider-independent device identity unless explicitly defined elsewhere in the contract.

### `JPHardwareInfo`

Aggregates normalized hardware information.

```c
JPCpuInfo cpu;
JPFirmwareInfo firmware;
JPBootDeviceInfo boot_device;
JPDetectionState overall_state;
```

### `JPBootEnvironment`

Represents the selected normalized boot environment.

```c
JPBootEnvironmentType type;
JPDetectionState state;
void *platform_context;
JPBootEnvironmentLifecycle lifecycle;
```

`lifecycle` records how far the provider has got, so that the rules about when input and the other operations are allowed can be enforced by the generic code. Its values are `JP_BOOT_ENVIRONMENT_LIFECYCLE_UNINITIALIZED` (zero), `JP_BOOT_ENVIRONMENT_LIFECYCLE_INITIALIZED`, `JP_BOOT_ENVIRONMENT_LIFECYCLE_ACTIVE`, `JP_BOOT_ENVIRONMENT_LIFECYCLE_HANDOFF_PREPARED`, and `JP_BOOT_ENVIRONMENT_LIFECYCLE_FAILED`; they are the states drawn under "State / Lifecycle" of `boot_environment.h`. Only `boot_environment.c` writes it.

`platform_context` is opaque.

Generic Stage 1 code must never interpret provider-private state.

### `JPMemoryRegion`

```c
uint64_t base_address;
uint64_t length;
JPMemoryRegionType type;
```

### `JPMemoryMap`

```c
JPDetectionState state;
size_t region_count;
JPMemoryRegion regions[JP_MAX_MEMORY_REGIONS];
```

### `JPFramebufferInfo`

```c
JPDetectionState state;
uint64_t base_address;
uint32_t width;
uint32_t height;
uint32_t pitch;
JPFramebufferFormat format;
```

`pitch` is measured in bytes per scanline.

### `JPKernelImage`

```c
JPDetectionState state;
const void *buffer;
size_t size;
```

### `JPKernelHeader`

```c
uint32_t magic;
uint16_t elf_class;
uint16_t machine;
uint16_t type;

uint64_t entry_point;
uint64_t program_header_offset;

uint16_t program_header_entry_size;
uint16_t program_header_count;
```

### `JPKernelSegment`

```c
uint64_t virtual_address;
uint64_t physical_address;

uint64_t file_offset;
uint64_t file_size;
uint64_t memory_size;

uint32_t flags;
```

The associated Stage 1 contract additionally defines lifecycle state for each segment.

The internal representation of lifecycle state remains implementation-defined. This implementation stores it in one more field of the segment, `JPKernelSegmentState state`, with the values `JP_KERNEL_SEGMENT_UNINITIALIZED` (zero), `JP_KERNEL_SEGMENT_VALIDATED`, `JP_KERNEL_SEGMENT_ALLOCATED`, `JP_KERNEL_SEGMENT_LOADED`, `JP_KERNEL_SEGMENT_MAPPED`, `JP_KERNEL_SEGMENT_PERMISSIONED`, and `JP_KERNEL_SEGMENT_FAILED` (a failure state that is distinct from every success state).

### `JPKernelInfo`

```c
JPKernelValidationState validation_state;
JPKernelLoadState load_state;

JPKernelImage image;
JPKernelHeader header;

JPKernelSegment segments[JP_MAX_KERNEL_SEGMENTS];
size_t segment_count;

uint64_t entry_point;
```

### `JPHandoffInfo`

The current normalized handoff state and all architecture-significant handoff information required by the Stage 1 → Stage 2 contract.

The exact internal field representation may evolve as long as the required handoff state remains representable and verifiable.

The contract includes:

- readiness state;
- kernel entry address;
- initial stack pointer;
- page-table root;
- CPU execution state;
- interrupt state;
- ownership-transfer state;
- the memory that holds the boot page tables and descriptor table (`page_table_memory_base`, `page_table_memory_size`, `page_table_memory_used`; the first two are facts that Stage 2 adopts, the third counts the pages that Stage 1 has already handed out of that memory);
- provider transition state where normalized state is required.
- architecture
- execution mode
- paging state
- page-table root
- stack state
- interrupt state
- kernel entry state
  or in fact:

```text
x86_64:
    CR3
    long mode
    ...

AArch64:
    TTBR0/TTBR1
    EL state
    ...
```

Architecture-specific handoff state may be represented internally
through architecture-specific structures.

Generic Stage 1 code must not interpret architecture-specific
register state directly.

The handoff contract must expose enough normalized state to verify
that the architecture-specific entry requirements are satisfied.

### `JPBootContext`

The central Stage 1 runtime context.

Conceptually:

```c
JPBootStage current_stage;

JPBootEnvironment environment;

JPHardwareInfo hardware;

JPMemoryMap memory_map;

JPFramebufferInfo framebuffer;

JPKernelInfo kernel;

JPHandoffInfo handoff;

JPBootInteraction interaction;

JPBootResult stage2_result;

JPBootStage2Area stage2;

JPBootStage3Area stage3;

JPBootStage4Area stage4;
```

`stage2` is storage reserved inside the context for the Stage 2 memory ownership table (type `JPBootStage2Area`, declared in `include/justos/boot/boot_types.h` like `JPBootStage3Area`, because the context must be one complete object; its layout is the one in the rules of `memory.h` below). Stage 1 never reads, writes, or interprets it; it is zeroed together with the context.

`stage2_result` is owned by Stage 2. It reads as `JP_BOOT_SUCCESS` (zero) until `jp_kernel_stage2_failure()` records a failing result; the caller that provides the context zero-initializes it. Stage 1 never reads or writes it.

The `stage3` and `stage4` areas are storage reserved inside the context. Their types are defined by Stage 3 and Stage 4 and are opaque to Stage 1: Stage 1 never reads, writes, or interprets them. Reserving them in the context keeps their ownership and lifetime identical to the context across the platform transition. The header decomposition needed to avoid cyclic includes is implementation-defined provided every type and symbol name is preserved.

`current_stage` is written only by the public functions of the Stage 1 pipeline, and each of them writes only the stage value of its own step (for example `jp_boot_discover_memory()` writes `JP_BOOT_STAGE_MEMORY_DISCOVERY`, and `jp_boot_prepare_handoff()` writes `JP_BOOT_STAGE_READY` when everything has succeeded). Internal helpers, providers, and architecture operations never write it. The function `jp_boot_run_pipeline()` (see below) only calls these functions in order and does not write `current_stage` itself.
The exact internal layout may remain implementation-defined provided all required state remains representable.

`JPBootContext` does not define `JPBootInfo`.

### `JPBootInputEvent`, `JPBootInteractionHook`, and `JPBootInteraction`

Normalized input and the pre-transition interaction hook. `JPBootContext` is forward-declared before these types. The enumerations `JPBootInputKind`, `JPBootInputKey`, and `JPBootInteractionState` are defined in Constants / Enums.

```c
typedef struct JPBootInputEvent {
    JPBootInputKind kind;
    JPBootInputKey key;
    uint32_t character;
} JPBootInputEvent;

typedef JPBootResult (*JPBootInteractionHook)(
    JPBootContext *context,
    void *user_data
);

typedef struct JPBootInteraction {
    JPBootInteractionState state;
    JPBootInteractionHook hook;
    void *user_data;
} JPBootInteraction;
```

`key` is meaningful only when `kind == JP_BOOT_INPUT_KEY`; `character` is meaningful only when `kind == JP_BOOT_INPUT_CHARACTER` and holds a printable character normalized by the provider. Keys that the provider cannot map to this set are reported as `JP_BOOT_INPUT_NONE`. The interaction state records only what actually happened: `COMPLETED` is never set unless the hook actually ran and returned `JP_BOOT_SUCCESS`.

## Constants / Enums

### `JPBootInputKind` and `JPBootInputKey`

```c
typedef enum JPBootInputKind {
    JP_BOOT_INPUT_NONE = 0,
    JP_BOOT_INPUT_KEY,
    JP_BOOT_INPUT_CHARACTER
} JPBootInputKind;

typedef enum JPBootInputKey {
    JP_BOOT_KEY_UP,
    JP_BOOT_KEY_DOWN,
    JP_BOOT_KEY_LEFT,
    JP_BOOT_KEY_RIGHT,
    JP_BOOT_KEY_ENTER,
    JP_BOOT_KEY_ESCAPE,
    JP_BOOT_KEY_F10,
    JP_BOOT_KEY_BACKSPACE
} JPBootInputKey;
```

### `JPBootInteractionState`

```c
typedef enum JPBootInteractionState {
    JP_BOOT_INTERACTION_NOT_REGISTERED = 0,
    JP_BOOT_INTERACTION_REGISTERED,
    JP_BOOT_INTERACTION_COMPLETED,
    JP_BOOT_INTERACTION_FAILED
} JPBootInteractionState;
```

### `JPBootResult`

```c
JP_BOOT_SUCCESS = 0,
JP_BOOT_NOT_INITIALIZED,
JP_BOOT_NOT_DETECTED,
JP_BOOT_UNKNOWN,
JP_BOOT_FAILED,
JP_BOOT_ERROR_INVALID_PARAM,
JP_BOOT_ERROR_CORRUPTED_DATA
```

### `JPBootStage`

```c
JP_BOOT_STAGE_UNINITIALIZED = 0,
JP_BOOT_STAGE_HARDWARE_DISCOVERY,
JP_BOOT_STAGE_MEMORY_DISCOVERY,
JP_BOOT_STAGE_FRAMEBUFFER_INIT,
JP_BOOT_STAGE_KERNEL_IMAGE_ACQUISITION,
JP_BOOT_STAGE_KERNEL_VALIDATION,
JP_BOOT_STAGE_KERNEL_LOADING,
JP_BOOT_STAGE_HANDOFF_PREPARATION,
JP_BOOT_STAGE_READY
```

### `JPDetectionState`

```c
JP_DETECTION_STATE_UNKNOWN = 0,
JP_DETECTION_STATE_NOT_DETECTED,
JP_DETECTION_STATE_NOT_INITIALIZED,
JP_DETECTION_STATE_DETECTED,
JP_DETECTION_STATE_FAILED
```

### `JPBootEnvironmentType`

```c
JP_BOOT_ENVIRONMENT_UNKNOWN = 0,
JP_BOOT_ENVIRONMENT_UEFI,
JP_BOOT_ENVIRONMENT_BIOS
```

### `JPMemoryRegionType`

```c
JP_MEMORY_REGION_UNKNOWN = 0,
JP_MEMORY_REGION_USABLE,
JP_MEMORY_REGION_RESERVED,
JP_MEMORY_REGION_ACPI,
JP_MEMORY_REGION_MMIO,
JP_MEMORY_REGION_FIRMWARE,
JP_MEMORY_REGION_UNUSABLE
```

### `JPFramebufferFormat`

```c
JP_FRAMEBUFFER_FORMAT_UNKNOWN = 0,
JP_FRAMEBUFFER_FORMAT_RGB_888,
JP_FRAMEBUFFER_FORMAT_BGR_888
```

### `JPKernelValidationState`

```c
JP_KERNEL_VALIDATION_UNKNOWN = 0,
JP_KERNEL_VALIDATION_NOT_INITIALIZED,
JP_KERNEL_VALIDATION_VALID,
JP_KERNEL_VALIDATION_INVALID,
JP_KERNEL_VALIDATION_FAILED
```

### `JPKernelLoadState`

contract

```text
JPKernelSegment
    ↓
normalized segment metadata

internal loader state
    ↓
validated/allocated/loaded/mapped/permissioned
```

```c
JP_KERNEL_LOAD_UNKNOWN = 0,
JP_KERNEL_LOAD_NOT_INITIALIZED,
JP_KERNEL_LOAD_LOADED,
JP_KERNEL_LOAD_FAILED
```

Generic ELF constants:

```c
#define JP_ELF_MAGIC 0x464C457F
#define JP_ELF_CLASS_64 2
#define JP_ELF_TYPE_EXEC 2
#define JP_ELF_TYPE_DYN 3
#define JP_ELF_PT_LOAD 1
```

Capacity constants:

```c
#define JP_MAX_MEMORY_REGIONS 128
#define JP_MAX_KERNEL_SEGMENTS 16
```

## `JPArchitectureType`

```c
JP_ARCH_UNKNOWN = 0,
JP_ARCH_X86_64,
JP_ARCH_AARCH64
```

## implementation:

x86_64:

```text
Architecture:
    x86_64
    long mode

Page-table root:
    CR3 contains valid root page-table physical address

Paging:
    enabled

Kernel entry:
    canonical x86_64 virtual address
    mapped executable

Stack:
    valid mapped stack
    valid stack pointer
    ABI-compatible alignment

Interrupt state:
    defined and explicitly disabled/enabled according to Stage 2 contract

CPU state:
    required segment/register state established

Entry:
    control transferred using x86_64 calling convention
```

AArch64:

```text
Architecture:
    AArch64

Exception level:
    defined Stage 2 EL

Page-table root:
    required TTBR register(s) contain valid translation-table base

Paging:
    MMU state explicitly defined

Kernel entry:
    valid AArch64 virtual address
    mapped executable

Stack:
    valid mapped stack
    valid SP
    ABI-compatible alignment

Interrupt state:
    explicitly defined

Execution state:
    required PSTATE state defined

Cache/TLB state:
    required synchronization completed before entry

Entry:
    control transferred according to AArch64 calling convention

```

## machine constants

```c
#define JP_ELF_MACHINE_X86_64 62
#define JP_ELF_MACHINE_AARCH64 183
```

```text
ELF x86_64 → x86_64 handoff
ELF AArch64 → AArch64 handoff
ELF architecture ≠ boot architecture → reject
```

```text
Boot environment
      ↓
detected boot architecture
      ↓
Stage 1 architecture
      ↓
ELF machine must match
      ↓
handoff architecture
```

## Public API

None.

`boot_types.h` exposes shared types and constants only.

## Internal API

None.

## Algorithm

None.

This file defines contracts and declarations.

## State / Lifecycle

State definitions are consumed by the modules that own the corresponding operation.

A state indicating successful completion may only be written after the corresponding operation has actually completed.

`JPBootInteractionState` follows `NOT_REGISTERED → REGISTERED → COMPLETED` or `FAILED`. `COMPLETED` is written only after the registered hook has actually run and returned `JP_BOOT_SUCCESS`; `FAILED` is written when it returned any other result.

## Ownership / Lifetime

- `JPBootContext` is owned by the Stage 1 runtime before handoff.
- Ownership transfers to Stage 2 at the Stage 1 → Stage 2 boundary.
- Stage 1 has no access, mutation, or lifecycle-management rights after handoff.
- Stage 2 owns the context through the Stage 2 completion continuation (or the Stage 2 failure path).
- After the Stage 2 completion continuation, the later Milestone 0 stages (Stage 3, Stage 4) use the same context object as defined by their contracts; after `kernel_main(context)` begins, the next runtime stage owns context lifetime.
- Nested normalized structures are owned by the context unless explicitly documented otherwise.
- `platform_context` remains provider-owned.
- `JPKernelImage.buffer` remains valid until all Stage 1 operations requiring it are complete.
- Kernel destination memory belongs to the prepared kernel execution environment after successful allocation/reservation.
- Handoff state remains valid through the Stage 1 → Stage 2 transition.

## Invariants

- No successful state may be fabricated.
- Region count must not exceed `JP_MAX_MEMORY_REGIONS`.
- Segment count must not exceed `JP_MAX_KERNEL_SEGMENTS`.
- `file_size <= memory_size`.
- Address arithmetic must not overflow.
- `JPBootInfo` must not be declared here.
- `JPBootInputEvent.key` is meaningful only for `JP_BOOT_INPUT_KEY` and `character` only for `JP_BOOT_INPUT_CHARACTER`.
- `JPBootInteractionState` is never `COMPLETED` without an executed, successful hook.
- Generic types must not depend directly on provider-native structures.

## Dependencies

```c
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
```

No UEFI, BIOS, VBE, GOP, or other firmware header may be included.

## Boundary Rules

This file is the boundary between normalized Stage 1 contracts and provider-specific implementations.

Provider-native data must be converted before entering generic structures.

## Failure Semantics

The type contract itself does not perform operations.

Invalid state combinations are contract violations.

## Implementation Freedom

Internal structure layout, padding, helper types, and state-storage mechanisms remain implementation-defined unless they affect an observable Stage 1 contract.

## Verification

Verify:

- all required normalized fields exist;
- no provider-native dependency exists;
- all Stage 1 constants are centralized;
- `JPBootInfo` is absent;
- the input and interaction types are present and contain no provider-native dependency;
- every architecture-significant Stage 1 state is representable.

---

# 2. `include/justos/boot/boot_environment.h`

## File

Generic boot-environment interface.

## Purpose

Defines the platform-independent interface between generic Stage 1 logic and platform-specific providers.

## Responsibilities

- Initialize the selected boot environment.
- Expose normalized provider behavior.
- Keep provider-private state opaque.
- Dispatch platform operations through the normalized contract.
- Dispatch normalized provider input (`jp_boot_environment_read_input()`) while provider services are valid.

## Types / Structures

No provider-native structures.

## Constants / Enums

None.

## Public API

```c
JPBootResult jp_boot_environment_initialize(
    JPBootContext *context
);

JPBootResult jp_boot_environment_get_memory_map(
    JPBootContext *context
);

JPBootResult jp_boot_environment_get_framebuffer(
    JPBootContext *context
);

JPBootResult jp_boot_environment_get_kernel_image(
    JPBootContext *context
);

JPBootResult jp_boot_environment_prepare_handoff(
    JPBootContext *context
);

JPBootResult jp_boot_environment_read_input(
    JPBootContext *context,
    JPBootInputEvent *event
);
```

`jp_boot_environment_read_input()` dispatches to the active provider's input operation and returns the provider result unchanged; it follows the Pre-Transition Interaction Point contract.

## Internal API

Provider dispatch helpers remain internal.

## Algorithm

Who chooses the provider: the caller does, before it calls `jp_boot_environment_initialize()`. The platform entry point (the code the firmware starts first) knows whether it is a UEFI or a BIOS entry point, so it sets `context->environment.type` to `JP_BOOT_ENVIRONMENT_UEFI` or `JP_BOOT_ENVIRONMENT_BIOS`, and it sets `context->environment.platform_context` to the provider-owned context (`JPUefiContext` or `JPBiosContext`). `jp_boot_environment_initialize()` therefore never detects the firmware itself; this keeps the rule that there is no second firmware detection path.

1. Validate context: it is not null, `environment.type` is UEFI or BIOS, and `platform_context` is not null. Otherwise return `JP_BOOT_ERROR_INVALID_PARAM`.
2. Select the provider that matches `environment.type`.
3. Verify provider availability (a combination the build does not support is rejected).
4. Initialize provider.
5. Only after the provider reports success, set `environment.state` to `JP_DETECTION_STATE_DETECTED`; after a failure set it to `JP_DETECTION_STATE_FAILED`.
6. Expose only normalized provider operations.
7. Return actual provider result.

### Decided rules (a build for one architecture)

A boot application contains only the architecture it is built for. The private header `src/boot/arch_dispatch.h` offers one macro, `JP_ARCH_DISPATCH(architecture, x86_64_call, aarch64_call)`, which the generic modules `hardware.c`, `kernel_loader.c`, and `handoff.c` use instead of choosing by hand. In the default build (the tests) it picks the call by the value of `architecture`. When the macro `JP_BOOT_ONLY_X86_64` (or `JP_BOOT_ONLY_AARCH64`) is defined, the macro always uses that architecture's call and the other call is removed, so the other architecture's functions are not needed at link time; and in such a build only that architecture counts as a known architecture (any other value of `handoff.architecture` is treated like `JP_ARCH_UNKNOWN`).

### Decided rules (build without the BIOS provider)

A boot application for a machine without BIOS (the UEFI application on x86_64 and on AArch64) must not contain the BIOS provider. Defining the macro `JP_BOOT_WITHOUT_BIOS` when `boot_environment.c` is compiled removes every call of a `jp_bios_*` function and the include of `bios.h`; in such a build `jp_boot_environment_initialize()` answers the environment type `JP_BOOT_ENVIRONMENT_BIOS` like the forbidden combination of AArch64 and BIOS: `JP_BOOT_FAILED`, `environment.state` and `lifecycle` become `FAILED`, and nothing is called. The default build (the tests) contains both providers.

### Decided rules (build without the UEFI provider)

The BIOS loader (one flat binary, see the BIOS provider rules) must not contain the UEFI provider, in the same way as the UEFI application must not contain the BIOS provider. Defining the macro `JP_BOOT_WITHOUT_UEFI` when `boot_environment.c` is compiled removes every call of a `jp_uefi_*` function and the include of `uefi.h`; in such a build `jp_boot_environment_initialize()` answers the environment type `JP_BOOT_ENVIRONMENT_UEFI` with `JP_BOOT_FAILED`, sets `environment.state` and `lifecycle` to `FAILED`, and calls nothing. The two macros are not used together.

### Decided rules (lifecycle and combinations)

- `jp_boot_environment_initialize()` also rejects the combination `environment.type == JP_BOOT_ENVIRONMENT_BIOS` with `handoff.architecture == JP_ARCH_AARCH64` (AArch64 has no BIOS execution path): the result is `JP_BOOT_FAILED`, `environment.state` becomes `FAILED`, `lifecycle` becomes `FAILED`, and the provider is not called. A second call while `lifecycle` is not `UNINITIALIZED` returns `JP_BOOT_FAILED` and changes nothing.
- A successful initialization sets `lifecycle` to `INITIALIZED`; a failed one sets `FAILED`.
- `get_memory_map`, `get_framebuffer`, `get_kernel_image`, and `prepare_handoff` require `lifecycle` to be `INITIALIZED` or `ACTIVE`; otherwise they return `JP_BOOT_NOT_INITIALIZED` without calling the provider. A successful `get_*` call sets `lifecycle` to `ACTIVE`; a failed `get_*` call changes nothing, so a later call may still succeed. A successful `prepare_handoff` sets `lifecycle` to `HANDOFF_PREPARED`; a failed one sets `FAILED`.
- `read_input` is allowed only in `INITIALIZED` and `ACTIVE`; it never changes `lifecycle`.

`jp_boot_environment_read_input()`:

1. Validate the context and the event pointer.
2. Verify the provider state permits input.
3. Dispatch to the active provider's input operation.
4. Return the provider result unchanged.

### Decided rules (memory reservation and the final memory map)

One more operation joins the boot-environment interface (and each provider):

```c
JPBootResult jp_boot_environment_reserve_memory(JPBootContext *context, uint64_t physical, uint64_t length);
JPBootResult jp_uefi_reserve_memory(JPUefiContext *context, uint64_t physical, uint64_t length);
JPBootResult jp_bios_reserve_memory(JPBiosContext *context, uint64_t physical, uint64_t length);
```

Reserving means telling the firmware (or the provider) that a physical range now belongs to the kernel execution environment, so that nothing else uses it. `jp_boot_environment_reserve_memory()` follows the rules of `get_memory_map` (it needs the lifecycle `INITIALIZED` or `ACTIVE`, otherwise `JP_BOOT_NOT_INITIALIZED`; a null context is `JP_BOOT_ERROR_INVALID_PARAM`; a successful call sets `ACTIVE`; a failed call changes nothing) and dispatches to the provider. A reservation succeeds only after it really happened. The provider rounds the range outward to whole pages (`JP_BOOT_PAGE_SIZE`); `length == 0` is `JP_BOOT_ERROR_INVALID_PARAM`.

The final memory map: `jp_boot_environment_prepare_handoff()`, after the provider reported success, asks the provider for the final memory map (the provider holds it from the transition) and stores it in `context->memory_map` with the state `DETECTED`. The count check of `jp_boot_discover_memory()` applies (more than `JP_MAX_MEMORY_REGIONS` regions is `JP_BOOT_ERROR_CORRUPTED_DATA` and the lifecycle becomes `FAILED`). The caller, `jp_boot_prepare_handoff()`, validates this new map again (see its rules). The final map, not the early map, is the one that the kernel receives, because it shows the memory that Stage 1 reserved.

## State / Lifecycle

```text
UNINITIALIZED
    ↓
INITIALIZED
    ↓
ACTIVE
    ↓
HANDOFF_PREPARED
```

Failure transitions to `FAILED`.

Input is permitted in `INITIALIZED` and `ACTIVE`. In `UNINITIALIZED` and from `HANDOFF_PREPARED` onward `jp_boot_environment_read_input()` returns `JP_BOOT_NOT_INITIALIZED`, because provider services must not be used once final platform state acquisition has begun.

## Ownership / Lifetime

- Generic boot code owns the reference to the active provider context.
- The provider owns the provider-private context.
- The provider context remains valid for the period defined by the provider contract.
- Generic code must not destroy provider-private objects directly.

## Invariants

- Only one active provider is selected.
- Provider-private state remains opaque.
- No provider is considered active before actual initialization.
- No successful operation is reported without actual provider success.
- Input events are never fabricated and are never delivered after final platform state acquisition has begun.

## Dependencies

```c
#include "justos/boot/boot_types.h"
```

## Boundary Rules

This is the only generic interface through which Stage 1 accesses platform providers.

## Failure Semantics

Provider failure propagates as a failed Stage 1 operation. Input failures (`JP_BOOT_NOT_DETECTED`, `JP_BOOT_NOT_INITIALIZED`, `JP_BOOT_FAILED`) are returned unchanged.

## Implementation Freedom

Provider dispatch tables, function-pointer mechanisms, and internal provider selection remain implementation-defined.

## Verification

Verify:

- generic code contains no provider-native structures;
- provider ownership is explicit;
- supported providers expose equivalent normalized operations;
- failures propagate deterministically;
- input dispatch returns provider results unchanged and is rejected outside the permitted states.

---

# 3. `src/boot/boot_environment.c`

## File

Generic boot-environment implementation.

## Purpose

Implements provider selection and dispatch without embedding UEFI or BIOS implementation logic.

## Responsibilities

- Validate the boot context.
- Select the active provider.
- Initialize it.
- Dispatch normalized operations.
- Maintain generic provider state.
- Dispatch normalized provider input.

## Types / Structures

No provider-native structures.

## Constants / Enums

No platform-specific constants.

## Public API

```c
JPBootResult jp_boot_environment_initialize(
    JPBootContext *context
);

JPBootResult jp_boot_environment_get_memory_map(
    JPBootContext *context
);

JPBootResult jp_boot_environment_get_framebuffer(
    JPBootContext *context
);

JPBootResult jp_boot_environment_get_kernel_image(
    JPBootContext *context
);

JPBootResult jp_boot_environment_prepare_handoff(
    JPBootContext *context
);

JPBootResult jp_boot_environment_read_input(
    JPBootContext *context,
    JPBootInputEvent *event
);
```

## Internal API

Provider dispatch helpers.

## Algorithm

```text
Detect active environment
        ↓
Select provider
        ↓
Initialize provider
        ↓
Store opaque context
        ↓
Expose normalized operations
```

Input dispatch: validate context and event, check provider state, dispatch to the provider input operation, return its result unchanged.

## State / Lifecycle

The generic environment state must reflect actual provider initialization. Input dispatch is permitted only in the states defined by `boot_environment.h`.

## Ownership / Lifetime

The provider remains owner of provider-private context.

## Invariants

- No fake environment detection.
- No second independent firmware detection path.
- No direct provider-native access.

## Dependencies

```c
#include "justos/boot/boot_environment.h"
```

## Boundary Rules

No UEFI or BIOS implementation may be embedded here.

## Failure Semantics

Initialization failure propagates. Input dispatch failures propagate unchanged.

## Implementation Freedom

Dispatch architecture is implementation-defined.

## Verification

Verify provider selection and state transitions using both supported providers, and input dispatch (`NONE`, event, `NOT_DETECTED`, `NOT_INITIALIZED` outside permitted states) for each provider.

---

# 4. `include/justos/boot/platform/uefi.h`

## File

UEFI provider contract.

## Purpose

Defines the UEFI-specific provider interface used by generic Stage 1.

## Responsibilities

- UEFI initialization.
- UEFI memory-map acquisition.
- GOP framebuffer acquisition.
- Kernel image acquisition.
- Non-blocking firmware input acquisition before the platform transition.
- Final UEFI handoff preparation.

## Types / Structures

### `JPUefiContext`

Provider-owned context.

It contains or references the UEFI handles, buffers, memory-map metadata, and transition state required by the provider contract.

Provider-private fields remain implementation-defined.

## Constants / Enums

UEFI-specific constants may be defined here where they form part of the provider contract.

## Public API

```c
JPBootResult jp_uefi_initialize(
    JPUefiContext *context
);

JPBootResult jp_uefi_get_memory_map(
    JPUefiContext *context,
    JPMemoryMap *memory_map
);

JPBootResult jp_uefi_get_framebuffer(
    JPUefiContext *context,
    JPFramebufferInfo *framebuffer
);

JPBootResult jp_uefi_get_kernel_image(
    JPUefiContext *context,
    JPKernelImage *image
);

JPBootResult jp_uefi_prepare_handoff(
    JPUefiContext *context
);

JPBootResult jp_uefi_read_input(
    JPUefiContext *context,
    JPBootInputEvent *event
);
```

`jp_uefi_read_input()` obtains non-blocking input from the firmware input services, which are valid only before the platform transition. It serves both x86_64 and AArch64 UEFI and follows the Pre-Transition Interaction Point contract.

## Internal API

Provider-specific acquisition, input, and transition helpers.

## Algorithm

The provider:

1. initializes the UEFI environment;
2. acquires required firmware information;
3. acquires the memory map;
4. acquires GOP information;
5. acquires the kernel image;
6. performs all boot-services-dependent operations;
7. serves pending input requests through `jp_uefi_read_input()` while boot services are valid, before the final memory map is obtained;
8. obtains the final memory map;
9. preserves its key;
10. establishes `ExitBootServices` readiness;
11. performs the transition according to the handoff contract.

## State / Lifecycle

```text
UNINITIALIZED
    ↓
INITIALIZED
    ↓
ACTIVE
    ↓
FINAL_MAP_ACQUIRED
    ↓
EXIT_BOOT_SERVICES_READY
    ↓
EXITED
```

Failure is terminal for the current provider attempt.

Input is permitted in `INITIALIZED` and `ACTIVE` only. From `FINAL_MAP_ACQUIRED` onward `jp_uefi_read_input()` returns `JP_BOOT_NOT_INITIALIZED`, because boot services must not be called while the final key is held.

## Ownership / Lifetime

The provider owns:

- UEFI handles;
- temporary UEFI buffers;
- memory-map buffers;
- provider-private state.

The final transition invalidates boot-services resources according to UEFI semantics.

## Invariants

- Final memory-map key matches the final memory-map retrieval.
- Stale keys are rejected.
- Input never occurs while a final memory-map key is held.
- Boot-services-dependent operations are completed before the transition.
- No provider state is falsely reported as ready.

## Dependencies

UEFI firmware interfaces and normalized Stage 1 types.

## Boundary Rules

UEFI-native structures do not escape into generic Stage 1.

## Failure Semantics

Failure to obtain or validate required UEFI state prevents successful handoff preparation. Input returns `JP_BOOT_NOT_DETECTED` when no firmware input device exists, `JP_BOOT_NOT_INITIALIZED` when the provider state does not permit input, and `JP_BOOT_FAILED` on provider error; no event is fabricated.

## Implementation Freedom

UEFI buffer management, protocol lookup, retry strategy, and internal storage are implementation-defined unless architecture-significant.

## Verification

Verify:

- memory-map acquisition;
- final key preservation;
- GOP normalization;
- kernel acquisition;
- `ExitBootServices` transition semantics;
- input: no pending input, a mapped key, a character, an unmappable key, no input device, and rejection from `FINAL_MAP_ACQUIRED` onward, on both x86_64 and AArch64.

---

# 5. `src/boot/platform/uefi.c`

## File

UEFI provider implementation.

## Purpose

Implements the UEFI Stage 1 provider.

## Responsibilities

- obtain UEFI system context;
- obtain UEFI memory map;
- normalize memory descriptors;
- obtain GOP framebuffer;
- obtain kernel image;
- obtain non-blocking input events before the transition;
- prepare final UEFI handoff.

## Types / Structures

Provider-private state only.

## Constants / Enums

UEFI implementation constants only.

Generic Stage 1 constants must originate from `boot_types.h`.

## Public API

```c
JPBootResult jp_uefi_initialize(...);
JPBootResult jp_uefi_get_memory_map(...);
JPBootResult jp_uefi_get_framebuffer(...);
JPBootResult jp_uefi_get_kernel_image(...);
JPBootResult jp_uefi_prepare_handoff(...);
JPBootResult jp_uefi_read_input(...);
```

## Internal API

```text
acquire_uefi_memory_map()
acquire_uefi_framebuffer()
acquire_uefi_kernel_image()
read_uefi_input()
acquire_final_uefi_memory_map()
prepare_uefi_handoff()
```

## Algorithm

### Memory Map

1. Query required buffer size.
2. Allocate buffer.
3. Retrieve memory map.
4. Preserve key.
5. Preserve descriptor size/version.
6. Normalize descriptors.
7. Validate normalized regions.
8. Store result.

### Framebuffer

1. Locate GOP.
2. Obtain active mode.
3. Read framebuffer base.
4. Read dimensions.
5. Read pitch.
6. Normalize format.
7. Validate result.

### Kernel

1. Locate configured kernel image.
2. Read image.
3. Record buffer and size.
4. Validate accessible storage.

### Input

1. Verify the provider state permits input.
2. Query the firmware input service without blocking.
3. Map the result to a normalized `JPBootInputEvent`; keys that cannot be mapped become `JP_BOOT_INPUT_NONE`.
4. Return `JP_BOOT_NOT_DETECTED` when no input device exists.

### Handoff

1. Complete all boot-services-dependent work.
2. Obtain final memory map.
3. Preserve final key.
4. Establish transition-ready state.
5. Perform `ExitBootServices` according to the handoff contract.
6. Transfer execution.

## How?

```text
UEFI
    ↓
ExitBootServices
    ↓
architecture-specific transition
    ↓
kernel
```

### Decided rules (where the kernel file is)

On a UEFI disk the kernel is the file `\EFI\JustOS\kernel.elf` on the same FAT file system from which the boot application itself was loaded (the file system of the device handle in the loaded-image protocol of the boot application). `jp_uefi_get_kernel_image()` reads that one path and no other; choosing between several kernels is not part of Milestone 0 (see Future Extensions). If the file does not exist, the result is `JP_BOOT_NOT_DETECTED` with the kernel image state `NOT_DETECTED`; if it exists but cannot be read completely, the result is `JP_BOOT_FAILED`. The boot application itself is `\EFI\BOOT\BOOTX64.EFI` (x86_64) or `\EFI\BOOT\BOOTAA64.EFI` (AArch64), the names that firmware looks for on removable media.

### Decided rules (the UEFI provider)

**State.** `JPUefiContext` (private) holds: the image handle, the system table, the boot-services table, the lifecycle state of the provider (the states drawn in `uefi.h`), the memory-map key, the descriptor size and version, a pool buffer for the raw map, and a `JPMemoryMap` for the final normalized map. The lifecycle is `UNINITIALIZED`, `INITIALIZED`, `ACTIVE`, `FINAL_MAP_ACQUIRED`, `EXIT_BOOT_SERVICES_READY`, `EXITED`; a failure is terminal.

**Memory normalization.** Every firmware descriptor is turned into a region by this table; then touching regions of the same type are merged into one (this is the only merging; regions are never reordered or dropped):

| Firmware memory type | Normalized type |
|---|---|
| Conventional memory | `USABLE` |
| Loader code, loader data | `RESERVED` (the boot program, the kernel, the stack, the page tables, the context) |
| Boot services code and data | `RESERVED` in a map taken while boot services run, `USABLE` in the final map taken for the transition (the firmware gives this memory back at the exit) |
| Runtime services code and data | `FIRMWARE` |
| ACPI reclaimable and ACPI NVS | `ACPI` |
| Memory-mapped I/O and I/O port space | `MMIO` |
| Unusable memory | `UNUSABLE` |
| Reserved, persistent, and every other type | `RESERVED` |

More than `JP_MAX_MEMORY_REGIONS` regions after merging is `JP_BOOT_ERROR_CORRUPTED_DATA`.

**Memory map operation.** `jp_uefi_get_memory_map()` in the lifecycle `INITIALIZED` or `ACTIVE` calls the firmware (`GetMemoryMap`, growing the pool buffer and trying again while the firmware says the buffer is too small, at most 4 times) and normalizes. In the lifecycle `EXITED` it returns the final map that was stored at the transition and calls no firmware service. In any other state it returns `JP_BOOT_NOT_INITIALIZED`.

**Framebuffer.** `LocateProtocol` finds the graphics output protocol (GOP); if there is none, the result is `JP_BOOT_NOT_DETECTED`. The current mode gives the base address, the width, the height, and the pixels per scanline; the pitch is `pixels per scanline * 4`. The pixel format `RedGreenBlueReserved8BitPerColor` is `JP_FRAMEBUFFER_FORMAT_RGB_888`, `BlueGreenRedReserved8BitPerColor` is `JP_FRAMEBUFFER_FORMAT_BGR_888`, and every other format (bit mask, blt only) is `JP_FRAMEBUFFER_FORMAT_UNKNOWN`, which the generic code rejects.

**Kernel image.** The loaded-image protocol of the boot application gives the device handle; the simple file system protocol of that handle opens the volume; `\EFI\JustOS\kernel.elf` is opened and read completely into a loader-data pool buffer whose size comes from the file information. A missing file is `JP_BOOT_NOT_DETECTED`; any other failure is `JP_BOOT_FAILED`.

**Input.** The simple text input protocol of the system table is used with `ReadKeyStroke`. "Not ready" means no event (`JP_BOOT_INPUT_NONE` with success). The scan codes 0x01, 0x02, 0x03, 0x04 are up, down, right, left; 0x17 is Escape; 0x14 is F10; the character 0x0D is Enter; the character 0x08 is Backspace; a printable character 0x20 to 0x7E is `JP_BOOT_INPUT_CHARACTER`; everything else is `JP_BOOT_INPUT_NONE`. No input protocol is `JP_BOOT_NOT_DETECTED`; a firmware error is `JP_BOOT_FAILED`.

**Reservation.** `jp_uefi_reserve_memory()` rounds the range to pages and calls `AllocatePages` with the type "at this address" and the memory type "loader data". Any firmware error is `JP_BOOT_FAILED`.

**Transition (`jp_uefi_prepare_handoff`).** (1) Get the memory map into the pool buffer (growing it as above) and remember the key; the lifecycle becomes `FINAL_MAP_ACQUIRED` (from now on input is refused). (2) Normalize it as a *final* map (boot services memory becomes `USABLE`) and store it. (3) The lifecycle becomes `EXIT_BOOT_SERVICES_READY`. (4) Call `ExitBootServices` with the image handle and the key. If the firmware answers that the key is not valid (the map changed since), get a fresh map and key, normalize again, and try once more; a second refusal is `JP_BOOT_FAILED`. A stale key is never used a second time. (5) On success the lifecycle becomes `EXITED`, the pointers to the firmware tables are cleared, and the provider calls no firmware service after this point (no console output either).

**Creating the provider context.** The structure `JPUefiContext` is private, so `uefi.h` also declares two functions that the entry point uses: `size_t jp_uefi_context_size(void)` returns the size in bytes that the context needs (the entry allocates that many bytes), and `JPUefiContext *jp_uefi_context_setup(void *memory, void *image_handle, void *system_table)` fills the zeroed memory with the image handle and the system table and returns the context (a null pointer for null arguments). Both handles are passed as opaque pointers, so no UEFI type appears in a header. A third function, `bool jp_uefi_boot_services_active(const JPUefiContext *context)`, tells the entry point whether the firmware services can still be used: it is true from the successful `jp_uefi_initialize()` until `ExitBootServices` has succeeded, and false before that and after it. `jp_uefi_initialize()` then checks that the system table carries the signature `IBI SYS` (the 64-bit value `0x5453595320494249`) and that the boot-services pointer is not null; otherwise the result is `JP_BOOT_ERROR_CORRUPTED_DATA` and the provider is `FAILED`. A call in any other lifecycle state than `UNINITIALIZED` is `JP_BOOT_FAILED`.

An empty kernel file is read successfully with size 0 (the generic code rejects a zero size). A file name that does not fit the information buffer (256 bytes) cannot happen for the fixed path and is `JP_BOOT_FAILED`.

The only file that may include `uefi_defs.h` is `uefi.c`, and `uefi_entry.c` for the entry symbol.

## State / Lifecycle

Provider state must correspond to actual UEFI operations.

## Ownership / Lifetime

Temporary buffers remain provider-owned until released.

## Invariants

No stale key.

No fake readiness.

No normalized state without successful acquisition.

No fabricated input event.

## Dependencies

UEFI firmware interfaces and `boot_types.h`.

## Boundary Rules

Only this module may directly depend on UEFI implementation interfaces.

## Failure Semantics

Any failed required UEFI operation prevents successful handoff. Input failures are reported with the normalized input results and never fabricate an event.

## Implementation Freedom

UEFI-specific internal control flow is free within the normative provider contract.

## Verification

Test both normal and failure paths, especially final memory-map changes and transition failure, and the input path (no input, event, no device, rejection after the final memory map is acquired).

---

# 6. `include/justos/boot/platform/bios.h`

## File

BIOS provider contract.

## Purpose

Defines the BIOS Stage 1 provider interface.

## Responsibilities

- BIOS initialization;
- memory-map acquisition;
- framebuffer acquisition;
- boot-device/kernel acquisition;
- non-blocking keyboard input before the platform transition;
- handoff preparation.

## Types / Structures

### `JPBiosContext`

Provider-owned BIOS state.

## Constants / Enums

BIOS/VBE provider constants only.

## Public API

```c
JPBootResult jp_bios_initialize(
    JPBiosContext *context
);

JPBootResult jp_bios_get_memory_map(
    JPBiosContext *context,
    JPMemoryMap *memory_map
);

JPBootResult jp_bios_get_framebuffer(
    JPBiosContext *context,
    JPFramebufferInfo *framebuffer
);

JPBootResult jp_bios_get_kernel_image(
    JPBiosContext *context,
    JPKernelImage *image
);

JPBootResult jp_bios_prepare_handoff(
    JPBiosContext *context
);

JPBootResult jp_bios_read_input(
    JPBiosContext *context,
    JPBootInputEvent *event
);
```

`jp_bios_read_input()` obtains non-blocking input from the PS/2 keyboard controller (BIOS keyboard services cannot be used in long mode; see the BIOS provider rules), valid only before the platform transition, and follows the Pre-Transition Interaction Point contract.

## Internal API

```text
acquire_bios_memory_map()
acquire_bios_framebuffer()
acquire_bios_kernel_image()
read_bios_input()
prepare_bios_handoff()
```

## Algorithm

The provider:

1. initializes BIOS state;
2. obtains supported memory information;
3. normalizes memory regions;
4. obtains supported framebuffer information;
5. acquires the kernel image;
6. validates provider state;
7. serves pending input requests through `jp_bios_read_input()` while BIOS services are valid, before handoff preparation;
8. prepares the CPU and memory state required by the Stage 1 handoff;
9. returns success only after actual preparation.

## State / Lifecycle

```text
UNINITIALIZED
    ↓
INITIALIZED
    ↓
ACTIVE
    ↓
HANDOFF_PREPARED
```

Input is permitted in `INITIALIZED` and `ACTIVE`. In `UNINITIALIZED` and from `HANDOFF_PREPARED` onward `jp_bios_read_input()` returns `JP_BOOT_NOT_INITIALIZED`.

## Ownership / Lifetime

BIOS provider state remains provider-owned throughout Stage 1.

Temporary provider buffers remain provider-owned until released.

## Invariants

- BIOS detection must be real.
- Normalized memory must be valid.
- Unsupported framebuffer modes must fail.
- Kernel acquisition failure must propagate.
- Handoff cannot succeed with missing required provider state.
- No input event is fabricated, and none is delivered after handoff preparation.

## Dependencies

BIOS/VBE interfaces and normalized Stage 1 types.

## Boundary Rules

BIOS-native structures must not escape the provider.

## Failure Semantics

Unsupported or unavailable provider functionality fails explicitly. Input returns `JP_BOOT_NOT_DETECTED` when no keyboard service exists, `JP_BOOT_NOT_INITIALIZED` when the provider state does not permit input, and `JP_BOOT_FAILED` on provider error.

## Implementation Freedom

BIOS interrupt/API mechanisms and provider-private data structures remain implementation-defined.

## Verification

Verify provider behavior against the same normalized Stage 1 contract required of UEFI, including the input path (no input, key, character, unmappable key, no keyboard service, rejection after handoff preparation).

---

# 7. `src/boot/platform/bios.c`

## File

BIOS provider implementation.

## Purpose

Implements the BIOS Stage 1 provider.

## Responsibilities

- BIOS boot initialization;
- memory-map acquisition;
- framebuffer acquisition;
- kernel image acquisition;
- non-blocking keyboard input;
- handoff preparation.

## Types / Structures

Provider-private state only.

## Constants / Enums

BIOS/VBE implementation constants may be private.

## Public API

```c
JPBootResult jp_bios_initialize(...);
JPBootResult jp_bios_get_memory_map(...);
JPBootResult jp_bios_get_framebuffer(...);
JPBootResult jp_bios_get_kernel_image(...);
JPBootResult jp_bios_prepare_handoff(...);
JPBootResult jp_bios_read_input(...);
```

## Internal API

```text
acquire_bios_memory_map()
acquire_bios_framebuffer()
acquire_bios_kernel_image()
read_bios_input()
prepare_bios_handoff()
```

## Algorithm

### Memory

1. Query supported BIOS memory information.
2. Normalize entries.
3. Validate ranges.
4. Publish normalized map.

### Framebuffer

1. Detect supported VBE mode.
2. Obtain framebuffer base.
3. Obtain dimensions.
4. Obtain pitch.
5. Determine supported format.
6. Normalize.
7. Validate.

### Kernel

1. Access boot device.
2. Read kernel image.
3. Record buffer and size.
4. Validate result.

### Input

1. Verify the provider state permits input.
2. Read the PS/2 keyboard controller without blocking (status port first, data port only when a byte waits).
3. Map the result to a normalized `JPBootInputEvent`; keys that cannot be mapped become `JP_BOOT_INPUT_NONE`.
4. Return `JP_BOOT_NOT_DETECTED` when no keyboard service exists.

### Handoff

1. Complete required provider operations.
2. Prepare CPU state required by the contract.
3. Prepare required memory state.
4. Prepare execution state.
5. Return success only after actual preparation.

### Decided rules (the BIOS disk image layout)

A BIOS disk has no file system in Milestone 0. The image is a fixed sequence of 512-byte sectors (LBA is the number of a sector, counted from 0), and every number below is stored in little-endian byte order:

| Sector (LBA) | Content |
|--------------|---------|
| 0 | The boot sector: exactly 512 bytes, ending with the bytes `0x55 0xAA` (the BIOS starts it). |
| 1 | The image header: the 8 ASCII bytes `JUSTBIOS`, then `uint32` version (1), `uint32` loader first sector (2), `uint32` loader sector count, `uint64` kernel first sector (2048), `uint64` kernel size in bytes. The rest of the sector is zero. |
| 2 up to 2047 | The loader (the code that the boot sector loads and starts), `loader sector count` sectors. |
| 2048 and on | The kernel file (`kernel.elf`), `kernel size` bytes. |

The boot sector reads the header and the loader with the BIOS disk service (interrupt 13h), checks the magic text and the version, and starts the loader. The BIOS provider finds the kernel by the header: `jp_bios_get_kernel_image()` reads `kernel size` bytes from sector 2048 on. A wrong magic text, an unknown version, a zero kernel size, or a loader that does not fit before sector 2048 is `JP_BOOT_ERROR_CORRUPTED_DATA`. `scripts/make_image.sh x86_64-bios` builds this layout from a boot sector file, a loader file, and a kernel file.

### Decided rules (the BIOS provider and its boot chain)

BIOS services (memory map, video, disk, keyboard) can be called only in real mode, but the generic Stage 1 code is 64-bit code. Therefore a 16-bit **loader** does all BIOS calls first, writes the results into a **boot data block** in low memory, switches the processor to long mode, and starts 64-bit code (`jp_bios_main`). The BIOS provider functions only read that block; they call no BIOS service. The boot chain is: boot sector (`bios_boot_sector.S`) -> loader (`bios_loader.S`) -> `jp_bios_main` (`bios_entry.c`).

**Fixed physical addresses** (the first 8 MiB belong to the boot environment, so the BIOS path needs at least 16 MiB of memory; with less the loader stops with an error text):

| Address | Size | Content |
|---|---|---|
| `0x00007C00` | 512 B | the boot sector (placed there by the BIOS) |
| `0x00008000` | up to 56 KiB | the loader |
| `0x00010000` | 16 KiB | the boot data block (`bios_block.h`) |
| `0x00020000` | 24 KiB | the loader's page tables (identity map of the first 4 GiB with 2 MiB pages) |
| `0x00030000` | 4 KiB | the loader's GDT |
| `0x00031000` | 28 KiB | the stack of `jp_bios_main` (its top is `0x00038000`) |
| `0x00200000` | up to 2 MiB | the kernel file as the loader read it (the staging area) |
| `0x00400000` | 1 MiB | the `JPBootContext` |
| `0x00500000` | 64 KiB | the `JPBiosContext` |

**Memory map.** `jp_bios_get_memory_map()` takes the entries that the loader recorded (the BIOS function `INT 15h, EAX = E820h`), and normalizes them: type 1 is `USABLE`, 2 is `RESERVED`, 3 and 4 are `ACPI`, 5 is `UNUSABLE`, everything else is `RESERVED`. Then it marks the ranges of the table above (from `0x00000000` up to `0x00800000`, everything from `0x100000000` (4 GiB) upward, plus every range reserved through `jp_bios_reserve_memory()`) as `RESERVED`, by cutting them out of usable regions (a usable region may split into two). Memory from 4 GiB upward is cut out because the loader maps only the first 4 GiB (its page tables identity-map 4 GiB with 2 MiB pages), so code that touched that memory would fault; the kernel therefore does not see it in the BIOS path (a known limitation of Milestone 0). More than `JP_MAX_MEMORY_REGIONS` regions is `JP_BOOT_ERROR_CORRUPTED_DATA`.

**Framebuffer.** The loader chose a video mode (see below) and wrote it into the block. `jp_bios_get_framebuffer()` copies it: the base address, the width, the height, the pitch, and the format. No mode (the block says "no mode") is `JP_BOOT_NOT_DETECTED`.

**Kernel image.** The loader read `kernel.elf` into the staging area. `jp_bios_get_kernel_image()` returns that address and the size from the block; a block that says the kernel was not read is `JP_BOOT_NOT_DETECTED`.

**Input.** BIOS keyboard services cannot be used in long mode, so `jp_bios_read_input()` reads the PS/2 keyboard controller directly (status port `0x64`, data port `0x60`, scan code set 1). A byte is waiting when bit 0 of the status is set. Make codes (key pressed) are mapped: `0x48` up, `0x50` down, `0x4D` right, `0x4B` left, `0x1C` Enter, `0x01` Escape, `0x44` F10, `0x0E` Backspace, and the keys of the letter and digit rows to their lower-case ASCII character; break codes (key released, bit 7 set) and everything else give `JP_BOOT_INPUT_NONE`. No byte waiting is also `JP_BOOT_INPUT_NONE` with success. Two details of the controller: if bit 5 of the status is set the waiting byte comes from the mouse; it is read (so that it does not block the controller) and gives `JP_BOOT_INPUT_NONE`. If the status port reads `0xFF` (a bus that nothing drives, so no controller exists), the result is `JP_BOOT_NOT_DETECTED`. The extended-key prefix byte `0xE0` is an unmappable byte (`JP_BOOT_INPUT_NONE`), and the byte that follows it is mapped like any other make code.

**Reservation.** `jp_bios_reserve_memory()` records the rounded range in a private table of at most 64 ranges (a full table is `JP_BOOT_FAILED`) and checks that it lies inside a usable region of the memory map (otherwise `JP_BOOT_FAILED`).

**Transition (`jp_bios_prepare_handoff`).** There is no firmware to leave. It builds the final memory map (the table above applied), stores it for `jp_bios_get_memory_map()` in the lifecycle `HANDOFF_PREPARED`, and returns success only after that.

**The loader (`bios_loader.S`).** In real mode it: (1) enables the A20 line (a processor line that gives access to memory above 1 MiB) with the fast method, port `0x92`; (2) collects the E820 entries (at most 128, 24 bytes each) into the block; (3) chooses a video mode with the VBE functions (`INT 10h`, `AX = 4F00h`, `4F01h`, `4F02h`): a 32-bit-per-pixel mode with a linear framebuffer, 1024 x 768 if it exists, otherwise the largest such mode up to 1920 x 1080; it writes base address, width, height, pitch, and format into the block (or "no mode"); (4) reads `kernel.elf` from the disk image with the extended disk read (`INT 13h`, `AH = 42h`) in chunks of 64 sectors into a low buffer and copies each chunk to the staging area with `INT 15h, AH = 87h`; it writes the staging address and the size into the block (the kernel must be at most 2 MiB; larger means an error text); (5) builds the page tables and the GDT (addresses above), enables PAE and long mode (`CR4.PAE`, `EFER.LME`, `CR0.PG`), loads the stack pointer with the top of the 64-bit stack, and jumps to `jp_bios_main` with the address of the block in `RDI`. At its very start the loader initializes the serial port COM1 (115200 baud, 8 data bits, no parity, one stop bit) and writes the character `L` followed by a carriage return and a line feed, so that the lines of the test kernel that come later start at the beginning of a line (the Q-level tests use this character to know that the loader started). After a real-mode error the loader prints a short text, starting with `JustOS loader: `, to the screen with `INT 10h, AH = 0Eh` and to the serial port, and halts.

**Where the 64-bit code lives (decided).** The loader file is one flat binary that the boot sector reads to `0x8000` and starts at its first byte. It holds, one after the other, the 16-bit loader code and then all the 64-bit code (`bios_entry.c`, `bios.c`, and the generic Stage 1 modules with `x86_64.c`). The 64-bit code is linked for the address where it lies after loading (it runs in place; there is no copying), so it must not contain any writable static data (the project forbids it anyway). The whole file must fit in 56 KiB (`0x8000` up to the block at `0x10000`); `scripts/make_image.sh` refuses a bigger loader. The 16-bit part is padded with zeros to exactly 4 KiB, so the 64-bit part starts at the fixed address `0x9000`, and the linker script puts `jp_bios_main` first in it; the loader jumps to `0x9000` in 64-bit mode.

**The private port access.** `jp_bios_read_input()` must read a processor I/O port, which C cannot do. The private header `src/boot/platform/bios_io.h` declares `uint8_t jp_bios_port_read8(uint16_t port)`; `bios_entry.c` implements it with the instruction `inb`, and the host test replaces it with a fake that returns scripted bytes. So `bios.c` itself is plain C and runs in the host test.

**Creating the provider context.** `bios.h` also declares `size_t jp_bios_context_size(void)` and `JPBiosContext *jp_bios_context_setup(void *memory, const void *block)` (the memory is zeroed by the caller; the function stores the block pointer and returns the context; a null pointer for null arguments). `jp_bios_initialize()` checks the block (magic text, version, `e820_count` at most `JP_BIOS_BLOCK_MAX_E820`; otherwise `JP_BOOT_ERROR_CORRUPTED_DATA`; the flag `JP_BIOS_BLOCK_FLAG_A20` must be set, otherwise `JP_BOOT_FAILED`) and moves the provider from `UNINITIALIZED` to `INITIALIZED`; a call in any other state is `JP_BOOT_FAILED`.

**The boot sector (`bios_boot_sector.S`).** It is exactly 512 bytes ending with `0x55 0xAA`. It saves the boot drive number (given by the BIOS in `DL`), reads sector 1 (the image header) to `0x7E00` with the extended disk read, checks the text `JUSTBIOS` and version 1, reads the loader (`loader sector count` sectors from the header) to `0x8000`, and jumps to `0x8000` with the drive number in `DL`. A failure prints one character `!` and halts.

## State / Lifecycle

Provider state must correspond to actual operations.

## Ownership / Lifetime

Provider-private state remains valid through handoff preparation.

## Invariants

No fake BIOS state.

No unsupported format accepted.

No incomplete kernel acquisition accepted.

No fabricated input event.

## Dependencies

BIOS/VBE interfaces and normalized Stage 1 types.

## Boundary Rules

This is the only Stage 1 source file permitted to directly depend on BIOS/VBE implementation interfaces.

## Failure Semantics

Provider failures propagate to generic Stage 1. Input failures are reported with the normalized input results.

## Implementation Freedom

Internal BIOS mechanisms remain implementation-defined.

## Verification

Verify the provider against the normalized UEFI-equivalent Stage 1 behavior, including input.

---

# 8. `include/justos/boot/hardware.h`

## File

Hardware discovery contract.

## Purpose

Defines the public hardware discovery interface.

## Responsibilities

- CPU discovery;
- firmware state discovery;
- boot-device discovery;
- normalization into `JPHardwareInfo`.

## Types / Structures

None.

## Constants / Enums

None.

## Public API

```c
JPBootResult jp_boot_discover_hardware(
    JPBootContext *context
);
```

## Internal API

None.

## Algorithm

Defined by the implementation contract.

The header exposes the public operation only.

## State / Lifecycle

Hardware state transitions from uninitialized to actual discovery result.

## Ownership / Lifetime

`JPBootContext` remains caller-owned.

Hardware information is stored in `context->hardware`.

## Invariants

No hardware component may be reported as detected without actual discovery.

## Dependencies

```c
#include "justos/boot/boot_types.h"
```

## Boundary Rules

No provider-native hardware types are exposed.

## Failure Semantics

Failed discovery propagates.

## Implementation Freedom

Discovery mechanisms remain implementation-defined.

## Verification

Verify CPU, firmware, and boot-device discovery independently and in aggregate.

---

# 9. `src/boot/hardware.c`

## File

Hardware discovery implementation.

## Purpose

Implements hardware discovery using the active boot environment and architecture contract.

## Responsibilities

- discover CPU;
- discover firmware environment;
- discover boot device;
- aggregate hardware state.

## Types / Structures

None.

## Constants / Enums

None.

## Public API

```c
JPBootResult jp_boot_discover_hardware(
    JPBootContext *context
);
```

## Internal API

```text
discover_cpu()
discover_firmware()
discover_boot_device()
aggregate_hardware_state()
```

## contract

```text
boot_environment
    ↓
authoritative environment selection
    ↓
hardware.firmware.type
```

## Algorithm

1. Validate context.
2. Set hardware discovery stage.
3. Discover CPU.
4. Discover firmware.
5. Discover boot device.
6. Read provider results.
7. Aggregate state.
8. Store normalized hardware state.
9. Return actual result.

State aggregation precedence:

```text
FAILED
↓
UNKNOWN
↓
NOT_INITIALIZED
↓
NOT_DETECTED
↓
DETECTED
```

### Decided rules (hardware discovery)

- The caller (the platform entry point) sets `context->handoff.architecture` to `JP_ARCH_X86_64` or `JP_ARCH_AARCH64` before discovery, in the same way as it sets `environment.type`. If it is `JP_ARCH_UNKNOWN`, discovery returns `JP_BOOT_ERROR_INVALID_PARAM`. A null context also returns `JP_BOOT_ERROR_INVALID_PARAM`.
- CPU: discovery calls `jp_x86_64_discover_cpu()` or `jp_aarch64_discover_cpu()` (declared in the architecture headers) and stores the result in `hardware.cpu`. If the operation fails, `cpu.state` becomes `JP_DETECTION_STATE_FAILED`, `overall_state` becomes `FAILED`, and the failing result of the operation is returned unchanged.
- Firmware: `firmware.type` is a copy of `environment.type`. `firmware.state` is `DETECTED` only if `environment.state` is `DETECTED`; otherwise it is `NOT_INITIALIZED`. Discovery never selects a firmware by itself.
- Boot device: the provider is the source of the kernel image, so `boot_device.state` is `DETECTED` only if `environment.state` is `DETECTED`; otherwise it is `NOT_INITIALIZED`.
- `overall_state` is the first state that applies from the precedence list above, taken over the states of cpu, firmware, and boot device.
- The result is `JP_BOOT_SUCCESS` only if `overall_state` is `DETECTED`. Otherwise it is `JP_BOOT_FAILED` (for `FAILED`), `JP_BOOT_UNKNOWN`, `JP_BOOT_NOT_INITIALIZED`, or `JP_BOOT_NOT_DETECTED`, matching `overall_state`.
- `current_stage` is set to `JP_BOOT_STAGE_HARDWARE_DISCOVERY` at the start.

## State / Lifecycle

```text
NOT_INITIALIZED
    ↓
DISCOVERY
    ↓
DETECTED / FAILED
```

## Ownership / Lifetime

Hardware data belongs to `JPBootContext`.

## Invariants

- Firmware detection must correspond to the selected provider.
- CPU discovery must not silently perform another firmware-selection operation.
- Aggregate state must reflect component states.

## Dependencies

```c
#include "justos/boot/hardware.h"
```

## Boundary Rules

No direct UEFI/BIOS logic.

## Failure Semantics

Any mandatory discovery failure prevents successful hardware discovery.

## Implementation Freedom

Architecture-specific CPU instructions may be used internally.

## Verification

Verify real discovery and failure propagation.

---

# 10. `include/justos/boot/memory_map.h`

## File

Memory-map contract.

## Purpose

Defines the public normalized memory-map interface.

## Responsibilities

- acquire memory map;
- validate normalized regions;
- expose memory state to later Stage 1 operations.

## Types / Structures

Uses:

- `JPMemoryMap`;
- `JPMemoryRegion`.

## Constants / Enums

Uses `JPMemoryRegionType`.

## Public API

```c
JPBootResult jp_boot_discover_memory(
    JPBootContext *context
);

JPBootResult jp_boot_validate_memory_map(
    JPBootContext *context
);
```

## Internal API

None.

## Algorithm

Defined by `memory_map.c`.

## State / Lifecycle

```text
NOT_INITIALIZED
    ↓
ACQUIRED
    ↓
VALIDATED
```

Failure prevents validation success.

## Ownership / Lifetime

Normalized memory-map storage belongs to `JPBootContext`.

## Invariants

- region count is within capacity;
- base + length does not overflow;
- required region types are valid;
- overlap policy is respected;
- no silent overlap reinterpretation occurs.

## Dependencies

```c
#include "justos/boot/boot_types.h"
```

## Boundary Rules

Provider-native memory descriptors must not escape.

## Failure Semantics

Malformed or unavailable memory information fails explicitly.

## Implementation Freedom

Sorting, temporary buffers, and normalization algorithms remain implementation-defined where contract semantics are unchanged.

## Verification

Verify valid maps, malformed maps, overflow, overlap, and unavailable memory.

---

# 11. `src/boot/memory_map.c`

## File

Memory-map acquisition and validation implementation.

## Purpose

Implements normalized memory-map acquisition and validation.

## Responsibilities

- obtain platform memory map;
- normalize descriptors;
- validate region ranges;
- validate region count;
- enforce overlap semantics.

## Types / Structures

None.

## Constants / Enums

Generic constants come from `boot_types.h`.

## Public API

```c
JPBootResult jp_boot_discover_memory(
    JPBootContext *context
);

JPBootResult jp_boot_validate_memory_map(
    JPBootContext *context
);
```

## Internal API

```text
acquire_memory_map()
validate_memory_region()
validate_memory_layout()
validate_memory_overflow()
validate_memory_overlap()
```

## Algorithm

### Acquisition

1. Validate context.
2. Set memory discovery stage.
3. Request provider memory map.
4. Normalize entries.
5. Reject excessive region count.
6. Store normalized regions.
7. Return actual result.

### Validation

1. Validate context.
2. Require acquired memory map.
3. Validate region count.
4. Validate every region.
5. Check base + length overflow.
6. Check overlap according to the contract.
7. Return success only when all required invariants hold.

### Decided rules (memory map)

The enumeration `JPDetectionState` has no separate "acquired" and "validated" values, so the state of the map is stored as follows: `NOT_INITIALIZED` before acquisition, `DETECTED` after a successful acquisition, `FAILED` after a failed acquisition or a failed validation. "Validated" is the successful result of `jp_boot_validate_memory_map()`; it does not change the state.

Acquisition (`jp_boot_discover_memory`): a null context returns `JP_BOOT_ERROR_INVALID_PARAM`; `current_stage` becomes `JP_BOOT_STAGE_MEMORY_DISCOVERY`; the map is requested through `jp_boot_environment_get_memory_map()`; if that fails, the result is returned unchanged and `memory_map.state` becomes `FAILED`; a `region_count` above `JP_MAX_MEMORY_REGIONS` returns `JP_BOOT_ERROR_CORRUPTED_DATA` and sets `FAILED`; on success `memory_map.state` becomes `DETECTED`. The provider's order of regions is kept.

Validation (`jp_boot_validate_memory_map`): a null context returns `JP_BOOT_ERROR_INVALID_PARAM`; a map whose state is not `DETECTED` returns `JP_BOOT_NOT_INITIALIZED`. The following return `JP_BOOT_ERROR_CORRUPTED_DATA` and set the state to `FAILED`: `region_count` is 0 or above `JP_MAX_MEMORY_REGIONS`; a region has `length == 0`; a region has type `JP_MEMORY_REGION_UNKNOWN` or a value outside `JPMemoryRegionType`; `base_address + length` overflows `uint64_t`; two regions overlap (regions that only touch, where one ends exactly where the next begins, are allowed); there is no region of type `JP_MEMORY_REGION_USABLE`. Validation never merges, reorders, or changes regions.

## State / Lifecycle

The memory state reflects actual acquisition and validation.

## Ownership / Lifetime

Normalized map remains in `JPBootContext`.

Provider-native descriptors remain provider-owned.

## Invariants

No valid state before actual acquisition.

No silent merging or reinterpretation of overlapping regions.

## Dependencies

```c
#include "justos/boot/memory_map.h"
```

## Boundary Rules

Platform acquisition occurs through the boot-environment contract.

## Failure Semantics

Any invalid mandatory region prevents successful validation.

## Implementation Freedom

Normalization algorithms remain free if normalized semantics remain identical.

## Verification

Verify overflow, overlap, invalid type, invalid length, and maximum region count.

---

# 12. `include/justos/boot/framebuffer.h`

## File

Framebuffer contract.

## Purpose

Defines the normalized framebuffer interface.

## Responsibilities

- acquire framebuffer information;
- validate framebuffer parameters;
- expose normalized pixel representation.

## Types / Structures

Uses `JPFramebufferInfo`.

## Constants / Enums

Uses `JPFramebufferFormat`.

## Public API

```c
JPBootResult jp_boot_initialize_framebuffer(
    JPBootContext *context
);

JPBootResult jp_boot_validate_framebuffer(
    JPBootContext *context
);
```

## Internal API

None.

## Algorithm

Defined by `framebuffer.c`.

## State / Lifecycle

```text
NOT_INITIALIZED
    ↓
ACQUIRED
    ↓
VALIDATED
```

## Ownership / Lifetime

Framebuffer memory ownership remains defined by the provider and handoff contract.

## Invariants

- base address valid;
- width non-zero;
- height non-zero;
- pitch valid;
- format supported;
- normalized memory representation unambiguous.

## Dependencies

```c
#include "justos/boot/boot_types.h"
```

## Boundary Rules

Provider-specific framebuffer formats must be normalized before generic consumption.

## Failure Semantics

Unsupported formats fail explicitly.

## Implementation Freedom

Provider-specific format conversion remains implementation-defined if normalized semantics are preserved.

## Verification

Verify RGB/BGR representations, pitch semantics, dimensions, and unsupported formats.

---

# 13. `src/boot/framebuffer.c`

## File

Framebuffer implementation.

## Purpose

Acquires and validates the normalized framebuffer.

## Responsibilities

- request framebuffer from provider;
- normalize format;
- validate dimensions;
- validate pitch;
- publish state.

## Types / Structures

None.

## Constants / Enums

No duplicated framebuffer contract values.

## Public API

```c
JPBootResult jp_boot_initialize_framebuffer(
    JPBootContext *context
);

JPBootResult jp_boot_validate_framebuffer(
    JPBootContext *context
);
```

## Internal API

```text
acquire_framebuffer()
normalize_framebuffer_format()
validate_framebuffer()
```

## Algorithm

1. Validate context.
2. Set framebuffer stage.
3. Request provider framebuffer.
4. Normalize format.
5. Validate base address.
6. Validate dimensions.
7. Validate pitch.
8. Validate format.
9. Publish state.

### Decided rules (framebuffer)

`JP_FRAMEBUFFER_FORMAT_RGB_888` and `JP_FRAMEBUFFER_FORMAT_BGR_888` both use 4 bytes per pixel (three 8-bit color channels and one unused byte); this is the layout that UEFI GOP provides.

Acquisition (`jp_boot_initialize_framebuffer`): a null context returns `JP_BOOT_ERROR_INVALID_PARAM`; `current_stage` becomes `JP_BOOT_STAGE_FRAMEBUFFER_INIT`; the framebuffer is requested through `jp_boot_environment_get_framebuffer()`. If that fails, the result is returned unchanged; `framebuffer.state` becomes `NOT_DETECTED` when the result was `JP_BOOT_NOT_DETECTED` (a machine without a framebuffer is a valid situation) and `FAILED` otherwise. If the provider reports format `JP_FRAMEBUFFER_FORMAT_UNKNOWN`, the result is `JP_BOOT_FAILED` and the state `FAILED`. Then the values are validated as below; on success the state becomes `DETECTED`.

Validation (`jp_boot_validate_framebuffer`, and the same checks inside acquisition): a null context returns `JP_BOOT_ERROR_INVALID_PARAM`; a state other than `DETECTED` returns `JP_BOOT_NOT_INITIALIZED`. These return `JP_BOOT_ERROR_CORRUPTED_DATA` and set `FAILED`: `base_address == 0`; `width == 0` or `height == 0`; format `UNKNOWN`; `pitch < width * 4`; `pitch` is not a multiple of 4. The product `width * 4` is computed in 64 bits so that it cannot wrap around.

## State / Lifecycle

State only becomes detected after real framebuffer acquisition.

## Ownership / Lifetime

Framebuffer memory ownership remains provider-defined until the handoff contract transfers or preserves it.

## Invariants

No framebuffer success without actual provider result.

## Dependencies

```c
#include "justos/boot/framebuffer.h"
```

## Boundary Rules

No direct UEFI GOP or BIOS/VBE access.

## Failure Semantics

Provider failure or unsupported format prevents successful initialization.

## Implementation Freedom

Format normalization implementation remains free.

## Verification

Verify real framebuffer data and invalid configurations.

---

# 14. `include/justos/boot/kernel_image.h`

## File

Kernel image acquisition contract.

## Purpose

Defines the normalized kernel image acquisition interface.

## Responsibilities

- acquire kernel image;
- validate image storage;
- preserve image lifetime through loading.

## Types / Structures

Uses `JPKernelImage`.

## Constants / Enums

None.

## Public API

```c
JPBootResult jp_boot_acquire_kernel_image(
    JPBootContext *context
);
```

## Internal API

None.

## Algorithm

Defined by implementation.

## State / Lifecycle

```text
NOT_INITIALIZED
    ↓
ACQUIRED
    ↓
VALIDATED
```

## Ownership / Lifetime

The image remains valid through all Stage 1 operations requiring its contents.

## Invariants

- buffer is accessible;
- size is non-zero;
- state reflects actual acquisition.

## Dependencies

```c
#include "justos/boot/boot_types.h"
```

## Boundary Rules

Kernel acquisition is provider-specific but normalized before generic consumption.

## Failure Semantics

Incomplete or invalid acquisition fails.

## Implementation Freedom

Storage mechanism remains provider-defined.

## Verification

Verify valid image, unavailable image, truncated image, and invalid storage.

---

# 15. `src/boot/kernel_image.c`

## File

Kernel image acquisition implementation.

## Purpose

Acquires the kernel image through the active provider.

## Responsibilities

- request kernel image;
- validate returned storage;
- preserve image lifetime.

## Types / Structures

None.

## Constants / Enums

None.

## Public API

```c
JPBootResult jp_boot_acquire_kernel_image(
    JPBootContext *context
);
```

## Internal API

```text
acquire_kernel_image()
validate_kernel_image_buffer()
```

## Algorithm

1. Validate context.
2. Set kernel acquisition stage.
3. Request kernel image from provider.
4. Validate buffer.
5. Validate size.
6. Store image.
7. Return actual result.

### Decided rules (kernel image)

`jp_boot_acquire_kernel_image`: a null context returns `JP_BOOT_ERROR_INVALID_PARAM`; `current_stage` becomes `JP_BOOT_STAGE_KERNEL_IMAGE_ACQUISITION`; the image is requested through `jp_boot_environment_get_kernel_image()`. If that fails, the result is returned unchanged and `kernel.image.state` becomes `NOT_DETECTED` for `JP_BOOT_NOT_DETECTED` and `FAILED` otherwise. A `NULL` buffer or `size == 0` returns `JP_BOOT_ERROR_CORRUPTED_DATA` and sets `FAILED`. On success `kernel.image.state` becomes `DETECTED`, and `kernel.validation_state` and `kernel.load_state` become `NOT_INITIALIZED`.

## State / Lifecycle

Image state reflects actual acquisition.

## Ownership / Lifetime

Image storage remains valid until kernel loading no longer requires it.

## Invariants

No acquired state without accessible image storage.

## Dependencies

```c
#include "justos/boot/kernel_image.h"
```

## Boundary Rules

No direct UEFI/BIOS image-loading logic.

## Failure Semantics

Acquisition or validation failure propagates.

## Implementation Freedom

File/device reading mechanisms remain provider-defined.

## Verification

Verify actual kernel image acquisition and lifetime.

---

# 16. `include/justos/boot/kernel_loader.h`

## File

Public Stage 1 kernel-loader interface.

## Purpose

Declares the generic ELF validation and kernel-loading operations used by Stage 1. The header exposes the normalized loader contract and contains no provider-specific firmware structures or implementation logic.

## Responsibilities

- declare kernel-image validation;
- declare kernel loading and mapping;
- expose the shared Stage 1 result/context types;
- preserve the boundary between generic loading and platform providers.

## Public API

```c
JPBootResult jp_boot_validate_kernel(
    JPBootContext *context
);

JPBootResult jp_boot_load_kernel(
    JPBootContext *context
);
```

## Dependencies

```c
#include "justos/boot/boot_types.h"
```

## Boundary Rules

The header must not expose UEFI, BIOS, provider-native image structures, page-table internals, or `JPBootInfo`.

## Failure Semantics

The declared operations propagate validation, loading, mapping, permission, allocation, and invalid-parameter failures through `JPBootResult`.

## Implementation Freedom

Parsing, allocation, copying, mapping, and permission mechanisms remain implementation-defined within the normative Stage 1 contract.

## Verification

Verify that the header declarations match `src/boot/kernel_loader.c` and that the header compiles without provider-specific dependencies.

---

# 17. `src/boot/kernel_loader.c`

## File

Generic ELF64 kernel validation and loading implementation.

## Purpose

Implements the complete Stage 1 kernel loading pipeline.

## Responsibilities

- ELF header parsing;
- ELF validation;
- program-header validation;
- `PT_LOAD` discovery;
- segment validation;
- entry-point validation;
- segment allocation/reservation;
- segment loading;
- zero-fill;
- mapping;
- permissions.

## Types / Structures

Implementation-local parsing structures may be used.

No provider-native structures.

## Constants / Enums

Generic ELF constants originate from `boot_types.h`.

## Public API

```c
JPBootResult jp_boot_validate_kernel(
    JPBootContext *context
);

JPBootResult jp_boot_load_kernel(
    JPBootContext *context
);
```

## Internal API

```text
validate_elf_header()
parse_elf_image()
validate_elf_segments()
validate_kernel_entry()
allocate_kernel_segment_memory()
load_elf_segment()
zero_kernel_segment_tail()
map_kernel_segment()
apply_kernel_segment_permissions()
```

## Algorithm

### contract

```text
ELF segment flags
        ↓
architecture memory-permission abstraction
        ↓
actual page-table attributes
```

### mechanics

```text
PF_X → executable
PF_W → writable
PF_R → readable
W + X simultaneously
```

### ELF validation

1. Validate context.
2. Set kernel validation stage.
3. Require acquired image.
4. Validate ELF identification/header bounds.
5. Validate magic.
6. Validate ELF64
7. Validate target architecture
   8.Validate supported ELF type
8. Validate program-header table.
9. Parse program headers.
10. Select `PT_LOAD`.
11. Reject zero loadable segments.
12. Validate segment count.
13. Validate file ranges.
14. Validate memory ranges.
15. Validate alignment.
16. Validate flags.
17. Validate entry point.
18. Store normalized metadata.
19. Set validation state to `VALID`.

### Decided rules (ELF validation)

Which architecture is meant: `context->handoff.architecture` (set by the caller; see the hardware rules). The machine check is delegated to `jp_x86_64_validate_elf_machine()` or `jp_aarch64_validate_elf_machine()`, and the failing result of that call is returned unchanged.

`jp_boot_validate_kernel` returns `JP_BOOT_ERROR_INVALID_PARAM` for a null context or an unknown architecture, and `JP_BOOT_NOT_INITIALIZED` if `kernel.image.state` is not `DETECTED`. Every rule below that is violated returns `JP_BOOT_ERROR_CORRUPTED_DATA`, sets `kernel.validation_state` to `JP_KERNEL_VALIDATION_INVALID`, and leaves `segment_count` at 0 (nothing is stored from an invalid image). The rules:

1. The image is at least 64 bytes long (the ELF header size).
2. Magic is `JP_ELF_MAGIC`; class is `JP_ELF_CLASS_64`; the machine matches the architecture.
3. Type is `JP_ELF_TYPE_EXEC`. `JP_ELF_TYPE_DYN` is rejected, because Milestone 0 has no load bias, and it is never treated as `ET_EXEC`.
4. The program-header entry size is 56, and the table `[program_header_offset, program_header_offset + count * 56)` lies inside the image without overflow.
5. Entries of type `JP_ELF_PT_LOAD` are selected; other entries are ignored. There is at least one and at most `JP_MAX_KERNEL_SEGMENTS`.
6. For every loadable segment: `file_size <= memory_size`; `file_offset + file_size` lies inside the image without overflow; `virtual_address + memory_size` does not overflow; `virtual_address` is a multiple of 4096; `file_offset mod 4096` equals `virtual_address mod 4096`; the flags contain at least one of read (4), write (2), execute (1) and no other bit; write and execute are not both set.
7. The entry point lies inside `[virtual_address, virtual_address + memory_size)` of a loadable segment whose flags contain execute.

On success: the result is `JP_BOOT_SUCCESS`, `validation_state` is `VALID`, and `kernel.header`, `kernel.segments` (in table order, with `physical_address` 0 until loading and `state` `JP_KERNEL_SEGMENT_VALIDATED`), `segment_count` and `kernel.entry_point` are filled. `current_stage` is set to `JP_BOOT_STAGE_KERNEL_VALIDATION` at the start.

### Decided rules (physical memory, copying, mapping)

Three constants join the Stage 1 constants in `boot_types.h`: `JP_BOOT_PAGE_SIZE` (4096), `JP_BOOT_PROTECTED_LOW_MEMORY` (0x100000, the first 1 MiB), and `JP_BOOT_KERNEL_STACK_SIZE` (16384).

Two operations join each architecture header (they are the machine-dependent part of loading):

```c
JPBootResult jp_<arch>_map_segment(JPBootContext *context, size_t index);
void *jp_<arch>_phys_access(JPBootContext *context, uint64_t physical, uint64_t length);
```

`jp_<arch>_map_segment()` creates the mapping of `kernel.segments[index]`: the virtual range of the segment onto its `physical_address`, with the access rights of its flags (read, write, execute). It returns `JP_BOOT_SUCCESS` only after the mapping really exists. `jp_<arch>_phys_access()` returns a pointer through which the bytes of the physical range `[physical, physical + length)` can be read and written now, or `NULL` if that range cannot be accessed.

`jp_boot_load_kernel` returns `JP_BOOT_ERROR_INVALID_PARAM` for a null context or an unknown architecture, and `JP_BOOT_NOT_INITIALIZED` if `kernel.validation_state` is not `VALID` or `memory_map.state` is not `DETECTED`. `current_stage` becomes `JP_BOOT_STAGE_KERNEL_LOADING`. Then every segment is handled in table order, one after another, in these three steps:

1. **Place it and reserve it.** Memory comes from regions of type `JP_MEMORY_REGION_USABLE`, taken in map order. In a region the first fitting address is used: the start is the larger of the region start, `JP_BOOT_PROTECTED_LOW_MEMORY`, and the end of the segment placed just before it in the same region, rounded up to `JP_BOOT_PAGE_SIZE`. The range must lie completely inside the region and must not touch the bytes of the kernel image itself (`image.buffer` up to `image.buffer + image.size`; if the start falls inside it, the start moves to the end of the image, rounded up). The chosen address is stored in `segments[i].physical_address`. If no region fits, the result is `JP_BOOT_FAILED`. Then the range is reserved with `jp_boot_environment_reserve_memory(physical_address, memory_size)`; a failing reservation gives `JP_BOOT_FAILED` (the range is reserved before anything is copied into it, so the firmware cannot give it to anyone else).
2. **Copy it.** The destination is obtained with `jp_<arch>_phys_access(physical_address, memory_size)`; `NULL` gives `JP_BOOT_FAILED`. Exactly `file_size` bytes from `image.buffer + file_offset` are copied, and the bytes from `file_size` up to `memory_size` are set to zero.
3. **Map it.** `jp_<arch>_map_segment()` is called; a failing result is returned unchanged.

The state of every segment follows `VALIDATED` -> `ALLOCATED` (step 1 done) -> `LOADED` (step 2 done) -> `MAPPED` -> `PERMISSIONED` (step 3 done; `jp_<arch>_map_segment()` creates the mapping together with its access rights, so after one successful call the segment is both mapped and permissioned, and the loader writes `PERMISSIONED`). A segment must be in the state `VALIDATED` when it is handled; otherwise `jp_boot_load_kernel` returns `JP_BOOT_NOT_INITIALIZED` before it changes anything. If a step fails for a segment, that segment becomes `FAILED`, the segments before it keep their states, and the segments after it stay `VALIDATED`.

If any step fails, `load_state` becomes `JP_KERNEL_LOAD_FAILED`, the result is returned as described, and the next segments are not handled. When all segments are done, every segment is `PERMISSIONED`, `load_state` becomes `JP_KERNEL_LOAD_LOADED`, and the result is `JP_BOOT_SUCCESS`.

### Segment allocation

For every loadable segment:

1. Determine destination range.
2. Check overflow.
3. Check overlap with protected boot memory.
4. Allocate or reserve physical memory.
5. Store physical address.
6. Mark segment allocated.

### Segment loading

1. Validate source range.
2. Copy exactly `file_size`.
3. Zero:

```text
[file_size, memory_size)
```

4. Mark segment loaded.

### Mapping

1. Determine virtual range.
2. Determine physical range.
3. Create mapping.
4. Validate mapping.
5. Apply permissions.
6. Mark segment mapped and permissioned.

### Final load state

`JP_KERNEL_LOAD_LOADED` is allowed only when:

- every selected segment is allocated;
- every segment is loaded;
- every required zero-fill range is initialized;
- every segment is mapped;
- every required permission is established;
- entry point is valid.

## State / Lifecycle

Kernel validation:

```text
UNKNOWN
    ↓
NOT_INITIALIZED
    ↓
VALID / INVALID / FAILED
```

Kernel loading:

```text
UNKNOWN
    ↓
NOT_INITIALIZED
    ↓
LOADED / FAILED
```

Each segment separately follows:

```text
UNINITIALIZED
    ↓
VALIDATED
    ↓
ALLOCATED
    ↓
LOADED
    ↓
MAPPED
    ↓
PERMISSIONED
```

## Ownership / Lifetime

- Kernel image remains valid during validation and loading.
- Segment metadata belongs to `JPBootContext`.
- Destination memory belongs to the prepared kernel execution environment.
- Kernel mappings remain valid through handoff.
- Raw image storage may be released only after no Stage 1 operation requires it.

## Invariants

- `file_size <= memory_size`.
- Source range lies within image.
- Destination range does not overflow.
- Destination memory does not overwrite protected memory.
- Every loaded segment is mapped.
- Every mapped segment has correct permissions.
- Entry point lies inside executable loaded memory.
- No malformed ELF data is silently repaired.

## Dependencies

```c
#include "justos/boot/kernel_loader.h"
```

Low-level architecture and memory mechanisms may be used internally.

Direct UEFI/BIOS dependencies are prohibited.

## Boundary Rules

ELF parsing belongs entirely to Stage 1.

Stage 2 must not be required to perform Stage 1 ELF loading work.

`JPBootInfo` is not constructed here.

## Failure Semantics

Malformed ELF, unsupported architecture, invalid segment, allocation failure, mapping failure, or permission failure prevents successful kernel loading.

## Implementation Freedom

Copying, zeroing, allocation, mapping, and parsing helper mechanisms remain implementation-defined.

## Verification

Verify:

- valid ELF;
- invalid magic;
- invalid class;
- invalid architecture;
- invalid type;
- malformed program-header table;
- invalid `PT_LOAD`;
- overflow;
- invalid entry;
- allocation failure;
- mapping failure;
- permission failure;
- successful complete load.

---

# 18. `include/justos/boot/handoff.h`

## File

Final Stage 1 handoff contract.

## Purpose

Defines the public contract for validating and preparing the Stage 1 → Stage 2 transition.

The module does not define `JPBootInfo`.

## Responsibilities

- validate all Stage 1 prerequisites;
- prepare platform transition;
- run the optional pre-transition interaction hook;
- establish final handoff state;
- define kernel entry state.

## Types / Structures

Uses `JPHandoffInfo`, `JPBootInteraction`, and `JPBootInteractionHook`.

## Constants / Enums

Handoff states are part of the normalized contract.

## Public API

```c
JPBootResult jp_boot_register_interaction(
    JPBootContext *context,
    JPBootInteractionHook hook,
    void *user_data
);

JPBootResult jp_boot_prepare_handoff(
    JPBootContext *context
);
```

`jp_boot_register_interaction()` records at most one hook and sets the interaction state to `REGISTERED`; it fails with `JP_BOOT_ERROR_INVALID_PARAM` for a null context or hook and with `JP_BOOT_FAILED` if a hook is already registered or handoff preparation has already started. Registration alone does not execute the hook.

## Internal API

None exposed.

## Algorithm

1. Validate context.
2. Validate hardware.
3. Validate memory map.
4. Validate framebuffer where required.
5. Validate kernel.
6. Validate kernel mappings.
7. Validate stack.
8. Validate paging.
9. Validate CPU entry state.
10. Invoke the registered interaction hook, if any (exactly once).
11. Dispatch platform handoff preparation.
12. Validate resulting handoff state.
13. Set `JP_BOOT_STAGE_READY` only after all required conditions are satisfied.

## State / Lifecycle

```text
UNINITIALIZED
    ↓
PREREQUISITES_VALID
    ↓
PLATFORM_PREPARED
    ↓
READY
```

Any failure produces:

```text
FAILED
```

The interaction hook has its own lifecycle (`JPBootInteractionState`): `NOT_REGISTERED → REGISTERED → COMPLETED` or `FAILED`. Registration is valid only while the handoff state is `UNINITIALIZED`. The hook runs between `PREREQUISITES_VALID` and `PLATFORM_PREPARED`.

## Ownership / Lifetime

- `JPBootContext` remains runtime-owned until handoff.
- Kernel mappings persist.
- Page-table memory remains valid for kernel entry.
- Kernel stack remains valid for kernel entry.
- Provider resources remain provider-owned until the platform transition invalidates or transfers them.
- The hook's `user_data` is caller-owned; Stage 1 never frees, copies, or interprets it.

## Invariants

Readiness requires:

```text
environment valid
hardware valid
memory map valid
framebuffer valid where required
kernel valid
kernel loaded
kernel mappings established
kernel permissions established
kernel stack valid
paging valid
CPU entry state valid
platform handoff prepared
```

No invalid prerequisite may coexist with `READY`.

The interaction hook runs at most once per handoff attempt and never after platform handoff preparation has begun.

## Dependencies

```c
#include "justos/boot/handoff.h"
```

## Boundary Rules

Handoff preparation is the final generic Stage 1 operation.

The actual platform transition is performed according to the provider contract.

## Failure Semantics

Any missing mandatory prerequisite prevents readiness. A hook that returns a failure result fails handoff and its result is propagated unchanged; registration failures are returned without side effects.

## Implementation Freedom

Validation ordering and internal helper decomposition remain implementation-defined unless the order itself affects platform semantics.

## Verification

Verify successful and failed handoff preparation and hook registration (valid, null, duplicate, late).

---

# 19. `src/boot/handoff.c`

## File

Final Stage 1 handoff implementation.

## Purpose

Implements final readiness validation and platform handoff preparation.

## Responsibilities

- validate final Stage 1 state;
- prepare provider handoff;
- invoke the registered pre-transition interaction hook exactly once, if registered;
- publish final entry state;
- establish `JP_BOOT_STAGE_READY`.

## Types / Structures

None.

## Constants / Enums

None beyond shared contract types.

## Public API

```c
JPBootResult jp_boot_register_interaction(
    JPBootContext *context,
    JPBootInteractionHook hook,
    void *user_data
);

JPBootResult jp_boot_prepare_handoff(
    JPBootContext *context
);
```

Implements exactly the operations declared by `handoff.h`.

## Internal API

```text
validate_handoff_prerequisites()
validate_kernel_entry_state()
validate_paging_state()
validate_stack_state()
invoke_interaction_hook()
prepare_platform_handoff()
publish_handoff_state()
```

## Algorithm

```text
Hardware ready
        ↓
Memory map ready
        ↓
Framebuffer ready / explicitly not required
        ↓
Kernel validated
        ↓
Kernel loaded
        ↓
Mappings established
        ↓
Permissions established
        ↓
Paging ready
        ↓
Stack ready
        ↓
CPU entry state ready
        ↓
Pre-transition interaction (optional, once)
        ↓
Platform handoff prepared
        ↓
JP_BOOT_STAGE_READY
```

```
generic handoff
    ↓
provider prepares state
    ↓
generic layer calls provider transition
```

`JP_BOOT_STAGE_READY` is never entered merely because prerequisite checks were attempted.

### Decided rules (handoff preparation)

`jp_boot_register_interaction` follows `handoff.h`: a null context or hook returns `JP_BOOT_ERROR_INVALID_PARAM`; an already registered hook, or a `handoff.state` other than `UNINITIALIZED`, returns `JP_BOOT_FAILED`; a failed call changes nothing; a successful call stores the hook and `user_data` and sets the interaction state to `REGISTERED`.

`jp_boot_prepare_handoff` works through these steps in this order. Any failure at any step sets `handoff.state` to `FAILED`, leaves `current_stage` unchanged from `JP_BOOT_STAGE_HANDOFF_PREPARATION`, stops the work, and returns the result of the failing step unchanged (a failing prerequisite check returns `JP_BOOT_FAILED`).

1. A null context returns `JP_BOOT_ERROR_INVALID_PARAM`. A `handoff.state` other than `UNINITIALIZED` returns `JP_BOOT_FAILED` without changes (a handoff is prepared only once).
2. `current_stage` becomes `JP_BOOT_STAGE_HANDOFF_PREPARATION`.
3. The prerequisites are checked: `environment.state`, `hardware.overall_state`, `memory_map.state`, and `kernel.image.state` are `DETECTED`; `kernel.validation_state` is `VALID`; `kernel.load_state` is `LOADED` and every kernel segment is `PERMISSIONED`; `framebuffer.state` is `DETECTED` or `NOT_DETECTED` (a machine without a framebuffer can still start, because Stage 3 has a text fallback; any other framebuffer state fails).
4. The kernel stack is placed: a range of `JP_BOOT_KERNEL_STACK_SIZE` bytes, taken from usable memory with the placement rule of the loader (after the last kernel segment), so it overlaps no segment and not the kernel image. `handoff.stack_pointer` is set to the end (the highest address) of that range. The stack is identity-mapped: its virtual address equals its physical address. The range is then reserved with `jp_boot_environment_reserve_memory()`. If no range fits, or the reservation fails, the result is `JP_BOOT_FAILED`.
5. The architecture operations are called in this order: `jp_<arch>_setup_paging()`, `jp_<arch>_prepare_entry()`, `jp_<arch>_validate_entry_state()`.
6. `handoff.state` becomes `PREREQUISITES_VALID`.
7. If a hook is registered, it is called exactly once with the context and its `user_data`. A result other than `JP_BOOT_SUCCESS` sets the interaction state to `FAILED`, fails the handoff, and is returned unchanged; `JP_BOOT_SUCCESS` sets the interaction state to `COMPLETED`. With no hook the interaction state stays `NOT_REGISTERED`.
8. `jp_boot_environment_prepare_handoff()` is called (it also stores the final memory map in `context->memory_map`); then `handoff.state` becomes `PLATFORM_PREPARED`.
9. The resulting handoff state is checked once more, after the platform step, because the hook and the platform step ran since the architecture check. The final memory map must pass `jp_boot_validate_memory_map()`, and all of these must hold: `handoff.architecture` still names the architecture that was used; `handoff.kernel_entry` equals `kernel.entry_point`; `handoff.page_table_root` is not zero and `handoff.paging_enabled` is true; `handoff.interrupts_disabled` is true; `handoff.stack_pointer` is not zero and is a multiple of the stack alignment of the architecture (`JP_X86_64_STACK_ALIGNMENT` or `JP_AARCH64_STACK_ALIGNMENT`, both 16). If one does not hold, the handoff fails with `JP_BOOT_FAILED`.
10. `handoff.ownership_transferred` becomes true, `handoff.state` becomes `READY`, and `current_stage` becomes `JP_BOOT_STAGE_READY`. Nothing is set to a ready value before this step.

`jp_<arch>_prepare_entry()` therefore expects `handoff.stack_pointer` to be set already; it records the kernel entry address, the architecture, and the interrupt state, and returns a failing result if the stack pointer is not set.

## State / Lifecycle

```text
UNINITIALIZED
    ↓
PREREQUISITES_VALID
    ↓
PLATFORM_PREPARED
    ↓
READY
```

Failure is terminal for the current handoff attempt.

The interaction state advances only as defined by `handoff.h`: `REGISTERED` on registration, `COMPLETED` after a successful hook, `FAILED` after a failing hook.

## Ownership / Lifetime

- `JPBootContext` remains caller/runtime-owned.
- `JPHandoffInfo` is embedded in the context.
- Kernel mappings remain active.
- Page tables remain valid.
- Stack remains valid.
- Provider state remains valid until the platform transition.
- The hook's `user_data` is caller-owned and never freed or interpreted here.

## Invariants

The following must hold before `READY`:

```text
environment.state == DETECTED

hardware.overall_state == DETECTED

memory_map.state == DETECTED

kernel.validation_state == VALID

kernel.load_state == LOADED

kernel.entry_point is valid

required mappings exist

required permissions exist

kernel stack is valid

paging state is valid

CPU entry state is valid
```

```text
page-table root
mapping granularity
virtual → physical mapping
permissions
page-table ownership
lifetime
```

If framebuffer is mandatory for the selected target, framebuffer state must also be `DETECTED`.

The following combinations are forbidden:

```text
READY + failed prerequisite
READY + unknown prerequisite
READY + uninitialized prerequisite
READY + unloaded kernel
READY + invalid kernel
READY + invalid stack
READY + invalid paging state
```

## Dependencies

```c
#include "justos/boot/handoff.h"
```

## Boundary Rules

This file does not:

- construct `JPBootInfo`;
- perform ELF parsing;
- perform kernel loading;
- implement provider internals.

## Failure Semantics

Any failed prerequisite prevents successful readiness. A failing interaction hook sets the interaction state to `FAILED`, fails handoff, and propagates the hook result unchanged.

## Implementation Freedom

Internal validation helpers and ordering remain implementation-defined unless required by provider transition semantics.

## Verification

Verify all success and failure combinations required by the handoff contract, including: no hook registered, hook registered and succeeding, hook failing, duplicate registration, registration after handoff started, hook invoked exactly once, and no memory-map key held across the hook.

---

# 20. `include/justos/boot/architecture/x86_64.h`

## File

The x86_64 architecture contract of Stage 1.

## Purpose

Generic Stage 1 code (`handoff.c`, `kernel_loader.c`) must not contain instructions or register names of one processor family. This header is the border: it lists the few operations the generic code may ask of an x86_64 machine, and the fixed values that describe the state in which x86_64 enters the kernel. The machine-dependent work itself is done in `src/boot/architecture/x86_64.c`.

The common types stay in `boot_types.h`. This header does not copy them; it only adds what differs between processor families.

## Responsibilities

- Define the x86_64 entry-state requirements as named constants.
- Declare the operations the generic code calls.
- Define the role of the page-table root register on x86_64.

## Types / Structures

None. The operations work on the shared `JPBootContext` and `JPKernelHeader`.

## Constants / Enums

```c
#define JP_X86_64_STACK_ALIGNMENT 16
```

The state required at kernel entry (each item is a fact that `jp_x86_64_validate_entry_state()` checks):

| Item | Required value |
|------|----------------|
| Execution mode | long mode (the 64-bit mode of the processor) |
| Interrupts | `RFLAGS.IF = 0` (the processor ignores interrupt requests) |
| Paging | enabled |
| Page-table root | register `CR3` holds the physical address of the top-level page table; this address is stored in `handoff.page_table_root` |
| Stack | the initial stack pointer is a mapped address aligned to `JP_X86_64_STACK_ALIGNMENT` bytes (the System V AMD64 calling convention requires it) |
| Kernel entry | a canonical virtual address (an address whose upper bits are a copy of bit 47) that is mapped as executable |

## Public API

```c
JPBootResult jp_x86_64_validate_entry_state(
    const JPBootContext *context
);

JPBootResult jp_x86_64_setup_paging(
    JPBootContext *context
);

JPBootResult jp_x86_64_prepare_entry(
    JPBootContext *context
);

JPBootResult jp_x86_64_validate_elf_machine(
    const JPKernelHeader *header
);

JPBootResult jp_x86_64_discover_cpu(
    JPCpuInfo *cpu
);

JPBootResult jp_x86_64_map_segment(
    JPBootContext *context,
    size_t index
);

void *jp_x86_64_phys_access(
    JPBootContext *context,
    uint64_t physical,
    uint64_t length
);

_Noreturn void jp_x86_64_enter_kernel(
    JPBootContext *context
);
```

What each operation does:

- `jp_x86_64_validate_entry_state()` reads `context->handoff` and `context->kernel` and checks every row of the table above. It changes nothing. It returns `JP_BOOT_SUCCESS` only if every row holds. A missing or inconsistent value returns `JP_BOOT_FAILED`; a null context returns `JP_BOOT_ERROR_INVALID_PARAM`.
- `jp_x86_64_setup_paging()` builds the boot-time page tables that map the kernel segments and the kernel stack with the permissions the kernel loader recorded, and then stores the physical address of the top-level table in `handoff.page_table_root` and sets `handoff.paging_enabled`. It sets those fields only after the tables really exist.
- `jp_x86_64_prepare_entry()` records the initial stack pointer and the kernel entry address in `handoff`, sets `handoff.interrupts_disabled`, and sets `handoff.architecture` to `JP_ARCH_X86_64`. It writes each field only after the corresponding state has really been established.
- `jp_x86_64_validate_elf_machine()` returns `JP_BOOT_SUCCESS` only if `header->machine == JP_ELF_MACHINE_X86_64`; otherwise `JP_BOOT_ERROR_CORRUPTED_DATA`. A null pointer returns `JP_BOOT_ERROR_INVALID_PARAM`.
- `jp_x86_64_map_segment()` and `jp_x86_64_phys_access()` are the machine-dependent part of kernel loading; their behavior is defined in "Decided rules (physical memory, copying, mapping)" of section 17.
- `jp_x86_64_discover_cpu()` fills `cpu` with the information the processor itself reports (`state` and `logical_cores`). It sets `cpu->state` to `JP_DETECTION_STATE_DETECTED` only after the information was really read; it returns `JP_BOOT_ERROR_INVALID_PARAM` for a null pointer and a failing result if the information cannot be read, and then it does not report `DETECTED`.

## Internal API

None.

## Algorithm

The generic code calls the operations in this order: `validate_elf_machine` (during kernel validation), `setup_paging` and `prepare_entry` (after the kernel is loaded), `validate_entry_state` (during handoff prerequisite validation).

## State / Lifecycle

The operations write only the `handoff` fields named above, in the states defined by `handoff.h`. They never set `JP_HANDOFF_STATE_READY`; only `jp_boot_prepare_handoff()` does.

## Ownership / Lifetime

The page tables built by `jp_x86_64_setup_paging()` belong to the prepared kernel execution environment and stay valid through kernel entry (see Boot-Time Paging).

## Invariants

- No field is reported as established before the work has been done.
- Operations never change fields outside `handoff` (and the page-table memory).
- The header is included only by generic code that is built for x86_64, and by `x86_64.c`.

## Dependencies

```c
#include "justos/boot/boot_types.h"
```

## Boundary Rules

No UEFI or BIOS logic is allowed here or in `x86_64.c`: how the firmware hands over control is a provider matter. Generic modules contain no x86_64 instruction or register.

## Failure Semantics

An operation that cannot complete returns a failing `JPBootResult` and leaves the `handoff` fields it did not finish unchanged.

## Implementation Freedom

Page size, table layout inside the contract, and helper functions are implementation-defined.

## Verification

Verify: every row of the entry-state table accepted and rejected (each separately); null context; machine 62 accepted and 183 rejected; paging fields unset until `setup_paging` succeeds.

---

# 21. `src/boot/architecture/x86_64.c`

## File

x86_64 implementation of the operations declared in `x86_64.h`.

## Purpose

Does the machine-dependent work (reading and setting `CR3`, building page tables, checking `RFLAGS`) that the generic Stage 1 code must not do itself.

## Responsibilities

Implement exactly the eight operations of `x86_64.h` with the behavior written there.

### Decided rules (x86_64: paging, descriptor table, and the jump)

**The table memory (the pool).** The page tables and the descriptor table live in one block of `JP_X86_64_TABLE_POOL_PAGES` (128) pages (512 KiB), zero-filled before use. The block is created by the first call of `jp_x86_64_map_segment()` (or of `jp_x86_64_setup_paging()` if no segment was mapped before): it is placed at the top of the highest usable region of the memory map that can hold it (page aligned, above `JP_BOOT_PROTECTED_LOW_MEMORY`; far from the segments and the stack, which are placed from the bottom up), reserved through `jp_boot_environment_reserve_memory()`, and described by `handoff.page_table_memory_base` and `handoff.page_table_memory_size`. The memory map that Stage 1 holds is the early one, and the firmware keeps allocating after it was taken (it allocates from the top of memory downward), so a reservation can be refused. In that case the next lower candidate is tried: the candidate whose end is at most the start of the refused one, in the same or another usable region, with the same rules (at most `JP_X86_64_POOL_ATTEMPTS`, 16, reservations in all). If no region can hold the pool, or every attempt is refused, the result is `JP_BOOT_FAILED`. Page 0 of the block is the top-level table (the value of `handoff.page_table_root`), page 1 is the global descriptor table, and every other table is taken from the next free page; `handoff.page_table_memory_used` counts the pages handed out. When the 128 pages are not enough the result is `JP_BOOT_FAILED`.

**`jp_x86_64_map_segment(context, index)`.** The segment must exist (`index < segment_count`, otherwise `JP_BOOT_ERROR_INVALID_PARAM`) and be in the state `LOADED` (otherwise `JP_BOOT_NOT_INITIALIZED`). For every 4 KiB page of its memory size, it adds an entry to the page table tree that maps the virtual page to the physical page: present; writable only if the segment flags contain write; "no execute" (bit 63) set unless the segment flags contain execute. Missing intermediate tables are created. A page that is already mapped is `JP_BOOT_FAILED` (two segments may not share a virtual page). The call returns success only after the entries exist.

**`jp_x86_64_setup_paging(context)`.** It may be called once (a second call, or a call when `handoff.paging_enabled` is already true, is `JP_BOOT_FAILED`). It adds (a) the *identity map*: every region of the memory map is mapped to itself with 2 MiB pages that are present, writable, and executable (the pages that touch the region are mapped; neighbouring regions may share a page), except regions of type `MMIO` or `UNUSABLE`, and regions of type `RESERVED` that are `JP_X86_64_IDENTITY_RESERVED_MAX` (1 GiB) or larger (such a region is an address range that the firmware keeps for itself, for example for devices, not memory that is in use; the real firmware reports one near 1 TiB, and mapping it would use up the table pool). The boot code, the context, and the tables are reached by their physical addresses until the jump, and the kernel reaches the context the same way; Stage 2 replaces this map; (b) the framebuffer, if its state is `DETECTED`: its range maps to itself with 2 MiB pages that are present, writable, no-execute, and cache-disabled (bit 4); (c) the global descriptor table with three 8-byte entries in page 1: the null entry, a 64-bit code segment `0x00AF9A000000FFFF` (selector `0x08`), and a data segment `0x00CF92000000FFFF` (selector `0x10`). An identity page that is already mapped as the same identity page with the same attributes (two regions share it) is accepted; any other existing mapping in its place (for example the tables of a kernel segment) is a conflict. If any identity or framebuffer page conflicts with a mapping that a kernel segment already made, or the pool is full, the result is `JP_BOOT_FAILED`. Only after all of this exists does it set `handoff.paging_enabled`. It does **not** load `CR3` or change a register: that happens in `jp_x86_64_enter_kernel()`.

**`jp_x86_64_prepare_entry(context)`.** It requires `handoff.page_table_root` and `handoff.paging_enabled` (set by the two operations above), a non-zero `handoff.stack_pointer` that is a multiple of `JP_X86_64_STACK_ALIGNMENT`, and a non-zero canonical `kernel.entry_point` (an address whose bits 63 to 48 repeat bit 47); otherwise `JP_BOOT_FAILED`. Then it disables interrupts for real (`jp_x86_64_disable_interrupts()`, the `cli` instruction) and only after that writes `handoff.kernel_entry`, `handoff.architecture = JP_ARCH_X86_64`, and `handoff.interrupts_disabled = true`.

**`jp_x86_64_validate_entry_state(context)`.** It reads the real state of the processor (`jp_x86_64_read_cpu_state()`) and requires: long mode active (`EFER.LMA`, bit 10), paging on (`CR0.PG`, bit 31), and interrupts off (`RFLAGS.IF`, bit 9, clear); and, in the normalized state: architecture `JP_ARCH_X86_64`, `paging_enabled`, `interrupts_disabled`, a page-table root that is not zero and page aligned, a stack pointer that is not zero and aligned, a canonical `kernel_entry` equal to `kernel.entry_point` that lies in a `PERMISSIONED` segment that has the execute flag. Anything else is `JP_BOOT_FAILED`. It changes nothing.

**`jp_x86_64_discover_cpu(cpu)`.** It uses the `CPUID` instruction. If leaf 1 is available, `logical_cores` is the "logical processors per package" field (bits 23 to 16 of `EBX`) when the hyper-threading bit (bit 28 of `EDX`) is set, otherwise 1; `state` becomes `DETECTED` only after the instruction really ran. A null pointer is `JP_BOOT_ERROR_INVALID_PARAM`.

**`jp_x86_64_phys_access(context, physical, length)`.** In Milestone 0 all memory that Stage 1 touches is reachable at its physical address (UEFI and the BIOS loader run with the identity map), so the function returns that address as a pointer, but only if `length` is not zero, the range does not overflow, `physical` is not zero, and the whole range lies inside one region of the memory map; otherwise it returns a null pointer.

**`jp_x86_64_validate_elf_machine(header)`.** `JP_BOOT_SUCCESS` only for machine `JP_ELF_MACHINE_X86_64`; `JP_BOOT_ERROR_CORRUPTED_DATA` otherwise; a null pointer is `JP_BOOT_ERROR_INVALID_PARAM`.

**The instructions that cannot be written in C** are in `x86_64_enter.S` and are declared in the private header `x86_64_cpu.h`; all of them use the System V argument registers (`RDI`, `RSI`, `RDX`, `RCX`, `R8`), even when the C code is built for UEFI: `jp_x86_64_disable_interrupts()` (`cli`), `jp_x86_64_read_cpu_state(JPX86CpuState *)` (reads `RFLAGS`, `CR0`, `CR4`, and `EFER`), `jp_x86_64_jump_to_kernel(root, gdt, stack, entry, context)` (the register steps below), and `jp_x86_64_park()` (`cli` and `hlt` in a loop). `jp_x86_64_enter_kernel()` itself is C: it checks that the handoff is `READY` and paging is enabled (otherwise it calls `jp_x86_64_park()`; the function cannot return an error), takes the five values from the context, and calls `jp_x86_64_jump_to_kernel()`. On the host the tests replace the four assembly functions with fakes, so every other line of `x86_64.c` is tested on the host.

`jp_x86_64_enter_kernel(context)` runs after the handoff is `READY`. It never returns. Steps: (1) disable interrupts (`cli`); (2) set the `NXE` bit in `EFER` (the model-specific register `0xC0000080`, bit 11) and the `WP` bit in `CR0`, so that the no-execute and read-only bits are honoured; (3) load the GDT (`lgdt`; its address is `page_table_memory_base + 4096`, its limit is 23) and reload the segment registers with the selectors above (a far return for `CS`); (4) load `CR3` with `handoff.page_table_root`; (5) set the stack pointer to `handoff.stack_pointer` and push a zero value (this makes the stack look as if the kernel entry had been called, as the System V calling convention expects); (6) put the context pointer into `RDI`; (7) jump to `handoff.kernel_entry`.

## Public API

Exactly the eight operations declared in `x86_64.h`.

## Internal API

Private `static` helpers only (for example page-table construction).

## Algorithm

`jp_x86_64_setup_paging()`:

1. Validate the context and require that the kernel is loaded and has recorded segments.
2. Take the memory for the page tables from the memory the memory map marks as usable (one table per level, zero-filled).
3. For every kernel segment, map each page from its virtual address to its physical address with the permissions of the segment.
4. Map the kernel stack.
5. Store the physical address of the top-level table in `handoff.page_table_root`, then set `handoff.paging_enabled`.

`jp_x86_64_prepare_entry()` and the two validation operations follow the descriptions in `x86_64.h`.

## State / Lifecycle, Ownership, Invariants

As in `x86_64.h`.

## Dependencies

```c
#include "justos/boot/architecture/x86_64.h"
```

## Boundary Rules

No firmware logic; no `JPBootInfo`; no ELF parsing.

## Failure Semantics

As in `x86_64.h`.

## Implementation Freedom

Mapping granularity and table allocation strategy.

## Verification

As in `x86_64.h`; page-table contents are verified in the QEMU tests.

---

# 22. `include/justos/boot/architecture/aarch64.h`

## File

The AArch64 architecture contract of Stage 1.

## Purpose

The same border as `x86_64.h`, for the AArch64 processor family: the operations the generic code may ask of an AArch64 machine and the fixed values that describe the state in which AArch64 enters the kernel. The common types stay in `boot_types.h`.

## Responsibilities

- Define the AArch64 entry-state requirements as named constants.
- Declare the operations the generic code calls.
- Define the roles of the translation-table base registers.

## Types / Structures

None.

## Constants / Enums

```c
#define JP_AARCH64_STACK_ALIGNMENT 16
```

The state required at kernel entry:

| Item | Required value |
|------|----------------|
| Exception level (the privilege level of the processor) | EL1, as defined by the Stage 2 contract |
| Interrupts | `PSTATE.DAIF = 1111` (debug, SError, IRQ and FIQ are all masked) |
| MMU (the unit that translates virtual addresses) | enabled, 4 KiB granule, 48-bit virtual addresses |
| Kernel mappings | `TTBR1_EL1` holds the physical address of the active top-level translation table; this address is stored in `handoff.page_table_root` |
| `TTBR0_EL1` | not used at kernel entry |
| Stack | the initial `SP` is a mapped address aligned to `JP_AARCH64_STACK_ALIGNMENT` bytes (the AArch64 calling convention requires it) |
| Kernel entry | a valid virtual address mapped as executable |
| Caches and TLB (the cache of address translations) | `DSB`, `TLBI`, `DSB`, `ISB` completed after the tables were written and before entry |

## Public API

```c
JPBootResult jp_aarch64_validate_entry_state(
    const JPBootContext *context
);

JPBootResult jp_aarch64_setup_paging(
    JPBootContext *context
);

JPBootResult jp_aarch64_prepare_entry(
    JPBootContext *context
);

JPBootResult jp_aarch64_validate_elf_machine(
    const JPKernelHeader *header
);

JPBootResult jp_aarch64_discover_cpu(
    JPCpuInfo *cpu
);

JPBootResult jp_aarch64_map_segment(
    JPBootContext *context,
    size_t index
);

void *jp_aarch64_phys_access(
    JPBootContext *context,
    uint64_t physical,
    uint64_t length
);

_Noreturn void jp_aarch64_enter_kernel(
    JPBootContext *context
);
```

The eight operations have the same meaning, return values, and write rules as the x86_64 operations of section 20, with these differences: the entry-state table above is the one that is checked; `setup_paging` builds the translation tables for `TTBR1_EL1` and performs the cache and TLB synchronization; `prepare_entry` sets `handoff.architecture` to `JP_ARCH_AARCH64`; `validate_elf_machine` accepts only machine `JP_ELF_MACHINE_AARCH64` (183); `jp_aarch64_discover_cpu()` reads the AArch64 identification registers instead.

## Internal API, Algorithm, State / Lifecycle, Ownership, Invariants, Dependencies, Boundary Rules, Failure Semantics, Implementation Freedom

As in section 20, with `aarch64` in place of `x86_64`. No BIOS logic exists on AArch64.

## Verification

As in section 20 with the AArch64 table: each row accepted and rejected, machine 183 accepted and 62 rejected, `TTBR0_EL1` unused, synchronization completed before entry.

---

# 23. `src/boot/architecture/aarch64.c`

## File

AArch64 implementation of the operations declared in `aarch64.h`.

### Decided rules (AArch64: exception level, translation, and the jump)

AArch64 translates addresses with two trees: `TTBR1_EL1` for the upper half of the address space (the kernel) and `TTBR0_EL1` for the lower half (the identity map). The page size is 4 KiB and addresses have 48 bits. In the descriptors below, "AttrIdx" selects an entry of `MAIR_EL1`, "AP" is the access permission, "PXN" is "privileged execute never", and "UXN" is "unprivileged execute never".

**The table memory (the pool).** The same pool as on x86_64 (`JP_AARCH64_TABLE_POOL_PAGES` pages, 128; placed, reserved, and retried in the same way, with `JP_AARCH64_POOL_ATTEMPTS`, 16, tries). Page 0 is the top-level table of the `TTBR1_EL1` tree (its address is `handoff.page_table_root`), page 1 is the top-level table of the `TTBR0_EL1` tree (its address is `page_table_memory_base + 4096`), and every other table is taken from the next free page. `handoff.page_table_memory_used` counts the pages handed out (2 after creation). Before use the whole pool is zero-filled.

**`jp_aarch64_map_segment(context, index)`** has the same preconditions and failures as the x86_64 operation (a loaded segment, a free pool, no page mapped twice). It maps every 4 KiB page of the segment in the tree of its half: an address whose upper 16 bits are all ones goes to the `TTBR1_EL1` tree, an address whose upper 16 bits are all zeros goes to the `TTBR0_EL1` tree, and any other address is `JP_BOOT_FAILED`. A page descriptor has the bits: valid and "page" (bits 1:0 = `0b11`), AttrIdx 1 (normal memory), inner shareable (bits 9:8 = `0b11`), access flag (bit 10), AP `0b00` (kernel read and write, bit 7 clear) or `0b10` (kernel read only, bit 7 set), UXN (bit 54) always set, and PXN (bit 53) set unless the segment flags contain execute.

**`jp_aarch64_setup_paging(context)`** may be called once. It adds (a) the *identity map* in the `TTBR0_EL1` tree: every region of the memory map is mapped to itself with 2 MiB blocks (valid and "block", bits 1:0 = `0b01`), except `UNUSABLE` regions, regions of type `RESERVED` that are `JP_AARCH64_IDENTITY_RESERVED_MAX` (1 GiB) or larger, and regions of type `MMIO` of that size or larger. Normal memory regions get AttrIdx 1, inner shareable, access flag, kernel read and write, UXN set, PXN clear (executable: the boot code is reached by its physical address until the jump); `MMIO` regions get AttrIdx 0 (device memory), access flag, kernel read and write, and both PXN and UXN set. (Unlike on x86_64 the `MMIO` regions are mapped, because the first devices of the machine, such as the serial port, are reached by memory addresses.) An identity block that is already mapped in the same way is accepted (two regions share it); any other existing mapping in its place is `JP_BOOT_FAILED`. (b) The framebuffer, if its state is `DETECTED`: its range is mapped with 2 MiB blocks with AttrIdx 0 (device memory) and PXN and UXN set; a block that is already the same identity block gets these attributes (replaced). If the pool is full the result is `JP_BOOT_FAILED`. Only after all of this exists does it set `handoff.paging_enabled`. It writes no register.

**`jp_aarch64_prepare_entry(context)`** requires what the x86_64 operation requires, except that a canonical kernel entry is an address whose upper 16 bits are all ones or all zeros, and that the stack alignment is `JP_AARCH64_STACK_ALIGNMENT` (16). It disables interrupts for real (`jp_aarch64_disable_interrupts()`, the instruction `msr daifset, #0xf`) and only then writes `handoff.kernel_entry`, `handoff.architecture = JP_ARCH_AARCH64`, and `handoff.interrupts_disabled = true`.

**`jp_aarch64_validate_entry_state(context)`** reads the real state of the processor (`jp_aarch64_read_cpu_state()`) and requires: the current exception level is EL1 or EL2; the interrupt mask `DAIF` is all ones (debug, SError, IRQ, FIQ all masked); the MMU of the current level is on (bit 0 of `SCTLR_EL1`, or of `SCTLR_EL2` at EL2). In the normalized state it requires the same as the x86_64 operation (architecture `JP_ARCH_AARCH64`). It changes nothing.

**`jp_aarch64_discover_cpu(cpu)`** reads the main identification register `MIDR_EL1` (`jp_aarch64_read_cpu_state()`). A value of zero means that it could not be read: the result is `JP_BOOT_FAILED`. Otherwise `state` is `DETECTED` and `logical_cores` is 1: **only the boot processor is counted**, because AArch64 has no instruction that tells how many processors the machine has (that comes from firmware tables, which Stage 2 and later read). A null pointer is `JP_BOOT_ERROR_INVALID_PARAM`.

**`jp_aarch64_phys_access`** and **`jp_aarch64_validate_elf_machine`** work like their x86_64 counterparts (identity access inside one region; machine `JP_ELF_MACHINE_AARCH64`).

**The instructions that cannot be written in C** are in `aarch64_enter.S` and are declared in the private header `aarch64_cpu.h`: `jp_aarch64_disable_interrupts()`, `jp_aarch64_read_cpu_state(JPA64CpuState *)` (current exception level, `DAIF`, the system control register of the current level, and `MIDR_EL1`), `jp_aarch64_jump_to_kernel(ttbr1, ttbr0, stack, entry, context)`, and `jp_aarch64_park()` (`wfe` in a loop with interrupts masked). `jp_aarch64_enter_kernel()` is C, like on x86_64: it parks unless the handoff is `READY` and paging is enabled, takes `page_table_root`, `page_table_memory_base + 4096`, `stack_pointer`, `kernel_entry`, and the context pointer, and calls the jump.

**The jump (`jp_aarch64_jump_to_kernel`).** (1) Mask all interrupts. (2) Write `MAIR_EL1` = `0x000000000000FF00` (attribute 0 is device memory `0x00`, attribute 1 is normal write-back memory `0xFF`). (3) Write `TCR_EL1`: 4 KiB granule for both halves (`TG0 = 0b00`, `TG1 = 0b10`), 48-bit addresses for both halves (`T0SZ = T1SZ = 16`), inner shareable, write-back write-allocate walks (`SH0 = SH1 = 0b11`, `IRGN0 = ORGN0 = IRGN1 = ORGN1 = 0b01`), both halves enabled, and `IPS` copied from the processor (`ID_AA64MMFR0_EL1.PARange`, at most 5 = 48 bits); the fixed part has the value `0xB5103510`. (4) Write `TTBR1_EL1` and `TTBR0_EL1`. (5) If the exception level is EL1: synchronize (`isb`, `dsb ish`, `tlbi vmalle1`, `dsb ish`, `isb`), set the bits `M` (MMU), `C` (data cache), and `I` (instruction cache) of `SCTLR_EL1` (the bit `WXN` is **not** set, because the identity map is writable and executable until Stage 2 replaces it), `isb`, set `SP` to the stack, put the context into `X0`, and branch to the entry. (6) If the exception level is EL2: set `HCR_EL2.RW` (bit 31, so that EL1 is 64-bit) and no other bit of `HCR_EL2`; write `SCTLR_EL1` = `0x30D00800` with `M`, `C`, `I` set (the fixed part is the reserved-to-one bits); synchronize with `dsb sy` and `isb`; write `SP_EL1`; write `SPSR_EL2` = `0x3C5` (EL1 with its own stack, all interrupts masked) and `ELR_EL2` with the address of the instruction that puts the context into `X0` and branches to the entry; then `eret`. Any other exception level is a state that `jp_aarch64_validate_entry_state()` has already refused.

## Purpose, Responsibilities, Public API, Internal API

As in section 21, for the eight operations of `aarch64.h`.

## Algorithm

`jp_aarch64_setup_paging()` follows the five steps of section 21 with translation tables (4 KiB granule, 48-bit addresses) and stores the root in `handoff.page_table_root` from `TTBR1_EL1`; after the tables are written it performs `DSB`, `TLBI`, `DSB`, `ISB`.

## Dependencies

```c
#include "justos/boot/architecture/aarch64.h"
```

## Boundary Rules, Failure Semantics, Implementation Freedom, Verification

As in section 21 and section 22.

---

# 24. `include/justos/boot/boot_pipeline.h`

## File

The Stage 1 pipeline interface.

## Purpose

One function that runs the whole Stage 1 from "the environment is chosen" to "the handoff is ready". The platform entry points (the UEFI application and the BIOS 64-bit entry) call it; they do not repeat the order of the steps.

## Public API

```c
JPBootResult jp_boot_run_pipeline(JPBootContext *context);
```

## Dependencies

```c
#include "justos/boot/boot_types.h"
```

## Verification

The header compiles on its own and declares exactly this function.

---

# 25. `src/boot/boot_pipeline.c`

## File

The Stage 1 pipeline.

## Purpose

Calls the public Stage 1 functions in the one order in which they can succeed, and stops at the first failure.

## Preconditions

The caller has zero-filled the context and has set `environment.type`, `environment.platform_context`, and `handoff.architecture`. If a pre-transition interaction hook is wanted, the caller has already registered it with `jp_boot_register_interaction()`.

## Algorithm

1. `jp_boot_environment_initialize()`
2. `jp_boot_discover_hardware()`
3. `jp_boot_discover_memory()`
4. `jp_boot_validate_memory_map()`
5. `jp_boot_initialize_framebuffer()`. The result `JP_BOOT_NOT_DETECTED` is accepted (the machine has no framebuffer and the pipeline goes on); any other failure stops the pipeline.
6. `jp_boot_acquire_kernel_image()`
7. `jp_boot_validate_kernel()`
8. `jp_boot_load_kernel()`
9. `jp_boot_prepare_handoff()`

The first failing result (other than the accepted one in step 5) is returned unchanged and nothing after it runs. After step 9 succeeds the function returns `JP_BOOT_SUCCESS`; the context is then `READY`. The function does not enter the kernel: the caller does that with the architecture operation `jp_<arch>_enter_kernel()`. A null context is `JP_BOOT_ERROR_INVALID_PARAM`.

## Invariants

The function writes no field of the context itself; every change comes from the functions it calls. It contains no UEFI, BIOS, or architecture code.

## Verification

On the host, with the fake environment and the fake architecture: all nine steps run in the order above; each failing step stops the pipeline with its own result and the later steps do not run; a missing framebuffer does not stop it; a null context is rejected.

---

# 26. `include/justos/boot/platform/bios_block.h`

## File

The boot data block that the BIOS loader leaves for the 64-bit code.

## Purpose

The loader (16-bit code) and `bios.c` (64-bit code) must agree on one fixed layout. This header is that agreement. It is the only place where the layout is written.

## Constants

```c
#define JP_BIOS_BLOCK_ADDRESS 0x00010000
#define JP_BIOS_BLOCK_MAGIC "JUSTBLK1"      /* 8 bytes, no end marker is stored */
#define JP_BIOS_BLOCK_VERSION 1
#define JP_BIOS_BLOCK_MAX_E820 128

#define JP_BIOS_BLOCK_FLAG_A20 0x1           /* the A20 line is enabled */
#define JP_BIOS_BLOCK_FLAG_VIDEO 0x2         /* the video fields are valid */
#define JP_BIOS_BLOCK_FLAG_KERNEL 0x4        /* the kernel fields are valid */

#define JP_BIOS_VIDEO_FORMAT_RGB 1
#define JP_BIOS_VIDEO_FORMAT_BGR 2
```

## Structures

All numbers are little-endian and the structures have no padding.

```c
typedef struct JPBiosE820Entry {
    uint64_t base;
    uint64_t length;
    uint32_t type;                 /* 1 usable, 2 reserved, 3 ACPI reclaimable, 4 ACPI NVS, 5 bad memory */
    uint32_t attributes;
} JPBiosE820Entry;                 /* 24 bytes */

typedef struct JPBiosBlock {
    char magic[8];
    uint32_t version;
    uint32_t flags;
    uint32_t boot_drive;
    uint32_t e820_count;
    JPBiosE820Entry e820[JP_BIOS_BLOCK_MAX_E820];
    uint64_t framebuffer_base;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t video_format;
    uint64_t kernel_address;       /* where the loader left kernel.elf */
    uint64_t kernel_size;
} JPBiosBlock;
```

## Invariants

The size of the block is at most 16 KiB (checked with a static assertion). `e820_count` is at most `JP_BIOS_BLOCK_MAX_E820`; a larger value is `JP_BOOT_ERROR_CORRUPTED_DATA` in `bios.c`. A wrong magic text or version is `JP_BOOT_ERROR_CORRUPTED_DATA`.

## Verification

Static assertions for the offset of every field and for the size of the structures; a host test that builds a block by hand and feeds it to the BIOS provider.

---

# 27. `src/boot/platform/uefi_defs.h`

## File

The private UEFI definitions.

## Purpose

There is no EDK2 or gnu-efi on the project, so the few UEFI structures that `uefi.c` needs are written here by hand, exactly as the UEFI specification lays them out. Only `uefi.c` and `uefi_entry.c` include this file.

## Content

- the basic types: a handle, a status code (a 64-bit number whose highest bit marks an error), a GUID, and the table header (24 bytes);
- the status codes that the provider uses: success, buffer too small, invalid parameter, not found, not ready, unsupported, and load error;
- the system table, with the fields up to and including the boot-services pointer (`BootServices` at offset 96, `ConIn` at 48, `ConOut` at 64);
- the boot-services table with the functions that are used, at the offsets of the specification (`AllocatePages` 40, `FreePages` 48, `GetMemoryMap` 56, `AllocatePool` 64, `FreePool` 72, `HandleProtocol` 152, `ExitBootServices` 232, `Stall` 248, `OpenProtocol` 280, `LocateProtocol` 320); the other entries are kept as unnamed pointers so that the offsets stay right;
- the memory descriptor and the list of memory types;
- the protocols: loaded image, simple file system, file, graphics output (with the mode and the mode information), and simple text input (with the input key);
- the GUIDs of those protocols;
- the calling-convention macro `JP_UEFI_API`: on x86_64 it is `__attribute__((ms_abi))` (UEFI uses the Microsoft x64 convention there); on AArch64 it is empty (UEFI uses the standard AArch64 convention).

## Verification

Static assertions (`_Static_assert` with `offsetof` and `sizeof`) for every offset and size above, compiled on the host for both targets.

---

# 28. `src/boot/platform/uefi_entry.c`

## File

The entry point of the UEFI boot application, for x86_64 and AArch64.

## Purpose

The function that the firmware starts: it creates the context, chooses the provider, runs the pipeline, and enters the kernel.

## Algorithm

`efi_main(image_handle, system_table)`:

1. Allocate memory for the `JPBootContext` and for the `JPUefiContext` with `AllocatePages` (memory type "loader data", whole pages) and fill it with zeros. The context is not placed on the stack and not in a static variable: it must live on after the boot program ends, and the memory map then marks it `RESERVED`.
2. Fill the `JPUefiContext` with the image handle and the system table, then set `environment.type` to `JP_BOOT_ENVIRONMENT_UEFI`, `environment.platform_context` to the provider context, and `handoff.architecture` to the architecture that the application was built for.
3. (When Stage 4 exists: call `jp_boot_manager_prepare(context)` here, so that it registers its menu hook.)
4. Call `jp_boot_run_pipeline(context)`.
5. If it failed: when `jp_uefi_boot_services_active()` is true, write a short text to the console that names the failing result and the stage at which it happened (`current_stage`) (the text output protocol of the system table, `ConOut`) and return the status "load error" to the firmware; otherwise the services were already left and the processor is halted (the entry calls `jp_<arch>_enter_kernel()` on the context, which halts when the handoff is not `READY`).
6. If it succeeded, call `jp_<arch>_enter_kernel(context)`. It does not return.

## Failure semantics

An allocation failure at step 1 returns "out of resources" to the firmware before anything else happens.

## Verification

Q-level: x86_64 UEFI and AArch64 UEFI in QEMU with the test kernel (the result is the serial protocol of the test kernel).

---

# 29. `src/boot/platform/bios_boot_sector.S`

## File

The BIOS boot sector (16-bit real-mode code, x86_64 only).

## Purpose and algorithm

The file is linked with `src/boot/platform/bios_boot_sector.ld` (address `0x7C00`, signature at `0x7DFE`). Exactly as the paragraph "The boot sector" under "Decided rules (the BIOS provider and its boot chain)": 512 bytes ending with `0x55 0xAA`; saves the boot drive; reads sector 1 and checks the header; reads the loader to `0x8000`; jumps there.

## Verification

`make_image.sh` accepts the file (512 bytes, ends with `0x55 0xAA`); Q-level: x86_64 BIOS in QEMU reaches the loader (the loader writes one character to the serial port at its start).

---

# 30. `src/boot/platform/bios_loader.S`

## File

The BIOS loader (16-bit real-mode code that ends in 64-bit mode, x86_64 only).

## Purpose and algorithm

The file is linked together with the 64-bit code by `src/boot/platform/bios_loader.ld` (address `0x8000`; the 16-bit part is padded to 4 KiB; the script refuses writable static data and a size above 56 KiB). Exactly as the paragraph "The loader" under "Decided rules (the BIOS provider and its boot chain)": A20, E820, video mode, reading the kernel, page tables and GDT, long mode, jump to `jp_bios_main`.

## Invariants

The loader never leaves a block field set that it did not obtain: a missing video mode clears the flag `JP_BIOS_BLOCK_FLAG_VIDEO`, and a kernel that was not read clears `JP_BIOS_BLOCK_FLAG_KERNEL`. It fills the block only through the layout of `bios_block.h`.

## Verification

Q-level: x86_64 BIOS in QEMU; the test kernel prints the memory map, the framebuffer, and the kernel segments that came from the block.

---

# 31. `src/boot/platform/bios_entry.c`

## File

The 64-bit entry point of the BIOS path.

## Purpose and algorithm

`jp_bios_main(const JPBiosBlock *block)`, started by the loader in long mode:

1. Check the block (magic, version, `e820_count`); a bad block halts the processor after one character `!` on the serial port.
2. Zero-fill the memory at `0x00400000` (1 MiB) and use it as the `JPBootContext`; use `0x00500000` as the `JPBiosContext` and put the block pointer into it.
3. Set `environment.type` to `JP_BOOT_ENVIRONMENT_BIOS`, `environment.platform_context`, and `handoff.architecture` to `JP_ARCH_X86_64`.
4. (When Stage 4 exists: call `jp_boot_manager_prepare(context)`.)
5. Call `jp_boot_run_pipeline(context)`.
6. If it failed, write the name of the failing result to the serial port (COM1) and halt the processor; no other output exists in this path.
7. If it succeeded, call `jp_x86_64_enter_kernel(context)`.

## Verification

Q-level: x86_64 BIOS in QEMU with the test kernel.

---

# 32. `src/boot/architecture/x86_64_enter.S` and `src/boot/architecture/aarch64_enter.S`

## File

The assembly parts of the architecture operation `jp_<arch>_enter_kernel`.

## Purpose

The register-level steps of "Decided rules (x86_64: paging, descriptor table, and the jump)" and "Decided rules (AArch64: exception level, translation, and the jump)" cannot be written in C. These two files hold exactly those steps; everything else of the architecture modules is C.

## Invariants

The functions never return and never enable interrupts. They use only the values that are already in the context (`page_table_root`, `stack_pointer`, `kernel_entry`).

## Verification

Q-level only (the steps change the machine state). The state that the test kernel reports (`handoff.paging_enabled`, `handoff.interrupts_disabled`, and a working stack) shows that the jump succeeded.

---

# Stage 1 Execution Order

The complete Stage 1 pipeline is:

```text
Boot Environment Initialization
        ↓
Hardware Discovery
        ↓
Memory Map Discovery
        ↓
Memory Map Validation
        ↓
Framebuffer Initialization
        ↓
Kernel Image Acquisition
        ↓
ELF Header Parsing
        ↓
ELF Validation
        ↓
Program Header Parsing
        ↓
PT_LOAD Discovery
        ↓
PT_LOAD Validation
        ↓
Kernel Entry Validation
        ↓
Kernel Segment Memory Allocation
        ↓
Kernel Segment Loading
        ↓
Kernel Segment Zero-Fill
        ↓
Kernel Segment Mapping
        ↓
Kernel Segment Permission Setup
        ↓
Boot-Time Paging Preparation
        ↓
Kernel Stack Preparation
        ↓
CPU Entry-State Preparation
        ↓
Handoff Prerequisite Validation
        ↓
Pre-Transition Interaction (optional)
        ↓
Platform Handoff Preparation
        ↓
JP_BOOT_STAGE_READY
        ↓
Stage 2 Entry Contract
```

The final line represents a boundary, not a Stage 2 implementation specification.

---

# Stage 1 Architectural Rules

## 1. No Fake State

A state may only be reported after the corresponding operation has actually occurred.

The following are forbidden without real execution:

```text
JP_BOOT_SUCCESS
JP_DETECTION_STATE_DETECTED
JP_KERNEL_VALIDATION_VALID
JP_KERNEL_LOAD_LOADED
JP_BOOT_STAGE_READY
```

---

## 2. Platform Independence

Generic modules:

```text
hardware.c
memory_map.c
framebuffer.c
kernel_image.c
kernel_loader.c
handoff.c
```

must not directly contain UEFI or BIOS implementation logic.

Platform-specific code belongs in:

```text
src/boot/platform/uefi.c
src/boot/platform/bios.c
```

---

## 3. Normalized Data

UEFI and BIOS providers may expose completely different native structures.

Generic Stage 1 modules operate only on normalized structures:

```text
JPHardwareInfo
JPMemoryMap
JPFramebufferInfo
JPKernelImage
JPKernelInfo
JPHandoffInfo
```

---

## 4. ELF Ownership

ELF parsing and kernel loading belong entirely to Stage 1.

Stage 2 receives an already prepared kernel execution environment.

Stage 2 must not be responsible for:

- discovering ELF headers;
- parsing program headers;
- finding `PT_LOAD`;
- copying kernel segments;
- zeroing segment tails;
- allocating kernel image memory;
- creating initial kernel mappings.

---

## 5. Boot Information Ownership

Stage 1 must not define `JPBootInfo`.

No Stage 1 file may include or require `JPBootInfo`.

Any later boot-information system must consume the normalized Stage 1 contract rather than changing it.

---

## 6. Deterministic Execution

Stage 1 operations execute in a defined order.

A failure at a required operation prevents dependent operations from claiming successful completion.

---

## 7. Explicit Failure Propagation

A lower-level failure must propagate to the public Stage 1 API.

An implementation must not silently convert failure into success.

---

## 8. Explicit Ownership

Every architecture-significant resource must have an owner.

Ownership must be defined across:

- provider lifetime;
- kernel loading;
- page-table lifetime;
- kernel stack lifetime;
- kernel segment memory;
- handoff.

---

## 9. Stage Boundary

Stage 1 prepares the machine for kernel execution.

Stage 1 does not become the complete kernel runtime environment merely because it establishes the minimum state required for entry.

---

## 10. No Architectural Invention

If implementation requires an architecture-significant decision that is not defined by the Stage 1 specification, implementation must stop at that decision and treat it as a specification gap.

The implementation must not silently invent the missing rule.

---

# Stage 1 → Stage 2 Boundary

Stage 1 provides the next kernel stage with:

```text
detected boot environment
        +
normalized hardware information
        +
normalized memory information
        +
validated framebuffer
        +
validated kernel image
        +
validated ELF metadata
        +
loaded kernel segments
        +
segment mappings
        +
segment permissions
        +
kernel physical-memory ownership
        +
boot-time page-table state
        +
valid kernel stack
        +
defined CPU execution state
        +
defined interrupt state
        +
validated kernel entry point
        +
defined provider transition state
```

The Stage 2 entry contract is therefore:

```text
x86_64:
    RIP = validated kernel entry address
    RSP = valid initial kernel stack pointer
    CR3 = required page-table root
    CPU = long mode
    Paging = enabled
    IF = 0

AArch64:
    PC = validated kernel entry address
    SP = valid initial kernel stack pointer
    TTBR state = required page-table root
    Exception level = defined
    MMU = enabled as required
    PSTATE.DAIF = 1111

Both architectures:
    Required mappings = established
    Required permissions = established
    Provider state = defined
    Transferred ownership = defined
```

The exact instruction sequence used to establish this state is implementation-defined.

The resulting machine state is not.

Stage 2 must not be required to reconstruct Stage 1 state from provider internals.

Stage 1 does not define the Stage 2 runtime subsystems.

## BootInfo Lifecycle

`JPBootInfo` is not part of Stage 1 or Stage 2. The authoritative Stage 1 to Stage 2 transfer contract is:

```text
JPBootContext
    +
JPHandoffInfo
    +
explicit ownership-transfer state
```

`JPBootContext` remains valid through the Stage 1 to Stage 2 transition and for as long as Stage 2 subsystems require it. `JPHandoffInfo` remains valid until the architecture-specific entry contract has been consumed.

Ownership is normative:

```text
Before handoff:
    Stage 1 owns JPBootContext.

At the Stage 1 → Stage 2 boundary:
    ownership transfers to Stage 2.

After transfer:
    Stage 1 performs no access, mutation, or lifecycle management.

Until kernel_main() no longer requires it:
    Stage 2, the later Milestone 0 stages (Stage 3, Stage 4), and kernel_main() use the same context object.
```

The context is never copied during handoff. Stage 2 may update only runtime, subsystem, and explicitly permitted ownership fields. It must not rewrite historical Stage 1 discovery or firmware state. `JPHandoffInfo` remains embedded in the context as consumed historical/validation state unless a later specification explicitly permits reclamation.

The generic term `BootInfo` must not be used as an alternative name for this contract. Any later `BootInfo` or `JPBootInfo` object must define its own producer, consumer, ABI, ownership, lifetime, conversion source, and stage boundary without redefining this transfer contract.

## Minimal Boot Security Scope

Milestone 0 Boot Security is limited to preserving boot integrity and valid kernel execution. It is not the complete security ecosystem.

Stage 1 Boot Security owns:

- kernel image structural validation;
- ELF class, type, machine, bounds, segment, and entry-point validation;
- rejection of malformed images and unsupported architectures;
- validation of memory regions before use;
- prevention of kernel overlap with reserved or protected memory;
- prevention of invalid boot-time mappings;
- protection of firmware-reserved memory;
- deterministic failure before `READY` when a required validation fails.

Boot validation is not cryptographic authentication. Milestone 0 must not imply image authenticity unless signature verification is separately specified and implemented. Trust, authentication, and structural validation remain distinct concepts.

Identity Engine, Trust Engine, Authentication, Authorization, Runtime Protection, Audit, Recovery, package trust, user/session security, IPC security, and AI security are outside this minimal scope.

---

# 1 Specification Lock Criteria

Stage 1 may be considered **Specification Locked** only when all requirements below are explicitly defined, internally consistent, and independently verifiable.

## Architecture Target

- Supported architectures are x86_64 and AArch64.
- `EM_X86_64` is required for x86_64.
- `EM_AARCH64` is required for AArch64.
- ELF64 is required for both architectures.
- Other architectures are excluded unless separately specified.

## Platform Target

```text
x86_64 / UEFI:  SUPPORTED
x86_64 / BIOS:  SUPPORTED
AArch64 / UEFI: SUPPORTED
AArch64 / BIOS:  NOT SUPPORTED
```

Each supported architecture/provider combination has a complete normative contract and functional implementation requirement.

## Provider Contract

Every supported provider defines:

- initialization;
- inputs;
- outputs;
- context ownership;
- context lifetime;
- destruction/invalidation;
- failure behavior;
- normalized result semantics;
- handoff responsibility.

## Hardware Discovery

CPU, firmware, and boot-device discovery have explicit ownership.

No undocumented second firmware-detection path exists.

## Pre-Transition Interaction

- Provider input is normalized (`JPBootInputEvent`) and defined for UEFI (x86_64 and AArch64) and BIOS.
- The pre-transition interaction hook, its position, exactly-once rule, restrictions, and failure semantics are defined.
- No memory-map key is held across the interaction point.
- Absence of a hook is explicit and never reported as completed interaction.

## Firmware Metadata

Only firmware metadata required by Stage 1 is normalized.

Other metadata remains outside the Stage 1 contract.

## Boot Device

`JPBootDeviceInfo` is explicitly defined as a normalized discovery-state contract unless device identity is promoted into the normative Stage 1 requirements.

## Memory Map

The contract defines:

- address representation;
- length;
- type;
- validity;
- overflow;
- ordering;
- alignment where required;
- overlap policy;
- ownership.

## Framebuffer

The contract defines:

- base;
- dimensions;
- pitch;
- format;
- channel order;
- bits per pixel;
- bytes per pixel;
- unused bits;
- memory interpretation.

## Kernel Image

The contract defines:

- source;
- image availability;
- storage;
- size;
- ownership;
- lifetime;
- validation state.

## ELF

The contract defines:

- class;
- machine;
- object type;
- header fields;
- program headers;
- `PT_LOAD`;
- bounds;
- overflow;
- alignment;
- file/memory ranges;
- zero-fill;
- entry point;
- failure semantics.

## Kernel Segment Lifecycle

Every loadable segment has a verifiable lifecycle:

```text
UNINITIALIZED
VALIDATED
ALLOCATED
LOADED
MAPPED
PERMISSIONED
```

Failure and invalid states are distinguishable.

## Kernel Memory Ownership

Physical memory ownership is explicit for:

- kernel image;
- kernel segments;
- page tables;
- kernel stack;
- other permanent Stage 1 execution memory.

## Paging

The minimum boot-time paging state is defined.

The complete kernel virtual-memory subsystem is not implicitly part of Stage 1.

## Kernel Stack

The contract defines:

- physical address;
- virtual address;
- size;
- alignment;
- mapping;
- permissions;
- ownership;
- lifetime;
- initial stack pointer.

## CPU Entry State

The contract defines:

- x86_64 long mode;
- paging enabled;
- page-table root;
- stack;
- kernel entry;
- interrupt state;
- required CPU state.

## Handoff

The contract distinguishes:

1. preparation;
2. final state acquisition;
3. final memory-map acquisition;
4. transition readiness;
5. actual transition;
6. execution transfer.

UEFI final memory-map key semantics are explicit.

BIOS handoff semantics are explicit.

## Result Semantics

`JPBootResult` remains coarse-grained.

Detailed diagnostic classification is not required for Specification Lock.

## Cross-Stage Contract

Stage 1 → Stage 2 ownership, mapping, stack, paging, CPU state, interrupt state, architecture state, and provider-transition semantics are explicit for both supported architectures.

## Implementation Freedom

Implementation mechanisms remain free where they do not alter:

- observable behavior;
- ownership;
- lifetime;
- invariants;
- architecture-significant state;
- cross-stage contracts.

---

# 1 Specification Traceability

Every Specification Lock Criterion shall be traceable to one or more normative Stage 1 definitions.

A criterion is satisfied only when:

- required behavior is explicitly defined;
- a normative owner section exists;
- dependent definitions are consistent;
- required behavior is observable or verifiable;
- no architecture-significant interpretation remains necessary.

A criterion is not satisfied merely because an implementation chooses a reasonable behavior.

Conversely, the absence of a prescribed implementation mechanism is not a specification gap when the normative behavior is fully defined.

---

# 1 Specification Completeness Verification

Before Stage 1 may receive Specification Locked status, a completeness verification pass shall be performed.

Each criterion is evaluated using:

```text
Criterion
    ↓
Normative Definition
    ↓
Exact Specification Location
    ↓
Dependent Definition Consistency
    ↓
Architecture-Significant Ambiguity Check
    ↓
Observable / Verifiable Boundary
    ↓
PASS / FAIL
```

Verification shall cover at minimum:

- architecture target;
- UEFI target;
- BIOS target;
- provider contracts;
- provider context ownership;
- provider lifetime;
- boot environment;
- CPU discovery;
- firmware discovery;
- boot-device discovery;
- firmware metadata;
- memory-map semantics;
- memory ownership;
- overlap policy;
- framebuffer semantics;
- kernel image contract;
- ELF semantics;
- ELF type support;
- segment lifecycle;
- segment permissions;
- kernel physical-memory ownership;
- boot-time paging;
- kernel stack;
- CPU entry state;
- interrupt state;
- kernel entry state;
- UEFI memory-map key;
- `ExitBootServices()` semantics;
- BIOS handoff semantics;
- handoff state;
- public result semantics;
- Stage 1 → Stage 2 boundary;
- Stage 1 non-goals;
- implementation freedom;
- prohibition of architectural invention.

A criterion fails if:

- behavior is not normatively defined;
- definitions contradict one another;
- an architecture-significant decision is left to implementation;
- required state cannot be verified;
- ownership/lifetime is undefined where cross-stage behavior depends on it;
- a supported provider lacks a complete normative contract.

A criterion passes only when the required behavior is explicitly defined, traceable, consistent, and independently verifiable.

The verification result shall be recorded as:

```text
Specification Completeness Verification: PASS
```

or:

```text
Specification Completeness Verification: FAIL
```

A `FAIL` result prevents Specification Lock.

---

# 1 Specification Lock Decision

Stage 1 may be declared **Specification Locked** only when:

- all Specification Lock Criteria pass;
- Specification Traceability is complete;
- Specification Completeness Verification is `PASS`;
- no architecture-significant ambiguity remains;
- no cross-stage ownership or lifetime ambiguity remains;
- UEFI has a complete normative contract;
- BIOS has a complete normative contract;
- Stage 1 → Stage 2 entry state is explicitly defined;
- `JPBootInfo` is not a Stage 1 dependency;
- implementation completion is not being used as evidence of specification completeness.

Specification Lock confirms that the specification is sufficiently complete to constrain independent implementations without requiring them to invent architecture-significant behavior.

---

# 1 Implementation Acceptance

Implementation Acceptance is separate from Specification Lock.

Specification Lock means:

```text
the contract is complete
```

Implementation Acceptance means:

```text
the implementation satisfies the contract
```

A provider declared as `SUPPORTED` must be functionally implemented and verified against its normalized provider contract.

Stage 1 implementation acceptance requires separate passing acceptance for each supported architecture/provider combination:

```text
x86_64 / UEFI:  required
x86_64 / BIOS:  required
AArch64 / UEFI: required
AArch64 / BIOS: not applicable and must be rejected
```

For the required combinations:

- provider implemented;
- architecture-specific entry/handoff path implemented;
- normalized provider contract verified;
- boot-environment initialization functional;
- hardware discovery functional;
- memory-map acquisition and validation functional;
- framebuffer acquisition and validation functional;
- kernel acquisition functional;
- ELF validation functional;
- segment loading functional;
- segment mapping functional;
- permissions functional;
- boot-time paging functional;
- kernel stack preparation functional;
- CPU entry state preparation functional;
- handoff preparation functional;
- pre-transition interaction functional (hook absent, succeeding, failing) with provider input verified per supported provider;
- no fake success states;
- no implementation-defined behavior violating the specification.

Acceptance must include at least one successful and one failure case for each required architecture, including AArch64 UEFI handoff, AArch64 ELF machine validation, AArch64 entry state, and architecture-mismatch rejection.

Implementation completion does not retroactively define unspecified behavior.

---

# 1 Final Lock Statement

Following a successful Specification Completeness Verification:

> **Stage 1 Specification Status: SPECIFICATION LOCKED**
>
> All architecture-significant requirements within the scope of Stage 1 are explicitly defined, traceable to normative requirements, internally consistent, and independently verifiable.
>
> The supported target architectures are x86_64 and AArch64.
>
> UEFI is required for both x86_64 and AArch64. BIOS is required only for x86_64. Every supported architecture/provider combination must satisfy a complete normalized provider contract.
>
> Stage 1 defines the required boot environment, hardware discovery, memory-map semantics, framebuffer representation, kernel acquisition, ELF validation, segment lifecycle, kernel memory ownership, kernel mappings, segment permissions, boot-time paging, kernel stack, CPU entry state, interrupt state, kernel entry state, and platform handoff semantics.
>
> The Stage 1 → Stage 2 execution boundary is explicitly defined. The kernel receives a validated entry point, valid stack, required page-table root, enabled paging, established mappings and permissions, defined CPU execution state, defined interrupt state, and explicitly transferred ownership.
>
> `JPBootInfo` is not part of Stage 1 and is not a Stage 1 dependency.
>
> `JPBootContext` has defined semantic ownership, lifetime, validity, and cross-stage responsibilities and is not treated as an implicit or unrestricted state container.
>
> Implementation mechanisms that do not affect the normative observable contract remain implementation-defined.
>
> No architecture-significant implementation decision remains implicitly delegated to the implementation within the defined Stage 1 scope.
>
> Specification Lock does not imply implementation completion. Implementation completion is governed separately by the Stage 1 Implementation Acceptance criteria.
>
> Therefore:
>
> ```text
> Stage 1 Specification Status: SPECIFICATION LOCKED
> ```
>
> only after **Specification Completeness Verification: PASS**.

---

# 2 Stage 2 — Platform Initialization

# 2 Stage 2 Definition

Stage 2 — Platform Initialization defines the early kernel-side platform initialization performed after the Stage 1 boot pipeline has successfully prepared and transferred the kernel execution environment.

Stage 2 begins at the Stage 1 → Stage 2 boundary.

Stage 2 does not perform firmware-side boot operations.

Stage 2 consumes the execution environment established by Stage 1 and initializes the kernel runtime environment, and hands the prepared environment to the Stage 2 completion continuation. Stage 2 does not provide diagnostics, recovery, or Boot UI.

The normative Stage 2 initialization flow is:

```text
Stage 1 Handoff
      │
      ▼
Architecture-specific Stage 2 entry stub
    │
    ▼
jp_kernel_init(context)
    │
    ▼
CPU
      │
      ▼
Memory
      │
      ▼
Paging
      │
      ▼
Stack
      │
      ▼
Heap
      │
      ▼
ELF
      │
      ▼
Stage 2 Complete
    │
    ▼
Stage 2 completion continuation
```

The Stage 2 completion continuation is the single non-returning transfer point that follows `Stage 2 Complete`. It receives the same `JPBootContext *` under the calling convention and machine state defined by the entry-stub contract.

Stage 2 fully prepares the kernel runtime environment and exits through the continuation. The Milestone 0 flow that follows Stage 2 is Stage 3 (normal entry), then Stage 4, then `kernel_main(context)`. The continuation is the symbol `jp_kernel_stage2_complete(JPBootContext *context)`, declared by Stage 2 in `include/justos/kernel/init.h` as non-returning and implemented by the Milestone 0 orchestration owner (Stage 4, `boot_manager.c`). Stage 2 depends only on that declaration; it does not implement the continuation and does not depend on Stage 3 or Stage 4 behavior. The failure sink transfers to the same symbol after recording a non-success result; the continuation selects the Stage 3 normal or failure entry from the recorded Stage 2 result.

The initialization sequence is deterministic.

Each subsystem must establish the required state for the next subsystem before that subsystem may be considered successfully initialized.

`init` acts as the Stage 2 orchestration component.

It coordinates initialization but does not replace the responsibilities of the individual subsystems.

---

# 2 Specification Lock

Stage 2 is a Specification Lock.

Before implementation begins, this Stage must define the complete technical contract required for its implementation.

The Stage 2 specification must define, where applicable:

- architecture target;
- platform assumptions;
- Stage 1 input contract;
- subsystem responsibilities;
- subsystem boundaries;
- initialization order;
- data ownership;
- resource lifetime;
- state transitions;
- public interfaces;
- internal responsibilities;
- algorithms;
- invariants;
- failure semantics;
- dependencies;
- Stage 2 → Stage 3 boundary;
- completion criteria;
- verification requirements.

Implementation must not introduce architecture-significant behavior that is not defined by the Stage 2 specification.

If implementation requires an architecture-significant decision that is missing or ambiguous, implementation must stop for the affected area and report the specification gap.

The specification must be updated and re-verified before implementation continues.

---

# 2 Architecture Target

Stage 2 targets:

```text
x86_64:  ACTIVE AND REQUIRED IMPLEMENTATION TARGET
AArch64: ACTIVE AND REQUIRED IMPLEMENTATION TARGET
```

Stage 2 operates on the architecture-specific execution environment established by Stage 1.

Architecture-dependent behavior required by Stage 2 must be explicitly defined by the Stage 2 specification.

This includes, where applicable:

- CPU initialization;
- paging;
- virtual-memory initialization;
- stack initialization;
- heap initialization;
- kernel entry state;
- low-level kernel runtime preparation.

Architectures other than x86_64 and AArch64 are outside the Stage 2 target.

Both x86_64 and AArch64 require complete architecture-specific implementation and acceptance before Milestone 0 can close. Supporting another architecture requires a separate approved architecture target and corresponding specification extension.

Stage 2 must not silently generalize architecture-specific behavior between x86_64 and AArch64.

## Architecture-Specific Stage 2 Contract

Generic Stage 2 responsibilities and sequencing are shared by both architectures. Architecture-specific mechanisms remain isolated within the relevant subsystem and entry implementation.

```text
x86_64:
    long mode
    CR3 page-table root
    x86_64 page tables
    RSP stack state
    System V AMD64-compatible C calling convention
    IF = 0 during Stage 2

AArch64:
    defined exception level
    TTBR page-table state
    AArch64 translation tables
    SP stack state
    AArch64 C calling convention
    PSTATE.DAIF = 1111 during Stage 2
```

Each Stage 2 component must provide the equivalent contract, ownership, lifecycle, failure semantics, and verification for both architectures. A component specification that currently describes x86_64 mechanisms is incomplete until its AArch64 counterpart is defined.

## Stage 2 Architecture Contract Matrix

The following matrix is normative for every Stage 2 implementation. Each component has one common behavioral contract and two architecture-specific machine contracts. The architecture-specific implementation may differ, but the resulting observable state, ownership, lifetime, failure behavior, and verification boundary must satisfy the common contract.

### Common Stage 2 Contract

Every component receives the same non-copied `JPBootContext *` transferred by the Stage 1 handoff.

Every component must validate inherited state before mutation, preserve memory ownership, keep interrupts masked, avoid firmware and `JPBootInfo`, report success only after actual completion, propagate failure, and keep architecture-specific state inside its architecture boundary.

### `cpu`

Common:

```text
validate inherited CPU state
validate architecture compatibility
establish required early runtime CPU state
keep interrupts masked
validate final CPU state
```

x86_64:

```text
long mode
required control/register state valid
RFLAGS.IF = 0
```

AArch64:

```text
exception level = EL1
required PSTATE state valid
PSTATE.DAIF = 1111
SCTLR_EL1 state is valid for the inherited MMU regime
TCR_EL1 and MAIR_EL1 state is preserved for paging.c
```

`cpu` may access architecture registers and instructions but may not construct page tables, allocate heap memory, initialize the stack, or access firmware.

### `memory`

Architecture boundary:

```text
memory.c is the sole physical-memory ownership authority
paging.c consumes PAGE_TABLES ownership
stack.c consumes KERNEL_STACK ownership
heap.c consumes EARLY_HEAP ownership
no independent physical allocators
```

The complete memory ownership classes, operations, conflict semantics, and x86_64/AArch64 range rules are defined only in `include/justos/kernel/memory.h` and `src/kernel/memory.c`.

### `paging`

Common:

```text
adopt Stage 1 paging state
create or extend required kernel mappings
preserve segment permissions
map kernel entry and stack
validate active translation state
```

x86_64:

```text
CR3 identifies the active root
long-mode paging is enabled
PML4/PDPT/PD/PT hierarchy is valid
virtual addresses are canonical
required TLB synchronization is complete
```

AArch64:

```text
EL1 translation regime
4 KiB translation granule
48-bit virtual-address regime
TTBR1_EL1 identifies kernel translation tables
TTBR0_EL1 holds the identity map; Stage 1 builds the first one and paging.c replaces it with a W^X identity map at the same addresses (see "Decided rules (replacing the identity map)")
TCR_EL1 and MAIR_EL1 are valid for selected mappings
translation descriptors are 64-bit
table descriptors select the next level
block/page descriptors select mapped memory
valid, access-flag, shareability, read/write, execute-never, and memory-attribute bits are defined for every mapping
required TLBI/DSB/ISB synchronization is complete
```

Both implementations map the kernel entry, segments, stack, and required runtime memory with the common permission contract. Mapping ownership belongs to `paging`; physical page ownership belongs to `memory`.

### `stack`

Common:

```text
consume KERNEL_STACK ownership
validate active stack range
validate mapping and write permission
validate ABI alignment
preserve active stack lifetime
do not switch to an undocumented replacement stack
```

x86_64 validates `RSP` and System V AMD64-compatible alignment. AArch64 validates `SP` and AArch64 ABI alignment.

### `heap`

Common:

```text
consume EARLY_HEAP ownership
use one initially assigned region
minimum alignment = 16 bytes
allocation granularity = 16 bytes
zero-size allocation = NULL
free is public and required
exhaustion = NULL
growth and additional physical-page requests = forbidden in Stage 2
```

x86_64 validates the heap virtual range in the active x86_64 address space. AArch64 validates it in the active AArch64 translation regime using mappings established by `paging`.

### `elf`

Common:

```text
validate the already loaded kernel image state
validate architecture and segment consistency
validate loaded/mapped/permissioned lifecycle
validate executable entry point
do not acquire, copy, relocate, map, or permission segments
```

x86_64 requires `EM_X86_64`. AArch64 requires `EM_AARCH64`. Stage 2 ELF is loaded-kernel-image consistency validation only, not a second loader.

### `init`

Common:

```text
receive one JPBootContext pointer
run CPU → Memory → Paging → Stack → Heap → ELF serially
stop at first failure
return the original subsystem result
return success only after all subsystems succeed
```

x86_64 preserves the x86_64 entry state. AArch64 preserves the AArch64 entry state. The architecture-specific entry stubs own the transition to the Stage 2 completion continuation; `init` never performs that transfer directly.

### Verification Rule

Each component must have at least one valid x86_64 case, one valid AArch64 case, one architecture-mismatch case, one invalid inherited-state case, one ownership-conflict case where applicable, and one failure-propagation case. A component is not Specification Locked if its AArch64 behavior is only described as equivalent without observable state and failure conditions.

---

# 2 Platform Target

Stage 2 begins from the normalized kernel execution environment produced by Stage 1.

Stage 2 does not directly interact with firmware.

The following are outside the Stage 2 platform interface:

- UEFI boot services;
- BIOS services;
- GOP;
- VBE;
- firmware-native memory descriptors;
- firmware-native boot-device interfaces;
- firmware-native kernel loading;
- firmware-specific ELF acquisition.

Platform-specific boot behavior has already been resolved by Stage 1.

Stage 2 consumes the normalized Stage 1 execution contract.

The Stage 2 platform boundary is therefore:

```text
Stage 1
    │
    │ normalized kernel execution environment
    ▼
Stage 2
    │
    │ kernel-side platform initialization
    ▼
Kernel Runtime
```

Stage 2 must not reconstruct platform information from firmware-specific state.

---

# 2 Stage 2 Responsibilities

Stage 2 is responsible for:

- validating the CPU state inherited from Stage 1;
- performing the CPU/platform initialization required by the early kernel runtime;
- initializing the kernel-side memory representation;
- adopting the memory state transferred from Stage 1;
- initializing kernel-side paging;
- establishing the kernel-side virtual-memory foundation;
- initializing the kernel stack runtime state;
- initializing the early kernel heap;
- validating the already loaded kernel ELF representation where required;
- coordinating early kernel subsystem initialization;
- propagating initialization failures;
- establishing the kernel runtime state required at the Stage 2 completion continuation.

The responsibilities are divided between the following components:

```text
cpu
memory
paging
stack
heap
elf
init
```

The roles are:

```text
cpu
    CPU and architecture/platform initialization

memory
    kernel-side memory state and memory initialization

paging
    page-table and kernel-side virtual-memory initialization

stack
    kernel stack runtime initialization

heap
    early kernel heap initialization

elf
    runtime handling and validation of the already loaded kernel ELF state

init
    orchestration of Stage 2 initialization
```

`init` is not an additional subsystem implementation.

It is the coordinator of the Stage 2 initialization sequence.

No subsystem may silently absorb the responsibilities of another subsystem.

## Stage 2 Physical-Memory Ownership Boundary

The detailed physical-memory ownership contract is defined by `memory.h` and `memory.c`. At the Stage 2 architecture boundary, only these rules are repeated:

```text
memory.c
    = sole physical-memory ownership authority

paging.c
    → consumes PAGE_TABLES ownership

stack.c
    → consumes KERNEL_STACK ownership

heap.c
    → consumes EARLY_HEAP ownership
```

`paging.c`, `stack.c`, and `heap.c` must not contain independent physical allocators or independently claim `USABLE` memory. They consume typed ownership through the memory subsystem and propagate ownership conflicts.

---

# 2 Stage 2 Non-Goals

Stage 2 does not implement:

- Boot Manager;
- Boot Manager UI;
- `JPBootInfo`;
- boot configuration;
- UEFI interaction;
- BIOS interaction;
- firmware initialization;
- kernel image acquisition;
- initial kernel ELF acquisition;
- initial kernel ELF loading;
- initial `PT_LOAD` discovery performed by Stage 1;
- kernel segment copying performed by Stage 1;
- initial Stage 1 kernel mappings;
- Stage 3 diagnostics;
- Stage 3 recovery;
- Stage 4 boot-information integration;
- desktop services;
- user-space execution;
- scheduler;
- complete process management;
- networking;
- JSL runtime;
- application runtime;
- Desktop Environment functionality.

Stage 2 must not recreate a Stage 1 responsibility merely because the corresponding state originated in Stage 1.

Stage 2 consumes the Stage 1 contract.

Stage 2 must not depend on:

- `JPBootInfo`;
- Stage 3 implementation;
- Stage 4 implementation;
- undocumented Stage 1 internals.

---

# 2 Folder Structure

```text
include/justos/kernel/
├── cpu.h
├── paging.h
├── stack.h
├── heap.h
├── memory.h
├── elf.h
└── init.h

src/kernel/
├── entry_x86_64.S
├── entry_aarch64.S
├── cpu.c
├── paging.c
├── stack.c
├── heap.c
├── memory.c
├── elf.c
└── init.c
```

The production folder structure contains only Stage 2 implementation files.

Test files are not part of the production folder structure.

Testing is defined separately by the Stage 2 verification and implementation-acceptance requirements.

Every file listed above requires a complete normative File Specification before implementation begins.

**Private machine-access files (added to this list after the first implementation; they were first named in the "Decided rules" blocks of `cpu`, `paging`, `stack`, `init`).** A C program cannot read a control register or stop the processor, and the unit tests on the host cannot run such instructions. So every Stage 2 module reaches the machine only through small private headers, and the host tests replace them with fakes. These files hold no logic: each function is one or two instructions (inline assembly). They are private to `src/kernel` (no public header names them), and each `.c` file is built only for its own architecture.

```text
src/kernel/
├── arch_dispatch.h        chooses the architecture code by handoff.architecture; the build macros
│                          JP_KERNEL_ONLY_X86_64 / JP_KERNEL_ONLY_AARCH64 remove the other architecture's code
├── cpu_regs.h             read the registers that cpu.c checks, write EFER.NXE and CR0.WP, CPUID, the
│                          physical-address width; the constants of the bits
├── cpu_regs_x86_64.c      the x86_64 functions of cpu_regs.h
├── cpu_regs_aarch64.c     the AArch64 functions of cpu_regs.h
├── paging_hw.h            table address to pointer, read and write the root register, set WXN
├── paging_hw_x86_64.c     CR3
├── paging_hw_aarch64.c    TTBR0_EL1 with the TLB synchronization, SCTLR_EL1.WXN
├── stack_hw.h             read the stack pointer
├── stack_hw_x86_64.c      RSP
├── stack_hw_aarch64.c     SP
├── halt_hw.h              stop the processor
├── halt_hw_x86_64.c       cli and hlt in a loop
└── halt_hw_aarch64.c      mask all interrupts and wfi in a loop
```

The kernel (and the test kernel) is built with `-ffreestanding -fno-builtin -mgeneral-regs-only` and, on x86_64, `-mno-red-zone` (without it the stack pushes of the register reader damage local variables of the C code; this was found by the first QEMU run).

---

# File Specifications

# 0. `src/kernel/entry_x86_64.S` and `src/kernel/entry_aarch64.S`

## File

Architecture-specific Stage 2 entry stubs.

## Purpose

Receive the same `JPBootContext *` produced by the Stage 1 handoff, establish the architecture-specific C-call boundary, invoke `jp_kernel_init(context)`, and transfer successful execution to the Stage 2 completion continuation.

## Common Contract

```text
Stage 1 final handoff
    ↓
architecture-specific entry symbol
    ↓
jp_kernel_init(context)
    ↓
Stage 2 completion continuation
```

The context pointer is not copied. It remains the same runtime object throughout Stage 2 and the initial kernel runtime entry.

## x86_64 Contract

```text
Symbol: just_kernel_entry_x86_64
Entry state: long mode, paging enabled, CR3 valid, IF = 0
Argument: JPBootContext * in the first System V AMD64-compatible argument register
Call target: jp_kernel_init(JPBootContext *)
Success target: jp_kernel_stage2_complete(JPBootContext *)
```

## AArch64 Contract

```text
Symbol: just_kernel_entry_aarch64
Entry state: required exception level, MMU enabled, TTBR state valid, PSTATE.DAIF = 1111
Argument: JPBootContext * in X0
Call target: jp_kernel_init(JPBootContext *)
Success target: jp_kernel_stage2_complete(JPBootContext *)
```

## Ownership and Failure

Ownership of `JPBootContext` transfers from Stage 1 to Stage 2 at handoff. After transfer, Stage 1 performs no access, mutation, or lifecycle management of the context. Stage 2 owns it for as long as Stage 2 subsystems require it; after the Stage 2 completion continuation the same context object is used by the later Milestone 0 stages.

If `jp_kernel_init(context)` returns `JP_BOOT_SUCCESS`, the stub transfers control to the Stage 2 completion continuation and does not return to Stage 1. If it returns any other result, the stub does not enter the Stage 2 completion continuation directly and transfers the result to `jp_kernel_stage2_failure()`. The stub does not implement diagnostics or recovery itself.

The canonical failure symbol is:

```c
_Noreturn void jp_kernel_stage2_failure(
    JPBootContext *context,
    JPBootResult result
);
```

The symbol is implemented by `src/kernel/init.c` and declared by `include/justos/kernel/init.h`. On x86_64 the context pointer and result follow the System V AMD64-compatible C ABI. On AArch64 the context pointer is passed in `X0` and the result is passed in `W1` according to the entry-stub failure call contract.

The failure path stores the result in `context->stage2_result`, preserves the context and ownership state, and does not return to Stage 1, and transfers to `jp_kernel_stage2_complete(context)` as its only exit. It performs no diagnostics or recovery itself and has no dependency on Stage 3; the Stage 3 failure entry is selected by the continuation from the recorded result.

## Boundary Rules

The stubs must not duplicate subsystem initialization, modify firmware state, create a second context, construct `JPBootInfo`, or enable interrupts. `JPBootContext` and `JPHandoffInfo` remain valid through the transition; `JPHandoffInfo` becomes preserved historical/validation state after the entry contract is consumed.

## Verification

Verify both symbols, argument passing, calling conventions, success transfer, failure transfer, interrupt masking, context identity, and non-return behavior.

# 1. `include/justos/kernel/cpu.h`

## File

Stage 2 CPU initialization contract.

## Purpose

`cpu.h` defines the public kernel-side CPU initialization interface used during Stage 2.

The file establishes the contract for validating and initializing the CPU execution state inherited from Stage 1 and preparing the CPU state required for continued kernel execution.

The file contains declarations and contract definitions only.

It contains no runtime implementation.

It contains no architecture-specific instruction sequences.

Architecture-specific implementation details belong to `cpu.c`.

---

## Responsibilities

The CPU subsystem is responsible for:

- validating the CPU execution state inherited from Stage 1;
- verifying that the current architecture matches the Stage 2 target;
- performing early kernel CPU initialization;
- establishing the CPU runtime state required by later Stage 2 subsystems;
- exposing the normalized CPU initialization interface;
- publishing CPU initialization status to the Stage 2 orchestrator.

The CPU subsystem does **not** perform:

- paging initialization;
- virtual-memory mapping;
- stack allocation;
- heap initialization;
- ELF loading;
- firmware interaction;
- Stage 1 boot operations.

---

## Types / Structures

The public interface exposes no provider-native or architecture-native structures.

The subsystem consumes the normalized types already defined by `boot_types.h`.

Architecture-specific runtime state remains implementation-defined inside `cpu.c`.

---

## Constants / Enums

The CPU subsystem reuses the architecture definitions from Stage 1.

No duplicated architecture constants are permitted.

A normalized CPU lifecycle is required conceptually:

```c
JP_CPU_STATE_UNKNOWN = 0,
JP_CPU_STATE_NOT_INITIALIZED,
JP_CPU_STATE_VALIDATED,
JP_CPU_STATE_INITIALIZED,
JP_CPU_STATE_FAILED
```

The exact storage representation remains implementation-defined.

---

## Public API

```c
JPBootResult jp_kernel_cpu_initialize(
    JPBootContext *context
);
```

The function:

- validates the Stage 2 context;
- validates the inherited CPU state;
- performs required CPU initialization;
- publishes the resulting CPU state;
- returns success only after actual initialization.

---

## Internal API

Implementation helpers remain private:

```text
validate_cpu_state()
validate_cpu_architecture()
initialize_cpu_state()
initialize_cpu_features()
validate_final_cpu_state()
publish_cpu_state()
```

Additional architecture-specific helpers may exist internally.

---

## Algorithm

### CPU initialization pipeline

```text
Stage 1 CPU state
        ↓
Validate inherited state
        ↓
Validate architecture
        ↓
Initialize CPU runtime
        ↓
Enable required CPU features
        ↓
Validate resulting state
        ↓
Publish initialized state
```

### Initialization sequence

1. Validate `JPBootContext`.
2. Verify the inherited Stage 1 CPU state.
3. Verify the active architecture.
4. Validate architecture-specific prerequisites.
5. Initialize the required CPU runtime state.
6. Enable mandatory CPU features.
7. Validate the resulting CPU state.
8. Publish `INITIALIZED`.
9. Return `JP_BOOT_SUCCESS`.

Failure at any mandatory step terminates CPU initialization.

---

## Stage 1 Input Contract

Stage 2 consumes the CPU execution environment established by Stage 1.

For the selected architecture target:

```text
Architecture:
    x86_64 or AArch64

Execution mode:
    architecture-specific state defined by the selected contract

CPU state:
    Valid kernel execution state

Interrupt state:
    Defined by Stage 1 handoff

Kernel entry:
    Canonical executable entry point
```

Stage 2 must not assume CPU guarantees that are absent from the Stage 1 contract.

---

## Architecture Requirements

After successful initialization the CPU must satisfy:

```text
x86_64:
    long mode active
    required control/register state valid
    RFLAGS.IF = 0
    canonical kernel execution state established

AArch64:
    EL1 active
    required system-register state valid
    PSTATE.DAIF = 1111
    required kernel execution state established

Both architectures:
    mandatory features available
    interrupt state masked
```

The specification defines the required observable state rather than the instruction sequence used to achieve it.

---

## CPU Features

Feature initialization follows:

```text
Required feature
        ↓
Availability verified
        ↓
Configuration performed
        ↓
State validated
```

Mandatory features are required for successful initialization.

Optional features do not affect Stage 2 success unless explicitly required elsewhere.

---

## State / Lifecycle

```text
UNKNOWN
    ↓
NOT_INITIALIZED
    ↓
VALIDATED
    ↓
INITIALIZED
```

Failure:

```text
UNKNOWN / NOT_INITIALIZED / VALIDATED
                ↓
             FAILED
```

`INITIALIZED` may only be published after all required CPU conditions are satisfied.

---

## Ownership / Lifetime

- CPU initialization state belongs to the Stage 2 runtime.
- Architecture-specific register state remains CPU-owned.
- Other subsystems may consume the CPU contract but may not modify CPU initialization state directly.
- The CPU subsystem remains responsible for CPU validity throughout Stage 2.

---

## Invariants

Successful CPU initialization guarantees:

```text
Architecture == the selected supported Stage 2 architecture

Valid execution state

Required Stage 1 guarantees preserved

Mandatory CPU features initialized

CPU runtime internally consistent
```

Forbidden combinations:

```text
INITIALIZED + invalid CPU state

INITIALIZED + unsupported architecture

INITIALIZED + failed prerequisite

INITIALIZED + incomplete mandatory configuration
```

---

## Dependencies

```c
#include "justos/boot/boot_types.h"
```

The header must not include:

- UEFI headers
- BIOS headers
- GOP
- VBE
- firmware-native structures
- architecture assembly headers

---

## Boundary Rules

The CPU subsystem is the only Stage 2 subsystem responsible for CPU initialization.

Dependencies:

```text
CPU
 ├── Paging (depends on CPU)
 ├── Stack  (depends on CPU)
 └── Init   (orchestrates CPU)
```

The CPU subsystem must never initialize:

- page tables;
- heap;
- ELF;
- firmware state;
- Stage 1 resources.

---

## Failure Semantics

CPU initialization fails when:

- `context` is invalid;
- inherited CPU state is invalid;
- architecture mismatch occurs;
- a mandatory CPU feature is unavailable;
- required CPU initialization cannot complete;
- final CPU validation fails.

A partially initialized CPU must never be reported as initialized.

---

## Implementation Freedom

The implementation may freely choose:

- instruction ordering;
- MSR access mechanism;
- internal helper decomposition;
- assembly/C boundary;
- feature-detection strategy;
- temporary state representation.

Only the observable Stage 2 CPU contract is normative.

---

## Verification

### Successful initialization

```text
Valid Stage 1 CPU state
        ↓
CPU initialization
        ↓
INITIALIZED
```

### Invalid inherited state

```text
Invalid Stage 1 state
        ↓
Initialization rejected
        ↓
FAILED
```

### Architecture mismatch

```text
Boot architecture ≠ Stage 2 target
                ↓
            FAILED
```

### Mandatory feature unavailable

```text
Required CPU feature missing
            ↓
         FAILED
```

### Final validation

Verify that:

```text
UNINITIALIZED ≠ INITIALIZED

FAILED ≠ INITIALIZED

INITIALIZED only after complete CPU initialization
```

---

# 2. `src/kernel/cpu.c`

## File

Stage 2 CPU initialization implementation.

## Purpose

`cpu.c` implements the x86_64 CPU initialization required after the Stage 1 → Stage 2 transition.

The module consumes the CPU execution state established by Stage 1 and establishes the CPU state required by the Stage 2 kernel runtime.

The module does not perform firmware-side operations.

The module does not acquire or load the kernel image.

The module does not construct `JPBootInfo`.

## Responsibilities

- Validate the CPU state inherited from Stage 1.
- Validate the active architecture.
- Validate the required execution state of the selected architecture.
- Establish CPU state required by the early kernel runtime.
- Establish the CPU state required by subsequent Stage 2 subsystems.
- Establish the explicitly defined interrupt state.
- Validate the resulting CPU state.
- Report initialization success only after the required CPU state has actually been established.

## Architecture Target

```text
Architecture:
    x86_64 or AArch64

Execution mode:
    architecture-specific state defined by the selected contract
```

AArch64 is an equally required Stage 2 implementation target. Architectures other than x86_64 and AArch64 are outside the current Stage 2 implementation target.

The implementation must not silently interpret another architecture as x86_64.

## Architecture-Specific Contract

The common CPU contract is inherited-state validation, required early-runtime setup, masked interrupts, final-state validation, and deterministic failure.

For x86_64:

```text
long mode
RFLAGS.IF = 0
required control/register state valid
```

For AArch64:

```text
EL1
PSTATE.DAIF = 1111
required system-register state valid
```

The selected architecture's state is the only valid success condition. The implementation must not accept x86_64 state for AArch64 or the reverse.

## Public API

```c
JPBootResult jp_kernel_cpu_initialize(
    JPBootContext *context
);
```

The function receives the Stage 2 runtime context.

`JPBootContext` remains owned by the Stage 2 runtime.

## Internal API

The following operations remain internal to `cpu.c`:

```text
validate_cpu_context()
validate_cpu_architecture()
validate_cpu_execution_mode()
validate_cpu_entry_state()
initialize_cpu_state()
validate_cpu_runtime_state()
```

The exact helper decomposition is implementation-defined.

## Algorithm

### Context Validation

1. Validate that `context` is not `NULL`.
2. Validate that the Stage 1 handoff state is available.
3. Validate that the inherited architecture is `JP_ARCH_X86_64` or `JP_ARCH_AARCH64`.
4. Reject unknown or unsupported architecture state.

CPU initialization must not proceed from an invalid context.

### Architecture Validation

The implementation must verify that the inherited execution environment matches the Stage 2 architecture target.

```text
Stage 2 target:
    x86_64 or AArch64

Inherited architecture:
    selected supported architecture

Result:
    accepted when equal
```

The following must fail:

```text
unknown architecture
unsupported architecture
```

### Execution-Mode Validation

The inherited CPU state must satisfy the selected architecture's Stage 2 execution requirement.

```text
Required execution mode:
    x86_64 long mode or AArch64 EL1/MMU state
```

The implementation must validate the actual CPU execution mode before reporting successful initialization.

### Inherited CPU State Validation

The CPU subsystem consumes the CPU-related state prepared by Stage 1.

Where represented by the Stage 1 handoff contract, the implementation must validate:

```text
architecture
execution mode
CPU entry state
interrupt state
```

CPU-related validation may also verify consistency with:

```text
page-table root
paging state
stack state
kernel entry state
```

Validation of these values does not transfer ownership of paging or stack initialization to `cpu.c`.

### CPU Initialization

After successful validation, the implementation establishes the CPU state required by the Stage 2 runtime.

Architecture-specific x86_64 or AArch64 mechanisms may be used.

The implementation may establish:

```text
required control-register state
required segment state
required register state
required CPU feature state
required execution state
required interrupt state
```

Only CPU responsibilities explicitly required by the Stage 2 contract may be established here.

### Final CPU-State Validation

After initialization, the resulting CPU state must be validated.

Successful return is permitted only when the resulting CPU state satisfies the Stage 2 CPU contract.

Executing initialization instructions alone is not sufficient to establish successful state.

### Decided rules (mandatory CPU state)

"Mandatory" means exactly this list. Nothing else (a feature that is merely present) is mandatory.

x86_64: the processor is in long mode (`CR0.PE = 1`, `CR0.PG = 1`, `CR4.PAE = 1`, `EFER.LME = 1`, `EFER.LMA = 1`); the privilege level is 0; `RFLAGS.IF = 0`; `CPUID` reports "no execute" support (leaf `0x80000001`, `EDX` bit 20) and `cpu.c` sets `EFER.NXE = 1` and `CR0.WP = 1`, then reads them back. If "no execute" is missing the result is `JP_BOOT_FAILED`. SSE and the floating-point unit are **not** required and `cpu.c` does not turn them on: the kernel is compiled without them (`-mgeneral-regs-only`), because the BIOS path leaves them off.

AArch64: the exception level is EL1; `PSTATE.DAIF = 1111`; `SCTLR_EL1.M = 1` (the MMU is on); `TCR_EL1` and `MAIR_EL1` are the values that Stage 1 wrote and `cpu.c` does not change them. `SCTLR_EL1.WXN` may be 0 or 1 at this point (`paging.c` sets it, see "Decided rules (replacing the identity map)"). The physical-address width is read by `memory.c` itself from `ID_AA64MMFR0_EL1.PARange` (see the AArch64 memory-range rule of `memory.h`); `cpu.c` does not read it.

A required bit that is wrong makes the result `JP_BOOT_FAILED`; the original state is not repaired silently.

**The exact steps of `jp_kernel_cpu_initialize()`.** A null `context` is `JP_BOOT_ERROR_INVALID_PARAM`. Every other failure below is `JP_BOOT_FAILED`. The function reads registers only through the private hook header `src/kernel/cpu_regs.h` (the instructions that C cannot write; the host tests replace them with a scripted fake, the QEMU tests run the real ones).

1. `context->handoff.architecture` must be `JP_ARCH_X86_64` or `JP_ARCH_AARCH64` and must be an architecture this build supports (a build made for one architecture refuses the other); otherwise no register is read.
2. x86_64. Read `RFLAGS`, `CR0`, `CR4`, `EFER`, and `CS`. Require `CR0.PE` (bit 0), `CR0.PG` (bit 31), `CR4.PAE` (bit 5), `EFER.LME` (bit 8), `EFER.LMA` (bit 10), the privilege level `CS & 3 == 0`, `RFLAGS.IF` (bit 9) equal to 0, and "no execute" support from `CPUID`. Then write `EFER` with `NXE` (bit 11) added and `CR0` with `WP` (bit 16) added, each with every other bit unchanged (a bit that is already set is not written again), read all four registers again, and require that `NXE` and `WP` are set and that every condition above still holds. The function never writes `RFLAGS`, `CR4`, or `CS`.
3. AArch64. Read `CurrentEL`, `DAIF`, `SCTLR_EL1`, `TCR_EL1`, and `MAIR_EL1`. Require: the exception level (`CurrentEL` bits 3:2) is 1; the four mask bits of `DAIF` (bits 9:6) are all 1; `SCTLR_EL1.M` (bit 0) is 1; `TCR_EL1` has `T0SZ` (bits 5:0) and `T1SZ` (bits 21:16) equal to 16, `TG0` (bits 15:14) equal to `0b00`, `TG1` (bits 31:30) equal to `0b10`, and `EPD0` (bit 7) and `EPD1` (bit 23) equal to 0; `MAIR_EL1` equals `0x000000000000FF00`. The function writes no register at all.
4. Success is returned only after the checks of steps 2 or 3 passed on the registers as they are after the writes.

**The physical-address width.** The hook `jp_kernel_cpu_physical_address_bits()` (used by `memory.c`) returns, on x86_64, `CPUID` leaf `0x80000008`, `EAX` bits 7:0 (0 when the processor has no such leaf); on AArch64 it decodes `ID_AA64MMFR0_EL1.PARange` (bits 3:0): 0 is 32, 1 is 36, 2 is 40, 3 is 42, 4 is 44, 5 is 48, 6 is 52, any other value is 0 (unknown).

## CPU State Contract

After successful initialization:

```text
Architecture:
    selected supported architecture

Execution mode:
    architecture-specific state defined by the selected contract

CPU execution state:
    valid for Stage 2

Required CPU state:
    established

Interrupt state:
    explicitly defined

CPU state required by paging:
    valid

CPU state required by stack:
    valid
```

The CPU module must not claim ownership of page tables, kernel stack memory, or heap state.

## Interrupt State

The interrupt state must be explicitly defined by the Stage 2 execution contract.

The implementation must not depend on an undocumented inherited interrupt state.

Stage 2 requires interrupts to remain disabled during early initialization. On x86_64, `cpu.c` must establish and validate `IF = 0`. On AArch64, `cpu.c` must establish and validate `PSTATE.DAIF = 1111`.

## Segment and Register State

The implementation must establish the architecture-specific segment, register, exception-level, and control state required for execution of the early kernel runtime.

The implementation must not depend on undocumented firmware state remaining valid after the Stage 1 → Stage 2 transition.

Only state required by the Stage 2 CPU contract must be established here.

## CPU Feature State

The implementation may validate CPU features required by Stage 2.

A CPU feature may be treated as mandatory only when explicitly required by the Stage 2 specification or by another already-defined Stage 2 subsystem contract.

If a required feature is unavailable, initialization must fail.

Optional CPU features must not become mandatory through implementation assumptions.

## Paging Boundary

`cpu.c` may validate CPU state relevant to paging.

It does not own Stage 2 page-table construction or virtual-memory management.

The responsibility boundary is:

```text
cpu.c
    ↓
CPU state required for paging
    ↓
paging.c
    ↓
kernel-side paging initialization
```

`cpu.c` must not implement the paging subsystem.

## Stack Boundary

`cpu.c` may validate CPU state required for stack execution.

It does not own kernel stack initialization.

The responsibility boundary is:

```text
cpu.c
    ↓
CPU execution compatibility
    ↓
stack.c
    ↓
kernel stack initialization
```

## State / Lifecycle

```text
NOT_INITIALIZED
    ↓
VALIDATING
    ↓
INITIALIZING
    ↓
VALIDATED
    ↓
READY
```

Failure transitions to:

```text
FAILED
```

Intermediate state representation remains implementation-defined.

Observable initialization results must correspond to actual CPU state.

## Ownership / Lifetime

- `JPBootContext` remains owned by the Stage 2 runtime.
- CPU state remains processor-owned.
- CPU configuration established by `cpu.c` remains active for the kernel execution environment.
- `cpu.c` does not own memory belonging to the memory, paging, stack, heap, or ELF subsystems.
- CPU initialization must not invalidate resources required by later Stage 2 subsystems.

## Invariants

After successful initialization:

```text
context != NULL

architecture == JP_ARCH_X86_64 or JP_ARCH_AARCH64

execution mode == selected architecture contract

required CPU state is established

interrupt state is explicitly defined

CPU state is compatible with paging

CPU state is compatible with stack

unsupported architectures are rejected

undocumented firmware state is not required
```

The following combinations are forbidden:

```text
READY + unknown architecture
READY + unsupported architecture
READY + invalid execution mode
READY + undefined interrupt state
READY + invalid CPU state
```

## Dependencies

```c
#include "justos/kernel/cpu.h"
```

Architecture-specific low-level facilities may be included where required by the implementation.

No UEFI or BIOS headers may be included.

## Boundary Rules

`cpu.c` may:

- contain x86_64- or AArch64-specific implementation;
- use instructions and registers of the selected architecture;
- access architecture-specific CPU registers;
- validate architecture-specific Stage 1 CPU state;
- establish CPU state required by Stage 2.

`cpu.c` must not:

- interact with UEFI;
- interact with BIOS;
- load the kernel image;
- parse the kernel ELF image;
- construct `JPBootInfo`;
- implement memory management;
- implement page-table management;
- implement kernel stack management;
- implement heap management.

## Failure Semantics

Initialization must fail when:

- `context` is `NULL`;
- architecture is unknown;
- architecture is not a supported Stage 2 architecture;
- execution mode is invalid;
- required CPU state is missing;
- required CPU state is inconsistent;
- required CPU configuration cannot be established;
- a required CPU feature is unavailable;
- required interrupt state cannot be established;
- final CPU-state validation fails.

Failure must propagate to `init.c`.

The subsystem must never report successful initialization after an unsuccessful or incomplete operation.

## Implementation Freedom

The following remain implementation-defined:

- exact x86_64 or AArch64 instructions;
- helper-function decomposition;
- register-access mechanisms;
- CPU feature-detection mechanisms;
- internal state representation;
- validation implementation;
- low-level compiler mechanisms required by the target architecture.

Implementation freedom does not permit changing the observable CPU contract.

Architecture-significant behavior must remain consistent with the Stage 2 specification.

## Verification

### Architecture

```text
x86_64
    → accepted

AArch64
    → accepted

unknown
    → rejected
```

### Execution Mode

```text
long mode
    → accepted

invalid execution mode
    → rejected
```

### Context

```text
valid context
    → accepted

NULL context
    → rejected
```

### CPU State

```text
valid inherited CPU state
    → accepted

missing required state
    → rejected

invalid required state
    → rejected
```

### Interrupt State

```text
explicitly defined state
    → accepted

undefined state
    → rejected
```

### Required CPU Features

```text
available
    → accepted

required feature unavailable
    → rejected
```

### Final State

Successful initialization must demonstrate that the resulting CPU state satisfies the Stage 2 CPU contract.

Execution of initialization instructions without final state validation is insufficient.

## Completion Criteria

`cpu.c` is complete when:

- x86_64 is explicitly enforced;
- inherited CPU state is validated;
- long mode is validated;
- required CPU initialization is performed;
- required CPU state is established;
- interrupt state is explicitly defined;
- final CPU state is validated;
- failures propagate to the Stage 2 orchestrator;
- no firmware dependency exists;
- no Stage 1 responsibility is duplicated;
- paging responsibilities remain in `paging.c`;
- stack responsibilities remain in `stack.c`;
- heap responsibilities remain in `heap.c`;
- ELF responsibilities remain in `elf.c`;
- successful initialization proves the CPU state required by the next Stage 2 subsystem.

---

# 3. `include/justos/kernel/paging.h`

## File

Stage 2 kernel-side paging interface.

## Purpose

Defines the public interface for initialization and management of the kernel-side paging environment established after the Stage 1 → Stage 2 transition.

The header defines the interface consumed by the Stage 2 initialization orchestrator and other Stage 2 subsystems.

It contains declarations and contract definitions only.

It contains no runtime paging logic.

It contains no firmware-specific structures.

## Responsibilities

- Define the public Stage 2 paging interface.
- Provide the interface for adopting the paging state inherited from Stage 1.
- Provide the interface for validating the inherited paging state.
- Provide the interface for establishing the kernel-side paging foundation.
- Keep page-table implementation details private to `paging.c`.
- Define the boundary between CPU initialization and kernel-side paging initialization.

## Architecture Target

```text
Architecture:
    x86_64 or AArch64

Paging model:
    architecture-specific hardware translation tables

Page-table root:
    CR3 on x86_64; TTBR1_EL1 on AArch64

Paging:
    enabled before Stage 2 entry
```

Stage 2 paging is required for both x86_64 and AArch64. Architectures other than x86_64 and AArch64 are outside the current paging interface contract.

The x86_64 paging contract uses long-mode page tables and CR3. The AArch64 paging contract uses EL1, a 4 KiB granule, a 48-bit virtual-address regime, TTBR1_EL1 for kernel translation tables, TTBR0_EL1 for the identity map (first built by Stage 1, replaced by `paging.c` at the same addresses), valid TCR_EL1/MAIR_EL1, and completed TLBI/DSB/ISB synchronization.

## Types / Structures

No public page-table implementation structures are defined by this header.

Page-table representation, entry structures, allocation metadata, mapping metadata, and architecture-specific paging state remain private to `paging.c` unless explicitly required by another Stage 2 contract.

The Stage 2 context is passed through:

```c
JPBootContext
```

The context remains runtime-owned.

## Constants / Enums

No public paging constants are introduced by this interface unless required by the Stage 2 paging contract.

Architecture-specific page-table constants remain implementation-defined where they do not affect an observable Stage 2 contract.

## Public API

```c
JPBootResult jp_kernel_paging_initialize(
    JPBootContext *context
);

JPBootResult jp_kernel_paging_validate(
    JPBootContext *context
);
```

### `jp_kernel_paging_initialize`

Initializes the kernel-side paging environment required by Stage 2.

The function consumes the paging state inherited from Stage 1 and establishes the kernel-side paging foundation required by subsequent Stage 2 subsystems.

### `jp_kernel_paging_validate`

Validates that the active paging state satisfies the Stage 2 paging contract.

The function must not report success merely because paging-related state exists.

The resulting hardware and page-table state must satisfy the defined invariants.

## Internal API

The following responsibilities remain private to `paging.c`:

```text
validate_paging_context()
validate_page_table_root()
validate_paging_state()
validate_kernel_mappings()
create_kernel_page_tables()
allocate_page_table()
create_mapping()
remove_temporary_mapping()
apply_page_permissions()
activate_page_table()
validate_active_paging()
```

The exact helper decomposition is implementation-defined.

## Algorithm

The public interface establishes the following normative sequence:

```text
Stage 1 paging state
        ↓
validate inherited paging state
        ↓
adopt required state
        ↓
establish kernel-side paging structures
        ↓
establish required mappings
        ↓
establish required permissions
        ↓
activate required paging state
        ↓
validate final paging state
```

The exact internal algorithm is defined by `paging.c`.

The header does not expose the implementation mechanism.

## Stage 1 Input Contract

Stage 2 receives paging state established by Stage 1.

The inherited state must include, where required by the Stage 1 handoff contract:

```text
architecture
paging enabled state
page-table root
kernel virtual address space state
kernel mappings
```

Stage 2 must not assume undocumented firmware or bootloader paging state.

The paging subsystem must consume the normalized Stage 1 handoff state.

## Kernel-Side Paging Foundation

The paging subsystem is responsible for establishing the kernel-side virtual-memory foundation required by Stage 2.

This includes, where required:

```text
page-table hierarchy
kernel virtual mappings
kernel physical mappings
kernel executable mappings
kernel writable mappings
kernel read-only mappings
kernel stack mappings
required early-heap mappings
```

The exact mapping layout remains implementation-defined unless explicitly exposed by the Stage 2 contract.

## Mapping Ownership

Mappings established by Stage 2 become part of the kernel execution environment.

Page-table memory required by the active kernel environment must remain valid through the Stage 2 initialization sequence and subsequent kernel execution.

Temporary page tables or temporary mappings may be released only after no active subsystem depends on them.

## Permissions

Kernel mappings must preserve the permissions required by the loaded ELF segments.

The paging subsystem must support the permission distinction required by the kernel image:

```text
readable
writable
executable
```

The implementation must not silently convert an executable mapping into writable-and-executable unless such a mapping is explicitly required by the Stage 2 contract.

The mapping mechanism must preserve the ELF-derived memory-permission intent established by Stage 1.

## Page-Table Root

The active page-table root must be valid for the selected architecture execution environment.

The root must:

```text
identify valid page-table memory
remain accessible while active
represent the required kernel mappings
be compatible with the active paging mode
```

The implementation must validate the root before publishing successful initialization.

## State / Lifecycle

The public paging lifecycle is:

```text
NOT_INITIALIZED
        ↓
INHERITED
        ↓
VALIDATED
        ↓
INITIALIZED
        ↓
ACTIVE
```

Failure transitions to:

```text
FAILED
```

The exact representation of intermediate states remains implementation-defined.

## Ownership / Lifetime

- `JPBootContext` remains owned by the Stage 2 runtime.
- Page-table memory belongs to the kernel memory environment after successful initialization.
- Active page tables remain valid while they are used by the processor.
- Kernel mappings remain valid through the Stage 2 → Stage 3 boundary.
- Temporary paging resources may be released only after their dependencies have been removed.
- `paging.c` owns its internal paging metadata.

## Invariants

After successful paging initialization:

```text
architecture == JP_ARCH_X86_64 or JP_ARCH_AARCH64

paging is enabled

CR3 references a valid page-table root

page-table hierarchy is valid

required kernel mappings exist

required mappings reference valid physical memory

required virtual addresses are canonical

required permissions are established

active page-table state is internally consistent
```

The following combinations are forbidden:

```text
READY + invalid CR3 root

READY + invalid page-table hierarchy

READY + missing kernel mapping

READY + invalid virtual address

READY + missing required permission

READY + inaccessible active page tables
```

## Dependencies

```c
#include "justos/kernel/paging.h"
```

The implementation may additionally depend on architecture-specific x86_64 paging facilities.

No UEFI or BIOS headers may be included.

## Boundary Rules

`paging.h` exposes only the public paging contract.

It must not expose:

- UEFI page-table structures;
- BIOS page-table structures;
- provider-native structures;
- private page-table entry representations;
- firmware memory descriptors;
- implementation-specific allocator internals.

The header must not require callers to understand the internal page-table implementation.

## Failure Semantics

Paging initialization must fail when:

- `context` is `NULL`;
- architecture is not a supported Stage 2 architecture;
- inherited paging state is invalid;
- the page-table root is invalid;
- required page-table memory cannot be established;
- required mappings cannot be created;
- required permissions cannot be established;
- a required virtual address is invalid;
- final paging validation fails.

Paging validation must fail when any required invariant is violated.

Failure propagates to `init.c`.

## Implementation Freedom

The following remain implementation-defined:

- page-table hierarchy representation;
- page-table allocation strategy;
- mapping helper implementation;
- temporary mapping strategy;
- page-table traversal;
- TLB management mechanism;
- internal mapping metadata;
- internal synchronization mechanisms;
- exact virtual-memory layout where not externally specified.

Implementation freedom does not permit changing the observable paging contract.

## Verification

Verify:

### Architecture

```text
x86_64
    → accepted

AArch64
    → accepted
```

### Page-Table Root

```text
valid architecture-specific root
    → accepted

invalid architecture-specific root
    → rejected
```

### Paging State

```text
paging enabled
    → accepted

paging disabled when Stage 2 requires it
    → rejected
```

### Kernel Mappings

```text
all required mappings present
    → accepted

missing required mapping
    → rejected
```

### Permissions

```text
required permissions established
    → accepted

incorrect permissions
    → rejected
```

### Address Validity

```text
canonical virtual address
    → accepted

non-canonical virtual address
    → rejected
```

### Final State

Successful initialization must prove that the active page-table environment satisfies the Stage 2 contract.

Execution of page-table setup operations without final validation is insufficient.

## Completion Criteria

`paging.h` is complete when:

- the public paging interface is defined;
- x86_64 is explicitly established as the current architecture target;
- Stage 1 paging state is identified as the input contract;
- page-table implementation details remain private;
- mapping and permission responsibilities are assigned to `paging.c`;
- ownership and lifetime are defined;
- failure semantics are defined;
- verification requirements are defined;
- no firmware dependency is exposed;
- the interface is sufficient for `init.c` to orchestrate paging initialization.

---

# 4. `src/kernel/paging.c`

## File

Stage 2 kernel-side paging implementation.

## Purpose

`paging.c` implements the selected architecture's kernel-side paging initialization performed after the Stage 1 → Stage 2 transition.

The module consumes the paging environment established by Stage 1, validates the inherited state, establishes the kernel-side paging foundation, and validates the resulting active paging environment.

The module does not perform firmware-side operations.

The module does not acquire or load the kernel image.

The module does not perform ELF parsing.

The module does not own the kernel heap.

The module does not construct `JPBootInfo`.

## Responsibilities

- Validate inherited Stage 1 paging state.
- Validate the active architecture-specific page-table root.
- Adopt the Stage 1 paging environment where required.
- Establish kernel-side page-table structures.
- Establish required kernel virtual-memory mappings.
- Preserve kernel segment permissions.
- Establish mappings required by the kernel stack.
- Establish mappings required by the early heap where required.
- Activate the required page-table root.
- Perform required TLB synchronization.
- Validate the final active paging state.
- Report successful initialization only after all required paging invariants hold.

## Architecture Target

```text
Architecture:
    x86_64 or AArch64

Paging mechanism:
    architecture-specific hardware translation tables

Active page-table root:
    CR3 on x86_64; TTBR1_EL1 on AArch64

Paging state:
    enabled
```

AArch64 is an equally required Stage 2 implementation target. Architectures other than x86_64 and AArch64 are outside the current Stage 2 implementation.

The implementation must not silently interpret another architecture as x86_64 paging state.

## Architecture-Specific Contract

For x86_64, the active root is `CR3`; long-mode paging is enabled; the PML4/PDPT/PD/PT hierarchy is valid; addresses are canonical; and required TLB synchronization is complete.

For AArch64, the translation regime is EL1 with a 4 KiB granule and 48-bit virtual addresses; `TTBR1_EL1` identifies kernel translation tables; `TTBR0_EL1` holds the identity map, through which the stack, the context, the page tables, the heap and the framebuffer are reached; `paging.c` replaces the Stage 1 identity map with a W^X one at the same addresses; `TCR_EL1` and `MAIR_EL1` are valid; and required `TLBI`, `DSB`, and `ISB` synchronization is complete.

### Decided rules (replacing the identity map)

**Why.** Stage 1 maps all memory to itself ("identity map": the virtual address equals the physical address) and makes every page both writable and executable, because the boot code has to run from wherever the firmware put it. Stage 1 says that Stage 2 replaces this map. A page that is writable and executable at the same time lets any bug turn data into running code, so Stage 2 closes it.

**What the new map is.** `jp_kernel_paging_initialize()` builds a new identity map that covers the same addresses as the Stage 1 one (the same regions, the same exclusions, the framebuffer) and then makes it active. The addresses stay the same, so the pointers to the context, the stack, the heap and the tables stay valid. Only the permissions change:

| memory | permissions in the new map |
|---|---|
| a range owned as `KERNEL_IMAGE` (a segment's physical pages) | by the segment's ELF flags: readable; writable only if the flag `W` is set; executable only if the flag `X` is set; never both writable and executable |
| any other normal memory (the stack, the tables, the context, the heap, the rest of the available memory) | readable and writable, not executable |
| the framebuffer | readable and writable, not executable, uncached (x86_64: cache-disable bit; AArch64: device memory) |
| AArch64 `MMIO` regions | as in Stage 1: device memory, not executable |

Pages of a kernel segment are 4 KiB pages, because a 2 MiB page cannot carry different flags for the first and the last part of it. Everything else may use 2 MiB pages. The high-half mappings of the kernel segments that Stage 1 built (virtual addresses of the kernel image) are kept as they are; they already carry the ELF flags.

**How.** (1) The new tables are taken from the unused pages of the Stage 1 table pool (see the rule of `memory.c`); if the pool is too small the result is `JP_BOOT_FAILED` and the active map is not touched. (2) The whole new tree is built and checked while the old map is still active; (3) only then the root is switched: x86_64 writes the new root to `CR3` (the high-half entries of the old root are copied into the new root, so the kernel image stays mapped); AArch64 writes the new tree to `TTBR0_EL1` and runs `dsb ishst`, `tlbi vmalle1`, `dsb ish`, `isb`; (4) the result is read back: the root register holds the new root and the stack, the context and the entry stay accessible. The old tables are not freed: they stay in the table pool (owner `PAGE_TABLES`).

**Which pages the new map contains (exactly).** The map is defined page by page (4 KiB). The *permission of a page* is the first rule that applies, in this order:
1. The page lies in a kernel segment (`context->kernel.segments[i]`, from `physical_address` for `memory_size` bytes, rounded outwards to whole pages). The flags of the segment give the permission: execute flag (`JP_ELF_PF_X`, value 1) only: read and execute; write flag (`JP_ELF_PF_W`, value 2) only or no flag: read and write for a segment with the write flag, read only for a segment with neither; read and write and execute together is a mistake of Stage 1 and the result is `JP_BOOT_FAILED`. A page that lies in two segments with different permissions is also `JP_BOOT_FAILED`.
2. The page lies in the framebuffer (`framebuffer.state` is `DETECTED`, from `base_address` for `pitch * height` bytes, rounded outwards to pages): read and write, no execute, uncached.
3. The page lies in a 2 MiB block that a region of the Stage 1 memory map touches (the region is rounded outwards to whole 2 MiB blocks, as Stage 1 does: the devices of a machine, for example the serial port of an AArch64 machine, lie next to the regions that the firmware lists, and Stage 1 already reaches them), except the regions that Stage 1 does not map: x86_64 skips `MMIO` and `UNUSABLE` regions and `RESERVED` regions of `JP_X86_64_IDENTITY_RESERVED_MAX` (1 GiB) or more; AArch64 skips `UNUSABLE` regions and `RESERVED` or `MMIO` regions of 1 GiB or more. The page is then read and write, no execute; the exception is an AArch64 `MMIO` region, whose pages are device memory, read and write, no execute. Two regions that touch the same 2 MiB block with a different kind of memory (normal and device) are a mistake in the map: `JP_BOOT_FAILED`. Page 0 (address 0) is mapped when the block that holds it is touched by a region, as in Stage 1.
4. Any other page is not mapped.
The *leaf size* is chosen like this: an aligned block of 2 MiB is one 2 MiB entry if no start or end of a kernel segment or of the framebuffer lies strictly inside the block, and the permission of its first page is not "not mapped"; otherwise the block gets a table of 512 pages, and each page gets its own permission (the pages that are "not mapped" are left empty). A block whose pages are all unmapped has no entry at all. Only blocks that touch a region, a kernel segment, or the framebuffer are visited. Every block is created once.

**The tables.** Four levels with 512 entries each (x86_64: PML4, PDPT, PD, PT; AArch64: levels 0 to 3), the index of a level being bits 47:39, 38:30, 29:21, 20:12 of the address. A new table is one page taken from the pool (page number `handoff.page_table_memory_used`, then `used` grows by one; the page is zero-filled before use); a pool with no page left is `JP_BOOT_FAILED`. On x86_64 the new root also receives the entries 256 to 511 of the old root (the root that `CR3` holds at the start), so the kernel half stays. On AArch64 the root of the `TTBR1_EL1` tree is not changed or copied, and the new root is the root of `TTBR0_EL1`.

**Switching.** `jp_kernel_paging_initialize()` follows the order: check the inputs, read the old root, build the whole new tree (nothing is written to a register), switch the root through the private hook `paging_hw.h` (`jp_kernel_paging_write_root()`: x86_64 `mov cr3`; AArch64 `msr ttbr0_el1` followed by `dsb ishst`, `tlbi vmalle1`, `dsb ish`, `isb`), read the root back and require that it is the new root, and on AArch64 set `WXN` (`jp_kernel_aarch64_set_wxn()`: read `SCTLR_EL1`, set bit 19, write, `isb`) and read `SCTLR_EL1` back to require the bit. If a step before the switch fails, no register is written; the table pages taken from the pool stay used.

**Errors.** `JP_BOOT_ERROR_INVALID_PARAM` for a null context. `JP_BOOT_NOT_INITIALIZED` when `jp_kernel_memory_initialize()` has not run (the ownership table is empty). `JP_BOOT_FAILED` for an unsupported architecture, a handoff without paging (`paging_enabled` false), a pool that is empty or not what the ownership table says (the range must be `PAGE_TABLES`), an old root that is zero or not page aligned, the permission conflicts above, and a full pool. `JP_BOOT_ERROR_CORRUPTED_DATA` for numbers that overflow.

**`jp_kernel_paging_validate()`** (it takes the context and nothing else) reads the root register and walks the tables by the hook `jp_kernel_paging_table_at()` (a physical address to a pointer: the identity map, so the same number). It returns `JP_BOOT_SUCCESS` only if: (1) the root register holds a page-aligned root inside the table pool; (2) for every kernel segment, every page is translated at its physical address in the identity map and at its virtual address (`virtual_address`, through the root of the kernel half: x86_64 the same root, AArch64 `handoff.page_table_root`) to the segment's physical page, both with the permission of rule 1; (3) every range of the ownership table owned as `KERNEL_STACK`, `PAGE_TABLES`, `EARLY_HEAP`, `STAGE2_RUNTIME` or `AVAILABLE` is mapped at its own address, read and write, no execute (for each leaf the walk advances by the size of the leaf); a page that Stage 1 never mapped (a region skipped by rule 3) is not an error only if it is not in such a range; (4) on AArch64 `SCTLR_EL1.WXN` is set. A walk that meets an entry that is not valid, a leaf at a level where none is allowed, or an address outside the 48 bits, is `JP_BOOT_FAILED`. A null context is `JP_BOOT_ERROR_INVALID_PARAM`; an empty ownership table is `JP_BOOT_NOT_INITIALIZED`.

**Descriptor bits.** The AArch64 descriptors of the new map use exactly the bit layout of the Stage 1 table (section `jp_aarch64_setup_paging`): block or page descriptor with bits 1:0, `AttrIdx` 1 for normal memory and 0 for device memory, access flag, inner shareable, `AP` for kernel read-write or kernel read-only, `UXN` always set, `PXN` set for every page that is not executable. A kernel-read-only page uses `AP = 0b10`. The x86_64 entries use present, writable (cleared for read-only pages), and no-execute (bit 63, set for every page that is not executable).

**What the CPU must have before.** x86_64: `EFER.NXE = 1` (otherwise the bit "no execute" is an error) and `CR0.WP = 1` (otherwise the kernel can write to read-only pages); `cpu.c` sets both. AArch64: after the new map is active, `paging.c` sets `SCTLR_EL1.WXN = 1` ("a writable page is never executable"), runs `isb`, and reads the bit back; this is the only `SCTLR_EL1` write that `paging.c` makes, and `cpu.c` accepts `WXN` as either 0 or 1.


Both paths provide the same observable mapping, permission, ownership, and failure contract.

## Public API

```c
JPBootResult jp_kernel_paging_initialize(
    JPBootContext *context
);

JPBootResult jp_kernel_paging_validate(
    JPBootContext *context
);
```

## Internal API

The following operations remain private to `paging.c`:

```text
validate_paging_context()
validate_architecture()
validate_inherited_paging_state()
validate_page_table_root()
validate_page_table_memory()
initialize_page_table_state()
allocate_page_table()
create_page_table_mapping()
map_kernel_segments()
map_kernel_stack()
map_early_heap()
apply_mapping_permissions()
activate_page_table()
flush_required_tlb_state()
validate_kernel_mappings()
validate_mapping_permissions()
validate_active_paging_state()
```

The exact helper decomposition is implementation-defined.

## Stage 1 Input

Stage 2 receives an already established paging environment from Stage 1.

The inherited state may include:

```text
paging enabled
CR3 page-table root
kernel virtual mappings
kernel physical mappings
kernel entry mapping
```

The implementation must use the normalized Stage 1 handoff state.

It must not reconstruct this state from firmware information.

It must not query UEFI or BIOS.

## Inherited Page-Table Root

The implementation must validate the page-table root inherited from Stage 1.

For x86_64 this means validating the physical address represented by `CR3`. For AArch64 this means validating the selected TTBR1_EL1 translation-table base and its EL1 translation regime.

The root must:

```text
be a valid physical address
refer to accessible page-table memory
be aligned according to the active paging mode
contain a valid page-table hierarchy
remain valid while active
```

An invalid root prevents successful paging initialization.

## Paging Adoption

Stage 2 may adopt the Stage 1 page-table hierarchy when it satisfies the requirements of the kernel runtime.

Adoption means that Stage 2 accepts the existing paging state as the starting point for kernel-side initialization.

Adoption does not imply that the Stage 1 page tables are permanently retained unchanged.

If the existing hierarchy is insufficient, `paging.c` must establish the required kernel-side paging structures before successful initialization.

The implementation must not silently discard required Stage 1 mappings.

## Kernel Virtual Address Space

The kernel-side virtual address space must contain all mappings required for continued kernel execution.

At minimum, this includes:

```text
kernel entry
kernel executable regions
kernel writable regions
kernel read-only regions
kernel stack
required early runtime memory
```

Additional mappings may be established when required by the Stage 2 implementation.

The exact virtual-memory layout remains implementation-defined unless another Stage specification explicitly constrains it.

## ELF Segment Mapping

The paging subsystem consumes the normalized segment information produced by Stage 1.

For each loaded `PT_LOAD` segment:

```text
virtual range
        ↓
physical destination range
        ↓
page-table mapping
        ↓
ELF-derived permissions
```

The paging subsystem must not reinterpret the segment's logical purpose.

It must preserve the permission requirements established by the ELF loader.

## Page Permissions

The implementation must map pages according to the segment permission requirements.

The relevant permissions are:

```text
PF_R → readable
PF_W → writable
PF_X → executable
```

The implementation must not grant permissions that are not required by the corresponding segment.

In particular:

```text
writable + executable
```

must not be introduced merely for implementation convenience.

If a segment explicitly requires both write and execute permissions, the implementation may establish both.

Otherwise the mapping must not contain both permissions.

## Page Granularity

The implementation must establish mappings using a page granularity supported by the active x86_64 paging configuration.

The exact page size and large-page strategy remain implementation-defined unless required by the Stage 2 contract.

The implementation must ensure that:

```text
virtual mapping range
physical mapping range
permission range
```

are internally consistent.

If a large-page optimization would change required permissions or violate alignment constraints, the implementation must use a compatible mapping strategy instead.

## Mapping Alignment

Every mapping must satisfy the alignment requirements of the page size used to establish it.

The implementation must validate:

```text
virtual address alignment
physical address alignment
mapping length
page-table level requirements
```

before publishing a successful mapping.

Misaligned state must not be silently rounded unless the resulting behavior is explicitly permitted by the Stage 2 contract.

## Page-Table Allocation

When new page-table structures are required, `paging.c` must obtain valid physical memory for them.

Allocated page-table memory must:

```text
be accessible
satisfy required alignment
not overlap protected memory
remain valid while referenced by active page tables
```

Page-table memory must not be allocated from memory already owned by another active kernel subsystem.

The exact allocation mechanism remains implementation-defined.

## Page-Table Initialization

New page-table memory must be initialized before being referenced by active translation structures.

Uninitialized page-table entries must not contain arbitrary active mappings.

The implementation must establish a known inactive state for unused entries.

## Page-Table Hierarchy

The selected architecture page-table hierarchy must be internally consistent.

Depending on the selected paging mode, the hierarchy may contain:

```text
PML4
PDPT
PD
PT
```

The implementation may use the appropriate hierarchy required by the active CPU configuration.

The exact internal representation remains private to `paging.c`.

## Mapping Creation

A mapping operation must establish:

```text
virtual address range
        ↕
physical address range
```

with the required permissions.

Before a mapping is published, the implementation must validate:

```text
virtual range
physical range
alignment
page-table availability
permission combination
overlap
```

Conflicting mappings must fail explicitly.

The implementation must not silently replace an existing mapping whose replacement semantics are not defined by the Stage 2 contract.

## Kernel Segment Overlap

The implementation must detect conflicting kernel mappings.

If two normalized kernel segments produce overlapping virtual ranges:

```text
same virtual range
    + compatible physical mapping
    + compatible permissions
```

the implementation may preserve the existing valid mapping.

If they produce:

```text
different physical destination
```

or incompatible permissions, initialization must fail.

No conflicting mapping may be silently overwritten.

## Protected Memory

Page-table creation and mapping operations must not overwrite memory belonging to protected Stage 1 or kernel resources.

Protected resources include, where still live:

```text
kernel image storage
kernel destination memory
page tables
kernel stack
framebuffer
other explicitly reserved Stage 1 resources
```

The exact protected-memory set is determined by the Stage 1 handoff and Stage 2 memory contracts.

## Kernel Entry Mapping

The kernel entry address must resolve to a valid executable mapping.

The implementation must validate:

```text
entry point
    ↓
canonical virtual address
    ↓
mapped physical memory
    ↓
executable permission
```

Successful paging initialization is forbidden when the kernel entry is unmapped or non-executable.

## Stack Mapping

The kernel stack must be mapped before any subsequent Stage 2 operation depends on it.

The paging subsystem establishes or adopts the mapping required by the stack subsystem.

`paging.c` does not determine the logical stack lifecycle.

The responsibility boundary is:

```text
paging.c
    ↓
stack memory mapping

stack.c
    ↓
stack runtime initialization
```

## Heap Mapping

The early kernel heap may require virtual-memory mappings.

Where the heap contract requires mapped virtual memory, `paging.c` must establish the mappings required for the heap to initialize.

`paging.c` does not own heap allocation policy.

The responsibility boundary is:

```text
paging.c
    ↓
heap virtual-memory foundation

heap.c
    ↓
heap allocation/runtime management
```

## Framebuffer Mapping

If the Stage 2 memory model requires the framebuffer to be accessed through kernel virtual addresses, `paging.c` must establish the corresponding mapping.

The implementation must preserve the framebuffer's physical location and required access permissions.

Framebuffer ownership remains outside the paging subsystem.

If the Stage 2 contract explicitly permits direct physical framebuffer access, an additional virtual mapping is not mandatory.

## Temporary Mappings

Temporary mappings may be created internally when required for page-table construction.

Temporary mappings must:

```text
have explicit ownership
have defined lifetime
not conflict with permanent kernel mappings
be removed when no longer required
```

A temporary mapping must not remain active merely because cleanup was omitted.

## Activation of Page Tables

After the required page tables have been established, the implementation may activate the resulting page-table root.

For x86_64 this requires updating the processor's active page-table root through `CR3` as appropriate.

The implementation must ensure that the currently executing instruction stream and active stack remain reachable after activation.

The new address space must therefore preserve every mapping required for continued execution.

## TLB Synchronization

When page-table changes require translation-cache synchronization, the implementation must perform the architecture-required synchronization before relying on the new mappings.

The exact mechanism remains implementation-defined.

Successful initialization requires that stale translations cannot invalidate the observable paging contract.

## Paging State Validation

After activation, the implementation must validate the resulting paging state.

Validation must verify, where applicable:

```text
paging enabled
CR3 valid
page-table hierarchy valid
kernel entry mapped
kernel entry executable
kernel segments mapped
kernel segment permissions correct
kernel stack mapped
required early runtime mappings present
```

## Canonical Virtual Addresses

All kernel virtual addresses used by the active paging environment must be valid addresses for the selected architecture and configured address-width regime. x86_64 addresses must be canonical; AArch64 addresses must satisfy the selected 48-bit regime.

The implementation must reject non-canonical virtual addresses.

No address may be silently truncated or sign-extended into another address.

## Physical Address Validation

Physical addresses used by page tables and mappings must be valid for the active platform.

The implementation must reject:

```text
overflowed physical ranges
unrepresentable addresses
misaligned page-table addresses
invalid page-table memory
```

The implementation must not assume that every 64-bit value is a valid physical address.

## State / Lifecycle

```text
NOT_INITIALIZED
        ↓
INHERITED
        ↓
VALIDATING
        ↓
INITIALIZING
        ↓
ACTIVE
        ↓
VALIDATED
```

Failure transitions to:

```text
FAILED
```

The exact internal representation of these states is implementation-defined.

The observable result must always correspond to actual paging state.

## Ownership / Lifetime

- `JPBootContext` remains owned by the Stage 2 runtime.
- Page-table memory belongs to the active kernel execution environment after successful initialization.
- `paging.c` owns its internal page-table metadata.
- Active page tables remain valid for as long as the processor depends on them.
- Kernel mappings remain valid through the Stage 2 → Stage 3 boundary.
- Temporary mappings remain owned by `paging.c` until explicitly removed.
- Memory used by active page tables must not be reclaimed while still referenced.
- Stage 1 memory may only be released when no remaining Stage 2 dependency exists.

## Invariants

After successful initialization:

```text
architecture == JP_ARCH_X86_64 or JP_ARCH_AARCH64

paging is enabled

CR3 contains a valid page-table root

active page-table hierarchy is valid

kernel entry is mapped

kernel entry is executable

all required kernel segments are mapped

segment permissions are correct

kernel stack mapping exists

required early-runtime mappings exist

all active page-table memory is valid

all active virtual addresses are canonical

no required mapping overlaps conflicting physical memory

no required mapping uses undefined permissions
```

The following combinations are forbidden:

```text
READY + invalid CR3

READY + invalid page-table hierarchy

READY + unmapped kernel entry

READY + non-executable kernel entry

READY + missing kernel mapping

READY + incorrect kernel permissions

READY + missing stack mapping

READY + invalid canonical address

READY + invalid active page-table memory
```

## Dependencies

```c
#include "justos/kernel/paging.h"
```

The implementation may additionally depend on:

```text
x86_64 architecture support
page-table primitives
low-level memory primitives
Stage 2 memory contract
```

No UEFI or BIOS headers may be included.

No firmware-specific structures may be required.

## Boundary Rules

`paging.c` may:

- use selected-architecture paging instructions;
- manipulate selected-architecture translation tables;
- allocate page-table memory;
- create kernel mappings;
- establish page permissions;
- activate the kernel page-table root;
- perform required TLB synchronization.

`paging.c` must not:

- interact with UEFI;
- interact with BIOS;
- acquire the kernel image;
- parse ELF headers;
- copy ELF segments;
- zero ELF segments;
- implement the kernel heap allocator;
- implement kernel stack runtime management;
- construct `JPBootInfo`;
- replace the CPU subsystem;
- perform Stage 3 initialization.

## Failure Semantics

Initialization must fail when:

- `context` is `NULL`;
- architecture is unsupported;
- inherited paging state is invalid;
- `CR3` is invalid;
- page-table memory is inaccessible;
- required page-table structures cannot be allocated;
- a required mapping cannot be created;
- a conflicting mapping is detected;
- a required permission cannot be established;
- a required virtual address is non-canonical;
- a physical range is invalid;
- required TLB synchronization cannot be completed;
- the resulting active paging state fails validation.

Failure must propagate to `init.c`.

No partial paging state may be reported as successful initialization.

## Rollback / Failure Handling

If initialization fails after modifying temporary paging state, the implementation must leave the system in a state that is either:

```text
known-valid inherited paging state
```

or:

```text
terminal FAILED state
```

The implementation must not report successful paging initialization after partial construction.

If rollback is not possible after activation of an incompatible page-table root, the failure must be treated as terminal for the current Stage 2 initialization attempt.

## Implementation Freedom

The following remain implementation-defined:

- page-table representation;
- page-table allocation strategy;
- exact virtual-memory layout;
- use of 4 KiB or larger pages;
- page-table traversal implementation;
- mapping helper implementation;
- temporary mapping strategy;
- TLB synchronization implementation;
- internal metadata structures;
- low-level assembly mechanisms;
- compiler-specific mechanisms required to access x86_64 paging state.

Implementation freedom does not permit changing:

```text
architecture target
paging invariants
kernel mapping requirements
permission semantics
ownership rules
lifetime requirements
failure semantics
```

## Verification

### Architecture

```text
x86_64
    → accepted

AArch64
    → accepted
```

### Inherited State

```text
valid Stage 1 paging state
    → accepted

invalid Stage 1 paging state
    → rejected
```

### Architecture-Specific Root

```text
valid CR3 root on x86_64 or valid TTBR1_EL1 root on AArch64
    → accepted

invalid architecture-specific root
    → rejected
```

### Page-Table Memory

```text
valid aligned page-table memory
    → accepted

invalid memory
    → rejected

overlapping protected memory
    → rejected
```

### Kernel Entry

```text
canonical + mapped + executable
    → accepted

non-canonical
    → rejected

unmapped
    → rejected

non-executable
    → rejected
```

### Kernel Segments

```text
all required segments mapped
    → accepted

missing segment mapping
    → rejected
```

### Permissions

```text
ELF-derived permissions preserved
    → accepted

unnecessary write permission
    → rejected

unnecessary execute permission
    → rejected

unjustified W+X mapping
    → rejected
```

### Mapping Conflicts

```text
compatible existing mapping
    → may be preserved

different physical mapping
    → rejected

incompatible permissions
    → rejected
```

### Stack

```text
required stack mapping exists
    → accepted

missing stack mapping
    → rejected
```

### TLB

```text
required synchronization completed
    → accepted

stale translation remains possible
    → rejected
```

### Final State

Successful initialization must prove that the active x86_64 paging environment satisfies every required Stage 2 paging invariant.

Page-table construction alone is insufficient.

## Completion Criteria

`paging.c` is complete when:

- inherited Stage 1 paging state is validated;
- x86_64 paging is explicitly enforced;
- the active page-table root is valid;
- required page-table structures are established;
- required kernel mappings exist;
- kernel entry is mapped and executable;
- ELF-derived permissions are preserved;
- kernel stack mappings exist;
- required early-runtime mappings exist;
- canonical virtual addresses are enforced;
- physical ranges are validated;
- required TLB synchronization is performed;
- final active paging state is validated;
- ownership and lifetime are respected;
- failures propagate deterministically to `init.c`;
- no firmware dependency exists;
- no Stage 1 ELF-loading responsibility is duplicated;
- heap runtime responsibilities remain in `heap.c`;
- stack runtime responsibilities remain in `stack.c`;
- CPU initialization responsibilities remain in `cpu.c`;
- the resulting paging environment is sufficient for the next Stage 2 initialization subsystem.

---

# 5. `include/justos/kernel/stack.h`

## File

Kernel stack runtime initialization contract.

## Purpose

Defines the public Stage 2 interface for adopting and initializing the kernel stack prepared by Stage 1.

Stage 2 does not allocate or establish the initial boot stack.

Stage 1 is responsible for preparing the execution stack required for kernel entry.

Stage 2 validates the inherited stack state and establishes the kernel-side stack runtime contract.

## Responsibilities

- Validate the stack state inherited from Stage 1.
- Verify that the active stack is mapped and usable.
- Verify that the stack pointer belongs to the valid stack range.
- Verify architecture-required stack alignment.
- Establish the kernel-side stack runtime state.
- Publish successful stack initialization only after all required validation has completed.

## Types / Structures

No public structures are introduced by this header.

Stack state is represented through the Stage 2 runtime context and the normalized Stage 1 handoff contract.

Architecture-specific stack metadata remains implementation-defined unless required by the observable Stage 2 contract.

## Constants / Enums

No stack-specific public constants are defined here unless required by the Stage 2 contract.

Architecture-specific alignment requirements must not be duplicated across unrelated modules.

## Public API

```c
JPBootResult jp_kernel_stack_initialize(
    JPBootContext *context
);
```

## Internal API

No internal functions are exposed through the public header.

Internal stack validation and initialization helpers remain private to `stack.c`.

## Algorithm

The implementation must:

1. Validate the supplied `JPBootContext`.
2. Verify that Stage 1 handoff state exists.
3. Verify that a valid kernel stack was established before Stage 2 entry.
4. Verify that the current stack pointer is valid.
5. Verify that the stack belongs to the expected mapped stack region.
6. Verify required stack mapping state.
7. Verify architecture-required stack alignment.
8. Establish Stage 2 stack runtime state.
9. Publish successful stack initialization only after all required checks succeed.

Stage 2 must not silently replace an invalid inherited stack with an undocumented alternative stack.

If Stage 2 requires a new runtime stack in a future design, that behavior must be explicitly added to the Stage 2 specification before implementation.

## State / Lifecycle

```text
NOT_INITIALIZED
    ↓
VALIDATING
    ↓
INITIALIZED
```

Failure transitions to:

```text
FAILED
```

Successful initialization means that the stack inherited from Stage 1 has been verified and is valid for continued Stage 2 execution.

## Ownership / Lifetime

The stack memory remains part of the kernel execution environment.

Stage 2 may establish ownership of the runtime stack only according to the Stage 2 memory and handoff contracts.

The stack must remain valid for all execution that depends on it.

The stack must not be released, unmapped, or repurposed while it remains the active kernel stack.

## Invariants

- A successful stack state must correspond to an actually valid stack.
- The stack pointer must reference the valid stack range.
- The stack mapping must remain active.
- Required page permissions must be compatible with stack usage.
- Required architecture-specific alignment must be satisfied.
- Stage 2 must not report successful initialization for an invalid or missing stack.
- Stack initialization must not invalidate the Stage 1 handoff environment.

## Dependencies

```c
#include "justos/boot/boot_types.h"
```

The header depends on the normalized Stage 1 boot types because `JPBootResult` and `JPBootContext` are part of the public stack initialization contract.

No firmware-specific headers may be included.

## Boundary Rules

Stage 2 consumes the stack execution environment established by Stage 1.

This module must not:

- perform firmware operations;
- access UEFI;
- access BIOS;
- acquire the kernel image;
- load ELF segments;
- construct `JPBootInfo`;
- recreate Stage 1 stack preparation;
- depend on Stage 3 state.

The stack module owns stack-specific runtime validation and initialization only.

## Failure Semantics

The public API must return a failure result when stack initialization cannot establish a valid Stage 2 stack state.

Failure conditions include:

- invalid context;
- missing Stage 1 handoff state;
- missing stack state;
- invalid stack pointer;
- invalid stack range;
- invalid stack mapping;
- invalid stack permissions;
- incorrect architecture-required alignment;
- any other mandatory stack invariant violation.

The exact `JPBootResult` value used for each failure condition is determined by the shared Stage 2 error contract and must not introduce a new undocumented error type through this header.

## Implementation Freedom

The header does not prescribe:

- internal validation helpers;
- private metadata;
- architecture-specific register access;
- stack validation implementation;
- runtime-state storage.

Those choices remain implementation-defined as long as they preserve the public Stage 2 contract.

The public interface, ownership semantics, lifetime requirements, failure semantics, and selected-architecture stack requirements are not implementation-defined.

## Verification

Verify:

- the header exposes the required stack initialization API;
- `JPBootContext` is available through the declared dependency;
- `JPBootResult` is available through the declared dependency;
- no firmware-specific dependency is introduced;
- no `JPBootInfo` dependency is introduced;
- no Stage 3 dependency is introduced;
- the public API matches the implementation in `stack.c`;
- the header exposes no undocumented internal stack functions;
- the header does not introduce unrelated stack-management responsibilities.

---

# 6. `src/kernel/stack.c`

## File

Stage 2 kernel-side stack runtime implementation.

## Purpose

Implements the kernel stack initialization contract defined by:

```c
#include "justos/kernel/stack.h"
```

The module validates and adopts the kernel stack execution environment inherited from Stage 1.

Stage 2 does not recreate the initial boot stack preparation performed by Stage 1.

The purpose of this module is to ensure that the active kernel stack is valid, mapped, correctly aligned, and safe for continued kernel execution before Stage 2 proceeds to the next initialization subsystem.

The stack module does not provide a general-purpose stack allocator.

It is responsible only for the kernel stack runtime state required by Stage 2.

## Responsibilities

The implementation is responsible for:

- validating the supplied `JPBootContext`;
- validating the Stage 1 handoff state required for stack initialization;
- identifying the active kernel stack;
- validating the active stack pointer;
- validating the stack memory range;
- validating the stack mapping;
- validating required stack permissions;
- validating the required selected-architecture stack alignment;
- establishing the Stage 2 kernel stack runtime state;
- preserving the active stack while Stage 2 continues execution;
- reporting stack initialization failure to the Stage 2 orchestrator.

The implementation must not:

- allocate an unrelated replacement stack;
- recreate Stage 1 stack preparation;
- modify the stack layout without specification authority;
- become a general-purpose kernel stack manager;
- absorb heap or memory-management responsibilities.

## Types / Structures

No new public structure is introduced by this source file.

The implementation may define private stack metadata when required.

Private metadata may contain:

- stack base;
- stack limit;
- current stack pointer;
- stack size;
- initialization state;
- architecture-specific validation state.

Private metadata is implementation-defined.

The implementation must not create a second authoritative representation of the Stage 1 handoff state.

The normalized `JPBootContext` remains authoritative for inherited Stage 1 state.

## Constants / Enums

No new public constants or enums are introduced by this source file.

Architecture-specific stack alignment values may be represented internally when required.

For x86_64 and AArch64, the implementation must enforce the architecture-specific stack-alignment requirements defined by the corresponding ABI and kernel-entry contract.

Architecture-specific constants must remain local to the appropriate implementation boundary.

They must not be duplicated across unrelated Stage 2 modules.

## Public API

The implementation must provide the public function declared by:

```c
#include "justos/kernel/stack.h"
```

```c
JPBootResult jp_kernel_stack_initialize(
    JPBootContext *context
);
```

The function is the only public entry point required by the Stage 2 stack module.

No additional public stack symbols may be introduced unless the Stage 2 specification is updated.

## Internal API

Internal functions remain private to `stack.c`.

The implementation may use private helpers for:

- context validation;
- Stage 1 handoff validation;
- stack-range validation;
- stack-pointer validation;
- stack-mapping validation;
- stack-permission validation;
- alignment validation;
- runtime-state publication.

Exact helper names and decomposition are implementation-defined.

Internal helpers must not expose stack implementation details to unrelated Stage 2 modules.

## Algorithm

The implementation must perform the following logical sequence:

1. Validate the supplied `JPBootContext`.
2. Verify that the required Stage 1 handoff state exists.
3. Identify the kernel stack inherited from the Stage 1 execution environment.
4. Obtain the active stack pointer.
5. Validate that the stack pointer belongs to the declared kernel stack range.
6. Validate that the stack range is non-empty and internally consistent.
7. Validate that the stack memory is mapped into the active virtual address space.
8. Validate that the mapping provides the permissions required for kernel stack use.
9. Validate the selected-architecture stack alignment required by the Stage 2 kernel execution ABI.
10. Verify that the stack remains usable after the Stage 2 paging state has been established.
11. Establish the Stage 2 kernel stack runtime state.
12. Publish successful initialization only after all required validation has completed.

The implementation must not report successful initialization before the active stack has passed all required checks.

The implementation must not silently replace an invalid stack.

If the inherited stack is invalid, initialization must fail.

If a future design requires Stage 2 to switch from the Stage 1 stack to a dedicated kernel runtime stack, that behavior must be explicitly added to the Stage 2 specification before implementation.

## Stack Pointer Validation

The active stack pointer must satisfy all applicable stack-range requirements.

For a downward-growing x86_64 kernel stack, the active stack pointer must identify an address within the valid stack region according to the finalized Stage 2 stack boundary definition.

The implementation must account for the usable stack boundary required by the ABI and runtime.

A stack pointer outside the declared stack region must cause initialization failure.

The implementation must not normalize, clamp, or silently replace an invalid stack pointer.

## Stack Range Validation

The stack range must describe a valid region of memory.

The implementation must reject:

- a zero-sized stack;
- an inverted range;
- an arithmetic overflow in range calculation;
- an address range that cannot be represented by the target architecture;
- a stack region that does not correspond to the inherited execution environment.

The implementation must treat the stack range as a memory region rather than assuming that the stack is valid solely because the current stack pointer is readable.

## Stack Mapping Validation

The stack memory must be mapped in the active virtual address space.

The implementation must verify that the mapping required for continued stack usage exists.

The implementation must reject a stack that:

- is unmapped;
- is only partially mapped where the required active region cannot be guaranteed usable;
- is mapped to invalid physical memory;
- has incompatible permissions;
- conflicts with another required kernel mapping.

The implementation does not need to validate unrelated mappings.

Mapping validation must remain limited to the requirements of the Stage 2 stack contract.

## Stack Permissions

The stack mapping must provide permissions compatible with kernel stack usage.

The implementation must reject a mapping that prevents required stack writes.

Executable stack permission is not required merely for stack operation.

The implementation must not introduce executable stack permissions unless explicitly required by the Stage 2 specification.

Permission validation must remain consistent with the paging subsystem.

The stack module must not independently rewrite page permissions.

If a permission change is required, the paging subsystem remains responsible for performing the mapping operation according to its own contract.

### Decided rules (what is checked and where the range comes from)

The range of the stack is read from the ownership table of `memory.c` (the entry with the owner `JP_KERNEL_MEMORY_KERNEL_STACK`, which is `handoff.stack_pointer - JP_BOOT_KERNEL_STACK_SIZE` up to `handoff.stack_pointer`). `stack.c` checks: (1) the entry exists and its length is `JP_BOOT_KERNEL_STACK_SIZE`; (2) `handoff.stack_pointer` (the top) is a multiple of 16; (3) the current stack pointer (`RSP` or `SP`, read in the function) lies inside that range, and is a multiple of 8 on x86_64 and of 16 on AArch64: on x86_64 the `call` instruction pushes an 8-byte return address, so inside a function the pointer is 8 modulo 16 by the System V rule; (4) the range is writable: `jp_kernel_paging_validate()` (it takes only the context) returns success; its check includes that every range of the ownership table owned as `KERNEL_STACK`, `PAGE_TABLES` or `EARLY_HEAP`, and every `AVAILABLE` range, is mapped read-write and not executable in the active map. Any failed check is `JP_BOOT_FAILED`. The stack pointer is never changed.

**The exact steps of `jp_kernel_stack_initialize()`.** A null `context` is `JP_BOOT_ERROR_INVALID_PARAM`; an empty ownership table is `JP_BOOT_NOT_INITIALIZED`; every check below that fails is `JP_BOOT_FAILED`. Let `top = handoff.stack_pointer` and `base = top - JP_BOOT_KERNEL_STACK_SIZE` (a `top` below the size fails).
1. `jp_kernel_memory_query(base, JP_BOOT_KERNEL_STACK_SIZE)` returns `JP_BOOT_SUCCESS` with the owner `KERNEL_STACK`. The byte at `base - 1` and the byte at `top` are not owned by `KERNEL_STACK` (a query that finds nothing there counts as "not owned"), so the owned range has exactly the size of the Stage 1 stack.
2. `top` is a multiple of 16.
3. The current stack pointer, read through the private hook `jp_kernel_stack_current_pointer()` (x86_64: `mov %rsp`; AArch64: `mov x, sp`), satisfies `base <= pointer <= top`, and is a multiple of 8 on x86_64 and of 16 on AArch64.
4. `jp_kernel_paging_validate(context)` returns `JP_BOOT_SUCCESS` (it includes that every `KERNEL_STACK` range is read-write and not executable).

## Stack Alignment

The Stage 2 target architectures are x86_64 and AArch64. The active stack contract must define the corresponding ABI alignment for each architecture.

## Architecture-Specific Contract

For x86_64, the active `RSP` must remain inside the mapped `KERNEL_STACK` range and satisfy the System V AMD64-compatible stack alignment.

For AArch64, the active `SP` must remain inside the mapped `KERNEL_STACK` range and satisfy the AArch64 ABI stack alignment. The stack must remain valid at EL1 under the active MMU translation regime.

Both paths consume the same typed `KERNEL_STACK` ownership and must not switch to an undocumented replacement stack.

The active stack pointer must satisfy the stack alignment required by the kernel execution ABI at the Stage 2 initialization boundary.

The implementation must validate the actual architectural stack state rather than assuming that Stage 1 alignment is correct.

Incorrect alignment is a fatal initialization error.

The implementation must not silently adjust the stack pointer.

Changing the stack pointer to satisfy alignment without an explicitly specified stack-switch operation is outside the module's implementation freedom.

## State / Lifecycle

The logical lifecycle is:

```text
NOT_INITIALIZED
    ↓
VALIDATING
    ↓
INITIALIZED
```

Failure transitions to:

```text
FAILED
```

The implementation may represent these states internally in another equivalent manner.

The externally observable behavior must remain equivalent to the defined lifecycle.

Successful initialization means:

- the inherited stack has been validated;
- the active stack pointer is valid;
- the stack mapping is usable;
- required permissions are satisfied;
- required alignment is satisfied;
- the stack can safely support continued Stage 2 execution.

## Initialization State Publication

Stack initialization must be treated as atomic from the perspective of the Stage 2 orchestrator.

Partial validation must not be reported as successful initialization.

If any required validation step fails:

```text
VALIDATING
    ↓
FAILED
```

The implementation must not publish `INITIALIZED`.

If all required validation succeeds:

```text
VALIDATING
    ↓
INITIALIZED
```

The initialized state must only become observable after all required checks have completed.

## Ownership / Lifetime

The kernel stack memory originates from the execution environment prepared by Stage 1.

Stage 2 must preserve the stack for as long as it remains the active kernel stack.

The stack must not be:

- released;
- unmapped;
- repurposed;
- overwritten as unrelated memory;

while it remains required for kernel execution.

If ownership of the stack is transferred from the Stage 1 execution environment to the Stage 2 kernel runtime, that transfer must follow the Stage 2 memory ownership contract.

The stack module must not independently release memory owned by another subsystem.

## Interaction with Paging

The stack module depends on the paging environment being compatible with stack usage.

The module may validate the stack after paging initialization has established the Stage 2 paging state.

The stack module must not independently replace the active page-table root.

The stack module must not construct an unrelated virtual-memory environment.

If paging initialization invalidates the active stack mapping, paging initialization must fail rather than allowing stack initialization to proceed against invalid state.

The stack module may rely on the paging module for mapping semantics but must still validate the resulting stack state required by its own contract.

## Interaction with Memory

The stack module may consume memory-state information established by the Stage 2 memory subsystem.

It must not become responsible for global physical-memory accounting.

It must not maintain an independent physical-memory allocator.

It must not reserve arbitrary memory outside the stack contract.

If the Stage 2 memory subsystem provides authoritative information about the stack region, the stack module must use that information rather than reconstructing memory ownership independently.

## Interaction with Heap

The stack module must not depend on the Stage 2 heap being initialized.

Stack initialization occurs before heap initialization in the Stage 2 sequence.

Therefore:

```text
Stack
    ↓
Heap
```

The stack module must be able to initialize without calling the kernel heap.

Dynamic allocation must not be required for successful stack initialization unless the Stage 2 specification explicitly permits an earlier allocation mechanism.

## Interaction with CPU

The stack module may use x86_64 architectural state to obtain and validate the active stack pointer.

It must not perform general CPU initialization.

CPU register setup unrelated to stack validation belongs to `cpu.c`.

The stack module must not modify unrelated CPU state.

## Invariants

The following invariants must hold after successful initialization:

- the active kernel stack is valid;
- the active stack pointer belongs to the valid stack range;
- the stack range is internally consistent;
- the required stack memory is mapped;
- required stack write permissions are present;
- the active stack pointer satisfies the required x86_64 alignment;
- the stack remains usable under the active Stage 2 paging state;
- the stack remains valid for continued kernel execution;
- the active stack is not released or repurposed;
- stack initialization does not invalidate the Stage 1 handoff state;
- stack initialization does not depend on heap initialization;
- stack initialization does not depend on firmware services;
- stack initialization does not require `JPBootInfo`;
- failed validation cannot result in a successful initialization state.

## Dependencies

The source file must include its public interface:

```c
#include "justos/kernel/stack.h"
```

It may depend on:

```c
#include "justos/boot/boot_types.h"
```

Additional dependencies may be introduced when required by the implementation.

Architecture-specific dependencies must remain limited to selected-architecture stack operations.

The implementation must not introduce dependencies on:

- UEFI;
- BIOS;
- firmware-specific headers;
- Stage 3 diagnostics;
- Stage 4 boot information;
- heap initialization.

## Boundary Rules

The stack implementation consumes the execution stack established by Stage 1.

It must not:

- perform UEFI operations;
- perform BIOS operations;
- call firmware services;
- acquire the kernel image;
- load ELF segments;
- discover ELF `PT_LOAD` segments;
- copy kernel segments;
- construct `JPBootInfo`;
- perform general physical-memory allocation;
- implement heap allocation;
- implement general virtual-memory management;
- perform Stage 3 diagnostics;
- perform Stage 3 recovery;
- depend on Stage 4 state.

The module must not recreate Stage 1 boot-stack preparation.

The module owns only kernel stack runtime validation and initialization.

## Failure Semantics

Initialization must fail when:

- `context` is `NULL` or otherwise invalid;
- the required Stage 1 handoff state is missing;
- no valid kernel stack exists;
- the active stack pointer cannot be obtained;
- the active stack pointer lies outside the valid stack range;
- the stack range is invalid;
- stack-range arithmetic overflows;
- the stack is not mapped;
- required stack memory is inaccessible;
- required stack write permissions are missing;
- the stack mapping conflicts with required kernel state;
- the active stack pointer violates the required x86_64 alignment;
- the stack cannot remain valid under the active paging environment;
- any other mandatory stack invariant is violated.

Failure must be returned to the Stage 2 initialization orchestrator.

A failed stack initialization must prevent the Stage 2 sequence from continuing as though the stack were valid.

The implementation must not silently:

- allocate a replacement stack;
- move the stack pointer;
- change stack permissions;
- change page tables;
- ignore alignment errors;
- ignore invalid memory mappings.

Any such behavior requires an explicit specification change.

## Determinism

Given the same valid Stage 1 execution environment, the stack initialization result must be deterministic.

The module must not depend on:

- firmware services;
- wall-clock time;
- random state;
- unrelated runtime state;
- heap allocation order;
- undefined initialization state.

The same inherited stack state must produce the same validation result.

## Resource Safety

Stack initialization must not leak resources.

Because the stack is inherited from Stage 1, the module must not release the stack merely because validation fails.

Failure reporting must leave the inherited memory ownership state consistent with the Stage 1 → Stage 2 contract.

The module must not modify unrelated memory.

The implementation must avoid writing to stack memory unless such writes are explicitly required for validation or initialization.

## Implementation Freedom

The implementation may choose:

- exact helper functions;
- internal state representation;
- architecture-specific register access;
- stack-range validation mechanism;
- mapping validation mechanism;
- private metadata;
- exact ordering of independent validation checks;
- whether validation is performed through private helpers or directly inside the initialization function.

The implementation may use architecture-specific mechanisms internally.

The implementation must not change:

- the public API;
- the Stage 2 initialization order;
- ownership semantics;
- stack lifetime;
- failure semantics;
- x86_64 and AArch64 target requirements;
- stack alignment requirements;
- stack mapping requirements;
- the Stage 1 → Stage 2 boundary.

The implementation must not introduce a stack switch unless such a switch is explicitly defined by the Stage 2 specification.

## Verification

Verify:

- successful initialization with a valid inherited stack;
- valid stack range;
- valid active stack pointer;
- valid stack mapping;
- valid stack permissions;
- correct selected-architecture stack alignment;
- invalid `JPBootContext`;
- missing Stage 1 handoff state;
- missing stack state;
- invalid stack range;
- zero-sized stack;
- inverted stack range;
- stack-range arithmetic overflow;
- stack pointer below the valid range;
- stack pointer above the valid range;
- unmapped stack;
- inaccessible stack memory;
- invalid stack permissions;
- incorrect stack alignment;
- conflicting stack mapping;
- preservation of the active stack;
- preservation of required stack mappings after paging initialization;
- absence of heap dependency;
- absence of firmware dependency;
- absence of `JPBootInfo` dependency;
- failure propagation to `init`;
- no silent replacement of an invalid stack;
- no silent stack-pointer adjustment;
- no unauthorized page-table replacement;
- deterministic initialization behavior.

---

# 7. `include/justos/kernel/heap.h`

## File

Kernel early-heap initialization contract.

## Purpose

Defines the public Stage 2 interface for initializing the early kernel heap.

The heap provides the dynamic memory allocation foundation required by kernel-side subsystems that execute after heap initialization.

The early heap is established only after CPU, memory, paging, and stack initialization have successfully completed.

The heap consumes the memory and virtual-memory environment established by the preceding Stage 2 subsystems.

The heap implementation must remain limited to the early kernel runtime requirements defined by Stage 2.

It is not the final general-purpose kernel memory allocator.

## Responsibilities

The heap module is responsible for:

- validating the supplied `JPBootContext`;
- validating that the required Stage 2 memory state exists;
- validating that the required paging state exists;
- determining the memory region assigned to the early kernel heap;
- establishing the initial heap state;
- establishing the heap's usable virtual-memory range;
- maintaining heap allocation state required by Stage 2;
- providing the initial kernel heap allocation interface;
- ensuring allocations remain inside the heap-owned region;
- reporting heap initialization and allocation failures.

The heap module must not:

- perform firmware operations;
- acquire physical memory directly from firmware;
- recreate Stage 1 memory allocation;
- replace the Stage 2 physical-memory subsystem;
- replace the Stage 2 paging subsystem;
- manage the kernel stack;
- perform ELF loading;
- become a complete kernel virtual-memory manager.

## Types / Structures

The public header must define only the structures required by the observable Stage 2 heap contract.

The heap state may be represented through an opaque implementation-owned structure if the finalized Stage 2 API requires persistent heap state.

If the heap state is represented directly through `JPBootContext`, the context remains the authoritative owner of shared Stage 2 runtime state.

The implementation may maintain private metadata for:

- heap base address;
- heap end address;
- heap size;
- current allocation position;
- free regions;
- allocation metadata;
- initialization state.

Private allocation metadata must remain internal to `heap.c`.

The public header must not expose allocator implementation details unnecessarily.

## Constants / Enums

No allocator-specific public constants or enums are required by this header unless explicitly introduced by the finalized Stage 2 contract.

Heap size and address-selection policy must not be silently encoded as undocumented public constants.

If Stage 2 defines a fixed minimum or maximum early-heap size, that value must be represented according to the shared project configuration conventions rather than duplicated across implementation files.

Architecture-specific virtual-address constraints must remain consistent with the selected architecture paging contract.

The observable early-heap contract is:

```text
minimum alignment: 16 bytes
allocation granularity: 16 bytes
zero-size allocation: returns NULL
maximum allocation: limited by the remaining EARLY_HEAP region and size_t
free: public and required
growth: not required
additional physical-page requests: forbidden in Stage 2
```

## Public API

The implementation must provide the heap initialization entry point:

```c
JPBootResult jp_kernel_heap_initialize(
    JPBootContext *context
);
```

The early heap must also provide the allocation interface required by Stage 2:

```c
void *jp_kernel_heap_alloc(
    JPBootContext *context,
    size_t size
);
```

The public interface must also provide:

```c
void jp_kernel_heap_free(
    JPBootContext *context,
    void *ptr
);
```

The public API must not expose internal allocator structures.

## Internal API

Internal heap helpers remain private to `heap.c`.

The implementation may use private helpers for:

- context validation;
- dependency-state validation;
- heap-region selection;
- heap-region validation;
- allocation-state initialization;
- free-region management;
- allocation metadata management;
- pointer-range validation;
- allocation-size validation;
- allocation-state updates.

Exact helper names and decomposition are implementation-defined.

## Algorithm

### Initialization

The heap initialization procedure must perform the following logical sequence:

1. Validate the supplied `JPBootContext`.
2. Verify that the required Stage 2 memory state exists.
3. Verify that paging initialization has completed successfully.
4. Verify that stack initialization has completed successfully.
5. Determine the memory region assigned to the early kernel heap.
6. Validate that the selected region is available for heap use.
7. Validate that the selected region does not overlap:
   - the kernel image;
   - active kernel mappings;
   - the active stack;
   - page-table structures;
   - reserved memory;
   - other protected Stage 2 regions.
8. Establish the heap's virtual address range.
9. Establish the heap allocation state.
10. Publish successful heap initialization only after the heap region is fully validated and usable.

The implementation must not report an initialized heap while the selected heap region is incomplete or invalid.

### Allocation

An allocation request must:

1. Validate the heap initialization state.
2. Validate the requested allocation size.
3. Reject zero-size requests.
4. Apply 16-byte alignment and 16-byte allocation granularity.
5. Determine whether sufficient free heap space exists.
6. Select a valid region according to the allocator's internal strategy.
7. Update allocation metadata.
8. Return a pointer contained within the heap-owned address range.

An allocation must never return memory outside the heap-owned region.

The implementation may use a bump allocator, free-list, or another deterministic strategy. Regardless of strategy, exhaustion returns `NULL`; the heap does not grow and does not request additional physical pages in Stage 2.

### Deallocation

Deallocation must:

1. Validate the heap initialization state.
2. Validate that the supplied pointer belongs to the heap-owned region.
3. Validate that the pointer corresponds to a valid allocation.
4. Update the allocator state.
5. Preserve all remaining valid allocations.

Invalid pointers must not corrupt allocator state.

The exact reuse and coalescing strategy is implementation-defined unless required by the Stage 2 contract.

### Decided rules (the early heap region)

The size of the early heap is one constant, `JP_KERNEL_EARLY_HEAP_SIZE`, declared in `include/justos/kernel/heap.h`, with the value 1 MiB (1048576 bytes). No other file repeats the number. `jp_kernel_heap_initialize()` asks the memory subsystem for the region with `jp_kernel_memory_allocate()` (owner `JP_KERNEL_MEMORY_EARLY_HEAP`, length `JP_KERNEL_EARLY_HEAP_SIZE`, alignment 4 KiB). `memory.c` answers with the lowest address that is at or above 1 MiB and where the whole range is `JP_KERNEL_MEMORY_AVAILABLE`. If there is no such range the result is `JP_BOOT_FAILED` and the heap stays not initialized. Because the new identity map makes all available memory readable, writable and not executable, the region is already mapped; `heap.c` calls `jp_kernel_paging_validate()` before it reports success (see the stack rule for what that check includes). The heap never grows: when the 1 MiB is used up, `jp_kernel_heap_alloc()` returns `NULL`.


**Where the heap keeps its state.** The context has no area for the heap, and no global variable is allowed, so the state is inside the heap region itself. The region starts with a 16-byte *header* `{ magic, region_size }` (two 64-bit numbers). The magic number is the "active" marker: it is written as the very last step of `jp_kernel_heap_initialize()`, so a heap that is not completely set up never looks active. After the header the region is a chain of *blocks*. A block starts with a 16-byte block header `{ size, state }` (`size` is the whole block with its header, a multiple of 16 and at least 32; `state` is one of two marker numbers, FREE or USED), and the memory that the caller gets (the *payload*) follows the block header, so every payload address is a multiple of 16. All three functions find the region by the ownership table: there must be exactly one entry with the owner `EARLY_HEAP`, its length must be `JP_KERNEL_EARLY_HEAP_SIZE`, and its header must say the same base size and carry the magic number. If any of this is not true the heap is "not active": `jp_kernel_heap_alloc()` returns `NULL` and `jp_kernel_heap_free()` does nothing.

**`jp_kernel_heap_initialize()` in order.** (1) A null `context` is `JP_BOOT_ERROR_INVALID_PARAM`; an empty ownership table is `JP_BOOT_NOT_INITIALIZED`. (2) If an `EARLY_HEAP` entry already exists, the heap was initialized (or the table is damaged): `JP_BOOT_FAILED`, nothing is changed. (3) `jp_kernel_memory_allocate(JP_KERNEL_EARLY_HEAP_SIZE, 4096, EARLY_HEAP)` (any failure is `JP_BOOT_FAILED`). (4) `jp_kernel_paging_validate()` must succeed (it now includes the new heap range, which must be read-write and not executable). If it fails, the region is given back with `jp_kernel_memory_release()` and the result is `JP_BOOT_FAILED`, so a failed initialization changes nothing. (5) The header and one FREE block that covers the whole rest of the region are written, then the magic number. The heap does not call `jp_kernel_stack_initialize()`: `jp_kernel_init()` runs the stack first, and the region cannot overlap the stack, the page tables or the kernel image because the ownership table gave it out of `AVAILABLE` memory only.

**`jp_kernel_heap_alloc(context, size)`.** `NULL` for a null context, a heap that is not active, `size == 0`, or `size` larger than `JP_KERNEL_EARLY_HEAP_SIZE` (it can never fit, and this also keeps the arithmetic from overflowing). Otherwise the needed block size is 16 (the header) plus `size` rounded up to a multiple of 16. The chain is searched from the start (the lowest address) and the *first* FREE block that is big enough is taken (first fit; the same state always gives the same answer). If the block is at least 32 bytes bigger than needed, it is cut in two and the rest stays FREE; otherwise the whole block is taken. No FREE block that is big enough: `NULL`. A chain that is not consistent (a size that is not a multiple of 16, below 32, or past the end of the region; an unknown state marker) makes the heap not active for this call: `NULL`.

**`jp_kernel_heap_free(context, ptr)`.** Does nothing for a null context, a null `ptr`, or a heap that is not active. Otherwise the chain is walked from the start to find the block whose payload address is exactly `ptr`; a `ptr` that is not the payload of a block (outside the region, in the middle of a block, the address of a header), or a block that is already FREE (a double free), is ignored and nothing is changed. A USED block becomes FREE, and then neighbouring FREE blocks are joined in one pass over the chain.

## Heap Region

The early heap must occupy memory explicitly available to the kernel runtime.

The heap region must not overlap memory belonging to:

- kernel code;
- kernel data;
- kernel read-only data;
- active stack;
- page tables;
- required Stage 2 runtime state;
- reserved memory;
- unavailable physical memory;
- other explicitly protected regions.

The selected heap region must be represented consistently between the memory and paging subsystems.

The heap must not assume that an arbitrary unused virtual address corresponds to available physical memory.

## Virtual-Memory Requirements

The heap operates inside the active architecture-specific virtual address space established by the paging subsystem.

The heap's virtual address range must correspond to valid mapped memory.

The heap must not create mappings by directly manipulating page tables unless such behavior is explicitly part of the finalized heap contract.

Additional heap pages are not requested or mapped by the Stage 2 heap subsystem. Heap growth requires a later specification change.

## Physical-Memory Requirements

The heap must not treat virtual address space as physical memory.

Every heap-backed allocation must correspond to memory that is actually available and mapped for kernel use.

Physical memory ownership must remain consistent with the Stage 2 memory subsystem.

The heap must not independently mark arbitrary physical pages as available.

## State / Lifecycle

The logical heap lifecycle is:

```text
NOT_INITIALIZED
    ↓
VALIDATING
    ↓
INITIALIZED
    ↓
ACTIVE
```

Failure transitions to:

```text
FAILED
```

Allocation activity occurs only in the `ACTIVE` state.

The heap must not service allocation requests before reaching `ACTIVE`.

A successful initialization must establish all state required for the heap to service valid allocation requests.

## Initialization State Publication

Heap initialization must be atomic from the perspective of the Stage 2 orchestrator.

The heap must not publish an active allocator before:

- the memory region has been validated;
- the virtual mappings have been validated;
- allocator metadata has been established;
- all mandatory dependencies have completed.

If initialization fails:

```text
VALIDATING
    ↓
FAILED
```

The heap must not enter `ACTIVE`.

## Allocation State

The allocator must maintain enough state to guarantee that every successful allocation belongs to the heap-owned region.

The implementation may choose:

- bump allocation;
- free-list allocation;
- block-based allocation;
- another deterministic early-allocation strategy.

The chosen strategy must preserve the public allocation contract.

Allocator metadata must not overlap memory returned to callers.

The implementation must prevent integer overflow during:

- size calculations;
- alignment calculations;
- address calculations;
- region-end calculations.

## Allocation Alignment

Every successful allocation must satisfy the alignment requirement defined by the Stage 2 kernel allocation contract.

The implementation may provide stronger alignment than the minimum requirement.

The implementation must not return a pointer with insufficient alignment.

Alignment calculations must be overflow-safe.

## Allocation Failure

Allocation must fail when:

- the heap is not initialized;
- the requested size is invalid;
- the requested size cannot be represented safely;
- alignment would overflow the address calculation;
- insufficient heap memory exists;
- the allocation metadata cannot represent the requested allocation;
- the requested allocation would leave the heap-owned range.

The allocator must not silently:

- access memory outside the heap;
- overwrite another allocation;
- overwrite allocator metadata;
- expand into an unrelated memory region;
- request firmware memory;
- corrupt the kernel stack.

## Ownership / Lifetime

The memory assigned to the early heap becomes heap-owned according to the Stage 2 memory ownership contract.

Memory returned by `jp_kernel_heap_alloc()` remains allocated until:

- it is explicitly released through the defined deallocation interface; or
- the entire early heap is destroyed or replaced according to a later kernel-memory-management transition.

The implementation must not invalidate active allocations unexpectedly.

The heap region itself must remain valid for as long as the early allocator remains active.

The heap must not release memory still required by another subsystem.

## Interaction with Memory

The heap depends on the Stage 2 memory subsystem for authoritative memory availability.

The memory subsystem owns global memory accounting.

The heap owns allocation state inside its assigned heap region.

The heap must not maintain an independent global physical-memory map.

If the memory subsystem reports that a region is unavailable, the heap must not use it.

## Interaction with Paging

The heap depends on paging initialization.

Paging must establish the mappings required for the heap before the heap reports successful initialization.

The heap must not silently bypass paging.

The heap must not modify unrelated page-table entries.

If additional mappings are required, they must be established through the defined paging contract.

## Interaction with Stack

Heap initialization occurs after stack initialization.

The heap must not use the active stack region as heap memory.

Heap allocation must never return an address belonging to the active stack.

Allocator metadata must not overwrite stack memory.

The heap must not require a stack switch.

## Interaction with CPU

The heap does not perform CPU initialization.

Architecture-specific CPU state required for heap operation is assumed to have been established by `cpu.c`.

The heap must not modify unrelated CPU control state.

## Interaction with ELF

The heap must not load ELF data.

It must only ensure that the heap region does not overlap the already loaded kernel image and any memory required by the kernel's executable state.

The ELF module remains responsible for runtime ELF validation.

## Invariants

After successful heap initialization:

- the heap is mapped;
- the heap region is valid;
- the heap region does not overlap protected kernel memory;
- the heap region does not overlap the active stack;
- the heap region does not overlap page-table memory;
- the heap region is owned according to the Stage 2 memory contract;
- allocator metadata is valid;
- allocation state is initialized;
- allocation requests cannot return memory outside the heap region;
- allocation alignment requirements are satisfied;
- allocation arithmetic cannot silently overflow;
- the heap does not depend on firmware services;
- the heap does not require `JPBootInfo`;
- the heap does not depend on Stage 3 state.

## Dependencies

The public header must depend on the shared boot/runtime types required by the API:

```c
#include "justos/boot/boot_types.h"
```

The allocation interface requires `size_t`.

The header must therefore include the project-approved definition of `size_t` if it is not already provided transitively by the project headers.

The header must not depend on:

- UEFI headers;
- BIOS headers;
- firmware-native memory structures;
- Stage 3 headers;
- Stage 4 headers.

## Boundary Rules

The heap module must not:

- access UEFI;
- access BIOS;
- call firmware services;
- acquire the kernel image;
- load ELF segments;
- perform Stage 1 memory allocation;
- construct `JPBootInfo`;
- perform CPU discovery;
- initialize the kernel stack;
- replace the paging subsystem;
- maintain global physical-memory accounting;
- implement Stage 3 diagnostics;
- depend on Stage 4 state.

The heap owns early kernel dynamic allocation only.

It must not silently become the permanent kernel allocator.

## Failure Semantics

Heap initialization must fail when:

- the context is invalid;
- required memory state is missing;
- required paging state is missing;
- required stack state is missing;
- no valid heap region exists;
- the heap region overlaps protected memory;
- the heap region is unavailable;
- the heap mapping is invalid;
- allocator state cannot be initialized;
- a mandatory heap invariant cannot be established.

Allocation must fail when:

- the heap is not active;
- the requested size is invalid;
- the allocation cannot satisfy required alignment;
- insufficient memory is available;
- an address or size calculation would overflow;
- the requested allocation cannot be represented safely.

Failures must not corrupt:

- the active stack;
- existing allocations;
- page tables;
- kernel image memory;
- memory accounting state.

The heap must not silently fall back to an undocumented memory source.

## Determinism

Given the same initialized memory and paging state, heap initialization must produce a deterministic result.

Allocation behavior must be deterministic according to the chosen allocator strategy.

The implementation must not depend on:

- firmware state;
- wall-clock time;
- random numbers;
- uninitialized memory;
- unrelated subsystem state.

If the implementation uses a free-list allocator, its selection order must remain deterministic for identical allocator state.

## Resource Safety

The heap must prevent:

- allocation outside the owned region;
- double ownership of the same memory;
- metadata overlap;
- arithmetic overflow;
- writes into protected kernel regions;
- writes into the active stack;
- corruption of page-table structures.

Invalid frees must not corrupt allocator state.

The implementation must preserve the lifetime of all memory still referenced by active kernel state.

## Implementation Freedom

The implementation may choose:

- bump allocation;
- free-list allocation;
- block metadata layout;
- allocation metadata representation;
- internal helper decomposition;
- heap-region selection mechanism;
- internal bookkeeping strategy;
- alignment implementation;
- private allocator state.

The implementation may provide stronger guarantees than the minimum public contract.

The implementation must not change:

- the public API;
- heap ownership semantics;
- heap lifetime;
- allocation boundary rules;
- alignment requirements;
- failure semantics;
- dependency ordering;
- Stage 2 initialization order;
- x86_64 and AArch64 target requirements.

The implementation must not introduce a permanent general-purpose allocator under the name of the Stage 2 early heap unless that transition is explicitly specified.

## Verification

Verify:

- successful heap initialization;
- invalid `JPBootContext`;
- missing memory state;
- missing paging state;
- missing stack state;
- valid heap region;
- invalid heap region;
- zero-sized heap;
- overlapping heap and kernel image;
- overlapping heap and active stack;
- overlapping heap and page-table memory;
- overlapping heap and reserved memory;
- invalid heap mapping;
- successful allocation;
- zero-size allocation handling;
- allocation alignment;
- multiple allocations;
- allocation exhaustion;
- allocation-size overflow;
- alignment overflow;
- allocation boundary enforcement;
- allocation outside the heap region is impossible;
- preservation of existing allocations;
- invalid free handling;
- double-free handling;
- allocator metadata integrity;
- stack preservation;
- page-table preservation;
- memory ownership consistency;
- failure propagation to `init`;
- absence of firmware dependencies;
- absence of `JPBootInfo` dependencies;
- absence of Stage 3 dependencies;
- deterministic initialization;
- deterministic allocation behavior;
- safe behavior when the heap is exhausted.

---

# 8. `src/kernel/heap.c`

## File

Stage 2 early kernel heap implementation.

## Purpose

Implements the early kernel heap contract defined by:

```c
#include "justos/kernel/heap.h"
```

The module establishes the first kernel-side dynamic memory allocation mechanism available after CPU, memory, paging, and stack initialization.

The heap operates entirely inside the kernel memory environment established by the preceding Stage 2 subsystems.

The early heap is intended to provide the minimum allocation capability required for continued kernel initialization.

It is not the final kernel allocator and must not silently evolve into a general-purpose memory-management subsystem.

## Responsibilities

The implementation is responsible for:

- validating the supplied `JPBootContext`;
- validating required Stage 2 memory state;
- validating required paging state;
- validating required stack state;
- determining or adopting the heap region defined by the Stage 2 memory contract;
- validating the heap region;
- establishing heap allocation metadata;
- establishing the initial allocator state;
- servicing valid allocation requests;
- maintaining allocation boundaries;
- enforcing allocation alignment;
- detecting allocation exhaustion;
- preventing access outside the heap-owned region;
- maintaining heap ownership state;
- reporting initialization and allocation failures.

The implementation is additionally responsible for:

- validating released pointers;
- updating allocation state;
- preserving remaining allocations;
- preventing invalid frees from corrupting allocator state.

The implementation must not:

- perform firmware allocation;
- recreate Stage 1 memory acquisition;
- modify the global memory map independently;
- replace the paging subsystem;
- manage the kernel stack;
- load ELF segments;
- construct `JPBootInfo`;
- perform Stage 3 functionality.

## Types / Structures

The implementation may define private structures required by the allocator.

A private heap structure may contain:

```c
struct JPKernelHeap {
    uintptr_t base;
    uintptr_t end;
    uintptr_t current;
    size_t size;
    size_t used;
};
```

The exact structure and fields are implementation-defined.

If the selected allocator requires additional metadata, private structures may represent:

- free blocks;
- allocated blocks;
- block sizes;
- alignment information;
- allocation state;
- heap lifecycle state.

Allocator metadata must remain private to `heap.c`.

The implementation must not expose allocator internals through the public header.

The implementation must not duplicate authoritative memory ownership information unnecessarily.

## Constants / Enums

Allocator-specific constants may remain private to `heap.c`.

Possible internal values include:

- allocation alignment;
- metadata alignment;
- minimum allocation size;
- internal heap-state markers.

Exact values are implementation-defined unless the Stage 2 specification explicitly makes them observable.

Architecture-specific constants must remain compatible with the selected architecture virtual-address and paging contract.

The implementation must not introduce public enums or constants without updating the Stage 2 specification.

## Public API

The implementation must provide:

```c
JPBootResult jp_kernel_heap_initialize(
    JPBootContext *context
);
```

and:

```c
void *jp_kernel_heap_alloc(
    JPBootContext *context,
    size_t size
);
```

The implementation must also provide:

```c
void jp_kernel_heap_free(
    JPBootContext *context,
    void *ptr
);
```

The implementation must match the declarations in:

```text
include/justos/kernel/heap.h
```

No additional public symbols may be introduced.

## Internal API

Internal helpers remain private to `heap.c`.

The implementation may provide helpers for:

- context validation;
- dependency validation;
- heap-region validation;
- heap-state initialization;
- alignment;
- allocation-size validation;
- free-space search;
- block creation;
- block splitting;
- block coalescing;
- pointer validation;
- allocator-state validation.

Exact helper names and decomposition are implementation-defined.

The implementation must keep allocator internals isolated from unrelated Stage 2 modules.

## Algorithm

### Initialization

The initialization algorithm must perform the following logical sequence:

1. Validate the supplied `JPBootContext`.
2. Verify that the required Stage 2 memory state exists.
3. Verify that paging initialization has completed successfully.
4. Verify that stack initialization has completed successfully.
5. Obtain the heap region defined by the Stage 2 memory contract.
6. Validate the heap region.
7. Verify that the region is available for kernel heap use.
8. Verify that the region does not overlap protected kernel memory.
9. Verify that the region does not overlap the active stack.
10. Verify that the region does not overlap page-table memory.
11. Verify that the region is mapped into the active virtual address space.
12. Initialize allocator metadata.
13. Initialize allocation state.
14. Publish successful heap initialization.
15. Transition the allocator into its active state.

The implementation must not publish an active heap before all required validation has completed.

### Allocation

The allocation algorithm must:

1. Validate the supplied context.
2. Verify that the heap is active.
3. Validate the requested size.
4. Calculate the required aligned allocation size.
5. Detect arithmetic overflow.
6. Locate sufficient free heap space.
7. Reserve the selected region.
8. Update allocator metadata.
9. Return a correctly aligned pointer.

If no suitable memory exists, the function must return:

```c
NULL
```

The implementation must not return partially initialized memory as a successful allocation.

### Bump Allocation

If a bump allocator is selected, the implementation may maintain a current allocation pointer:

```text
heap_base
    │
    ▼
allocated
    │
    ▼
current
    │
    ▼
free space
    │
    ▼
heap_end
```

Each successful allocation advances the current position.

The implementation must validate the resulting address before committing the allocation.

If the next allocation would exceed `heap_end`, allocation fails.

A bump allocator must not expand beyond the assigned heap region.

### Free-List Allocation

If a free-list allocator is selected, the implementation may maintain a list of free blocks.

The allocator must:

- maintain valid block boundaries;
- prevent overlapping blocks;
- preserve allocated blocks;
- update metadata consistently;
- validate all returned pointers.

Block selection order must remain deterministic for identical allocator state.

### Deallocation

The implementation must:

1. Verify that the heap is active.
2. Verify that `ptr` is non-NULL.
3. Verify that `ptr` belongs to the heap-owned region.
4. Verify that `ptr` corresponds to a valid active allocation.
5. Mark the allocation as released.
6. Update allocator metadata.
7. Coalesce adjacent free regions if the selected allocator supports coalescing.

Invalid pointers must not modify allocator state.

## Heap Region Validation

The heap region must be validated before allocator state is initialized.

The implementation must reject:

- a zero-sized region;
- an invalid base address;
- an invalid end address;
- an inverted range;
- arithmetic overflow;
- a region outside the permitted virtual-address range;
- an unmapped region;
- a region overlapping protected memory.

Protected memory includes:

- kernel image;
- kernel runtime state;
- active stack;
- page-table structures;
- reserved memory;
- memory owned by another Stage 2 subsystem.

The implementation must rely on authoritative Stage 2 memory state rather than reconstructing ownership from raw addresses.

## Address Arithmetic

All heap address calculations must be overflow-safe.

The implementation must validate:

- base + size;
- aligned allocation size;
- current + allocation size;
- metadata size calculations;
- block-end calculations.

An arithmetic overflow must result in failure.

The implementation must never wrap an address calculation around the selected architecture address space.

## Allocation Alignment

Every successful allocation must satisfy the alignment required by the Stage 2 allocation contract.

Alignment calculations must be performed without overflow.

The implementation may provide stronger alignment than required.

The implementation must not return a pointer whose alignment is weaker than the public contract.

## Allocation Boundaries

Every successful allocation must satisfy:

```text
heap_base <= allocation < heap_end
```

and the complete allocated range must remain inside the heap region.

The implementation must ensure:

```text
allocation_end <= heap_end
```

before committing the allocation.

An allocation must never overlap:

- another active allocation;
- allocator metadata;
- protected memory;
- the active stack;
- page-table memory;
- the kernel image.

## Metadata Placement

Allocator metadata must occupy memory belonging to the heap or another explicitly assigned kernel-runtime region.

Metadata must not overlap memory returned to callers.

If metadata is stored inside the heap region, the allocator must reserve that space before servicing allocations.

The metadata layout must remain consistent for the lifetime of the allocator.

## State / Lifecycle

The implementation must maintain the logical lifecycle:

```text
NOT_INITIALIZED
    ↓
VALIDATING
    ↓
INITIALIZED
    ↓
ACTIVE
```

Failure transitions to:

```text
FAILED
```

Allocation is permitted only in:

```text
ACTIVE
```

The implementation may use a different internal representation of these states.

The observable lifecycle must remain equivalent to the Stage 2 contract.

## Initialization State Publication

Heap initialization must be atomic from the perspective of callers.

Before initialization succeeds:

```text
jp_kernel_heap_alloc()
```

must not return a valid allocation.

After initialization succeeds:

```text
jp_kernel_heap_alloc()
```

may service valid allocation requests.

If initialization fails, the heap must remain unavailable.

The implementation must not publish partially initialized allocator state.

## Ownership / Lifetime

The heap region becomes owned according to the Stage 2 memory ownership contract.

The allocator owns only the region assigned to it.

It must not assume ownership of unrelated memory.

An active allocation remains valid for the duration specified by the public heap contract.

The implementation must not reclaim an active allocation without an explicit deallocation operation or a documented heap transition.

The heap region itself must remain mapped and valid while the allocator is active.

## Memory Ownership

The heap must not independently modify global physical-memory ownership.

The Stage 2 memory subsystem remains authoritative for global memory availability.

The heap may consume a region that has been explicitly assigned to it.

The implementation must not mark arbitrary physical memory as free.

The implementation must not use a region merely because it appears unused in the virtual address space.

## Paging Interaction

The heap depends on the Stage 2 paging environment.

The heap region must be mapped before the heap becomes active.

The heap implementation must not independently replace the active page-table root.

The heap implementation must not silently create unrelated mappings.

If additional mapping support is required by the finalized Stage 2 design, it must use the defined paging interface.

A failure to establish required heap mappings must cause heap initialization to fail.

## Stack Interaction

The active kernel stack must remain completely separate from the heap region.

The implementation must reject a heap region that overlaps the stack.

Allocation must never return memory belonging to the stack.

Allocator metadata must never overwrite the stack.

The heap must not require a stack switch.

The heap must not alter the active stack pointer.

## CPU Interaction

The heap assumes that the CPU state required by Stage 2 has already been established.

The heap may use x86_64 instructions or register state required for address or synchronization operations.

It must not perform general CPU initialization.

It must not modify unrelated CPU control registers.

## ELF Interaction

The heap must not load or relocate the kernel ELF image.

It must only ensure that heap memory does not overlap memory occupied by the already loaded kernel image.

The ELF subsystem remains responsible for runtime ELF validation.

## Failure Semantics

Heap initialization must fail when:

- the context is invalid;
- required memory state is missing;
- required paging state is missing;
- required stack state is missing;
- the heap region is unavailable;
- the heap region is invalid;
- the heap region is unmapped;
- the heap region overlaps protected memory;
- allocator state cannot be initialized;
- a mandatory heap invariant cannot be established.

Allocation must fail when:

- the context is invalid;
- the heap is not active;
- the requested size is zero if zero-size allocations are not part of the contract;
- the requested size is otherwise invalid;
- alignment calculation overflows;
- allocation-size calculation overflows;
- insufficient memory exists;
- allocator metadata cannot represent the request;
- the resulting allocation would exceed the heap boundary.

Failure must not corrupt:

- existing allocations;
- allocator metadata;
- memory ownership state;
- page tables;
- the active stack;
- kernel image memory.

## Invalid Free Semantics

If `jp_kernel_heap_free()` is implemented, the following must be rejected:

- NULL pointer if NULL is not a valid free request;
- pointer outside the heap;
- pointer into allocator metadata;
- pointer into the middle of an allocation;
- pointer to an already freed allocation;
- pointer that was not returned by the allocator.

Invalid free operations must not corrupt allocator state.

The exact failure-reporting mechanism for invalid frees must follow the finalized Stage 2 error-handling contract.

## Determinism

Heap initialization must be deterministic for identical input state.

Allocation behavior must be deterministic for identical allocator state and identical allocation requests.

The implementation must not depend on:

- wall-clock time;
- random state;
- firmware state;
- uninitialized metadata;
- unrelated subsystem behavior.

If multiple free regions satisfy an allocation request, the implementation must use a deterministic selection rule.

The exact selection rule may remain implementation-defined unless observable behavior requires it.

## Resource Safety

The implementation must prevent:

- out-of-bounds allocations;
- overlapping allocations;
- metadata corruption;
- arithmetic overflow;
- access to protected memory;
- access to the active stack;
- access to page-table memory;
- unauthorized physical-memory ownership;
- use of unmapped heap memory.

Allocator state must remain valid after every successful free.

## Invariants

After successful initialization:

- the heap region is valid;
- the heap region is mapped;
- the heap region is available for heap ownership;
- the heap region does not overlap protected memory;
- allocator metadata is initialized;
- allocation state is valid;
- the heap is active;
- allocations remain inside the heap region;
- allocation alignment is satisfied;
- allocation arithmetic is overflow-safe;
- the active stack is preserved;
- page tables are preserved;
- kernel image memory is preserved;
- global memory ownership remains consistent;
- firmware services are not required;
- `JPBootInfo` is not required;
- Stage 3 state is not required.

After every successful allocation:

- the returned pointer is valid;
- the allocation is aligned;
- the allocation belongs to the heap;
- the allocation does not overlap another active allocation;
- the allocation does not overlap allocator metadata.

## Dependencies

The source file must include:

```c
#include "justos/kernel/heap.h"
```

It may additionally include the shared runtime definitions required by the implementation:

```c
#include "justos/boot/boot_types.h"
```

Additional standard headers may be used for:

- `size_t`;
- `uintptr_t`;
- architecture-specific low-level operations.

Architecture-specific dependencies must remain isolated to the x86_64 implementation.

The implementation must not depend on:

- UEFI;
- BIOS;
- firmware-native allocation;
- Stage 3;
- Stage 4;
- `JPBootInfo`.

## Boundary Rules

The heap implementation must remain within the early kernel allocation boundary.

It must not:

- access firmware;
- allocate memory through UEFI;
- allocate memory through BIOS;
- recreate Stage 1 memory acquisition;
- construct `JPBootInfo`;
- load ELF segments;
- manage the active stack;
- replace paging;
- maintain the global physical-memory map;
- implement a scheduler;
- implement process memory management;
- implement user-space allocation;
- implement Stage 3 diagnostics.

The heap must not silently become the permanent kernel allocator.

## Implementation Freedom

The implementation may choose:

- bump allocation;
- free-list allocation;
- block allocation;
- internal metadata layout;
- private heap-state representation;
- allocation search strategy;
- block splitting strategy;
- block coalescing strategy;
- internal helper decomposition;
- exact alignment implementation;
- internal validation mechanisms.

The implementation may use stronger internal validation than required by the public contract.

The implementation must not change:

- the public API;
- ownership semantics;
- lifetime semantics;
- allocation boundary rules;
- alignment requirements;
- failure semantics;
- initialization order;
- x86_64 and AArch64 target requirements;
- dependency boundaries.

The implementation must not introduce a stack switch, paging replacement, firmware allocation path, or permanent memory-management subsystem without an explicit Stage 2 specification change.

## Verification

Verify:

- successful heap initialization;
- invalid `JPBootContext`;
- missing memory state;
- missing paging state;
- missing stack state;
- invalid heap region;
- zero-sized heap;
- inverted heap range;
- address arithmetic overflow;
- unmapped heap region;
- heap overlap with kernel image;
- heap overlap with active stack;
- heap overlap with page-table memory;
- heap overlap with reserved memory;
- successful allocation;
- allocation alignment;
- multiple allocations;
- allocation boundary enforcement;
- allocation exhaustion;
- allocation-size overflow;
- alignment overflow;
- metadata placement;
- metadata protection;
- preservation of existing allocations;
- allocation cannot escape heap boundaries;
- allocation cannot overlap protected memory;
- invalid free;
- double free;
- free of non-heap pointer;
- preservation of allocator metadata;
- preservation of active stack;
- preservation of page tables;
- preservation of kernel image;
- consistency with memory ownership;
- failure propagation to `init`;
- absence of firmware dependencies;
- absence of `JPBootInfo` dependencies;
- absence of Stage 3 dependencies;
- deterministic initialization;
- deterministic allocation behavior;
- safe behavior when the heap is exhausted.

---

# 9. `include/justos/kernel/memory.h`

## File

Stage 2 kernel-side memory interface.

## Purpose

Defines the public interface of the Stage 2 memory subsystem.

The header establishes the contract through which the Stage 2 initialization layer interacts with kernel-side memory initialization.

The header does not implement memory management.

## Responsibilities

The header is responsible for declaring:

- kernel memory initialization;
- public memory-related interfaces required by Stage 2;
- the types required by those interfaces;
- the boundary between the memory subsystem and other Stage 2 subsystems.

The header must not expose implementation details of `memory.c`.

The header must not provide:

- firmware interfaces;
- physical-memory discovery;
- UEFI memory descriptors;
- BIOS memory structures;
- page-table implementation;
- heap allocator implementation;
- stack implementation;
- ELF loading;
- Stage 3 functionality;
- `JPBootInfo`.

## Types / Structures

The header operates on the normalized Stage 1 boot state.

The primary Stage 1 state consumed by the memory subsystem is:

    JPBootContext

The header must reuse the authoritative memory structures defined by the Stage 1 boot contract.

It must not redefine:

    JPMemoryRegion
    JPMemoryMap

or any other Stage 1 memory representation.

If the Stage 2 specification defines a dedicated kernel-memory state structure, that structure must be declared here only when it is part of the public interface.

Implementation-only memory state remains private to:

    src/kernel/memory.c

## Constants / Enums

The public memory contract defines the ownership classes:

```c
typedef enum JPKernelMemoryOwner {
    JP_KERNEL_MEMORY_FIRMWARE_RESERVED = 0,
    JP_KERNEL_MEMORY_UNAVAILABLE,
    JP_KERNEL_MEMORY_STAGE1_RESERVED,
    JP_KERNEL_MEMORY_KERNEL_IMAGE,
    JP_KERNEL_MEMORY_KERNEL_STACK,
    JP_KERNEL_MEMORY_PAGE_TABLES,
    JP_KERNEL_MEMORY_FRAMEBUFFER,
    JP_KERNEL_MEMORY_STAGE2_RUNTIME,
    JP_KERNEL_MEMORY_EARLY_HEAP,
    JP_KERNEL_MEMORY_AVAILABLE
} JPKernelMemoryOwner;
```

## Architecture-Specific Memory Contract

x86_64 ownership validation must reject overflowed or unrepresentable physical ranges under the physical-address width reported by the Stage 1 x86_64 architecture contract. AArch64 ownership validation must reject overflowed or unrepresentable physical ranges under the physical-address width reported by `ID_AA64MMFR0_EL1.PARange` and preserve the architecture-defined memory attributes required by the active translation tables.

Both architectures use the same ownership classes and public operations. Physical ownership is independent of virtual mappings; `paging.c` establishes mappings after `memory.c` has accepted ownership.

Stage 1 memory-region types must be reused rather than duplicated.

The header must not introduce allocator-specific constants.

The header must not introduce paging-specific constants.

The header must not introduce firmware-specific constants.

## Public API

The memory subsystem provides:

    JPBootResult jp_kernel_memory_initialize(
        JPBootContext *context
    );

The memory subsystem is also the sole physical-memory ownership authority and must provide these operations:

```c
JPBootResult jp_kernel_memory_reserve(
    JPBootContext *context,
    uint64_t base_address,
    uint64_t length,
    JPKernelMemoryOwner owner
);

JPBootResult jp_kernel_memory_allocate(
    JPBootContext *context,
    uint64_t length,
    uint64_t alignment,
    JPKernelMemoryOwner owner,
    uint64_t *base_address
);

JPBootResult jp_kernel_memory_query(
    const JPBootContext *context,
    uint64_t base_address,
    uint64_t length,
    JPKernelMemoryOwner *owner
);

JPBootResult jp_kernel_memory_release(
    JPBootContext *context,
    uint64_t base_address,
    uint64_t length,
    JPKernelMemoryOwner owner
);

JPBootResult jp_kernel_memory_validate_ownership(
    const JPBootContext *context
);
```

`jp_kernel_memory_allocate()` may allocate only from ranges classified as available. All reserve, allocate, release, and query operations must reject overflow, invalid alignment, unavailable ownership, and overlapping conflicting ownership without partially committing state.

Ownership conflict semantics are deterministic:

```text
request range overlaps an incompatible owner
    → reject request
    → preserve all existing ownership state
    → return failure

request range is already owned by the same compatible owner
    → accept only when the operation is explicitly idempotent
    → otherwise reject as duplicate ownership

request range is outside an available region
    → reject request
```

The memory subsystem serializes ownership changes during Stage 2 initialization. Callers must not assume that a rejected request partially reserved or allocated any subrange.

This function initializes the kernel-side memory state required by Stage 2.

It consumes the normalized memory state established by Stage 1.

It does not perform physical-memory discovery.

It does not interact with firmware.

No additional public memory-ownership API may be introduced unless the Stage 2 specification is updated.

### Decided rules (the Stage 1 table pool)

Stage 1 puts every boot page table and, on x86_64, the descriptor table (GDT) into one block of 128 pages. The block is described by `context->handoff.page_table_memory_base` and `page_table_memory_size`; `page_table_memory_used` says how many of its pages Stage 1 has already handed out.

`jp_kernel_memory_initialize()` classifies the whole block (`base` to `base + size`) as `JP_KERNEL_MEMORY_PAGE_TABLES`, including the pages Stage 1 did not use and the page of the GDT. It does not split the block by `used`. The block is already marked reserved in the Stage 1 memory map, so no other owner can claim it. New tables that `paging` needs are taken from the unused pages of this block (from page number `used` on) and are not requested from `jp_kernel_memory_allocate()`. A block that is empty, not page-aligned, or outside a region of the Stage 1 map makes initialization fail with `JP_BOOT_ERROR_CORRUPTED_DATA`.

### Decided rules (the ownership table and the initial owners)

**Where the table lives.** The table "address range → owner" is stored in a new area of the context, `JPBootContext.stage2` (type `JPBootStage2Area`), next to `stage3` and `stage4`. Why there: the project forbids global mutable state and `malloc`, and the context is the one object that Stage 2 receives. The area holds a counter and a fixed array of 256 entries; an entry is `{ uint64_t base_address; uint64_t length; uint32_t owner; }`, where `owner` holds a `JPKernelMemoryOwner` value (it is a plain integer in `boot_types.h`, so that the Stage 1 header does not include a Stage 2 header). The entries are kept sorted by `base_address` and never overlap. Together they cover exactly the memory of the Stage 1 map (the map itself may have holes between regions; the table has the same holes). Two neighbouring entries with the same owner are merged into one. 256 is twice `JP_MAX_MEMORY_REGIONS`: the Stage 1 map has at most 128 regions and every region can be split by the ranges below. When an operation would need a 257th entry it fails with `JP_BOOT_FAILED` and changes nothing. `jp_kernel_memory_initialize()` zeroes the area before it fills it. The first words of the area carry no magic value; "initialized" is the counter being non-zero.

**How the initial owners are found.** `jp_kernel_memory_initialize()` builds the table in two passes. It reads only fields of the context, never firmware.

Pass 1, one entry per region of `context->memory_map`, by the region type:

| Stage 1 region type | owner |
|---|---|
| `JP_MEMORY_REGION_USABLE` | `JP_KERNEL_MEMORY_AVAILABLE` |
| `JP_MEMORY_REGION_RESERVED` | `JP_KERNEL_MEMORY_STAGE1_RESERVED` |
| `JP_MEMORY_REGION_ACPI`, `MMIO`, `FIRMWARE` | `JP_KERNEL_MEMORY_FIRMWARE_RESERVED` |
| `JP_MEMORY_REGION_UNUSABLE`, `UNKNOWN` | `JP_KERNEL_MEMORY_UNAVAILABLE` |

Pass 2, each of these ranges is cut out of the `STAGE1_RESERVED` entry that contains it and given its owner. A range that is not fully inside one `STAGE1_RESERVED` entry makes initialization fail with `JP_BOOT_ERROR_CORRUPTED_DATA`: Stage 1 reserved all of them, so a range outside means the handoff is broken.

| range | owner | where the numbers come from |
|---|---|---|
| each kernel segment, from `physical_address` for `memory_size` bytes, rounded to whole pages | `JP_KERNEL_MEMORY_KERNEL_IMAGE` | `context->kernel.segments[0 .. segment_count - 1]` |
| the stack, from `handoff.stack_pointer - JP_BOOT_KERNEL_STACK_SIZE` to `handoff.stack_pointer` | `JP_KERNEL_MEMORY_KERNEL_STACK` | `context->handoff` |
| the table pool, `page_table_memory_base` for `page_table_memory_size` bytes | `JP_KERNEL_MEMORY_PAGE_TABLES` | `context->handoff` (see the rule above) |
| the framebuffer, `base_address` for `pitch * height` bytes, rounded to whole pages; skipped when `framebuffer.state` is not `JP_DETECTION_STATE_DETECTED` | `JP_KERNEL_MEMORY_FRAMEBUFFER` | `context->framebuffer` |
| the context itself, from the address of `context` for `sizeof(JPBootContext)` bytes, rounded to whole pages | `JP_KERNEL_MEMORY_STAGE2_RUNTIME` | the pointer Stage 2 received |

Everything in `STAGE1_RESERVED` that is left after pass 2 stays `STAGE1_RESERVED` (for example, the provider's own context). The framebuffer of a real machine can lie outside the Stage 1 map (it is device memory); in that case it is not in the table and `jp_kernel_memory_query()` for it returns `JP_BOOT_NOT_DETECTED` — the module does not invent an owner. **The framebuffer, four cases.** (a) The range lies completely outside the table: it is skipped (the table does not own it). (b) It lies completely inside `STAGE1_RESERVED` entries: it is cut out as described. (c) It lies completely inside entries whose owner is `FIRMWARE_RESERVED` (the firmware lists the region as `ACPI`, `MMIO`, or `FIRMWARE`, and a real machine may put its framebuffer there): it is left as it is, owned by the firmware, and `jp_kernel_memory_query()` for it says `FIRMWARE_RESERVED` (paging still maps it, uncached). (d) Anything else (it touches an available or unavailable entry, or only part of it is inside the table or inside one kind of entry): `JP_BOOT_ERROR_CORRUPTED_DATA`, because available memory cannot be a framebuffer.

**Checks at the start of `jp_kernel_memory_initialize()`.** A null context is `JP_BOOT_ERROR_INVALID_PARAM`. The map must have `state == JP_DETECTION_STATE_DETECTED` and 1 to `JP_MAX_MEMORY_REGIONS` regions, and the kernel must have `load_state == JP_KERNEL_LOAD_LOADED` and at least one segment; otherwise `JP_BOOT_FAILED`. Every region must have a length above zero, end without overflow, stay below `2^bits` where `bits` is the physical-address width of the processor (x86_64: `CPUID` leaf `0x80000008`, `EAX` bits 7:0; AArch64: `ID_AA64MMFR0_EL1.PARange`; read through the private hook `jp_kernel_cpu_physical_address_bits()`), and must not overlap another region; otherwise `JP_BOOT_ERROR_CORRUPTED_DATA`. The regions may come in any order: the table is sorted while it is built.

**The operations (what the contract above leaves open).** Every operation except `initialize` returns `JP_BOOT_NOT_INITIALIZED` while the table is empty (`entry_count == 0`). A null context or null output pointer is `JP_BOOT_ERROR_INVALID_PARAM`, and so is a length of zero, an overflowing range, or an `owner` value outside the enumeration.

- `reserve(base, length, owner)`: `owner` must be one of `KERNEL_IMAGE`, `KERNEL_STACK`, `PAGE_TABLES`, `FRAMEBUFFER`, `STAGE2_RUNTIME`, `EARLY_HEAP` (any other owner is `JP_BOOT_ERROR_INVALID_PARAM`). `base` and `length` are multiples of 4 KiB. The whole range must be `AVAILABLE`; it then gets `owner`. Not available, partly available, or outside the table: `JP_BOOT_FAILED`. A second `reserve` of the same range is a duplicate and also `JP_BOOT_FAILED`; no operation is idempotent.
- `allocate(length, alignment, owner, *base)`: same `owner` rule as `reserve`; `length` is a non-zero multiple of 4 KiB; `alignment` is a power of two and at least 4 KiB. It chooses the lowest address that is at least `JP_BOOT_PROTECTED_LOW_MEMORY` (1 MiB), is a multiple of `alignment`, and where the whole range is `AVAILABLE` (first fit by address), gives it to `owner`, and writes it to `*base`. No such range: `JP_BOOT_FAILED` and `*base` is not written.
- `query(base, length, *owner)`: succeeds with the owner if the whole range lies inside the table and has exactly one owner. A range that is not completely in the table, or has more than one owner, gives `JP_BOOT_NOT_DETECTED` and `*owner` is not written.
- `release(base, length, owner)`: allowed only for `EARLY_HEAP`; the whole range must be owned by `EARLY_HEAP`; it becomes `AVAILABLE`. Any other owner is `JP_BOOT_FAILED` (the kernel image, stack, tables, framebuffer and the context are never given back), and so is a range that is not entirely owned by `owner`.
- `validate_ownership()`: `JP_BOOT_SUCCESS` only if: `1 <= entry_count <= JP_KERNEL_MEMORY_MAX_ENTRIES`; every entry has a non-zero length, no overflow, and a valid owner; entries are strictly increasing and do not overlap; two neighbours that touch never have the same owner (they would be merged); `AVAILABLE` and `EARLY_HEAP` entries lie inside `USABLE` regions of the Stage 1 map; and every region of the map is covered completely and the sum of the entry lengths equals the sum of the region lengths. Otherwise `JP_BOOT_ERROR_CORRUPTED_DATA`.

Every operation that changes the table first checks everything and that there is room for the entries it adds (one for each end of the range that cuts an existing entry, so at most two; an operation that cuts nothing needs no room, so even a full table can give a whole entry back), and only then changes it: a rejected request leaves the table byte for byte as it was.

A page is not owned just because Stage 1 mapped it (Stage 1 maps whole 2 MiB pages): ownership follows this table, mappings follow `paging`.

## Internal API

No internal functions are exposed by the header.

Internal memory-management operations remain private to:

    src/kernel/memory.c

These may include:

- memory-map validation;
- region validation;
- overlap detection;
- range validation;
- memory classification;
- ownership validation;
- kernel-region validation;
- runtime-region validation;
- consistency checking.

Exact internal decomposition is implementation-defined.

## Ownership / Lifetime

The memory subsystem receives the Stage 1 memory state through `JPBootContext`.

The header does not transfer ownership of the Stage 1 structures.

The lifetime of the input state is governed by the Stage 1 → Stage 2 contract.

After successful initialization, the kernel-side memory state must remain valid for all Stage 2 subsystems that depend on it.

## Dependencies

The header must include the declarations required by its public API.

The expected dependency is:

    #include "justos/boot/boot_types.h"

No firmware-specific headers may be included.

The header must not depend on:

- UEFI headers;
- BIOS headers;
- heap implementation headers;
- paging implementation headers;
- stack implementation headers;
- ELF implementation headers;
- Stage 3 headers;
- Stage 4 headers.

## Boundary Rules

The public memory interface must remain kernel-side and platform-independent at the Stage 2 boundary.

The header must not expose:

- UEFI state;
- BIOS state;
- firmware-native memory descriptors;
- firmware handles;
- firmware allocation interfaces;
- page-table internals;
- heap metadata;
- stack metadata;
- ELF loader state.

The memory header must not become a shared dumping ground for unrelated Stage 2 state.

## Invariants

The public interface must preserve the following invariants:

- Stage 1 remains the source of the initial memory state.
- Stage 2 does not rediscover physical memory.
- Stage 2 does not communicate directly with firmware.
- Stage 1 memory structures remain authoritative.
- memory ownership is not duplicated between subsystems;
- memory initialization occurs before dependent Stage 2 subsystems;
- invalid memory state is rejected by the memory subsystem;
- implementation details remain private.

## Failure Semantics

The initialization function returns a `JPBootResult`.

Failure indicates that the kernel-side memory state could not be initialized according to the Stage 2 contract.

Failure must not result in partially published memory state.

The exact failure values are determined by the authoritative `JPBootResult` definition.

The header must not redefine failure semantics independently of the shared boot types.

## Implementation Freedom

The implementation may choose the internal representation of kernel-side memory state.

It may also choose:

- validation helper structure;
- internal region-processing strategy;
- overlap-detection strategy;
- internal bookkeeping;
- consistency-check implementation.

The public interface is not implementation-defined.

The implementation must preserve:

- the declared API;
- Stage 1 memory ownership semantics;
- Stage 2 initialization order;
- subsystem boundaries;
- x86_64 and AArch64 Stage 2 targets;
- failure semantics.

If implementation requires a new architecture-significant public interface, the Stage 2 specification must be updated before that interface is implemented.

## Verification

Verify:

- the header compiles independently;
- `JPBootContext` is referenced through the authoritative Stage 1 definition;
- `JPMemoryRegion` is not redefined;
- `JPMemoryMap` is not redefined;
- the public initialization API matches `memory.c`;
- no implementation-only structures are exposed;
- no firmware-specific types are exposed;
- no paging internals are exposed;
- no heap internals are exposed;
- no stack internals are exposed;
- no ELF-loader internals are exposed;
- no `JPBootInfo` dependency exists;
- no Stage 3 dependency exists;
- the header remains limited to the Stage 2 memory subsystem boundary.

---

# 10. `src/kernel/memory.c`

## File

Stage 2 kernel-side memory implementation.

## Purpose

Implements the kernel-side memory initialization defined by `include/justos/kernel/memory.h`.

The module consumes the normalized memory state established by Stage 1 and prepares the memory state required by the remaining Stage 2 subsystems.

Stage 2 memory initialization does not discover physical memory and does not communicate with firmware.

## Responsibilities

The implementation is responsible for:

- validating the Stage 1 memory state;
- validating the memory map supplied through `JPBootContext`;
- validating memory-region boundaries;
- detecting overlapping regions;
- validating region types;
- establishing the kernel-side interpretation of the memory state;
- accounting for memory already occupied by the kernel;
- accounting for memory required by Stage 2 runtime structures;
- preserving memory required by paging;
- preserving memory required by the active kernel stack;
- identifying memory available for the early kernel heap;
- establishing the memory state required by later Stage 2 subsystems;
- rejecting invalid or contradictory memory state;
- reporting initialization failure.

The implementation must not:

- perform firmware memory discovery;
- parse UEFI memory descriptors;
- parse BIOS memory information;
- allocate memory through firmware;
- load the kernel;
- perform ELF loading;
- initialize paging;
- initialize the heap;
- initialize the stack;
- construct `JPBootInfo`.

## Public API

The implementation must provide the function declared by `memory.h`:

    JPBootResult jp_kernel_memory_initialize(
        JPBootContext *context
    );

The implementation must also provide every ownership operation declared by `memory.h`:

```c
JPBootResult jp_kernel_memory_reserve(...);
JPBootResult jp_kernel_memory_allocate(...);
JPBootResult jp_kernel_memory_query(...);
JPBootResult jp_kernel_memory_release(...);
JPBootResult jp_kernel_memory_validate_ownership(...);
```

These operations are the only authority for Stage 2 physical-memory ownership. Their exact declarations are defined by `memory.h` above; no independent physical allocator may exist in another Stage 2 component.

No additional public memory-ownership symbols may be introduced.

## Internal API

Internal functions remain private to `memory.c`.

The implementation may provide helpers for:

- context validation;
- memory-map validation;
- region validation;
- address-range validation;
- overlap detection;
- region classification;
- ownership validation;
- kernel-region validation;
- runtime-region validation;
- memory-state construction;
- memory-state consistency checks.
- typed ownership reservation;
- typed allocation;
- ownership query;
- ownership release/reclaim validation;
- ownership conflict rejection.

Exact helper names and decomposition are implementation-defined.

## Initialization Algorithm

The logical initialization sequence is:

1. Validate the supplied `JPBootContext`.
2. Verify that the Stage 1 memory map exists.
3. Verify that the memory-map metadata is valid.
4. Verify that the number of memory regions is within the defined capacity.
5. Validate every memory region.
6. Validate every region's address range.
7. Validate every region's size.
8. Detect arithmetic overflow in region calculations.
9. Detect overlapping memory regions where overlap is prohibited by the Stage 1 contract.
10. Validate region classifications.
11. Verify that required kernel memory is represented correctly.
12. Verify that memory occupied by the kernel cannot be treated as available.
13. Verify that memory required by Stage 2 runtime state can be represented.
14. Establish the kernel-side memory state.
15. Validate the resulting ownership state.
16. Publish the initialized memory state.
17. Return successful initialization.

The implementation must not publish partially initialized memory state.

## Stage 1 Memory Map

Stage 1 is the source of the initial memory map.

The memory subsystem consumes the normalized representation already present in `JPBootContext`.

The implementation must not reconstruct the memory map from:

- firmware structures;
- physical-memory probing;
- platform-specific descriptors;
- undocumented bootloader state.

The memory map must remain the authoritative source for Stage 2 memory initialization.

## Memory Region Validation

Each memory region must satisfy the memory-region contract.

Validation must include:

- valid base address;
- valid size;
- valid end address;
- valid region type;
- no arithmetic overflow;
- no invalid zero-sized region unless explicitly permitted;
- no address range outside the permitted physical address representation.

For a region represented by base and size:

    region_end = region_base + region_size

must be validated before the result is used.

An overflow must cause initialization failure.

## Region Ordering

The implementation may normalize region ordering internally.

If regions are sorted, the ordering must be deterministic.

Sorting must not modify the authoritative Stage 1 representation unless the Stage 2 contract explicitly permits such modification.

A private normalized representation may be used for overlap and ownership analysis.

## Overlap Detection

The implementation must detect memory regions that overlap in a manner prohibited by the memory contract.

For two regions:

    [A_start, A_end)
    [B_start, B_end)

the ranges overlap when:

    A_start < B_end
    and
    B_start < A_end

The implementation must use overflow-safe range calculations.

Contradictory ownership must not be silently resolved by choosing one region over another.

If an overlap cannot be reconciled according to the defined memory-region rules, initialization must fail.

## Reserved Memory

Memory marked as reserved or otherwise unavailable by the Stage 1 memory contract must not be treated as general-purpose usable memory.

The memory subsystem must preserve the reservation semantics supplied by Stage 1.

It must not convert reserved memory into usable memory merely because the region appears physically accessible.

## Kernel Memory

Memory occupied by the kernel image must remain protected from general-purpose allocation.

The implementation must account for all kernel memory ranges exposed by the Stage 1 kernel representation.

The memory subsystem must prevent such ranges from becoming available to:

- paging structures;
- heap allocations;
- unrelated runtime allocations.

The implementation must not move the kernel image.

## Stage 2 Runtime Memory

Memory required by Stage 2 runtime structures must be accounted for before it is exposed as available to later subsystems.

This includes memory required by:

- paging structures;
- kernel stack;
- early heap metadata;
- other explicitly defined Stage 2 runtime structures.

The exact allocation mechanism remains governed by the Stage 2 subsystem specifications.

Memory.c must not silently allocate these resources through firmware.

## Ownership

The memory subsystem must distinguish between:

    unavailable
    reserved
    kernel-owned
    runtime-owned
    available

where these categories are defined by the Stage 2 memory contract.

A memory region must have one authoritative ownership state.

Two Stage 2 subsystems must not independently claim the same memory.

## Memory Adoption

Stage 2 adopts the memory state transferred from Stage 1.

Adoption means that Stage 2 begins managing the normalized kernel-side interpretation of that memory.

Adoption does not mean that Stage 2 may disregard reservations established by Stage 1.

The implementation must preserve all ownership constraints inherited from Stage 1.

## Physical vs Virtual Memory

Memory.c operates on the memory representation required by the Stage 2 memory contract.

Physical memory availability and virtual address mappings must not be conflated.

The implementation must not assume that a physically available region is automatically mapped into the current virtual address space.

Paging is responsible for establishing virtual mappings.

## Paging Boundary

Memory.c provides the memory state required by paging.

Paging remains responsible for page-table construction and virtual-memory initialization.

Memory.c must not:

- create the active page tables;
- switch CR3;
- establish the final kernel virtual-address layout;
- modify paging control registers.

Memory.c may validate that memory required by paging is available and correctly accounted for.

## Stack Boundary

Memory.c must account for the memory reserved for the kernel stack when that state is defined by the Stage 2 contract.

It must not initialize or switch the active stack.

It must not modify the stack pointer.

The stack subsystem remains responsible for stack initialization.

## Heap Boundary

Memory.c must identify or expose the memory state required by the early heap according to the Stage 2 contract.

It must not implement allocation algorithms.

It must not maintain heap free lists.

It must not service allocation requests.

The heap subsystem remains responsible for allocator state.

## ELF Boundary

Memory.c must preserve memory occupied by the already loaded kernel image.

It must not:

- parse ELF program headers for loading;
- copy ELF segments;
- perform relocations;
- establish executable mappings.

Runtime ELF responsibilities remain in the ELF subsystem.

## State / Lifecycle

The memory subsystem follows the logical lifecycle:

    NOT_INITIALIZED
        ↓
    VALIDATING
        ↓
    INITIALIZED
        ↓
    ACTIVE

Failure results in:

    FAILED

The exact internal representation may differ.

The externally observable initialization result must remain consistent with the Stage 2 contract.

## State Publication

Memory state must not become observable as initialized before validation has completed.

If validation fails:

- the memory subsystem remains unavailable;
- partially constructed state must not be published;
- ownership must not be partially committed.

Successful initialization must publish a complete and internally consistent memory state.

## Ownership / Lifetime

The memory state remains valid for the lifetime of the Stage 2 kernel runtime unless a later stage explicitly defines a transition.

The implementation must not invalidate the memory representation while paging, stack, or heap initialization still depends on it.

The memory subsystem must not release or repurpose memory owned by another active subsystem.

## Determinism

Memory initialization must be deterministic for identical Stage 1 input.

The same memory map and identical kernel state must produce the same kernel-side memory classification.

The implementation must not depend on:

- wall-clock time;
- random values;
- firmware state;
- uninitialized memory;
- nondeterministic iteration order.

## Address Arithmetic

All address calculations must be overflow-safe.

This applies to:

- region end calculation;
- range comparison;
- region-size calculations;
- alignment calculations;
- kernel-range calculations;
- runtime-region calculations.

An arithmetic overflow must cause the affected validation to fail.

The implementation must never interpret an overflowed address as a valid memory range.

## Invariants

After successful initialization:

- the Stage 1 memory map has been validated;
- all required memory regions are represented correctly;
- invalid region ranges have been rejected;
- prohibited overlaps have been rejected;
- reserved memory remains unavailable;
- kernel memory remains protected;
- Stage 2 runtime memory is accounted for;
- memory required by paging is accounted for;
- memory required by the stack is accounted for;
- memory required by the heap can be validated against the memory state;
- ownership is internally consistent;
- physical and virtual memory are not conflated;
- firmware services are not required;
- `JPBootInfo` is not required;
- Stage 3 state is not required.

## Failure Semantics

Initialization must fail when:

- `context` is invalid;
- the Stage 1 memory map is missing;
- memory-map metadata is invalid;
- the region count exceeds the defined capacity;
- a region contains an invalid address range;
- a region calculation overflows;
- a prohibited overlap exists;
- an invalid region type is encountered;
- required kernel memory cannot be represented correctly;
- reserved memory would incorrectly become available;
- required Stage 2 runtime memory cannot be accounted for;
- resulting ownership state is contradictory.

Failure must not:

- corrupt the Stage 1 memory map;
- partially publish kernel memory state;
- modify firmware state;
- claim memory belonging to another subsystem.

## Boundary Rules

The implementation must remain strictly within the kernel-side memory initialization boundary.

It must not:

- interact with UEFI;
- interact with BIOS;
- perform physical-memory probing;
- load the kernel;
- load ELF segments;
- initialize the heap allocator;
- initialize the stack;
- construct page tables;
- switch paging state;
- construct `JPBootInfo`;
- implement Stage 3 diagnostics;
- implement recovery mechanisms.

## Dependencies

The implementation must include:

    #include "justos/kernel/memory.h"

It may include the shared boot-state definitions required to access `JPBootContext` and the Stage 1 memory representation.

Expected shared dependency:

    #include "justos/boot/boot_types.h"

Additional dependencies must be limited to functionality actually required by the implementation.

The implementation must not depend on firmware-specific headers.

## Implementation Freedom

The implementation may choose:

- private memory-state representation;
- region-validation helper structure;
- region iteration strategy;
- sorting strategy;
- overlap-detection implementation;
- internal classification representation;
- internal validation depth;
- internal consistency checks.

The implementation must not change:

- the Stage 1 memory input contract;
- memory ownership semantics;
- initialization order;
- public API;
- subsystem boundaries;
- failure semantics;
- x86_64 and AArch64 Stage 2 targets;
- paging/stack/heap ownership boundaries.

If an architecture-significant memory-management decision is required that is not defined by the Stage 2 specification, implementation must stop for that area and report the specification gap.

## Verification

Verify:

- successful initialization from valid Stage 1 memory state;
- invalid `JPBootContext`;
- missing memory map;
- invalid memory-map metadata;
- excessive region count;
- invalid region base;
- invalid region size;
- zero-sized region handling;
- inverted ranges;
- address arithmetic overflow;
- prohibited overlapping regions;
- contradictory ownership;
- reserved memory preservation;
- kernel-memory preservation;
- Stage 2 runtime-memory accounting;
- paging-memory accounting;
- stack-memory accounting;
- heap-memory accounting;
- physical/virtual memory separation;
- deterministic classification;
- failure without partial state publication;
- preservation of Stage 1 memory state;
- absence of firmware dependencies;
- absence of `JPBootInfo` dependency;
- absence of Stage 3 dependency;
- compatibility with paging initialization;
- compatibility with stack initialization;
- compatibility with heap initialization;
- compatibility with ELF runtime validation.

---

# 11. `include/justos/kernel/elf.h`

## File

Stage 2 kernel ELF interface.

## Purpose

Defines the public interface of the Stage 2 ELF subsystem.

The header establishes the boundary for runtime validation and handling of the kernel ELF state that was already acquired, validated, loaded, and mapped during Stage 1.

The header does not define the Stage 1 kernel-loading process.

## Responsibilities

The header is responsible for declaring:

- Stage 2 kernel ELF initialization;
- runtime validation interfaces required by Stage 2;
- access to ELF state required by the Stage 2 initialization sequence;
- the public boundary between the ELF subsystem and `init`.

The header must not expose:

- firmware-specific ELF acquisition;
- kernel-image loading implementation;
- segment-copying implementation;
- firmware file-system interfaces;
- UEFI structures;
- BIOS structures;
- page-table implementation;
- heap implementation;
- Stage 3 functionality.

## Types / Structures

The ELF subsystem operates on the normalized kernel ELF state supplied through the Stage 1 boot context.

The implementation must reuse the authoritative Stage 1 types:

    JPKernelImage
    JPKernelHeader
    JPKernelSegment
    JPKernelInfo
    JPBootContext

The header must not redefine these structures.

If Stage 2 requires a dedicated runtime ELF state structure, it may be declared here only if that structure is part of the public Stage 2 contract.

Implementation-only ELF validation state must remain private to:

    src/kernel/elf.c

## Constants / Enums

ELF constants already defined by the authoritative boot types must be reused.

The header must not duplicate:

    JP_ELF_MAGIC
    JP_ELF_CLASS_64
    JP_ELF_TYPE_EXEC
    JP_ELF_TYPE_DYN
    JP_ELF_PT_LOAD

Architecture constants must also be reused from the authoritative Stage 1 definitions.

The header must not introduce new ELF constants unless they are required by the Stage 2 public contract.

## Public API

The ELF subsystem provides:

    JPBootResult jp_kernel_elf_initialize(
        JPBootContext *context
    );

This function validates and adopts the kernel ELF state required by Stage 2.

It does not acquire the kernel image.

It does not copy kernel segments.

It does not perform the initial Stage 1 ELF loading process.

No additional public API may be introduced unless explicitly defined by the Stage 2 specification.

## Internal API

No internal ELF functions are exposed by this header.

Internal operations remain private to:

    src/kernel/elf.c

These may include:

- context validation;
- ELF-header validation;
- program-header validation;
- segment-state validation;
- entry-point validation;
- architecture validation;
- loaded-image validation;
- mapping-state validation;
- permission-state validation;
- ELF consistency checks.

Exact internal helper decomposition is implementation-defined.

## Stage 1 Boundary

Stage 1 is responsible for the initial kernel ELF pipeline.

This includes:

- kernel-image acquisition;
- ELF identification;
- ELF header validation required for loading;
- `PT_LOAD` discovery;
- segment allocation;
- segment loading;
- initial segment mapping;
- initial permission preparation.

Stage 2 must consume the resulting normalized ELF state.

The Stage 2 ELF subsystem must not repeat Stage 1 loading operations.

## Runtime ELF Validation

Stage 2 may validate that the already loaded kernel ELF state remains internally consistent before Stage 2 completes.

Validation may include:

- ELF identification;
- architecture compatibility;
- ELF class;
- executable or supported dynamic-image type;
- entry-point validity;
- segment count;
- segment boundaries;
- loaded segment state;
- mapping state;
- required permissions;
- address consistency.

The exact validation set must match the Stage 2 specification.

## Entry Point

The ELF subsystem must validate the kernel entry state required by Stage 2.

The entry address must:

- belong to the loaded kernel image;
- correspond to a valid executable location;
- be compatible with the selected architecture execution environment;
- satisfy the Stage 1 → Stage 2 → kernel runtime contract.

The ELF subsystem must not transfer control to the entry point.

The final transfer to the Stage 2 completion continuation remains an initialization/orchestration responsibility.

## Segment State

The ELF subsystem consumes the segment state established by Stage 1.

Each segment must retain its authoritative lifecycle state.

The Stage 2 ELF subsystem must not silently reinterpret:

    validated
    allocated
    loaded
    mapped
    permissioned

as a new loading process.

If a required segment is not in the state expected by Stage 2, initialization must fail.

## Ownership / Lifetime

The ELF subsystem does not become the owner of the physical memory containing the kernel image merely because it validates the ELF state.

Memory ownership remains governed by the Stage 2 memory contract.

The ELF representation must remain valid for the lifetime required by the kernel initialization sequence.

The subsystem must not free or relocate the kernel image.

## Dependencies

The header must include the definitions required by its public API.

Expected dependency:

    #include "justos/boot/boot_types.h"

No firmware-specific headers may be included.

The header must not depend on:

- UEFI;
- BIOS;
- firmware file systems;
- heap implementation;
- paging implementation;
- stack implementation;
- Stage 3;
- Stage 4.

## Boundary Rules

The ELF header must remain a kernel-side runtime interface.

It must not expose:

- firmware image acquisition;
- firmware handles;
- firmware file APIs;
- physical allocation implementation;
- page-table internals;
- heap metadata;
- stack metadata.

The ELF subsystem must not become responsible for the complete kernel-loading pipeline.

## Invariants

After successful ELF initialization:

- the kernel ELF state exists;
- the ELF state corresponds to the loaded kernel image;
- the ELF architecture matches the selected architecture;
- the ELF representation is internally consistent;
- required segments are present;
- required segment state is valid;
- the entry point is valid;
- executable state required by the runtime is valid;
- kernel image memory remains protected;
- Stage 1 loading state remains authoritative;
- no firmware interface is required;
- `JPBootInfo` is not required;
- Stage 3 state is not required.

## Failure Semantics

Initialization must fail when:

- `JPBootContext` is invalid;
- kernel ELF state is missing;
- ELF identification is invalid;
- ELF class is unsupported;
- ELF architecture is incompatible;
- ELF type is unsupported;
- program-header state is invalid;
- segment state is inconsistent;
- a required segment is missing;
- segment boundaries are invalid;
- the entry point is invalid;
- required loaded/mapped/permissioned state is missing;
- the kernel image state contradicts the Stage 1 contract.

Failure must not:

- reload the kernel;
- modify the kernel image;
- modify firmware state;
- modify page tables;
- modify the stack;
- modify heap state;
- partially publish runtime ELF state.

## Determinism

ELF validation must be deterministic for identical input state.

The same kernel ELF representation must produce the same validation result.

The implementation must not depend on:

- time;
- random state;
- firmware state;
- unrelated subsystem state.

## Implementation Freedom

The implementation may choose:

- internal validation order;
- private validation helpers;
- internal representation of temporary ELF state;
- validation granularity;
- consistency-check implementation.

The implementation must not change:

- the Stage 1 ELF loading boundary;
- the public API;
- ownership semantics;
- entry-point semantics;
- segment-state semantics;
- x86_64 and AArch64 target requirements;
- Stage 2 initialization order.

If a runtime ELF requirement is architecture-significant and absent from the Stage 2 specification, implementation must stop for that area and report the specification gap.

## Verification

Verify:

- the header compiles independently;
- all public declarations match `elf.c`;
- Stage 1 ELF structures are reused;
- ELF constants are not duplicated;
- selected-architecture validation is enforced;
- invalid ELF state is rejected;
- invalid segment state is rejected;
- invalid entry point is rejected;
- loaded-state validation is correct;
- mapped-state validation is correct;
- permission-state validation is correct;
- kernel image memory is preserved;
- no kernel image is reloaded;
- no segment copying is performed;
- no firmware dependency exists;
- no `JPBootInfo` dependency exists;
- no Stage 3 dependency exists;
- the ELF subsystem remains compatible with the Stage 1 → Stage 2 boundary.

---

# 12. `src/kernel/elf.c`

## File

Stage 2 kernel ELF implementation.

## Purpose

Implements the public ELF interface defined by `include/justos/kernel/elf.h`.

The module validates and adopts the kernel ELF state already established by Stage 1 so that the kernel can safely continue through the remaining Stage 2 initialization sequence.

Stage 2 ELF handling operates on an already loaded kernel image.

It does not recreate the Stage 1 kernel-loading pipeline.

## Responsibilities

The implementation is responsible for:

- validating the supplied `JPBootContext`;
- validating the kernel ELF state;
- validating the ELF identification state;
- validating the ELF class;
- validating the ELF architecture;
- validating the supported ELF type;
- validating the kernel ELF header;
- validating the program-header representation required by Stage 2;
- validating the kernel segment representation;
- validating segment boundaries;
- validating loaded segment state;
- validating mapped segment state;
- validating permission state where required;
- validating the kernel entry point;
- validating consistency between ELF metadata and the loaded kernel image;
- establishing the runtime ELF state required by Stage 2;
- reporting ELF initialization failures.

The implementation must not:

- acquire the kernel image;
- access firmware;
- read files from UEFI or BIOS;
- discover `PT_LOAD` segments for the first time;
- allocate kernel segments;
- copy kernel segments;
- establish the initial Stage 1 mappings;
- relocate the kernel;
- replace the paging subsystem;
- initialize the heap;
- initialize the stack;
- construct `JPBootInfo`.

## Public API

The implementation must provide:

    JPBootResult jp_kernel_elf_initialize(
        JPBootContext *context
    );

The implementation must match the declaration in:

    include/justos/kernel/elf.h

No additional public symbols may be introduced.

## Internal API

Internal functions remain private to `elf.c`.

The implementation may provide helpers for:

- context validation;
- ELF-header validation;
- ELF-identification validation;
- architecture validation;
- program-header validation;
- segment validation;
- segment-range validation;
- entry-point validation;
- loaded-image validation;
- mapping-state validation;
- permission-state validation;
- ELF-state consistency validation.

Exact helper names and decomposition are implementation-defined.

### Decided rules (the exact checks of `jp_kernel_elf_initialize()`)

The function only reads. A null `context` is `JP_BOOT_ERROR_INVALID_PARAM`; an empty ownership table is `JP_BOOT_NOT_INITIALIZED`; every check below that fails is `JP_BOOT_FAILED`. The checks are made in this order, and the first failure ends the function.
1. `kernel.validation_state` is `JP_KERNEL_VALIDATION_VALID` and `kernel.load_state` is `JP_KERNEL_LOAD_LOADED`; the architecture is supported by the build.
2. The header: `magic` is `JP_ELF_MAGIC`; `elf_class` is `JP_ELF_CLASS_64`; `type` is `JP_ELF_TYPE_EXEC` (the only type that Stage 1 accepts: Stage 2 does not relocate); `machine` is `JP_ELF_MACHINE_X86_64` for an x86_64 context and `JP_ELF_MACHINE_AARCH64` for an AArch64 one; `program_header_entry_size` is 56 (the size of an ELF64 program header) and `program_header_count` is at least `segment_count`.
3. `segment_count` is between 1 and `JP_MAX_KERNEL_SEGMENTS`.
4. For every segment: `state` is `JP_KERNEL_SEGMENT_PERMISSIONED`; `memory_size` is above zero and `file_size <= memory_size`; the virtual range `virtual_address` for `memory_size` bytes and the physical range `physical_address` for `memory_size` bytes do not overflow; the first and the last byte of the virtual range are canonical addresses (bits 63 to 47 all equal on x86_64; bits 63 to 48 all equal on AArch64); and the physical range, rounded outwards to pages, is owned as one range by `KERNEL_IMAGE` (`jp_kernel_memory_query()`).
5. No two segments overlap in virtual addresses.
6. The entry point: `kernel.entry_point`, `kernel.header.entry_point` and `handoff.kernel_entry` are the same number; it is canonical; and it lies inside the virtual range of a segment whose flags contain `JP_ELF_PF_X`.
7. `jp_kernel_paging_validate()` succeeds (it checks that every segment is mapped with the permission of its flags, so the mapping and permission states are checked there and nothing is created or changed here).

## ELF Identification Validation

The implementation must validate the ELF identification information required by Stage 2.

The ELF representation must identify a valid ELF image.

The implementation must validate the required ELF class.

The Stage 2 target architecture is selected from:

    x86_64
    AArch64

The implementation must reject an ELF class or architecture that does not match the selected Stage 2 architecture.

The implementation must use the authoritative ELF constants defined by the Stage 1 boot types.

## ELF Type

The implementation must accept only ELF image types explicitly supported by the Stage 2 contract.

The supported type set must remain consistent with the Stage 1 kernel-image contract.

The implementation must reject unsupported ELF types.

It must not silently reinterpret an unsupported image type as a supported one.

## Program Headers

Stage 1 is responsible for the initial discovery of the kernel program headers and `PT_LOAD` segments.

Stage 2 validates the resulting normalized representation.

The implementation may inspect the underlying ELF metadata when required to verify consistency, but it must not turn this validation into a second kernel-loading pipeline.

Program-header validation may include:

- valid program-header count;
- valid program-header representation;
- valid program-header boundaries;
- consistency with the loaded segment representation;
- valid `PT_LOAD` state where required.

## Segment Validation

Every kernel segment represented by the Stage 1 kernel state must be validated according to the Stage 2 contract.

Validation must include, where applicable:

- valid virtual address;
- valid physical/load address;
- valid file size;
- valid memory size;
- valid segment range;
- valid alignment;
- valid permissions;
- loaded state;
- mapped state;
- permissioned state.

The implementation must reject invalid segment ranges.

The implementation must detect arithmetic overflow when calculating segment boundaries.

## Segment Lifecycle

The Stage 1 segment lifecycle is:

    validated
        ↓
    allocated
        ↓
    loaded
        ↓
    mapped
        ↓
    permissioned

Stage 2 consumes this state.

Stage 2 must not restart the lifecycle.

A segment that has not reached the state required by Stage 2 must cause ELF initialization to fail.

The implementation must not silently transition a segment from one state to another as a substitute for the responsible Stage 1 operation.

## Segment Range Validation

For each segment:

    segment_end = segment_base + segment_memory_size

must be calculated with overflow protection.

The implementation must verify:

    segment_end > segment_base

when the segment has a non-zero size.

The resulting range must remain within the address space permitted by the selected architecture contract. x86_64 ranges must be canonical; AArch64 ranges must satisfy the selected 48-bit regime.

Invalid ranges must cause initialization failure.

## File Size vs Memory Size

Where ELF metadata exposes both file size and memory size, the implementation must preserve the ELF constraint that the memory representation cannot be smaller than the file-backed portion.

Conceptually:

    p_memsz >= p_filesz

An invalid relationship must cause ELF initialization failure.

The implementation must not modify the loaded image to compensate for invalid metadata.

## Entry Point Validation

The kernel entry point must be validated before Stage 2 initialization is considered complete.

The entry point must:

- belong to the loaded kernel image;
- correspond to an executable region;
- be compatible with the selected architecture execution environment;
- correspond to the entry state established by Stage 1.

The implementation must reject an entry point outside the loaded kernel image.

The implementation must reject an entry point located in a non-executable segment when executable permissions are part of the finalized contract.

The ELF subsystem must validate the entry point but must not transfer control to it.

## Loaded Image Validation

The implementation must verify that the ELF representation corresponds to the already loaded kernel image.

Validation may include:

- image base;
- image size;
- segment count;
- segment boundaries;
- segment addresses;
- segment sizes;
- entry address;
- segment state.

The implementation must reject contradictions between ELF metadata and normalized kernel state.

## Mapping Validation

The ELF subsystem may validate that the kernel segments have reached the mapping state required by Stage 2.

It must not replace or rebuild the page tables.

It must not switch the active page-table root.

It must not silently create missing mappings.

If required mappings are missing, ELF initialization fails.

Paging remains responsible for the actual virtual-memory implementation.

## Permission Validation

The implementation must validate the permission state required for kernel execution.

Where the Stage 1 representation provides segment permissions, the implementation must ensure that those permissions are consistent with the ELF metadata and the Stage 2 execution contract.

The implementation must not silently make a non-executable segment executable.

The implementation must not silently make a required executable segment non-executable.

## Architecture Validation

The Stage 2 architecture targets are x86_64 and AArch64.

The ELF machine type must therefore correspond to:

    JP_ELF_MACHINE_X86_64

Architectures other than x86_64 and AArch64 must be rejected by the Stage 2 ELF implementation. The ELF machine type must match the selected architecture.

The implementation must not generalize architecture handling beyond the defined Stage 2 target.

## Architecture-Specific ELF Contract

For x86_64, the machine type is `JP_ELF_MACHINE_X86_64` and the entry point must be a canonical executable x86_64 address.

For AArch64, the machine type is `JP_ELF_MACHINE_AARCH64` and the entry point must be a valid executable address in the selected AArch64 48-bit virtual-address regime.

Both paths validate the same loaded/mapped/permissioned lifecycle and reject a machine type that does not match the selected boot architecture.

## Memory Interaction

The ELF subsystem must preserve the kernel image memory already established by Stage 1.

It must not:

- free kernel segments;
- move kernel segments;
- allocate replacement segments;
- overwrite segment contents;
- modify kernel image ownership.

The memory subsystem remains responsible for authoritative memory ownership.

## Paging Interaction

The ELF subsystem depends on the paging environment established by the Stage 2 paging subsystem.

The ELF implementation may validate mapping state.

It must not:

- construct the active page tables;
- replace the page-table root;
- modify CR3;
- independently establish the kernel virtual-address layout.

If the finalized initialization order requires ELF validation after paging initialization, `init` must enforce that order.

## Stack Interaction

The ELF implementation does not initialize the kernel stack.

It must not:

- modify RSP;
- allocate stack memory;
- switch stacks;
- treat stack memory as part of the kernel image unless explicitly represented as such.

The stack subsystem remains responsible for stack initialization.

## Heap Interaction

The ELF implementation does not allocate heap memory.

Temporary validation state must not require the early heap unless the finalized Stage 2 contract explicitly permits that dependency.

The ELF implementation must not allocate or free heap blocks.

## State / Lifecycle

The ELF subsystem follows the logical lifecycle:

    NOT_INITIALIZED
        ↓
    VALIDATING
        ↓
    INITIALIZED
        ↓
    ACTIVE

Failure results in:

    FAILED

The exact internal representation is implementation-defined.

The observable initialization result must remain consistent with the Stage 2 contract.

## State Publication

Runtime ELF state must not be published before validation succeeds.

If any mandatory ELF validation fails:

- initialization fails;
- partial runtime state must not be published;
- the loaded kernel image remains unchanged.

Successful initialization establishes a complete runtime ELF state.

## Ownership / Lifetime

The ELF subsystem may retain references to the normalized Stage 1 ELF state.

It must not assume ownership of the physical memory containing the kernel image unless explicitly assigned by the Stage 2 memory contract.

The kernel image remains valid for the lifetime required by the kernel runtime.

The ELF implementation must not release or repurpose that memory.

## Determinism

ELF validation must be deterministic for identical kernel state.

Identical ELF metadata and identical loaded-image state must produce the same initialization result.

The implementation must not depend on:

- time;
- random state;
- firmware state;
- uninitialized memory;
- unrelated subsystem ordering.

## Address Arithmetic

All ELF address calculations must be overflow-safe.

This includes:

- segment end calculation;
- image end calculation;
- file-range calculation;
- memory-range calculation;
- entry-point range checks;
- alignment calculations.

An arithmetic overflow must cause validation failure.

## Invariants

After successful ELF initialization:

- the kernel ELF state is present;
- the ELF representation is valid;
- the ELF class is supported;
- the architecture is the selected supported architecture;
- the ELF type is supported;
- program-header state is consistent;
- kernel segments are valid;
- segment ranges are valid;
- required segment lifecycle state is established;
- the entry point is valid;
- the entry point belongs to executable kernel memory;
- required mapping state is valid;
- required permission state is valid;
- kernel image memory remains unchanged;
- memory ownership remains consistent;
- paging ownership remains with paging;
- no firmware interface is required;
- `JPBootInfo` is not required;
- Stage 3 state is not required.

## Failure Semantics

Initialization must fail when:

- `context` is invalid;
- kernel ELF state is missing;
- ELF identification is invalid;
- ELF class is unsupported;
- ELF machine type is not x86_64;
- ELF type is unsupported;
- program-header state is invalid;
- segment count is invalid;
- a segment has an invalid range;
- segment arithmetic overflows;
- `p_memsz < p_filesz`;
- required segment state is missing;
- loaded state is inconsistent;
- mapped state is inconsistent;
- permission state is inconsistent;
- the entry point is invalid;
- the entry point is outside the loaded image;
- the entry point is not executable when executability is required;
- ELF metadata contradicts normalized Stage 1 state.

Failure must not:

- modify the kernel image;
- modify firmware state;
- allocate replacement kernel segments;
- modify memory ownership;
- modify the stack;
- modify heap state;
- replace paging state;
- publish partial ELF runtime state.

## Boundary Rules

The ELF implementation must remain a validation and runtime-state subsystem.

It must not:

- acquire kernel files;
- communicate with firmware;
- rediscover the kernel image;
- perform the initial ELF load;
- copy `PT_LOAD` contents;
- allocate kernel segments;
- perform relocation;
- replace paging;
- initialize memory management;
- initialize the heap;
- initialize the stack;
- construct `JPBootInfo`;
- implement Stage 3 diagnostics;
- implement Stage 4 functionality.

## Dependencies

The implementation must include:

    #include "justos/kernel/elf.h"

It may use:

    #include "justos/boot/boot_types.h"

Additional standard headers may be included only when required by the implementation.

No UEFI or BIOS headers may be included.

No firmware-specific ELF acquisition dependencies may be introduced.

## Implementation Freedom

The implementation may choose:

- internal validation order;
- private helper functions;
- temporary validation state;
- internal consistency-check strategy;
- exact representation of runtime ELF state;
- exact mechanism used to compare normalized state with ELF metadata.

The implementation must not change:

- the public API;
- the Stage 1 → Stage 2 ELF boundary;
- x86_64 and AArch64 architecture requirements;
- segment lifecycle semantics;
- entry-point semantics;
- memory ownership;
- paging ownership;
- initialization order;
- failure semantics.

If a required ELF runtime behavior is not defined by the Stage 2 specification, implementation must stop for that area and report the specification gap.

## Verification

Verify:

- successful initialization from valid Stage 1 ELF state;
- invalid `JPBootContext`;
- missing kernel ELF state;
- invalid ELF identification;
- unsupported ELF class;
- machine type inconsistent with the selected architecture;
- unsupported ELF type;
- invalid program-header state;
- invalid segment count;
- invalid segment ranges;
- segment arithmetic overflow;
- invalid `p_filesz` / `p_memsz` relationship;
- invalid alignment;
- invalid loaded state;
- invalid mapped state;
- invalid permission state;
- invalid entry point;
- entry point outside the kernel image;
- entry point in non-executable memory;
- inconsistency between ELF metadata and normalized segment state;
- preservation of kernel image contents;
- preservation of kernel image ownership;
- preservation of page-table state;
- preservation of stack state;
- absence of heap allocation;
- absence of firmware dependencies;
- absence of kernel reloading;
- absence of segment copying;
- absence of relocation;
- absence of `JPBootInfo` dependency;
- absence of Stage 3 dependency;
- deterministic validation;
- failure without partial state publication.

---

# 13. `include/justos/kernel/init.h`

## File

Stage 2 initialization interface.

## Purpose

Defines the public interface of the Stage 2 initialization orchestrator.

The header establishes the entry point through which the kernel invokes the deterministic Stage 2 initialization sequence.

The `init` subsystem coordinates the Stage 2 subsystems but does not implement their responsibilities.

## Responsibilities

The header is responsible for declaring:

- Stage 2 initialization entry point;
- public initialization interface required by the kernel;
- the boundary between Stage 2 orchestration and the individual Stage 2 subsystems.

The header must not expose:

- CPU implementation details;
- paging implementation details;
- stack implementation details;
- heap implementation details;
- memory implementation details;
- ELF implementation details;
- firmware-specific state;
- Stage 3 state;
- Stage 4 state.

## Types / Structures

The initialization interface operates on the Stage 1 execution context:

    JPBootContext

The header must reuse the authoritative definition of `JPBootContext`.

It must not redefine:

    JPBootContext

It must not introduce a second global initialization-state structure unless explicitly required by the Stage 2 specification.

Any internal orchestration state remains private to:

    src/kernel/init.c

## Constants / Enums

No additional public constants or enums are required.

Stage 2 subsystem states and result values must use the authoritative definitions already established by the project.

The header must not duplicate:

- boot result values;
- CPU state enums;
- memory-region enums;
- ELF enums;
- paging state enums.

If a new public initialization state is required, it must first be defined by the Stage 2 specification.

## Public API

The initialization subsystem provides:

    JPBootResult jp_kernel_init(
        JPBootContext *context
    );

    _Noreturn void jp_kernel_stage2_failure(
        JPBootContext *context,
        JPBootResult result
    );

    _Noreturn void jp_kernel_stage2_complete(
        JPBootContext *context
    );

`jp_kernel_stage2_complete()` is declared here and implemented outside Stage 2 (Stage 4 `boot_manager.c`).

This function is the public Stage 2 initialization entry point.

It coordinates the complete Stage 2 initialization sequence:

    Stage 1 Handoff
        ↓
    CPU
        ↓
    Memory
        ↓
    Paging
        ↓
    Stack
        ↓
    Heap
        ↓
    ELF
        ↓
    Kernel Init
        ↓
    Stage 2 completion continuation

The exact relationship between the final `Kernel Init` step and `jp_kernel_init()` must remain consistent with the Stage 2 specification.

`jp_kernel_stage2_failure()` is the canonical non-returning failure sink used by both architecture-specific entry stubs. No additional public orchestration API may be introduced without a specification change beyond the declarations above.

## Internal API

No internal orchestration functions are exposed by this header.

Private helpers remain in:

    src/kernel/init.c

These may include:

- context validation;
- subsystem sequencing;
- subsystem-result handling;
- initialization-state tracking;
- failure propagation;
- completion validation.

Exact helper decomposition is implementation-defined.

## Initialization Order

The header defines the public entry point for the following normative order:

    CPU
        ↓
    Memory
        ↓
    Paging
        ↓
    Stack
        ↓
    Heap
        ↓
    ELF
        ↓
    Stage 2 completion continuation

The order is not interchangeable.

A subsystem must not be considered successfully initialized until its initialization function has returned success.

The orchestrator must not initialize a dependent subsystem before its required predecessor has completed.

## Responsibility Boundary

`init` is the coordinator.

It is not responsible for implementing:

    CPU
    Memory
    Paging
    Stack
    Heap
    ELF

Each subsystem remains responsible for its own initialization contract.

The orchestrator must call the appropriate subsystem interface and propagate its result.

## Ownership / Lifetime

The `init` subsystem does not become the owner of the resources initialized by the individual subsystems.

Ownership remains with the subsystem responsible for the corresponding resource.

For example:

    CPU    → CPU state
    Memory → kernel memory state
    Paging → page-table state
    Stack  → kernel stack state
    Heap   → heap allocator state
    ELF    → runtime ELF state

`init` coordinates their lifecycle but does not absorb their ownership.

## Failure Propagation

If a subsystem initialization fails, the orchestrator must:

- stop the Stage 2 initialization sequence;
- propagate the failure;
- prevent dependent subsystems from being initialized;
- avoid reporting Stage 2 as successfully initialized.

The orchestrator must not silently continue after a failed required subsystem.

The orchestrator must not replace a subsystem's failure with a generic success result.

## Dependencies

The header must include only the definitions required by its public API.

Expected dependency:

    #include "justos/boot/boot_types.h"

Additional dependencies are permitted only when required by the public interface.

The header must not include:

- UEFI headers;
- BIOS headers;
- paging implementation headers;
- heap implementation headers;
- stack implementation headers;
- ELF implementation headers;
- Stage 3 headers;
- Stage 4 headers.

## Boundary Rules

The initialization header must remain the top-level Stage 2 orchestration interface.

It must not become a general kernel API header.

It must not expose subsystem internals.

It must not provide:

- memory allocation;
- page-table manipulation;
- CPU control-register manipulation;
- stack switching;
- ELF loading;
- firmware access;
- diagnostic infrastructure;
- recovery infrastructure.

## Invariants

The public initialization interface must preserve:

- deterministic initialization order;
- explicit subsystem boundaries;
- one authoritative Stage 1 → Stage 2 context;
- failure propagation;
- dependency ordering;
- x86_64 and AArch64 Stage 2 targeting;
- absence of firmware dependencies;
- absence of `JPBootInfo`;
- absence of Stage 3 dependencies;
- absence of Stage 4 behavioral dependencies (the only Stage 4 link is the declared symbol `jp_kernel_stage2_complete`).

`init` must never claim that Stage 2 is complete while a required subsystem remains uninitialized.

## Failure Semantics

`jp_kernel_init()` returns a `JPBootResult`.

The function must return failure when any mandatory Stage 2 subsystem fails to initialize.

After a failure:

- initialization must stop;
- later dependent subsystems must not be invoked;
- the failure must remain observable to the caller.

The header must not define a separate failure mechanism that conflicts with `JPBootResult`.

## Implementation Freedom

The implementation may choose:

- internal initialization-state representation;
- exact subsystem-call structure;
- private helper functions;
- internal validation depth;
- result-propagation implementation;
- completion-state representation.

The implementation must not change:

- public API;
- initialization order;
- subsystem responsibilities;
- subsystem ownership;
- failure propagation;
- Stage 1 input contract;
- x86_64 and AArch64 targets;
- Stage 2 → Stage 3 boundary.

If orchestration requires an architecture-significant decision not defined by the Stage 2 specification, implementation must stop for the affected area and report the specification gap.

## Verification

Verify:

- the header compiles independently;
- the public declaration matches `init.c`;
- `JPBootContext` is reused rather than redefined;
- the Stage 2 initialization entry point is unique;
- no subsystem internals are exposed;
- no firmware types are exposed;
- no `JPBootInfo` dependency exists;
- no Stage 3 dependency exists;
- no Stage 4 behavioral dependency exists (only the declared symbol `jp_kernel_stage2_complete`);
- the initialization order is explicitly represented by the implementation;
- subsystem failures are propagated;
- dependent initialization stops after failure;
- `init` does not implement subsystem responsibilities;
- ownership remains with the individual subsystems;
- the interface remains limited to Stage 2 orchestration.

---

# 14. `src/kernel/init.c`

## File

Stage 2 initialization orchestrator implementation.

## Purpose

Implements the public Stage 2 initialization interface defined by:

    include/justos/kernel/init.h

The module coordinates the complete deterministic Stage 2 initialization sequence between the Stage 1 handoff and the point at which the kernel runtime environment is ready for the Stage 2 completion continuation.

`init.c` is an orchestration component.

It does not implement CPU, memory, paging, stack, heap, or ELF functionality itself.

## Responsibilities

The implementation is responsible for:

- validating the Stage 1 → Stage 2 context;
- establishing the Stage 2 initialization sequence;
- invoking CPU initialization;
- invoking memory initialization;
- invoking paging initialization;
- invoking stack initialization;
- invoking heap initialization;
- invoking ELF initialization;
- enforcing subsystem dependency order;
- stopping initialization when a mandatory subsystem fails;
- propagating initialization failures;
- validating Stage 2 completion state;
- returning the final Stage 2 initialization result.

The implementation must not:

- implement CPU initialization;
- implement memory management;
- construct page tables;
- initialize the stack;
- implement the heap allocator;
- load the kernel ELF image;
- access firmware;
- implement Stage 3 diagnostics;
- implement recovery;
- construct `JPBootInfo`.

## Public API

The implementation must provide:

    JPBootResult jp_kernel_init(
        JPBootContext *context
    );

    _Noreturn void jp_kernel_stage2_failure(
        JPBootContext *context,
        JPBootResult result
    );

The implementation must match:

    include/justos/kernel/init.h

`jp_kernel_stage2_complete()` is declared by `init.h` but implemented outside Stage 2; `init.c` only calls it (through `jp_kernel_stage2_failure()`).

No additional public symbols may be introduced.

`jp_kernel_stage2_failure()` is the architecture-neutral failure sink called by the entry stubs. It records the result in `context->stage2_result`, transfers to `jp_kernel_stage2_complete(context)`, and does not return. It does not access firmware or require Stage 3 to exist.

## Internal API

Internal helpers remain private to `init.c`.

The implementation may provide helpers for:

- context validation;
- subsystem invocation;
- subsystem-result handling;
- initialization-state tracking;
- completion validation;
- failure propagation.

Exact helper names and decomposition are implementation-defined.

## Initialization Sequence

The following sequence is normative:

    Stage 1 Handoff
          ↓
        CPU
          ↓
       Memory
          ↓
       Paging
          ↓
        Stack
          ↓
        Heap
          ↓
         ELF
          ↓
    Stage 2 completion continuation

The implementation must preserve this ordering.

A subsystem may be initialized only after all subsystems required by its contract have successfully completed.

The orchestrator must not reorder independent-looking operations merely for implementation convenience if doing so violates the defined Stage 2 sequence.

## CPU Initialization

The first Stage 2 subsystem invocation is CPU initialization.

The orchestrator must invoke the public CPU initialization interface defined by:

    include/justos/kernel/cpu.h

CPU initialization failure terminates Stage 2 initialization.

Memory initialization must not begin after a failed CPU initialization.

The orchestrator does not perform CPU operations itself.

## Memory Initialization

After successful CPU initialization, the orchestrator invokes the memory subsystem.

The memory subsystem is responsible for adopting and validating the Stage 1 memory state.

Memory initialization failure terminates Stage 2 initialization.

Paging initialization must not begin after a failed memory initialization.

The orchestrator does not inspect or modify memory ownership directly.

## Paging Initialization

After successful memory initialization, the orchestrator invokes the paging subsystem.

Paging establishes the kernel-side virtual-memory foundation required by later Stage 2 subsystems.

Paging initialization failure terminates Stage 2 initialization.

The orchestrator does not construct or modify page tables itself.

## Stack Initialization

After successful paging initialization, the orchestrator invokes the stack subsystem.

The stack subsystem establishes the kernel stack runtime state required by the remaining initialization process.

Stack initialization failure terminates Stage 2 initialization.

The orchestrator does not manipulate the stack pointer itself.

## Heap Initialization

After successful stack initialization, the orchestrator invokes the heap subsystem.

The heap subsystem establishes the early kernel allocation mechanism.

Heap initialization failure terminates Stage 2 initialization.

The orchestrator does not perform allocations itself unless the finalized Stage 2 contract explicitly requires a coordinator-owned allocation.

## ELF Initialization

After successful heap initialization, the orchestrator invokes the ELF subsystem.

The ELF subsystem validates and adopts the already loaded kernel ELF state.

ELF initialization failure terminates Stage 2 initialization.

The orchestrator does not load or modify the kernel image.

## Subsystem Dependency Rule

The initialization order represents a dependency chain:

    CPU
      ↓
    Memory
      ↓
    Paging
      ↓
    Stack
      ↓
    Heap
      ↓
    ELF

Each subsystem must return successful initialization before the next subsystem is invoked.

The orchestrator must not call a later subsystem after a required predecessor has failed.

## Failure Propagation

The orchestrator must propagate the failure result returned by the failed subsystem.

Conceptually:

    result = subsystem_initialize(context)

    if result != success:
        return result

The implementation may use a different internal control-flow structure, but the observable behavior must remain equivalent.

The orchestrator must not:

- replace a specific failure with success;
- continue after a mandatory failure;
- retry indefinitely;
- silently skip a failed subsystem.

## Partial Initialization

Stage 2 initialization may leave previously initialized subsystems active when a later subsystem fails.

The orchestrator must not falsely claim that those resources have been rolled back unless the Stage 2 specification explicitly defines rollback.

The orchestrator itself must not invent rollback semantics.

If rollback is not defined for a subsystem, initialization failure means:

    Stage 2 initialization failed

not:

    all previous subsystem state has been destroyed

## Completion

Stage 2 is complete only after all mandatory initialization steps have succeeded.

The successful state is:

    CPU       initialized
    Memory    initialized
    Paging    initialized
    Stack     initialized
    Heap      initialized
    ELF       initialized

Only after this condition is satisfied may the kernel proceed toward:

    Stage 2 completion continuation

The orchestrator must not report success before this state exists.

## Stage 2 Completion Boundary

`init.c` establishes the state required before the Stage 2 completion continuation and returns the Stage 2 result to the architecture-specific Stage 2 entry stub.

The architecture-specific Stage 2 entry stub is the sole component that transfers control to the Stage 2 completion continuation after `jp_kernel_init(context)` returns success. `jp_kernel_init()` must not call `kernel_main()` or the continuation directly, and no second undocumented entry layer may be introduced.

At the transition to the Stage 2 completion continuation:

```text
x86_64:
    System V AMD64-compatible C calling convention
    valid RSP
    long mode
    paging enabled
    IF = 0

AArch64:
    AArch64 C calling convention
    valid SP
    required exception level
    MMU state established
    PSTATE.DAIF = 1111
```

Both paths require the mappings, permissions, ownership, and architecture state established by the preceding Stage 2 contract.

## State / Lifecycle

The orchestrator follows the logical lifecycle:

    NOT_STARTED
        ↓
    CPU_INITIALIZED
        ↓
    MEMORY_INITIALIZED
        ↓
    PAGING_INITIALIZED
        ↓
    STACK_INITIALIZED
        ↓
    HEAP_INITIALIZED
        ↓
    ELF_INITIALIZED
        ↓
    COMPLETE

Any failed initialization step results in:

    FAILED

The exact internal state representation is implementation-defined.

The observable initialization behavior must remain equivalent to this lifecycle.

## State Ownership

`init.c` owns only the orchestration state.

It does not own:

- CPU state;
- memory ownership state;
- page-table state;
- stack state;
- heap allocator state;
- ELF image memory.

Those resources remain owned by their respective subsystems.

## Context Ownership

The `JPBootContext` supplied to `jp_kernel_init()` is the shared Stage 1 → Stage 2 initialization context.

The orchestrator passes the same context to the Stage 2 subsystem initialization functions.

The orchestrator must not create a replacement context containing duplicated authoritative state.

The orchestrator must not silently discard fields from the Stage 1 context that are required by a subsystem.

## Context Mutation

Subsystems may update the portions of `JPBootContext` explicitly assigned to them by their contracts.

`init.c` must not modify subsystem-owned state merely to simplify orchestration.

Stage 2 never writes `current_stage`: the enumeration `JPBootStage` has no Stage 2 values, and `current_stage` is written only by the public functions of the Stage 1 pipeline. After a successful Stage 2 it still reads `JP_BOOT_STAGE_READY`. The only field of the context that the orchestrator writes is `stage2_result` (through `jp_kernel_stage2_failure()`), and the areas that the subsystems own (`stage2`, see `memory.c`).

### Decided rules (checking the entry and a broken handoff)

The first step of `jp_kernel_init()`, before any subsystem runs, is to check the handoff. The result is `JP_BOOT_ERROR_INVALID_PARAM` when `context` is `NULL`, and `JP_BOOT_FAILED` when one of these is not true: `context->current_stage == JP_BOOT_STAGE_READY`, `context->handoff.state == JP_HANDOFF_STATE_READY`, `context->handoff.ownership_transferred`, `context->handoff.interrupts_disabled`, `context->handoff.paging_enabled`, `context->handoff.architecture` equals the architecture the code was built for. No subsystem is started after a failed check.

`jp_kernel_stage2_failure(context, result)` with `context == NULL` writes nothing (there is nowhere to write) and does not call `jp_kernel_stage2_complete()` (it would have no context to hand over). It stops the processor instead: a loop that waits for an interrupt with the interrupts masked (x86_64: `cli`, then `hlt` repeated; AArch64: `wfi` repeated). It never returns and never pretends that Stage 2 succeeded.


**The sequence, exactly.** After the entry check, `jp_kernel_init()` calls these functions with the same `context`, in this order, and stops at the first one that does not return `JP_BOOT_SUCCESS`: `jp_kernel_cpu_initialize`, `jp_kernel_memory_initialize`, `jp_kernel_paging_initialize`, `jp_kernel_stack_initialize`, `jp_kernel_heap_initialize`, `jp_kernel_elf_initialize`. It returns the result of the function that failed, unchanged, and calls nothing after it. When all six succeeded it checks the completion state: `jp_kernel_memory_validate_ownership(context)` and then `jp_kernel_paging_validate(context)` (the heap exists now, so these checks include it); the first one that does not succeed is returned unchanged. Only when both succeed does it return `JP_BOOT_SUCCESS`. Nothing is rolled back: after a failure the state of the earlier subsystems is left as it is, and the context is never used again for a normal start (the failure sink and what follows only read it).

**The failure sink, exactly.** `jp_kernel_stage2_failure(context, result)`: (1) a null `context` stops the processor (see above). (2) Otherwise `context->stage2_result` becomes `result`; but if `result` is `JP_BOOT_SUCCESS` (the caller made a mistake) it becomes `JP_BOOT_FAILED`, so a failure can never be recorded as success. (3) `jp_kernel_stage2_complete(context)` is called. No other field is written. Stopping the processor is done through the private hook `jp_kernel_halt()` (`halt_hw.h`; x86_64: `cli`, then `hlt` in a loop; AArch64: mask all interrupts, then `wfi` in a loop).

**The entry stubs, exactly.** `just_kernel_entry_x86_64` (`src/kernel/entry_x86_64.S`) and `just_kernel_entry_aarch64` (`src/kernel/entry_aarch64.S`) receive the context in the first argument register (`RDI`, `X0`). Each one saves the context in a register that C functions keep (`RBX`, `X19`), keeps the stack pointer aligned as the ABI requires at a call (x86_64: Stage 1 pushed a zero return address, so the stack is 8 modulo 16 at the entry and one `push` makes it 0 modulo 16; AArch64: the stack is 16-aligned at the entry and stays so), calls `jp_kernel_init(context)`, and looks at the 32-bit result: zero (`JP_BOOT_SUCCESS`) goes to `jp_kernel_stage2_complete(context)`; anything else goes to `jp_kernel_stage2_failure(context, result)` (result in `ESI` or `W1`). Neither call returns; if one does, the stub stops the processor in a `hlt` or `wfi` loop. The stubs touch no other register state, never enable interrupts, and never write memory except by `push` and `call`.

## Determinism

The initialization sequence must be deterministic.

For identical Stage 1 input and identical subsystem results, `init.c` must invoke the same subsystem sequence in the same order.

The implementation must not depend on:

- wall-clock time;
- random state;
- firmware behavior;
- thread scheduling;
- unspecified subsystem ordering.

Stage 2 initialization is single-sequence orchestration.

## Re-entry

Unless the public Stage 2 contract explicitly defines reinitialization, `jp_kernel_init()` must not be treated as safely reentrant.

Calling the initialization entry point after successful completion must not silently restart Stage 2.

Calling it after failure must follow the defined lifecycle semantics rather than creating a second independent initialization state.

The implementation must not invent reinitialization behavior.

## Concurrency

Stage 2 initialization is not a concurrent initialization pipeline.

The orchestrator must execute the mandatory subsystem sequence serially.

The implementation must not introduce parallel subsystem initialization.

This prevents a subsystem from observing another subsystem's incomplete state.

## Dependencies

The implementation must include:

    #include "justos/kernel/init.h"

It must additionally include the public interfaces of the Stage 2 subsystems it invokes:

    #include "justos/kernel/cpu.h"
    #include "justos/kernel/memory.h"
    #include "justos/kernel/paging.h"
    #include "justos/kernel/stack.h"
    #include "justos/kernel/heap.h"
    #include "justos/kernel/elf.h"

The exact include order may be implementation-defined.

The implementation must not include subsystem `.c` files.

It must not depend on:

- UEFI;
- BIOS;
- firmware-native APIs;
- Stage 3;
- Stage 4;
- `JPBootInfo`.

## Boundary Rules

`init.c` must remain an orchestration layer.

It must not become a location for miscellaneous initialization logic.

It must not directly:

- manipulate CR0;
- manipulate CR3;
- manipulate CR4;
- manipulate RSP;
- allocate heap blocks;
- modify memory-map ownership;
- parse ELF program headers;
- load kernel segments;
- communicate with firmware.

Those responsibilities belong to their respective subsystems.

## Error Handling

The orchestrator must treat a failed mandatory subsystem as a terminal failure for the current Stage 2 initialization attempt.

The returned `JPBootResult` must preserve the subsystem's failure information where the shared result contract permits it.

The implementation must not obscure a meaningful subsystem failure with an unrelated success or failure value.

## Invariants

After successful `jp_kernel_init()`:

- CPU initialization succeeded;
- memory initialization succeeded;
- paging initialization succeeded;
- stack initialization succeeded;
- heap initialization succeeded;
- ELF initialization succeeded;
- all required subsystem dependencies were satisfied in order;
- Stage 2 initialization is complete;
- control may transfer to the Stage 2 completion continuation;
- no firmware operation is required;
- `JPBootInfo` is not required;
- Stage 3 state is not required.

After a failed initialization:

- the failed subsystem's result is observable;
- later mandatory subsystems were not invoked;
- the orchestrator did not report Stage 2 success;
- the orchestrator did not invent rollback behavior.

## Failure Semantics

`jp_kernel_init()` must fail when:

- `context` is invalid;
- CPU initialization fails;
- memory initialization fails;
- paging initialization fails;
- stack initialization fails;
- heap initialization fails;
- ELF initialization fails;
- Stage 2 completion validation fails.

The first mandatory initialization failure terminates the sequence.

No later mandatory subsystem may be initialized after that failure.

## Implementation Freedom

The implementation may choose:

- `if`-based sequencing;
- a private initialization-step helper;
- a private state variable;
- a switch-based orchestration structure;
- exact failure-propagation control flow;
- exact completion-validation implementation.

The implementation must not change:

- initialization order;
- subsystem responsibilities;
- subsystem ownership;
- public API;
- Stage 1 context contract;
- failure propagation;
- x86_64 and AArch64 targets;
- Stage 2 completion boundary.

If a required orchestration behavior is not defined by the Stage 2 specification, implementation must stop for that area and report the specification gap.

## Verification

Verify:

- the implementation compiles independently;
- the public API matches `init.h`;
- the Stage 1 context is passed correctly;
- CPU is initialized first;
- memory is initialized only after CPU success;
- paging is initialized only after memory success;
- stack is initialized only after paging success;
- heap is initialized only after stack success;
- ELF is initialized only after heap success;
- failure stops the sequence;
- subsystem failure is propagated;
- later subsystems are not invoked after failure;
- Stage 2 success is reported only after all mandatory subsystems succeed;
- subsystem ownership is not modified by the orchestrator;
- no subsystem implementation is duplicated in `init.c`;
- no firmware dependency exists;
- no `JPBootInfo` dependency exists;
- no Stage 3 dependency exists;
- no Stage 4 behavioral dependency exists (only the declared symbol `jp_kernel_stage2_complete`);
- initialization is deterministic;
- initialization is serial;
- no undocumented rollback is performed;
- the Stage 2 completion boundary remains consistent with the specification.

---

## Stage 2 Execution Order

The Stage 2 initialization sequence is normative and must be executed in the following order:

        Stage 1 Handoff
          ↓
        Stage 2 Entry Stub
            ↓
        jp_kernel_init(context)
            ↓
          CPU
          ↓
       Memory
          ↓
       Paging
          ↓
        Stack
          ↓
        Heap
          ↓
           ELF
            ↓
        Stage 2 Complete
            ↓
        Stage 2 completion continuation

Each stage must complete successfully before the next stage begins.

The initialization order is part of the Stage 2 specification and must not be changed by implementation.

The `init` subsystem is responsible for orchestration only.

Subsystem-specific initialization remains the responsibility of the corresponding subsystem.

If any mandatory subsystem fails:

    current initialization
            ↓
          FAILED

No subsequent mandatory subsystem may be initialized after the failure.

---

## Stage 2 Architectural Rules

The following rules are normative.

### Architecture

- Stage 2 targets x86_64 and AArch64.
- Architectures other than x86_64 and AArch64 are outside the Stage 2 target.
- Architecture-specific behavior must not be generalized between architectures.
- Architecture-specific implementation must remain isolated inside the appropriate subsystem.

### Stage 1 Boundary

- Stage 2 begins after successful Stage 1 handoff.
- Stage 2 consumes the execution environment established by Stage 1.
- Stage 2 must not repeat Stage 1 responsibilities.
- Stage 2 must not access firmware directly.
- Stage 2 must not access UEFI directly.
- Stage 2 must not access BIOS directly.
- Stage 2 must not reconstruct firmware state.
- Stage 2 must not reacquire the kernel image.
- Stage 2 must not reload the kernel ELF image.
- Stage 2 must not repeat initial kernel segment loading performed by Stage 1.

### Subsystem Isolation

- CPU initialization belongs to `cpu`.
- Memory representation and adoption belong to `memory`.
- Virtual-memory initialization belongs to `paging`.
- Kernel stack initialization belongs to `stack`.
- Early allocation belongs to `heap`.
- Kernel ELF validation belongs to `elf`.
- Initialization orchestration belongs to `init`.

No subsystem may silently absorb another subsystem's responsibilities.

### Ownership

- Every persistent Stage 2 resource must have a defined owner.
- Ownership must not be duplicated.
- `init` owns orchestration state, not subsystem resources.
- Subsystems remain responsible for their own internal state.
- Stage 1-provided resources must retain the ownership semantics defined by the Stage 1 → Stage 2 contract.

### Determinism

- Stage 2 initialization is deterministic.
- Initialization order is fixed.
- Mandatory initialization is serial.
- Initialization must not depend on scheduling.
- Initialization must not depend on random state.
- Initialization must not depend on wall-clock time.
- Identical valid Stage 1 input must produce the same initialization sequence.

### Failure

- Mandatory subsystem failure terminates the initialization sequence.
- Failure must be propagated to the caller.
- Later mandatory subsystems must not be invoked after failure.
- The implementation must not silently convert failure into success.
- The implementation must not invent retry semantics.
- The implementation must not invent rollback semantics.

### Specification Authority

The Stage 2 specification is authoritative over implementation.

Implementation may choose internal mechanisms where the specification explicitly permits freedom.

Implementation may not redefine:

- architecture target;
- subsystem responsibilities;
- initialization order;
- ownership;
- public APIs;
- failure semantics;
- Stage boundaries;
- completion criteria.

---

## Stage 2 → Stage 3 Boundary

Stage 2 ends when the complete Stage 2 initialization environment has been established successfully.

At the Stage 2 → Stage 3 boundary:

- CPU state required by the kernel is initialized.
- Kernel memory state is established.
- Kernel paging state is established.
- Kernel stack is established.
- Early kernel heap is established.
- Kernel ELF state has been validated.
- All mandatory Stage 2 initialization steps have completed successfully.
- Stage 2 initialization has reported success.
- Control may continue into the next defined kernel execution stage.

Stage 2 does not implement Stage 3 diagnostics, recovery, logging infrastructure, panic handling, or other Stage 3 responsibilities.

Stage 3 may rely on the state established by Stage 2 according to the Milestone 0 architecture.

Stage 2 must not depend on Stage 3 functionality.

The canonical Milestone 0 flow is:

```text
Stage 1
    ↓
Stage 2
    ├── success → Stage 2 completion continuation
    │                 ↓
    │             Stage 3 (normal entry)
    │                 ↓
    │             Stage 4
    │                 ↓
    │             kernel_main(context)
    │                 ↓
    │             Milestone 0 Complete
    │
    └── failure → jp_kernel_stage2_failure(context, result)
                      ↓
                  jp_kernel_stage2_complete(context)
                      ↓
                  Stage 3 (failure entry)
                      ↓
                  recovery request (Stage 4) or terminal halt
```

Stage 2 fully prepares the kernel runtime environment. Stage 3 handles both the successful completion of Stage 2 and its failure, and in either case must not assume the availability of any resource whose initialization is not recorded as complete in the transferred context. Stage 4 works with the prepared environment to provide Boot UI and boot orchestration and never substitutes for Stage 2. `kernel_main(context)` is entered only after the Stage 4 boundary. Stage 3 and Stage 4 are not prerequisites for Stage 2 completion.

The Stage 2 → Stage 3 boundary must remain one-directional:

    Stage 2
       ↓
    Stage 3

Stage 3 functionality must not be required to complete Stage 2.

---

## 2. Specification Lock Criteria

Stage 2 may enter Specification Lock only when all architecture-significant behavior is explicitly defined.

The following criteria are mandatory.

### Architecture Contract

- [ ] Target architecture is explicitly defined.
- [ ] Unsupported architectures are explicitly excluded.
- [ ] Architecture-specific responsibilities are isolated.
- [ ] No architecture-significant behavior remains implicit.

### Stage 1 Input Contract

- [ ] Stage 1 → Stage 2 boundary is defined.
- [ ] Required Stage 1 state is identified.
- [ ] Ownership of Stage 1-provided resources is defined.
- [ ] Lifetime of Stage 1-provided resources is defined.
- [ ] Stage 2 does not require undocumented Stage 1 behavior.

### Subsystem Contract

For every Stage 2 subsystem:

- [ ] purpose is defined;
- [ ] responsibilities are defined;
- [ ] non-responsibilities are defined;
- [ ] public API is defined;
- [ ] state is defined;
- [ ] ownership is defined;
- [ ] lifetime is defined;
- [ ] dependencies are defined;
- [ ] failure semantics are defined;
- [ ] invariants are defined.

### Initialization Contract

- [ ] Complete execution order is defined.
- [ ] Dependency order is defined.
- [ ] Initialization success condition is defined.
- [ ] Initialization failure condition is defined.
- [ ] Failure propagation is defined.
- [ ] Partial initialization semantics are defined.
- [ ] Completion state is defined.

### Memory / Runtime Contract

- [ ] Memory adoption is defined.
- [ ] Paging foundation is defined.
- [ ] Stack state is defined.
- [ ] Heap state is defined.
- [ ] Resource ownership is defined.
- [ ] Required runtime invariants are defined.

### ELF Contract

- [ ] Stage 1 ELF loading boundary is defined.
- [ ] Stage 2 ELF validation responsibility is defined.
- [ ] ELF ownership is defined.
- [ ] ELF failure behavior is defined.
- [ ] Stage 2 does not duplicate Stage 1 ELF loading.

### Boundary Contract

- [ ] Stage 1 → Stage 2 boundary is defined.
- [ ] Stage 2 → Stage 3 boundary is defined.
- [ ] Stage 2 completion continuation and `kernel_main()` relationship are defined.
- [ ] No Stage 3 dependency is required for Stage 2 completion.

### File Contract

- [ ] Every Stage 2 production file is identified.
- [ ] Both architecture-specific entry stubs are identified and specified.
- [ ] Every file has a normative File Specification.
- [ ] Every Stage 2 component has Common, x86_64, AArch64, and Architecture Boundary contract sections.
- [ ] The Stage 2 Architecture Contract Matrix passes for every component.
- [ ] No required implementation file is missing from the specification.
- [ ] No undocumented production file is required.
- [ ] Header/API relationships are defined.

---

## 2. Specification Traceability

Every architecture-significant requirement must be traceable to one or more Stage 2 specifications.

| Requirement                  | Specification Location                | Verification                 |
| ---------------------------- | ------------------------------------- | ---------------------------- |
| x86_64 and AArch64 targets   | Stage 2 Definition / Architecture     | Architecture verification    |
| Stage 1 handoff consumption  | Stage 2 Definition / Stage 1 Boundary | Boundary verification        |
| CPU initialization           | `cpu.h` / `cpu.c`                     | CPU verification             |
| Memory initialization        | `memory.h` / `memory.c`               | Memory verification          |
| Paging initialization        | `paging.h` / `paging.c`               | Paging verification          |
| Stack initialization         | `stack.h` / `stack.c`                 | Stack verification           |
| Heap initialization          | `heap.h` / `heap.c`                   | Heap verification            |
| ELF validation               | `elf.h` / `elf.c`                     | ELF verification             |
| Initialization orchestration | `init.h` / `init.c`                   | Execution-order verification |
| Deterministic order          | Stage 2 Execution Order               | Integration verification     |
| Failure propagation          | subsystem specifications / `init.c`   | Failure verification         |
| Resource ownership           | subsystem specifications              | Ownership verification       |
| Stage 2 completion           | Stage 2 → Stage 3 Boundary            | Completion verification      |

Every row must resolve to an actual specification section.

No architecture-significant requirement may exist only in implementation.

---

## 2. Specification Completeness Verification

The Stage 2 specification is considered complete only if:

- [x] every required subsystem is specified;
- [x] every required production file is specified;
- [x] every public API is specified;
- [x] every architecture-significant structure is specified;
- [x] every architecture-significant enum is specified;
- [x] every required constant is specified;
- [x] every subsystem dependency is specified;
- [x] every ownership relationship is specified;
- [x] every lifetime requirement is specified;
- [x] every initialization dependency is specified;
- [x] every initialization order requirement is specified;
- [x] every failure path is specified;
- [x] every mandatory invariant is specified;
- [x] Stage 1 → Stage 2 boundary is specified;
- [x] Stage 2 → Stage 3 boundary is specified;
- [x] Stage 2 completion boundary is specified;
- [x] unsupported behavior is explicitly excluded;
- [x] implementation freedom is explicitly defined where applicable.

### Completeness Rule

If an implementation decision can change architecture, ownership, ABI, memory semantics, subsystem boundaries, initialization order, or observable failure behavior, that decision must be specified before implementation.

If such a decision is not specified:

    SPECIFICATION GAP
          ↓
    UPDATE SPECIFICATION
          ↓
    CONTINUE IMPLEMENTATION

The coder must not resolve an architecture-significant specification gap independently.

---

## 2. Specification Lock Decision

Stage 2 receives Specification Lock only if all of the following are true:

- [x] Specification Completeness Verification passed.
- [x] Specification Traceability is complete.
- [x] All Stage 2 subsystems have complete contracts.
- [x] All Stage 2 production files have complete File Specifications.
- [x] Execution order is unambiguous.
- [x] Ownership is unambiguous.
- [x] Failure semantics are unambiguous.
- [x] Stage boundaries are unambiguous.
- [x] Architecture target is unambiguous.
- [x] No architecture-significant implementation gap remains.

If any mandatory criterion fails:

    SPECIFICATION LOCK = REJECTED

If all mandatory criteria pass:

    SPECIFICATION LOCK = APPROVED

**Record of the decision (Stage 2).** `SPECIFICATION LOCK = APPROVED`, signed by the project owner on 2026-11-20. How it was reached, for the reader of the history: the specification text was audited against Stage 1 (the finished code) and against Part 1 of `milestone0_pre_lock.md` (all 27 items ticked), the gaps that were found were decided with the owner and written into the "Decided rules" blocks, and the contract was then implemented and tested (host tests, mutation checks, QEMU on x86_64 UEFI, x86_64 BIOS, and AArch64 UEFI). The two lists above (Completeness and Traceability) were ticked after that implementation, against the finished text, so they confirm that the text describes what was built. A later gap in an architecture-significant contract still follows the rule of "Completeness Rule": update the specification first. "Implementation Acceptance" below is a separate step and is not ticked by this decision.

Specification Lock approval means the architecture-significant contract is frozen for implementation.

It does not mean that every implementation detail must be predetermined.

---

## 2. Implementation Acceptance

After Specification Lock approval, the implementation is accepted only when it satisfies the locked contract.

### File Acceptance

- [x] Every specified file exists.
- [x] No required file has been omitted.
- [x] No undocumented production file has been introduced.
- [x] Headers match their implementations.
- [x] Public APIs match the specification.
- [x] Internal implementation remains within defined boundaries.

### Functional Acceptance

- [x] CPU initialization succeeds on valid Stage 1 state.
- [x] Memory initialization succeeds on valid Stage 1 state.
- [x] Paging initialization succeeds after memory initialization.
- [x] Stack initialization succeeds after paging initialization.
- [x] Heap initialization succeeds after stack initialization.
- [x] ELF initialization succeeds after heap initialization.
- [x] Successful initialization reaches the defined completion state.

### Failure Acceptance

- [x] Invalid context fails.
- [x] CPU failure stops initialization.
- [x] Memory failure stops initialization.
- [x] Paging failure stops initialization.
- [x] Stack failure stops initialization.
- [x] Heap failure stops initialization.
- [x] ELF failure stops initialization.
- [x] Failed initialization does not report success.
- [x] Later subsystems are not invoked after failure.
- [x] Failure information is propagated according to the contract.

### Boundary Acceptance

- [x] No Stage 1 responsibility is duplicated.
- [x] No firmware dependency exists.
- [x] No Stage 3 dependency exists.
- [x] No Stage 4 behavioral dependency exists (only the declared symbol `jp_kernel_stage2_complete`).
- [x] Stage 2 establishes all state required at its exit boundary.
- [x] The transition to the Stage 2 completion continuation follows the defined contract.
- [x] Both entry stubs pass success and failure-path verification for both architectures.
- [x] `jp_kernel_stage2_failure()` is available without a Stage 3 dependency and is non-returning.

### Build Acceptance

- [x] Stage 2 compiles.
- [x] Stage 2 links with the expected kernel environment.
- [x] No unresolved Stage 2 symbols remain.
- [x] No undocumented dependency is required.
- [x] Architecture-specific compilation targets x86_64 and AArch64.
- [x] Functional acceptance passes separately for x86_64 and AArch64.
- [x] Verification does not rely solely on successful compilation.

**Record of the acceptance (Stage 2).** Ticked on 2026-11-21 after the architecture review and the commit `Stage 2 — Platform Initialization`, with this evidence: the unit tests of all seven modules (gcc and clang, also under AddressSanitizer and UBSan) and their mutation checks; the QEMU tests on x86_64 UEFI, x86_64 BIOS (both also under KVM), and AArch64 UEFI (also with `virtualization=on`), in two variants each: the normal run (success path of the entry stub) and the run in which the ELF check is replaced by a fake that fails (`stage2_failure`: failure path of the stub, `jp_kernel_stage2_failure`, and the failed result reported by the test kernel); a search of `src/kernel` and `include/justos/kernel` for firmware, Stage 3, Stage 4, `malloc`, `memcpy`, `memset`, `TODO`, and mutable global data (none). **Level M (real hardware) is not part of this acceptance:** nothing of Stage 2 has run on a real machine, and the project owner will run the whole milestone by hand under KVM and on hardware after the last stage.

---

## 2. Final Lock Statement

Stage 2 — Platform Initialization is considered **SPECIFICATION LOCKED** when:

    Specification Completeness Verification = PASSED
    Specification Traceability            = PASSED
    Specification Lock Decision           = APPROVED

The Stage 2 architecture is then frozen as the implementation contract.

After implementation:

    Implementation Acceptance = PASSED

is required before Stage 2 can be considered fully complete.

The final state is:

    STAGE 2
       ↓
    SPECIFICATION LOCKED
       ↓
    IMPLEMENTED
       ↓
    VERIFIED
       ↓
    ACCEPTED
       ↓
    CLOSED

Any later modification affecting an architecture-significant Stage 2 contract requires:

    specification update
          ↓
    affected verification
          ↓
    lock re-validation

Implementation must never silently redefine a locked architectural contract.

Stage 2 is therefore formally closed only after both the Specification Lock and Implementation Acceptance criteria have passed.

# Stage 3 — Boot Diagnostics & Recovery

## Stage 3 Definition

Stage 3 defines the diagnostics and recovery infrastructure that operates after Stage 2 has either completed successfully or failed, and before `kernel_main(context)`.

Stage 3 consumes the successful Stage 2 runtime state and the failure state recorded by the Stage 2 failure sink. It makes boot failures observable, classifies them deterministically, presents safe diagnostic output, and provides controlled recovery entry without becoming a general operating-system recovery subsystem.

Stage 3 does not repair unspecified Stage 1 or Stage 2 behavior. If an earlier stage reports an invalid or failed state, Stage 3 records and presents that state; it must not convert it into success.

The normative Stage 3 scope is:

```text
Stage 2 failure or kernel diagnostic event
        ↓
Boot event recording
        ↓
Error classification
        ↓
Diagnostic presentation
        ↓
Recovery decision
        ↓
Recovery entry or terminal halt
```

## 3 Specification Lock

Stage 3 is a Specification Lock. Before implementation begins, this stage must define its public interfaces, event and error contracts, ownership, lifetime, execution context, architecture behavior, failure semantics, recovery boundaries, verification requirements, and Stage 3 → Stage 4 contract.

Stage 3 must not introduce architecture-significant behavior that is absent from this specification.

## 3 Execution Context

Stage 3 has two entry modes.

Normal entry:

```text
Stage 2 success
    ↓
Stage 2 completion continuation
    ↓
Stage 3 initialization
    ↓
diagnostic infrastructure available
```

Failure entry:

```text
jp_kernel_stage2_failure(context, result)
    ↓
failure state preserved
    ↓
jp_kernel_stage2_complete(context)
    ↓
Stage 3 diagnostic/recovery entry
```

The failure entry must not depend on Stage 3 heap, scheduler, framebuffer, Stage 4, or user-space availability. It must preserve the original `JPBootResult` and context before attempting optional presentation or recovery.

Stage 3 executes in the environment established by Stage 2 before `kernel_main(context)`. In both entry modes Stage 3 may rely only on resources whose initialization is recorded as complete in the transferred `JPBootContext`; it must not assume heap, paging, stack, or ELF runtime state that Stage 2 may not have finished initializing. During early failure entry, it may execute in a degraded runtime where only the transferred context, static storage, architecture state, and non-graphical output path are available.

The Stage 3 runtime extension is a named `stage3` area of the transferred `JPBootContext`. Stage 3 may initialize and mutate only that area and explicitly assigned diagnostic fields; it must not add a second context or reinterpret Stage 1/Stage 2 fields. The `stage3` area contains the log storage, status, current-error presence state, recovery state, panic state, output-failure metadata, and report publication state defined by the Stage 3 owner contracts.

## 3 Diagnostic Architecture

```text
Boot Attempt Identity
        ↓
Boot Log
        ↓
Boot Status
        ↓
Error Classification
        ↓
Diagnostic Output Router
        ├── framebuffer output when valid
        └── text/serial/terminal fallback
        ↓
Panic or Recovery Decision
        ↓
Boot Report data for Stage 4
```

Stage 3 operates during both successful and failed boot paths. A successful path records initialization progress; a failed path records the original failure, classifies it, presents diagnostics, and selects recovery or terminal halt.

## 3 Boot Attempt and Event Identity

Every Stage 3 event belongs to a boot attempt.

The boot attempt state contains:

```text
boot attempt identity
attempt sequence
retry count
event sequence
```

The event sequence is a monotonic counter beginning at zero for each boot attempt. It is the authoritative event ordering mechanism; Stage 3 must not require RTC or wall-clock time for ordering.

A retry creates a new boot attempt sequence while preserving the previous attempt's diagnostic history when storage permits. The previous failure event remains immutable.

## 3 Boot Status Contract

The normalized boot status lifecycle is:

```text
BOOTING
    ↓
INITIALIZING
    ↓
RUNNING
    ↓
FAILED
    ↓
RECOVERY
    ↓
HALTED
```

Status ownership is explicit:

```text
Stage 1: BOOTING / INITIALIZING before handoff
Stage 2: INITIALIZING and FAILED for Stage 2 results
Stage 3: RUNNING, RECOVERY, and HALTED transitions
Stage 4: no transitions of its own; as orchestrator of Stage 3 entry it requests, with the owning stage's tag, the transitions that stage would request; it cannot rewrite terminal state
```

`HALTED` is terminal. `FAILED` may transition only to `RECOVERY` or `HALTED`. `RUNNING` may additionally transition to `RECOVERY` when recovery is requested through the Stage 4 boot configuration without a recorded failure. `RECOVERY` may transition to a new `BOOTING` attempt or `HALTED`.

Before `jp_boot_status_initialize()` no status exists and `jp_boot_status_get()` returns `JP_BOOT_NOT_INITIALIZED`. `jp_boot_status_initialize()` sets the status to `BOOTING`. A successful status must never be restored after a terminal failure in the same attempt.

Status is stored in the transferred diagnostic/runtime state defined by Stage 3 and remains queryable through the Stage 3 → Stage 4 boundary.

## 3 Boot Log Contract

The boot log uses bounded storage with a defined maximum event count and static or already-owned storage. It must not require heap allocation during failure entry.

Events are ordered by the per-attempt monotonic event sequence.

When storage is full, Stage 3 uses this deterministic policy:

```text
retain all existing FATAL and ERROR events;
retain the earliest events when equal severity competes;
discard lower-severity later events first;
set truncation = true;
record the number of discarded events.
```

The log never silently overwrites an ERROR or FATAL event. Log retrieval exposes attempt identity, event sequence, truncation state, and discarded-event count.

## 3 Error Classification Contract

Error processing follows:

```text
JPBootResult
    ↓
error normalization
    ↓
severity classification
    ↓
recoverability classification
    ↓
diagnostic presentation
    ↓
recovery decision
```

The original `JPBootResult` is immutable after creation. Stage 3 may append classification and context but may not replace the original result.

Default recoverability mapping is:

```text
JP_BOOT_ERROR_INVALID_PARAM       → NON_RECOVERABLE
JP_BOOT_ERROR_CORRUPTED_DATA      → NON_RECOVERABLE
JP_BOOT_FAILED                    → UNKNOWN
JP_BOOT_NOT_INITIALIZED           → RECOVERABLE
JP_BOOT_NOT_DETECTED              → RECOVERABLE
JP_BOOT_UNKNOWN                   → UNKNOWN
JP_BOOT_SUCCESS                   → not an error
```

The source component may refine `UNKNOWN` only through an explicitly defined classification rule. Error classification is idempotent within one boot attempt; reclassification must not mutate the original result or history.

## 3 Diagnostic Output Contract

Diagnostic output is routed through a normalized output contract:

```text
diagnostic message
        ↓
output router
        ├── framebuffer output if framebuffer state == valid
        └── non-graphical fallback
```

The router selects framebuffer output only when the Stage 1 framebuffer contract is valid. It may emit to framebuffer and fallback simultaneously when both are available. Output failure is recorded but must not recursively invoke panic presentation.

Diagnostic output must not require heap allocation, scheduler state, interrupts, user-space, or Stage 4. It must not mutate `JPBootContext` ownership, kernel mappings, stack state, or memory ownership.

`diagnostic_output.h` and `diagnostic_output.c` are the sole owners of sink selection and output routing. `boot_log`, `error`, `panic`, and `recovery` may request output through this interface but must not implement competing routing rules.

## 3 Panic Lifecycle

The panic lifecycle is:

```text
NORMAL
    ↓
PANIC_REQUESTED
    ↓
PANIC_ACTIVE
    ↓
DIAGNOSTIC_PRESENTED
    ↓
TERMINAL
```

If panic is requested while `PANIC_ACTIVE`, the subsystem must not restart the full presentation sequence. It preserves the first panic record, records recursive-panic occurrence if possible, and proceeds to the non-returning terminal path.

`TERMINAL` is non-returning and cannot be changed to `RUNNING`, `RECOVERY`, or success within the same panic attempt.

## 3 Retry Contract

Retry state is owned by Stage 3 recovery state and contains:

```text
retry_count
maximum_retry_count
current_attempt_identity
previous_attempt_identity
exhaustion state
```

The retry algorithm is:

```text
retry_count = 0
    ↓
evaluate retry decision
    ↓
increment retry_count before restart
    ↓
if retry_count < maximum: create new attempt
if retry_count >= maximum: TERMINAL_HALT
```

The retry count survives the transition to a new boot attempt through the defined recovery state. Diagnostic history from previous attempts remains preserved when storage permits. Exhaustion is terminal and must not silently wrap the counter.

A failure is exhausted when `retry_count > 0` and `retry_count >= maximum_retry_count`, that is, when the retry budget was consumed by actual retries. Milestone 0 has no storage that survives a platform reset, so a retry counter cannot persist across a restart. Therefore the configured `maximum_retry_count` in Milestone 0 is `0`: `RETRY_BOOT` is never selected (`retry_count < maximum_retry_count` is never true), no failure is classified as exhausted, and `ENTER_RECOVERY` remains available whenever a recovery destination is defined. A retry is performed manually by the operator by restarting the platform. The value becomes configurable only when a persistent retry state is specified by a later milestone.

## 3 Architecture Target

Stage 3 supports the same required architectures as Milestone 0:

```text
x86_64:  REQUIRED
AArch64: REQUIRED
```

Architecture-specific diagnostic output mechanisms may differ, but event semantics, error identity, ownership, recovery decisions, and observable failure behavior must remain equivalent.

## 3 Stage 3 Responsibilities

Stage 3 owns:

1. Boot event recording.
2. Boot progress reporting.
3. Error code and error-context normalization.
4. Failure-state preservation.
5. Diagnostic output routing.
6. Panic-screen or terminal diagnostic presentation.
7. Recovery trigger evaluation.
8. Recovery-mode entry.
9. Safe fallback behavior when graphics are unavailable.
10. Stage 3 event and error retention until the Stage 3 → Stage 4 boundary.
11. Validation that diagnostics never mutate protected kernel or handoff state.

## 3 Stage 3 Non-Goals

Stage 3 does not own:

- Stage 1 boot providers;
- kernel image acquisition or loading;
- ELF loading or relocation;
- physical-memory ownership;
- kernel paging construction;
- kernel heap allocation policy;
- scheduler initialization;
- user-space recovery;
- desktop services;
- Boot Manager UI implementation;
- package repair;
- filesystem repair;
- security trust evaluation;
- cryptographic authentication;
- AI services;
- Stage 4 Boot Manager integration.

Stage 3 may request the recovery transition owned by the Stage 4 Boot Manager, but it must not invent a new boot provider or rewrite the Stage 1/Stage 2 contracts.

## 3 Folder Structure

```text
include/justos/boot/
├── boot_log.h
├── boot_status.h
├── error.h
├── diagnostic_output.h
├── boot_report.h
├── panic.h
└── recovery.h

src/boot/
├── boot_log.c
├── boot_status.c
├── error.c
├── diagnostic_output.c
├── boot_report.c
├── panic.c
└── recovery.c
```

The architecture-specific output mechanism remains behind the normalized Stage 3 interfaces. If assembly is required for a terminal architecture path, its file and ABI must be specified before implementation.

## 3 Stage 3 Common Contract

Every Stage 3 component must:

- operate on explicit event/error state;
- preserve the original failure result and context;
- be deterministic for identical input state;
- avoid firmware access after the Stage 1 → Stage 2 transition;
- avoid changing kernel ownership state;
- avoid claiming success for failed initialization;
- support both x86_64 and AArch64 through architecture boundaries;
- provide a non-graphical fallback path;
- remain safe when the heap, scheduler, or user-space are unavailable.

## 3 Stage 3 Field and Value Semantics

Stage 3 uses `JPDiagnosticStage` for diagnostic records so that Stage 1/Stage 2 `JPBootStage` values are not extended by a later subsystem:

```c
typedef enum JPDiagnosticStage {
    JP_DIAGNOSTIC_STAGE_UNKNOWN = 0,
    JP_DIAGNOSTIC_STAGE_STAGE1,
    JP_DIAGNOSTIC_STAGE_STAGE2,
    JP_DIAGNOSTIC_STAGE_STAGE3,
    JP_DIAGNOSTIC_STAGE_STAGE4
} JPDiagnosticStage;
```

The following field meanings are normative for every Stage 3 record:

| Field                | Meaning                                                               | Validity and mutation rules                                                                                            |
| -------------------- | --------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------- |
| `boot_attempt_id`    | Stable identity of the current boot attempt                           | Non-zero after Stage 3 initialization; immutable for the attempt                                                       |
| `attempt_sequence`   | Monotonic ordinal of the attempt within the retained recovery history | Starts at zero for the first attempt; increments exactly once for every retry                                          |
| `event_sequence`     | Ordering key for events in one attempt                                | Starts at zero; increments only after an event is accepted; never reused                                               |
| `source_component`   | Stable identifier of the producer                                     | `JP_BOOT_SOURCE_UNKNOWN` means unknown; other values come from the Stage 3 source registry and are opaque to consumers |
| `diagnostic_context` | Opaque bounded diagnostic value supplied by the producer              | Zero means absent; it is not a pointer, length, or authorization token                                                 |
| `original_result`    | The first normalized `JPBootResult` causing the error                 | Required for failure records and immutable after creation                                                              |
| `architecture`       | Active `JPArchitectureType` inherited from Stage 1/Stage 2            | Must match the selected execution path; never inferred from output availability                                        |

`uint32_t` context fields must not encode addresses or require dereferencing. Unknown optional values use the documented zero/unknown value and never cause a successful operation to be claimed.

The Stage 3 source registry is a closed Milestone 0 vocabulary:

```c
typedef enum JPBootSourceComponent {
    JP_BOOT_SOURCE_UNKNOWN = 0,
    JP_BOOT_SOURCE_STAGE1,
    JP_BOOT_SOURCE_STAGE2,
    JP_BOOT_SOURCE_BOOT_LOG,
    JP_BOOT_SOURCE_BOOT_STATUS,
    JP_BOOT_SOURCE_ERROR,
    JP_BOOT_SOURCE_DIAGNOSTIC_OUTPUT,
    JP_BOOT_SOURCE_BOOT_REPORT,
    JP_BOOT_SOURCE_PANIC,
    JP_BOOT_SOURCE_RECOVERY,
    JP_BOOT_SOURCE_BOOT_CONFIG,
    JP_BOOT_SOURCE_BOOT_INFO,
    JP_BOOT_SOURCE_BOOT_UI,
    JP_BOOT_SOURCE_BOOT_MANAGER
} JPBootSourceComponent;
```

Values are stable within the Milestone 0 ABI. A later component must receive a new registry value rather than reusing an existing one. The last four values are registered for the Stage 4 components `boot_config`, `boot_info`, `boot_ui`, and `boot_manager`.

## 3 Stage 3 Algorithms and State Rules

The public operations use these normative algorithms:

```text
jp_boot_log_append:
    validate context, event identity, stage, architecture, and lifecycle
    compute the next candidate event_sequence without committing it
    if storage has capacity, append the event
    otherwise select the lowest-severity latest retained event for replacement
    never replace an ERROR or FATAL event with a lower severity
    if no legal replacement exists, discard the new event without advancing event_sequence
    increment discarded_event_count for every discarded event
    set truncated when any event is discarded
```

```text
jp_boot_status_update:
    validate the requested transition against the current status
    reject invalid transitions without mutation
    reject every transition out of HALTED
    commit status and stage atomically
    retain the original failure result when the status is FAILED, RECOVERY, or HALTED
```

```text
jp_boot_error_create:
    validate input and reject JP_BOOT_SUCCESS as an error
    preserve original_result
    apply the normative severity and recoverability mapping
    treat UNSPECIFIED overrides as absent
    convert specified override values to the base severity/recoverability enums
    accept an override only when it does not weaken a FATAL or NON_RECOVERABLE result
    commit exactly one immutable error record for the current attempt
```

```text
diagnostic_output_route:
    validate the message and current output state
    if the Stage 1 framebuffer contract is valid, attempt framebuffer output
    if framebuffer output is unavailable or fails, attempt the non-graphical fallback
    record output failure without invoking panic recursively
    return output success only when at least one permitted sink accepted the message
```

Framebuffer failure never invalidates the diagnostic record, changes ownership, or suppresses the fallback attempt. The fallback path must remain usable when heap, scheduler, user-space, and display hardware are unavailable.

```text
jp_boot_recovery_decide:
    read the immutable error and current recovery state
    return TERMINAL_HALT for non-recoverable or exhausted failures
    return RETRY_BOOT only when retry_count < maximum_retry_count
    otherwise return ENTER_RECOVERY when a recovery destination is defined (in Milestone 0, the Stage 4 recovery diagnostics state)
    otherwise return TERMINAL_HALT
```

All failed validation paths leave the relevant record and lifecycle state unchanged.

## 3 Retry Reset and Preservation Contract

When `RETRY_BOOT` is committed, state changes are exactly:

| State                     | Reset for new attempt                       | Preserved across retry                            |
| ------------------------- | ------------------------------------------- | ------------------------------------------------- |
| `boot_attempt_id`         | Yes; allocate a new non-zero identity       | Previous identity in `previous_attempt_id`        |
| `attempt_sequence`        | No; increment once                          | Entire retry sequence                             |
| `retry_count`             | No; increment before restart                | Current count and maximum                         |
| `event_sequence`          | Yes; set to zero for the new attempt        | Previous events in retained history               |
| Boot log                  | New-attempt append position                 | Prior events, truncation, discarded count         |
| Current error             | New attempt starts without a current error  | Prior error record as immutable history           |
| Boot status               | Set to `BOOTING`                            | Prior terminal/recovery result in Boot Report     |
| Panic state               | Set to `NORMAL` only if no panic was active | A panic-terminal attempt cannot be retried        |
| Recovery state            | Set to the new current identity             | Retry count, limit, exhaustion, previous identity |
| Stage 1/Stage 2 ownership | Never reset by Stage 3                      | All ownership and mappings unchanged              |

`RETRY_BOOT` is therefore forbidden after `PANIC_TERMINAL`, `HALTED`, or an exhausted retry counter. `ENTER_RECOVERY` preserves the current attempt and diagnostic records; `TERMINAL_HALT` preserves all records and performs no reset.

The retry transition is atomic from the perspective of all Stage 3 consumers:

```text
validate decision and retry limit
    ↓
reserve new boot_attempt_id
    ↓
increment retry_count and attempt_sequence
    ↓
preserve previous attempt records
    ↓
reset only the fields listed above
    ↓
publish BOOTING for the new attempt
```

If any step fails, the old attempt remains current, `retry_count` is unchanged, and no partial reset is visible. `boot_attempt_id` allocation must not reuse an identity retained in the diagnostic history.

## 3 Architecture Verification Matrix

Every Stage 3 implementation must produce the following observable results independently for both architectures:

| Operation          | x86_64 observable contract                                                        | AArch64 observable contract                                                                       |
| ------------------ | --------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------- |
| Normal entry       | Valid `JPBootContext`, `IF = 0` inherited/preserved, status reaches `RUNNING`     | Valid `JPBootContext`, EL1 and `PSTATE.DAIF = 1111` inherited/preserved, status reaches `RUNNING` |
| Failure entry      | Original result retained; no interrupt enable; fallback remains available         | Original result retained; no interrupt enable; fallback remains available                         |
| Event/status/error | Same identity, ordering, transition, and immutability semantics                   | Same identity, ordering, transition, and immutability semantics                                   |
| Framebuffer output | Uses only a valid Stage 1 framebuffer contract; fallback remains valid            | Uses only a valid Stage 1 framebuffer contract; fallback remains valid                            |
| Panic              | Preserves `IF = 0` and reaches non-returning terminal path                        | Preserves EL1 and `PSTATE.DAIF = 1111` and reaches non-returning terminal path                    |
| Recovery           | Uses the same decision and retry rules; does not mutate Stage 1/Stage 2 ownership | Uses the same decision and retry rules; does not mutate Stage 1/Stage 2 ownership                 |

An architecture-specific mechanism may differ internally, but a test must be able to observe the same record values, lifecycle result, ownership preservation, and non-returning behavior. “Equivalent” without an observable state or failure assertion is insufficient.

## 3 Stage 3 Verification Matrix

The following cases are mandatory for each of x86_64 and AArch64. A case passes only when the expected record/state result and the ownership assertion both hold.

| Case                      | Initial state / input                                   | Expected result                                                                 | Required assertion                                                                   |
| ------------------------- | ------------------------------------------------------- | ------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------ |
| Normal initialization     | Valid transferred context, no prior error               | `BOOTING → INITIALIZING → RUNNING`                                              | Context, mappings, stack, and ownership are unchanged except for Stage 3 metadata    |
| Stage 2 failure entry     | Valid context plus non-success `JPBootResult`           | Failure record created before output/recovery                                   | Original result and context remain immutable                                         |
| Event append              | Valid event and available capacity                      | Event accepted with next sequence                                               | Previous events and ownership unchanged                                              |
| Event overflow            | Full log with mixed severities                          | Lowest-severity latest event discarded or new event rejected by retention rules | `truncated` and discard count are correct; ERROR/FATAL records are not silently lost |
| Invalid status transition | Any transition not listed in lifecycle rules            | Operation fails without state change                                            | No status, result, or attempt field is mutated                                       |
| Error reclassification    | Same attempt and same original result                   | Existing error remains authoritative                                            | Original result and prior history are immutable                                      |
| Framebuffer unavailable   | Invalid/unavailable framebuffer                         | Non-graphical fallback is used                                                  | No framebuffer dereference and no Stage 1 ownership mutation                         |
| First panic               | Non-terminal failure and `PANIC_NORMAL`                 | Panic reaches non-returning terminal state                                      | First panic record is preserved and interrupts remain disabled                       |
| Recursive panic           | `PANIC_ACTIVE` or `PANIC_TERMINAL`                      | No second presentation sequence; terminal path continues                        | First panic record remains authoritative                                             |
| Retry permitted           | Recoverable failure, `retry_count < maximum`            | New attempt is published as `BOOTING`                                           | Reset/preserve table is satisfied atomically                                         |
| Retry exhausted           | `retry_count > 0` and `retry_count >= maximum`          | `TERMINAL_HALT`                                                                 | No new attempt and no counter wrap                                                   |
| Recovery entry            | Defined recovery destination and no retry decision      | `ENTER_RECOVERY`                                                                | Current diagnostic history and ownership remain intact                               |
| Recovery transition       | Accepted transition request                    | New Stage 1 attempt or defined recovery destination                             | Rejected requests are atomic; Stage 3 history and ownership are preserved            |
| Diagnostic finalization   | Consistent final status, log, error, and recovery state | Finalized read-only Boot Report                                                 | No partial publication; all records and counters become immutable                    |
| Stage 4 handoff           | Final report available                                  | Read-only report is exposed                                                     | Stage 4 cannot rewrite results, order, status, or ownership                          |

The matrix is an acceptance checklist for the Stage 3 specification. It does not claim implementation success until each case has an executable test for both architectures.

## 3 Boot Event Contract

A boot event contains, at minimum:

```text
event identifier
stage
severity
result
timestamp or monotonic sequence
architecture
source component
diagnostic context
```

Event recording must be bounded and deterministic. If storage is exhausted, the implementation preserves the earliest or highest-severity events according to a specified deterministic policy and reports truncation explicitly.

## 3 Error Contract

An error record contains:

```text
error code
stage
severity
source component
JPBootResult where applicable
architecture
recoverability
diagnostic context
```

Error identity is not redefined by Stage 3 when an existing Stage 1/Stage 2 `JPBootResult` is available. Stage 3 may add diagnostic classification, but the original result remains authoritative.

Severity is ordered:

```text
INFO < WARNING < ERROR < FATAL
```

Recoverability is explicit:

```text
RECOVERABLE
NON_RECOVERABLE
UNKNOWN
```

Severity vocabulary is intentionally owned by `error.h` because severity is part of error classification and recoverability policy. `boot_log.h` consumes the same vocabulary for event ordering and retention; it must not define a second severity enum.

`diagnostic_context` is intentionally an opaque bounded `uint32_t` for Milestone 0. Zero means absent; non-zero values are producer-defined diagnostic codes and are never pointers, addresses, lengths, or authorization tokens. A later diagnostic registry may assign stable meanings without changing the Stage 3 record ABI.

## 3 Panic and Diagnostic Presentation Contract

The panic subsystem presents a terminal diagnostic state when recovery cannot safely continue.

Common behavior:

```text
preserve failure context
disable further normal initialization
prevent recursive panic presentation
attempt diagnostic output
fall back to non-graphical output
terminate through the architecture-specific terminal path
```

x86_64 terminal behavior must preserve the defined x86_64 interrupt and CPU state. AArch64 terminal behavior must preserve the defined AArch64 exception level and `PSTATE.DAIF = 1111` state. Neither path may return as if the panic were handled successfully.

Diagnostic output may use the framebuffer only when the framebuffer contract is valid. Otherwise it must use the defined text/serial/terminal fallback without assuming a display device.

## 3 Recovery Contract

Recovery entry is a controlled transition, not an implicit retry loop.

Recovery decision inputs include:

```text
failure severity
recoverability
requested recovery mode
available recovery provider
repeated failure state
```

The decision result is one of:

```text
ENTER_RECOVERY
RETRY_BOOT
TERMINAL_HALT
```

`RETRY_BOOT` is permitted only when the recovery contract defines which state is reset and which state is preserved. Recovery must not retry indefinitely; retry count and exhaustion behavior must be explicit.

If no safe recovery path exists, Stage 3 enters `TERMINAL_HALT` and preserves the failure record.

`ENTER_RECOVERY` requested by the Stage 4 Boot Manager through the boot configuration is accepted without a recorded error only while the status is `RUNNING`; in every other state a recorded error is required.

## 3 Recovery Transition Boundary

Stage 3 owns recovery classification, decision, retry accounting, and diagnostic-state preservation. It does not own the boot provider, firmware transition, kernel image acquisition, or platform re-entry mechanism.

The external transition contract is:

```text
Stage 3 recovery decision
    ↓
Stage 4 Boot Manager: jp_boot_manager_request_recovery()
    ↓
new boot attempt (Stage 1 providers, then Stage 2) or defined recovery destination
```

The recovery-transition owner is the Stage 4 Boot Manager (`boot_manager.c`). Responsibility is divided as follows:

| Component | Responsibility |
| --------- | -------------- |
| Stage 3 `recovery.c` | decides the recovery action (`JPRecoveryDecision`) and preserves recovery state |
| Stage 4 `boot_manager.c` | accepts the recovery request and organizes the next boot attempt |
| Stage 1 providers | perform the platform-specific boot operations of the new attempt |
| Stage 2 | re-initializes the kernel environment during the new pass |

Stage 3 decides what must be done; the Boot Manager decides how the next boot attempt is organized.

The Boot Manager must provide one operation, declared in `include/justos/boot/boot_manager.h` and specified by Stage 4, with these semantics:

```text
request: JPBootContext + JPRecoveryDecision + preserved recovery state
accept:  consume the request exactly once and begin the defined destination
reject:  return failure without mutating Stage 3 diagnostic history or ownership
terminal: enter the Stage 3 terminal-halt path when no safe destination exists
```

The required contract has this form:

```c
typedef enum JPBootTransitionResult {
    JP_BOOT_TRANSITION_ACCEPTED,
    JP_BOOT_TRANSITION_REJECTED,
    JP_BOOT_TRANSITION_TERMINAL
} JPBootTransitionResult;

JPBootTransitionResult jp_boot_manager_request_recovery(
    JPBootContext *context,
    const JPBootRecoveryState *recovery
);
```

`JP_BOOT_TRANSITION_ACCEPTED` means the request was consumed exactly once and the defined destination began. `JP_BOOT_TRANSITION_REJECTED` means failure without mutation of Stage 3 diagnostic history or ownership. `JP_BOOT_TRANSITION_TERMINAL` means no safe destination exists and Stage 3 enters its terminal-halt path. `JPBootRecoveryState` carries the `JPRecoveryDecision` and the preserved retry state defined by `recovery.h`.

For `RETRY_BOOT`, the Boot Manager starts a new boot attempt using the preserved platform/provider contract, so Stage 1 and Stage 2 run again; it must not reinterpret the Stage 3 context as a new provider context. For `ENTER_RECOVERY`, it enters only a provider or destination defined by Stage 4. Stage 3 must not invent either path.

The transition interface is a cross-stage dependency. Its concrete types and owner-side behavior must be specified by Stage 4 (`boot_manager.h`) before Milestone 0 implementation acceptance; the type and function names above are binding. Stage 3 consumes this boundary and does not implement the owner-side transition. This call is the only permitted Stage 3 → Stage 4 link: it is a one-way request through a single declared symbol and does not make Stage 3 depend on Stage 4 UI, configuration, or presentation. Rejection or a terminal result never leaves Stage 3 without its non-graphical terminal path.

## 3 Ownership and Lifetime

Stage 3 owns its event, error, panic, and recovery metadata after those records are created. It does not take ownership of kernel image memory, page tables, stack, framebuffer, or physical-memory ownership state.

The Stage 2 failure sink transfers the same `JPBootContext *` and `JPBootResult` to Stage 3's failure path. Stage 3 may append diagnostic state but must not rewrite historical Stage 1 discovery or successful Stage 2 state.

Diagnostic records remain valid until the Stage 3 → Stage 4 boundary or terminal halt, whichever occurs first.

The Stage 3 ownership matrix is:

| State          | Producer              | Consumer                       | Owner                              | Lifetime                   | Mutation authority                         |
| -------------- | --------------------- | ------------------------------ | ---------------------------------- | -------------------------- | ------------------------------------------ |
| Boot event     | Stage 1/2/3 component | diagnostics, Stage 4           | Stage 3 log                        | attempt to Stage 4 or halt | log subsystem only                         |
| Boot status    | owning stage          | diagnostics, recovery, Stage 4 | Stage 3 status state after handoff | attempt lifetime           | owning stage by transition rules           |
| Error record   | error subsystem       | panic, recovery, Stage 4       | Stage 3 error state                | attempt to Stage 4 or halt | error subsystem; original result immutable |
| Panic state    | panic subsystem       | terminal path                  | Stage 3 panic state                | terminal path              | panic subsystem only                       |
| Recovery state | recovery subsystem    | recovery provider, Stage 4     | Stage 3 recovery state             | retry sequence or halt     | recovery subsystem only                    |

Stage 3 never owns or mutates Stage 1/Stage 2 physical memory, page tables, kernel stack, heap ownership, kernel image, or historical discovery state.

## 3 Stage 3 Execution Flow

```text
Stage 2 success
    ↓
Stage 2 completion continuation
    ↓
Stage 3 initialization
    ↓
diagnostics infrastructure
    ↓
Stage 4 continuation or recovery decision
```

For a Stage 2 failure:

```text
jp_kernel_stage2_failure(context, result)
    ↓
record failure
    ↓
jp_kernel_stage2_complete(context)
    ↓
diagnostic presentation if available
    ↓
recovery decision
    ↓
recovery entry or terminal halt
```

Stage 3 must not require Stage 4 or desktop services to present a failure safely.

Normal Stage 3 entry records the successful Stage 2 completion before beginning later diagnostic or Boot Report work. Failure entry records the failure before attempting output, panic, recovery, or any optional subsystem.

## 3 Boot Report Contract

Stage 3 does not own the Stage 4 Boot Manager UI, but it prepares the canonical diagnostic data consumed by Stage 4.

The retained Boot Report data contains:

```text
boot attempt identity
final boot status
Stage 1 result summary
Stage 2 result summary
Stage 3 result summary
ordered boot events
error records
recovery outcome
retry history
truncation state
architecture
```

The report is read-only at the Stage 3 → Stage 4 boundary. Stage 4 may format or present it, but must not rewrite original results, event order, ownership state, or terminal status.

## 3 Diagnostic Finalization Contract

Diagnostic finalization freezes the Stage 3 diagnostic state before Stage 4 consumption or terminal halt. It is a coordination operation over the existing owner modules; it does not create a second copy of event, error, status, panic, or recovery state.

`boot_report.h` and `boot_report.c` are the sole owners of finalization and Boot Report publication. The public finalization API is `jp_boot_report_finalize(context)`; `jp_boot_log_finalize(context)` only closes the log owner and is not the report finalization operation.

The finalization sequence is:

```text
validate current Stage 3 lifecycle and report destination
    ↓
record the final Stage 3 status and recovery outcome
    ↓
finalize the boot log
    ↓
freeze the current error and retry history
    ↓
assemble the canonical Boot Report view
    ↓
publish read-only report state
```

Finalization succeeds only when every required record is internally consistent. On failure, the existing diagnostic state remains mutable only for failure preservation and the report is not published as final. Finalization must not allocate, access firmware, enable interrupts, mutate Stage 1/Stage 2 ownership, or require Stage 4.

After successful finalization:

- `JPBootLogStatus.finalized` is `true`;
- no new event, error replacement, status rewrite, retry, or panic recovery is permitted;
- the final status, original results, event order, truncation state, recovery outcome, and retry history are immutable;
- Stage 4 receives a read-only Boot Report view;
- terminal halt may consume the same finalized view without returning to normal initialization.

Finalization is idempotent. Repeating it returns the existing finalized report and does not change sequence numbers, counters, ownership, or report contents. A failed finalization attempt does not partially publish final state.

## 3 Decided rules (what the first implementation needed that the text above leaves open)

This block was written when Stage 3 was audited against the finished Stage 1 and Stage 2. Each rule says what is decided and why. The decisions on the serial port, the font, the journal, and the stopping routine were taken together with the project owner; the others follow from the text and are listed so that a reader can disagree.

**Where Stage 3 runs, and who calls it.** Stage 1 runs in the boot application. Stage 2 and Stage 3 run inside the kernel, after the entry stub, so the Stage 3 files (`src/boot/boot_log.c` and the others of the folder list) are linked into the kernel image, not into the boot application. Stage 3 has no entry function of its own: it is a set of functions that the Stage 4 Boot Manager calls, in the order that "Stage 4 Execution Flow" (steps 3 to 7 of `jp_kernel_stage2_complete`) gives. Stage 3 never starts itself.

**The stopping routine.** Stage 4 says "stop the machine with the Stage 3 stopping routine" when the context is `NULL` or the log cannot start (nothing can be recorded). That routine is `jp_boot_recovery_enter(NULL, JP_RECOVERY_TERMINAL_HALT)`: with a `NULL` context and only this decision, the function writes nothing (there is nowhere to write) and stops the processor (x86_64: `cli`, then `hlt` in a loop; AArch64: mask all interrupts, then `wfi` in a loop). With a `NULL` context and any other decision it does the same (it never returns and never pretends that a recovery began). With a real context and `JP_RECOVERY_TERMINAL_HALT` it first finalizes the report as the specification says, and then stops the processor.

**The `stage3` area.** The context holds one area for Stage 3 (`JPBootContext.stage3`, type `JPBootStage3Area`). It replaces the temporary byte block that Stage 1 reserved. It contains, in this order: the log storage (`JPBootLogStorage`); the attempt state (`JPBootAttemptState`); the status state (`JPBootStatusState`) and a flag that says whether it exists; the current error (`JPBootError`) and a flag that says whether one was recorded; the panic state (`JPBootPanicState`) and the first panic record (a copy of the `JPBootError` that caused it); the recovery state (`JPBootRecoveryState`); the output state (below); and the report (`JPBootReport`). Why an area inside the context: the project forbids global variables and allocation, and the failure path may not use the heap. The types are declared by the Stage 3 headers; so that `JPBootContext` is one complete object, the declarations that the area needs are collected in one header, `include/justos/boot/diagnostic_types.h`, that `boot_types.h` includes (and that gives an error if it is included on its own); the Stage 3 headers declare the functions and include `boot_types.h`; the header `include/justos/boot/boot_manager.h` is created with only the recovery-transition boundary that Stage 3 needs (Stage 4 adds the rest) (the exact header split is free as long as every type and function name keeps its spelling). `jp_boot_log_initialize()` clears the whole area to zero before it does anything else. A second call to `jp_boot_log_initialize()` or `jp_boot_status_initialize()` is `JP_BOOT_FAILED` and changes nothing.

**Boot attempt identity.** There is no clock and no random number source in Milestone 0, so the identity of an attempt is a counter: the first attempt has `attempt_sequence` 0 and `boot_attempt_id` 1; every retry adds one to both. An identity is never reused because the counter only grows. `maximum_retry_count` is the constant `JP_BOOT_MAXIMUM_RETRY_COUNT` (defined with the Stage 3 types, so every Stage 3 header sees it, `recovery.h` among them) and its value is 0: Milestone 0 never retries (Stage 4 also says so). `jp_boot_log_initialize()` sets `boot_attempt_id` to 1, `attempt_sequence` to 0, `retry_count` to 0, and the maximum.

**What the log holds at the start.** Stage 1 and Stage 2 never wrote to a log (Stage 3 did not exist), and Stage 3 does not invent events. `jp_boot_log_initialize()` writes exactly one event, number 0, that is only a copy of facts that the context already holds about Stage 1: stage `JP_DIAGNOSTIC_STAGE_STAGE1`, source `JP_BOOT_SOURCE_STAGE1`, the architecture from `handoff.architecture`, and diagnostic context 0. If `context->current_stage` is `JP_BOOT_STAGE_READY` and `handoff.state` is `JP_HANDOFF_STATE_READY`, the severity is `INFO` and the result is `JP_BOOT_SUCCESS`; if not, the severity is `ERROR` and the result is `JP_BOOT_FAILED` (this cannot happen after a Stage 2 that checked its entry, but the record must tell the truth). Every other event is added by the caller (Stage 4 adds the Stage 2 result, as its flow says).

**Default severity.** The text gives a default recoverability for every result but not a default severity. The default severity is `FATAL` for a result that is `NON_RECOVERABLE` (`JP_BOOT_ERROR_INVALID_PARAM`, `JP_BOOT_ERROR_CORRUPTED_DATA`) and `ERROR` for every other result that is not `JP_BOOT_SUCCESS`. An override may raise a severity but never lower a `FATAL` one (the text already says so for `FATAL` and `NON_RECOVERABLE`).

**The status graph, exactly.** `jp_boot_status_update(context, stage, status)` allows only these changes (the text above names the ends; the arrows are the whole graph): `BOOTING` to `INITIALIZING`; `INITIALIZING` to `RUNNING` or `FAILED`; `RUNNING` to `FAILED` or `RECOVERY`; `FAILED` to `RECOVERY` or `HALTED`; `RECOVERY` to `BOOTING` (a new attempt) or `HALTED`. Everything else, including "change to the same status", is refused with `JP_BOOT_FAILED` and changes nothing. The `stage` argument is the *tag* (the label that says which stage the change is about) and must fit the change: `BOOTING` accepts `STAGE1` or `STAGE3`; `INITIALIZING` accepts `STAGE1` or `STAGE2`; `RUNNING`, `RECOVERY`, and `HALTED` accept `STAGE3`; `FAILED` accepts any of the four stages. A tag that does not fit, or `JP_DIAGNOSTIC_STAGE_UNKNOWN`, is `JP_BOOT_ERROR_INVALID_PARAM`. A change to `FAILED` needs a recorded error (otherwise `JP_BOOT_FAILED`: a failure is never made up) and copies the severity, recoverability, and original result of that error into the status. A status change after the report is final is `JP_BOOT_FAILED`.

**The log when it is full, exactly.** When 256 events are kept and a new event arrives: find the retained event with the lowest severity (and, among those, the latest one). That event may be replaced only if its severity is `INFO` or `WARNING` and is lower than the severity of the new event; then it is removed, the later events move one place down, the new event is added at the end with the next sequence number, and one discard is counted. In every other case the new event is dropped: the sequence number is not used, and one discard is counted. In both cases `truncated` becomes `true`. So an `ERROR` or `FATAL` event that is already in the log is never removed, even for a newer `FATAL` event (the new one is dropped and counted). `boot_attempt_id` and `event_sequence` of an event are always set by the log; what the caller puts there is ignored. An event whose `architecture` is not the one of `context->handoff.architecture`, or whose stage, severity, source, or result is outside its enumeration, is `JP_BOOT_ERROR_INVALID_PARAM`.

**The error record, exactly.** `jp_boot_error_create()` records at most one error for an attempt: the second call is `JP_BOOT_FAILED` and changes nothing (the first error stays the only one). An override that would weaken the default (a severity lower than `FATAL` for a `FATAL` default, or a recoverability other than `NON_RECOVERABLE` for a `NON_RECOVERABLE` default) makes the whole call `JP_BOOT_ERROR_INVALID_PARAM`. `jp_boot_error_clear()` works only while the status is `BOOTING` and the report is not final (that is, for a new attempt); otherwise `JP_BOOT_FAILED`.

**What `jp_boot_recovery_decide()` returns in Milestone 0.** The error must exist (otherwise `JP_BOOT_NOT_INITIALIZED`, except that a recovery asked for through the configuration is allowed without an error while the status is `RUNNING`, as the text says). Then, in this order: a `NON_RECOVERABLE` error or an exhausted retry counter gives `JP_RECOVERY_TERMINAL_HALT`; `retry_count < maximum_retry_count` gives `JP_RECOVERY_RETRY` (never in Milestone 0, because the maximum is 0); otherwise `JP_RECOVERY_ENTER`, because the destination (the Stage 4 recovery diagnostics state) always exists in Milestone 0. So a `RECOVERABLE` or `UNKNOWN` error (for example `JP_BOOT_FAILED` from Stage 2) gives `JP_RECOVERY_ENTER`, and a corrupted-data error gives `JP_RECOVERY_TERMINAL_HALT`.

**Where the text goes (diagnostic output).** `jp_boot_diagnostic_output()` sends each message to two places and succeeds if at least one accepted it.
1. The framebuffer, if it is *valid*: `framebuffer.state` is `JP_DETECTION_STATE_DETECTED`, `base_address` is not zero, `width`, `height`, and `pitch` are not zero, the format is `JP_FRAMEBUFFER_FORMAT_RGB_888` or `JP_FRAMEBUFFER_FORMAT_BGR_888` (32 bits per pixel), and `pitch` is at least `width * 4` and a multiple of 4 (so that every pixel is one aligned 32-bit store). Pixels are written at the framebuffer's address (Stage 2 keeps it mapped, uncached). Text is drawn with a built-in font of 8 by 16 pixels (ASCII 32 to 126; any other character is drawn as `?`), white on black. A screen line is 16 pixels high and as many as `width / 8` characters wide, at most 80. The output state counts the next free line, starting at 0; each message takes the next line, the part of the message that is longer than the line is not shown on the screen (the serial output still has all of it), and when no line is left the message is not shown on the screen and counts as "this sink refused it". Drawing one line first fills its 16 pixel rows with black, so old content never shows through.
2. The serial port, the fallback that needs no display. x86_64: the port COM1 (I/O address `0x3F8`), 115200 baud, 8 data bits, no parity, 1 stop bit (the same as the BIOS loader). AArch64: the PL011 UART at the address `0x09000000` (the one that QEMU's `virt` machine has; the AArch64 machine of Milestone 0 has no device discovery, so the address is fixed). The first message probes the port: x86_64 writes two different values (`0x5A`, then `0xA5`) to the scratch register (port `0x3F8 + 7`) and reads each one back; AArch64 reads the PL011 identification registers (each is a 32-bit register whose lowest byte counts): the byte at offset `0xFE0` must be `0x11`, at `0xFE4` `0x10`, at `0xFE8` must have `0x4` in its low four bits, and the four bytes at offsets `0xFF0`, `0xFF4`, `0xFF8`, `0xFFC` must be `0x0D 0xF0 0x05 0xB1`. A port that fails the probe is *not available* and the result is remembered in the output state (`serial_probed`, `serial_available`); an unavailable port never receives a byte and is never reported as having printed (Rule 30). A message is sent as its characters, a carriage return, and a line feed; a message longer than 255 characters is cut at 255.
If both sinks refuse, the result is `JP_BOOT_FAILED`; the failure is counted in the output state (`failed_messages`) and never starts a panic. The output state is part of the `stage3` area: `next_line` (32 bits), `serial_probed`, `serial_available` (booleans), `failed_messages` (32 bits).

**The serial port in detail.** The hardware functions are only three, in `diagnostic_hw.h`: `jp_diagnostic_serial_read(offset)` and `jp_diagnostic_serial_write(offset, value)` (x86_64: one byte at the port `0x3F8 + offset`; AArch64: one 32-bit register at the address `0x09000000 + offset`) and the stopping function. Everything else is logic in `diagnostic_output.c`, so the host tests can run it with a fake port. *x86_64, the first message:* program the port (interrupts off: `IER` 0; `LCR` `0x80` to open the divisor; divisor 1 in `DLL`/`DLM`, which is 115200 baud; `LCR` `0x03` for 8N1; `FCR` `0xC7` to clear and enable the FIFO; `MCR` `0x0B`), then run the scratch-register probe; the port is available only if both values read back. *AArch64:* the port is used as the firmware left it (the clock of the UART is not known, so the baud rate is not touched); the probe is the identification check. *Sending a byte:* x86_64 waits until bit 5 of `LSR` (register 5) says that the transmitter is empty, then writes register 0; AArch64 waits until bit 5 of `FR` (offset `0x18`) says that the transmit FIFO is not full, then writes `DR` (offset 0). The wait is bounded (`JP_DIAGNOSTIC_SERIAL_SPIN_LIMIT`, 100000 reads); a byte that could not be sent makes the serial sink refuse the message (the rest of the message is not sent). Characters outside 32 to 126 are sent as `?`, so a message can never send a control sequence.

**The panic screen.** `jp_boot_panic()` sends these lines, in this order, through `jp_boot_diagnostic_output()` (one message per line, the stage tag `JP_DIAGNOSTIC_STAGE_STAGE3`, the source `JP_BOOT_SOURCE_PANIC`): `JustOS boot failure`, `attempt: <boot_attempt_id in decimal>`, `stage: <STAGE1|STAGE2|STAGE3|STAGE4|UNKNOWN>`, `result: <the name of the JPBootResult, for example JP_BOOT_FAILED>`, `severity: <INFO|WARNING|ERROR|FATAL>`, `recoverability: <RECOVERABLE|NON_RECOVERABLE|UNKNOWN>`, `source: <the name of the source component without the JP_BOOT_SOURCE_ prefix>`, `context: 0x<diagnostic_context in 8 hexadecimal digits>`. A name that is not in the vocabulary is `UNKNOWN`. Then it stops the processor.

**Panic, in detail.** `jp_boot_panic(context, error)` does this, in order. (1) A `NULL` context, or a context whose Stage 3 log never started, cannot hold or show anything: the processor is stopped. (2) If the panic state is not `NORMAL` (a panic is already requested, active, presented, or terminal), this is a *recursive* panic: no line is shown again, the first panic record stays as it is, one `FATAL` event with `diagnostic_context` 1 is added to the log if the log can take it (a refusal is ignored), the state becomes `TERMINAL`, and the processor is stopped. (3) Otherwise this is the first panic: the state becomes `REQUESTED` and then `ACTIVE`; the error that is shown is the `error` argument, or, when it is `NULL`, the recorded error of the attempt, or, when there is none, an error made of the result `JP_BOOT_UNKNOWN`, stage `JP_DIAGNOSTIC_STAGE_UNKNOWN`, severity `FATAL`, recoverability `UNKNOWN` (a panic never invents a cause); a copy is kept as the first panic record (`first_panic`); one `FATAL` event (source `JP_BOOT_SOURCE_PANIC`, stage `JP_DIAGNOSTIC_STAGE_STAGE3`, the result and the diagnostic context of that error) is added to the log if it can take it; the eight lines of the panic screen are sent (a refused line is ignored); the state becomes `DIAGNOSTIC_PRESENTED` and then `TERMINAL`; the processor is stopped. Interrupts are never turned on. The name that a line shows for a `JPBootResult` is its C name without extra text, for example `JP_BOOT_FAILED`; for a stage it is `STAGE1` to `STAGE4` or `UNKNOWN`; for a severity `INFO`, `WARNING`, `ERROR`, `FATAL`; for a recoverability `RECOVERABLE`, `NON_RECOVERABLE`, `UNKNOWN`; for a source the C name without `JP_BOOT_SOURCE_` (`BOOT_MANAGER` and so on). The numbers in the lines are written without any library: `attempt` in decimal, `context` as `0x` and eight hexadecimal digits with capital letters (`0x00001234`).

**Recovery, in detail.** `jp_boot_recovery_decide()` has the answers above plus: a status `HALTED` or a panic state `TERMINAL` always gives `JP_RECOVERY_TERMINAL_HALT`; no recorded error gives `JP_BOOT_FAILED`, except in the status `RUNNING` where it gives `JP_RECOVERY_ENTER` (recovery asked for through the configuration); `JP_RECOVERY_RETRY` is given only when the status is `FAILED` or `RECOVERY` and the panic state is `NORMAL`. `jp_boot_recovery_enter(context, decision)` stores the decision in the recovery state and then: *`JP_RECOVERY_ENTER`*: the status becomes `RECOVERY` (tag `STAGE3`; it is skipped when the status already is `RECOVERY`), and `jp_boot_manager_request_recovery()` is called with the recovery state; if it answers `ACCEPTED`, the destination has begun and the processor is stopped here (Stage 3 has nothing more to do); if it answers `REJECTED` or `TERMINAL` (or anything else), the terminal path below is taken. *`JP_RECOVERY_RETRY`*: allowed only when the decision rules above would give it now; otherwise the terminal path. The new attempt is prepared as the reset table says (new identity = the old one plus 1, `attempt_sequence` and `retry_count` plus 1, `previous_attempt_id` = the old identity, the log keeps its events and restarts `event_sequence` at 0, one event is added that holds the error of the old attempt (so the history keeps it), the current error is removed, the status goes `RECOVERY` and then `BOOTING`, the panic state stays `NORMAL`), `jp_boot_manager_request_recovery()` is called, and if it answers anything but `ACCEPTED` every change is undone (the old attempt stays current, `retry_count` is as it was) and the terminal path is taken; if it answers `ACCEPTED` the processor is stopped here. *`JP_RECOVERY_TERMINAL_HALT`, an invalid decision, and the terminal path*: with a `NULL` context or a context whose log never started, the processor is stopped at once; otherwise the status becomes `HALTED` when the graph allows it (from `FAILED` or `RECOVERY`), the report is finalized, and then, if an error is recorded, `jp_boot_panic()` shows it and stops the processor; if no error is recorded the processor is stopped.

**The Boot Report, in detail.** `jp_boot_report_finalize()` first checks that the records agree: the recovery state and the error (if any) belong to the current attempt (the same `boot_attempt_id`); the status `FAILED` has a recorded error, and for `FAILED`, `RECOVERY`, and `HALTED` with a recorded error the status holds that error's original result; `retry_count` is not above `maximum_retry_count`; the log holds at most `JP_BOOT_LOG_CAPACITY` events and the same number as its status says. If a check fails the result is `JP_BOOT_ERROR_CORRUPTED_DATA` and nothing is published. Then it sets `recovery.exhausted` (true when `retry_count > 0` and `retry_count >= maximum_retry_count`), closes the log, builds the report from copies (when no error is recorded, the `error` of the report is all zero: a real error never has the original result `JP_BOOT_SUCCESS`, so a zero error means "none"), and only then sets `finalized` in the report. A second call changes nothing and returns `JP_BOOT_SUCCESS`. `jp_boot_report_get()` returns `JP_BOOT_NOT_INITIALIZED` until the report is final.

**The machine access of Stage 3.** A C program cannot write a port, read a UART register, or stop the processor, and the host tests cannot run such instructions. So Stage 3 has its own small private files, in the same style as the private machine-access files of Stage 2: `src/boot/diagnostic_hw.h` (declares: write one byte to the serial port, read one byte of the serial port registers for the probe, stop the processor), `src/boot/diagnostic_hw_x86_64.c`, and `src/boot/diagnostic_hw_aarch64.c` (a few instructions each, no logic). The font is `src/boot/diagnostic_font.h` and `src/boot/diagnostic_font.c` (the data of the 95 characters; the file is generated by `scripts/gen_font.py`, as the font of the test kernel is). These files are in addition to the folder list above.

**How the QEMU tests run Stage 3 before Stage 4 exists.** The test kernel links the real Stage 3 files (and the Stage 2 files). The stand-in `jp_kernel_stage2_complete()` (`tests/qemu/test_kernel/fake_stage2_complete.c`) does the Stage 3 part of the Stage 4 flow (start the log and the status, record the Stage 2 result, set the status, finalize the report, go to `kernel_main`; after a Stage 2 failure it creates the error, decides, and calls `jp_boot_recovery_enter()`), and the stand-in `jp_boot_manager_request_recovery()` (`fake_boot_manager.c`) plays the recovery destination (it finalizes the report and goes to `kernel_main`). Both send lines of the form `JP|stage3.<key>=<value>` through `jp_boot_diagnostic_output()`, the real output, so a line that is missing from the serial log means that the real output failed. The keys are `output` (`OK`), `status` (the status name), `attempt` (the attempt identity), `events` (the number of log events), `error` (the name of the original result without `JP_BOOT_`, or `NONE`), and `report` (`FINAL` or `NOT_FINAL`). The expected values are in `tests/qemu/expected/<platform>.txt` and `<platform>.stage2_failure.txt`. The kernel (and the test kernel) is built with `-O2` and, on x86_64, `-mcmodel=kernel` (a kernel in the upper half of the address space needs it); no Stage 2 or Stage 3 code may need `memcpy`, `memset`, or any other library function (a structure assignment that is large enough makes the compiler call `memcpy`, so large records are copied field by field).

**What Stage 3 needs from Stage 4 before Stage 4 exists.** `jp_boot_recovery_enter()` ends in `jp_boot_manager_request_recovery()` (the only link to Stage 4). Until Stage 4 is written, the unit tests of `recovery` use a fake of that function, and the QEMU tests of Stage 3 use a stand-in in the test kernel (listed with the other stand-ins that are removed after Stage 4).

## 3 File Specifications

Each Stage 3 file follows the same structure as Stage 1 and Stage 2 file specifications: Purpose, Responsibilities, Types/Structures, Constants/Enums, Public API, Internal API, Algorithm, State/Lifecycle, Ownership/Lifetime, Invariants, Dependencies, Boundary Rules, Failure Semantics, Implementation Freedom, and Verification.

# 1. `include/justos/boot/boot_log.h`

## File

Public bounded boot-log header contract.

## Purpose

Public bounded boot-log contract.

## Responsibilities

Define event types, severity, stage, source, architecture, result, diagnostic context, bounded-storage state, and truncation state.

## Types / Structures

```c
typedef struct JPBootEvent {
    uint64_t boot_attempt_id;
    uint64_t event_sequence;
    JPDiagnosticStage stage;
    JPBootSeverity severity;
    JPArchitectureType architecture;
    JPBootSourceComponent source_component;
    JPBootResult result;
    uint32_t diagnostic_context;
} JPBootEvent;

typedef struct JPBootLogStatus {
    size_t event_count;
    size_t discarded_event_count;
    bool truncated;
    bool finalized;
} JPBootLogStatus;

#define JP_BOOT_LOG_CAPACITY 256

typedef struct JPBootLogStorage {
    JPBootEvent events[JP_BOOT_LOG_CAPACITY];
    JPBootLogStatus status;
    uint64_t next_event_sequence;
} JPBootLogStorage;
```

The Stage 3 runtime state embedded in `JPBootContext` contains one `JPBootLogStorage` owned by `boot_log.c`. The event sequence is the authoritative order; timestamps are optional diagnostic metadata and never determine ordering.

## Constants / Enums

`JP_BOOT_LOG_CAPACITY` is 256 retained events per boot-attempt log. The storage is a fixed-capacity array of `JPBootEvent` records supplied by Stage 3 runtime state; no allocation is permitted during failure entry. `boot_log.h` consumes `JPBootSeverity` from `error.h` and does not duplicate that enum.

## Public API

```c
JPBootResult jp_boot_log_initialize(JPBootContext *context);
JPBootResult jp_boot_log_append(JPBootContext *context, const JPBootEvent *event);
JPBootResult jp_boot_log_get(const JPBootContext *context, size_t index, JPBootEvent *event);
JPBootResult jp_boot_log_get_status(const JPBootContext *context, JPBootLogStatus *status);
JPBootResult jp_boot_log_finalize(JPBootContext *context);
```

The API must preserve event order, report truncation explicitly, and never mutate Stage 1/Stage 2 ownership state.

## Internal API

The implementation may use private validation, severity-ranking, retention-selection, and storage-access helpers. They are not callable outside `boot_log.c` and must not expose the storage address or mutate caller-owned event records.

## Algorithm

Initialization clears the embedded `JPBootLogStorage`, including all records and counters. Append validates the event, assigns the next sequence only after acceptance, and applies the Stage 3 retention policy when capacity is full. Retrieval checks bounds and copies a retained record without changing log state. Finalization prevents later append operations and leaves all retained records readable.

## State / Lifecycle

`UNINITIALIZED → ACTIVE → FINALIZED` is the only log lifecycle. Append is valid only in `ACTIVE`; initialize is valid only before the first active state; finalize is idempotent in `FINALIZED`. A failed operation does not advance sequence, count, or lifecycle state.

## Ownership / Lifetime

The log subsystem owns the fixed event array and status counters from initialization until terminal halt or the Stage 3 → Stage 4 boundary. Producers temporarily lend event values to append; they retain no ownership of stored copies.

## Invariants

`event_count <= JP_BOOT_LOG_CAPACITY`; retained sequences are unique and ordered; `discarded_event_count` never decreases; `truncated == true` exactly when at least one event was discarded; finalized logs are immutable.

## Dependencies

`boot_log.h` depends on `boot_types.h` for `JPBootContext`, `JPBootResult`, and `JPArchitectureType`, on the Stage 3 diagnostic types for stage/source/severity, and on `error.h` for `JPBootSeverity`. It owns the embedded `JPBootLogStorage` but not Stage 1/Stage 2 memory ownership.

`JPBootContext` contains a Stage 3-owned `stage3` runtime area. That area contains exactly one `JPBootLogStorage`, one status record, one current error record with presence state, one recovery state, one panic state, and one report publication state. The area is initialized once at Stage 3 entry and remains embedded in the same context across normal entry, failure entry, retry, finalization, and Stage 4 handoff.

## Boundary Rules

The module consumes only the transferred `JPBootContext` and Stage 3 storage. It must not access firmware, allocate heap memory, enable interrupts, rewrite handoff state, or depend on Stage 4.

## Failure Semantics

Public operations return a failing `JPBootResult` for an invalid context, an invalid event, unavailable storage, or a lifecycle-invalid call. A failed operation leaves retained records, counters, event sequence, and lifecycle state unchanged, and never mutates Stage 1/Stage 2 ownership state.

## Implementation Freedom

Ring, packed-array, or indexed-array storage is permitted only if retrieval order, retention policy, counters, lifecycle, and bounded-storage behavior remain identical.

## Verification

Verify initialization, append ordering, exact capacity behavior, mixed-severity overflow, retrieval bounds, finalization, and failed-operation atomicity.

# 2. `src/boot/boot_log.c`

## File

Bounded boot-log implementation.

## Purpose

Implement `boot_log.h`.

## Responsibilities

Validate and persist bounded events without changing the public header contract.

## Types / Structures

No additional public types. Private storage and helper state must implement the `JPBootEvent` and `JPBootLogStatus` contract.

## Constants / Enums

Consumes `JP_BOOT_LOG_CAPACITY` and the severity ordering defined by the owner headers.

## Public API

Implements exactly the public operations declared by `boot_log.h`.

## Internal API

Private helpers may validate event fields, rank severity, select a replacement, and update counters. No helper may bypass lifecycle validation.

## Algorithm

Use the bounded retention algorithm defined by Stage 3. A lower-severity later event is discarded before an earlier equal-severity event; an ERROR or FATAL record is never silently replaced by a lower severity.

## State / Lifecycle

Preserve `ACTIVE` state after accepted or discarded append; transition to `FINALIZED` only after successful finalize.

## Ownership / Lifetime

Own the storage and counters; borrow the input event only for the duration of the call.

## Invariants

Preserve sequence order and all status-counter invariants from `boot_log.h`.

## Dependencies

`boot_log.h` and the Stage 3 diagnostic storage supplied through `JPBootContext`; no heap, scheduler, firmware, or Stage 4 dependency is permitted.

## Boundary Rules

No firmware, heap, scheduler, user-space, Stage 4, or ownership mutation.

## Failure Semantics

Invalid context/event, unavailable storage, and invalid lifecycle transitions fail without corrupting existing records.

## Implementation Freedom

Helper decomposition and physical placement of the fixed array are free when the public retention and retrieval contract is unchanged.

The module owns bounded event storage, preserves the specified retention policy when full, rejects invalid event state, and reports truncation explicitly. It must work before heap/scheduler/user-space availability and provide equivalent x86_64/AArch64 behavior.

## Verification

Verify valid append/retrieval, ordering, truncation, invalid event, full storage, both architectures, and ownership preservation.

# 3. `include/justos/boot/boot_status.h`

## File

Normalized boot-status header contract.

## Purpose

Normalized boot progress and terminal status.

## Responsibilities

Define the status vocabulary, attempt state, transition contract, and read-only status query.

## Types / Structures

```c
typedef enum JPBootStatus {
    JP_BOOT_STATUS_BOOTING = 0,
    JP_BOOT_STATUS_INITIALIZING,
    JP_BOOT_STATUS_RUNNING,
    JP_BOOT_STATUS_FAILED,
    JP_BOOT_STATUS_RECOVERY,
    JP_BOOT_STATUS_HALTED
} JPBootStatus;

typedef struct JPBootAttemptState {
    uint64_t boot_attempt_id;
    uint64_t attempt_sequence;
    uint32_t retry_count;
    uint32_t maximum_retry_count;
} JPBootAttemptState;

typedef struct JPBootStatusState {
    JPDiagnosticStage stage;
    JPBootStatus status;
    JPBootSeverity severity;
    JPBootRecoverability recoverability;
    JPBootResult original_result;
    JPArchitectureType architecture;
} JPBootStatusState;
```

`boot_status.h` consumes the severity and recoverability enums defined by `error.h`; it does not redefine them.

`HALTED` is terminal. Status transitions are validated by `boot_status.c` according to the Stage 3 lifecycle contract.

## Constants / Enums

The status enum above is the sole status vocabulary for Stage 3. Transition legality and terminal-state rules are defined by the Boot Status Contract.

## Public API

```c
JPBootResult jp_boot_status_initialize(JPBootContext *context);
JPBootResult jp_boot_status_update(JPBootContext *context, JPDiagnosticStage stage, JPBootStatus status);
JPBootResult jp_boot_status_get(const JPBootContext *context, JPBootStatusState *status);
```

The contract represents stage, lifecycle status, severity, recoverability, architecture, and the original `JPBootResult` where applicable. It must not redefine Stage 1/Stage 2 result values.

## Internal API

Private transition validation and state-copy helpers may be used only by `boot_status.c`. They must not expose mutable status storage to Stage 4 or other producers.

## Algorithm

Validate the requested edge against the lifecycle graph, reject invalid or terminal-source transitions, then commit stage and status together. When entering `FAILED`, preserve the supplied original result; later `RECOVERY` and `HALTED` transitions cannot overwrite it.

## State / Lifecycle

The state graph is `BOOTING → INITIALIZING → RUNNING`; `INITIALIZING → FAILED`; `RUNNING → FAILED`; `RUNNING → RECOVERY` (only when recovery is requested through the Stage 4 boot configuration without a recorded failure); `FAILED → RECOVERY|HALTED`; `RECOVERY → BOOTING|HALTED`. `HALTED` has no outgoing edge.

## Ownership / Lifetime

The owning stage may request an allowed transition; `boot_status.c` owns the committed Stage 3 status state until the report boundary. Stage 4 receives a read-only view.

## Invariants

No invalid edge is observable; status and stage are mutually consistent; terminal status is never overwritten; the original failure result is immutable after failure.

## Dependencies

`boot_status.h` depends on `boot_types.h` and consumes `JPBootSeverity` and `JPBootRecoverability` from `error.h`. It does not redefine Stage 1 or Stage 2 `JPBootResult` values.

## Boundary Rules

Status updates cannot access firmware, change interrupts, mutate memory ownership, or infer architecture from output state.

## Failure Semantics

`jp_boot_status_update()` fails for any transition outside the defined state graph. A terminal `FAILED` or `HALTED` status is never overwritten by a success status. `jp_boot_status_get()` returns `JP_BOOT_NOT_INITIALIZED` before `jp_boot_status_initialize()` and is otherwise deterministic with no side effects. No failure path accesses firmware or mutates protected state.

## Implementation Freedom

Graph representation, locking mechanism, and private storage layout are free if transitions are atomic and the public graph remains exact.

## Verification

Verify every allowed edge, every rejected edge, terminal immutability, original-result preservation, retry re-entry, and architecture mismatch rejection.

# 4. `src/boot/boot_status.c`

## File

Boot-status implementation.

## Purpose

Implement status transitions and validation.

## Responsibilities

Validate and atomically publish status transitions.

## Types / Structures

No additional public types. Private transition state must implement `JPBootStatusState`.

## Constants / Enums

Consumes the `JPBootStatus` lifecycle vocabulary defined by `boot_status.h`.

## Public API

Implements exactly the public operations declared by `boot_status.h`.

## Internal API

Private transition-table and atomic-commit helpers only.

## Algorithm

Validate first, stage a complete replacement state, and publish it only after all checks pass. Failed validation leaves the prior state byte-for-byte unchanged.

## State / Lifecycle

Implement the exact graph defined by `boot_status.h`; never synthesize a success state from a failed result.

## Ownership / Lifetime

Own the mutable status record; return copies to callers and retain it through the report boundary.

## Invariants

Preserve lifecycle, result, architecture, and ownership invariants on both success and failure paths.

## Dependencies

`boot_status.h`, the transferred Stage 3 status state, and the status/error contracts; it must not access firmware or mutate protected Stage 1/Stage 2 state.

## Boundary Rules

No firmware or Stage 4 dependency; no writes to Stage 1/Stage 2 state.

## Failure Semantics

Invalid transitions fail, terminal failure state cannot be overwritten by success, and status queries are deterministic without firmware access or protected-state mutation.

## Implementation Freedom

Transition table representation and atomic publication primitive are implementation-defined within the observable contract.

## Verification

Verify valid transitions, invalid transitions, terminal state preservation, architecture mismatch, and failure propagation.

# 5. `include/justos/boot/error.h`

## File

Normalized error-record header contract.

## Purpose

Normalized error records.

## Responsibilities

Define error input, immutable error records, severity, recoverability, and classification rules.

## Types / Structures

```c
typedef enum JPBootSeverity {
    JP_BOOT_SEVERITY_INFO = 0,
    JP_BOOT_SEVERITY_WARNING,
    JP_BOOT_SEVERITY_ERROR,
    JP_BOOT_SEVERITY_FATAL
} JPBootSeverity;

typedef enum JPBootRecoverability {
    JP_BOOT_RECOVERABILITY_RECOVERABLE = 0,
    JP_BOOT_RECOVERABILITY_NON_RECOVERABLE,
    JP_BOOT_RECOVERABILITY_UNKNOWN
} JPBootRecoverability;

typedef enum JPBootSeverityOverride {
    JP_BOOT_SEVERITY_OVERRIDE_UNSPECIFIED = 0,
    JP_BOOT_SEVERITY_OVERRIDE_INFO,
    JP_BOOT_SEVERITY_OVERRIDE_WARNING,
    JP_BOOT_SEVERITY_OVERRIDE_ERROR,
    JP_BOOT_SEVERITY_OVERRIDE_FATAL
} JPBootSeverityOverride;

typedef enum JPBootRecoverabilityOverride {
    JP_BOOT_RECOVERABILITY_OVERRIDE_UNSPECIFIED = 0,
    JP_BOOT_RECOVERABILITY_OVERRIDE_RECOVERABLE,
    JP_BOOT_RECOVERABILITY_OVERRIDE_NON_RECOVERABLE,
    JP_BOOT_RECOVERABILITY_OVERRIDE_UNKNOWN
} JPBootRecoverabilityOverride;

typedef struct JPBootErrorInput {
    JPBootResult original_result;
    JPBootSourceComponent source_component;
    JPDiagnosticStage stage;
    JPBootSeverityOverride severity_override;
    JPBootRecoverabilityOverride recoverability_override;
    uint32_t diagnostic_context;
} JPBootErrorInput;

typedef struct JPBootError {
    uint64_t boot_attempt_id;
    JPBootResult original_result;
    JPDiagnosticStage stage;
    JPBootSeverity severity;
    JPBootRecoverability recoverability;
    JPArchitectureType architecture;
    JPBootSourceComponent source_component;
    uint32_t diagnostic_context;
} JPBootError;
```

`original_result` is immutable after creation. `*_OVERRIDE_UNSPECIFIED` means that the default mapping is used. A specified override is valid only when the classification contract permits it; it must not replace the original result.

## Constants / Enums

The severity and recoverability enums above are owned by `error.h`. Classification defaults and override validity are defined by the Error Classification Contract.

## Public API

```c
JPBootResult jp_boot_error_create(JPBootContext *context, const JPBootErrorInput *input);
JPBootResult jp_boot_error_get(const JPBootContext *context, JPBootError *error);
JPBootResult jp_boot_error_clear(JPBootContext *context);
```

The contract represents severity, recoverability, original result, source, stage, architecture, and diagnostic context. It distinguishes validation, trust, and authentication and makes no cryptographic-authenticity claim.

## Internal API

Private mapping, override-validation, and immutable-record commit helpers may be used only by `error.c`. No caller may mutate a committed `JPBootError` through an internal pointer.

## Algorithm

Reject null or malformed input and `JP_BOOT_SUCCESS`; preserve the original result; apply the default severity/recoverability mapping; validate overrides against fatal/non-recoverable protections; then commit one immutable record.

## State / Lifecycle

`EMPTY → RECORDED` is the error lifecycle for one attempt. A repeated create with the same attempt and original result is idempotent and returns the existing record; a different original result is rejected while `RECORDED`.

## Ownership / Lifetime

The error subsystem owns the committed record until terminal halt or the Stage 3 → Stage 4 boundary. Input context and classification values are copied, not retained by pointer.

## Invariants

`original_result` never changes; `JP_BOOT_SUCCESS` is never recorded as an error; severity and recoverability remain within their enums; architecture matches the active context.

## Dependencies

`error.h` depends on `boot_types.h` for context, stage, architecture, and result types. It is the owner of Stage 3 error classification types consumed by the log, status, panic, and recovery interfaces.

## Boundary Rules

Classification is diagnostic only: it does not authenticate data, rewrite results, evaluate trust, or mutate protected Stage 1/Stage 2 state.

## Failure Semantics

`jp_boot_error_create()` fails for malformed error context and for a different original result while the record is `RECORDED`; a repeated create with the same attempt and original result returns the existing record. The original `JPBootResult` is preserved and never converted into success. `jp_boot_error_get()` never fabricates a record when none has been recorded.

## Implementation Freedom

Lookup tables or switch-based classification are both permitted if mapping, override restrictions, idempotence, and immutable storage are identical.

## Verification

Verify every default mapping, valid and invalid overrides, duplicate creation, result immutability, malformed input, and architecture mismatch.

# 6. `src/boot/error.c`

## File

Error-classification implementation.

## Purpose

Implement deterministic error classification and record creation.

## Responsibilities

Normalize results, classify severity/recoverability, and commit immutable error records.

## Types / Structures

No additional public types. Private state must implement `JPBootError` and its attempt-local lifecycle.

## Constants / Enums

Consumes the default result mapping and severity/recoverability values defined by `error.h`.

## Public API

Implements exactly the public operations declared by `error.h`.

## Internal API

Private classifiers and record-commit helpers only; no public operation may expose mutable record storage.

## Algorithm

Execute the validation, mapping, override, idempotence, and commit sequence from `error.h`. On failure, return without modifying the current record or history.

## State / Lifecycle

Maintain `EMPTY` or `RECORDED`; `clear` returns to `EMPTY` only for the current attempt and never erases retained historical records.

## Ownership / Lifetime

Own the current error record and its copied context; borrow input only during the call.

## Invariants

Preserve original result, attempt identity, architecture, and classification validity.

## Dependencies

`error.h`, the immutable input `JPBootResult`, and the Stage 3 attempt state; classification must remain independent of Stage 4 and cryptographic services.

## Boundary Rules

No Stage 4, crypto, firmware, heap, or ownership-state dependency.

## Failure Semantics

Preserve the original `JPBootResult`, reject malformed error context, and never convert an underlying failure into success.

## Implementation Freedom

Classifier implementation and private record layout are free within the immutable public semantics.

## Verification

Verify valid errors, malformed context, severity/recoverability classification, result preservation, and both architectures.

# 7. `include/justos/boot/panic.h`

## File

Terminal-diagnostic header contract.

## Purpose

Public terminal-diagnostic interface.

## Responsibilities

Define panic lifecycle, recursive-panic behavior, diagnostic presentation, and terminal entry.

## Types / Structures

```c
typedef enum JPBootPanicState {
    JP_BOOT_PANIC_NORMAL = 0,
    JP_BOOT_PANIC_REQUESTED,
    JP_BOOT_PANIC_ACTIVE,
    JP_BOOT_PANIC_DIAGNOSTIC_PRESENTED,
    JP_BOOT_PANIC_TERMINAL
} JPBootPanicState;
```

The first panic record is authoritative. A recursive panic must not restart the full presentation sequence.

## Constants / Enums

`JPBootPanicState` defines the complete panic lifecycle. `PANIC_TERMINAL` is non-returning.

## Public API

```c
_Noreturn void jp_boot_panic(JPBootContext *context, const JPBootError *error);
```

The contract supports panic entry, recursive-panic prevention, diagnostic presentation, and non-returning terminal halt. x86_64 preserves `IF = 0`; AArch64 preserves `PSTATE.DAIF = 1111`.

## Internal API

Private recursion-guard, output-attempt, fallback, and terminal-path helpers may be used only by `panic.c`. The terminal helper is architecture-specific and never returns.

## Algorithm

Atomically move `NORMAL → REQUESTED → ACTIVE`; preserve the first error; attempt valid framebuffer output; emit non-graphical fallback regardless of framebuffer failure; move to `DIAGNOSTIC_PRESENTED`; enter `TERMINAL`. If already `ACTIVE` or terminal, skip presentation and enter the terminal path without replacing the first record.

## State / Lifecycle

`NORMAL → REQUESTED → ACTIVE → DIAGNOSTIC_PRESENTED → TERMINAL` is the only normal path. Recursive entry from `ACTIVE`, `DIAGNOSTIC_PRESENTED`, or `TERMINAL` cannot create a second panic record.

## Ownership / Lifetime

Panic state and the first error reference remain owned by the panic subsystem until terminal halt. Framebuffer memory remains owned by its existing Stage 1 contract.

## Invariants

Panic is non-returning; first-error identity is immutable; no panic path calls `kernel_main`, enables interrupts, or converts failure to success.

## Dependencies

`panic.h` depends on `boot_types.h` and `error.h`. It may consume the framebuffer contract for output validation but does not own framebuffer memory or Stage 4 presentation.

## Boundary Rules

The module may use only validated output state and architecture terminal primitives; it must not require heap, scheduler, user-space, Stage 4, or firmware.

## Failure Semantics

`jp_boot_panic()` never returns. Recursive entry does not create a second panic record or restart presentation. Output failure is recorded and does not recursively invoke panic. The non-graphical fallback is always attempted, and the original failure state is preserved.

## Implementation Freedom

Text layout, framebuffer rendering, and fallback encoding are free if recursion, first-error preservation, interrupt state, and non-returning terminal behavior remain observable.

## Verification

Verify first panic, recursion from every active state, framebuffer success/failure, fallback output, first-error preservation, interrupt state, and non-returning behavior on both architectures.

# 8. `src/boot/panic.c`

## File

Terminal-diagnostic implementation.

## Purpose

Implement terminal diagnostic presentation and halt.

## Responsibilities

Present the first panic record safely and enter the architecture-specific non-returning terminal path.

## Types / Structures

No additional public types. Private state must implement `JPBootPanicState` and the first-error reference.

## Constants / Enums

Consumes the panic lifecycle enum and terminal-state rules defined by `panic.h`.

## Public API

Implements exactly the public operation declared by `panic.h`.

## Internal API

Private state-guard, renderer, fallback, and architecture-terminal helpers only.

## Algorithm

Use the panic lifecycle and output order defined by `panic.h`; recursion bypasses rendering and proceeds directly to the terminal primitive.

## State / Lifecycle

Publish each state before executing the next externally observable action; never publish `RUNNING` or `RECOVERY` after panic activation.

## Ownership / Lifetime

Preserve the first error and panic state; borrow framebuffer access without taking framebuffer ownership.

## Invariants

Terminal path is non-returning and preserves architecture interrupt/exception state.

## Dependencies

`panic.h`, `error.h`, the framebuffer validity contract, and the architecture-specific terminal boundary; no scheduler, heap, or Stage 4 dependency is permitted.

## Boundary Rules

No firmware, heap, scheduler, user-space, Stage 4, or kernel-main dependency.

## Failure Semantics

Prevent recursive panic presentation, attempt framebuffer output only when valid, always provide the non-graphical fallback, preserve failure state, and never call `kernel_main()`.

## Implementation Freedom

Renderer and fallback implementation are free within the architecture matrix and non-returning contract.

## Verification

Verify first panic, recursive panic, valid/invalid framebuffer, fallback output, x86_64, AArch64, and non-return behavior.

# 9. `include/justos/boot/recovery.h`

## File

Recovery header contract.

## Purpose

Recovery decisions and entry operations.

## Responsibilities

Define recovery outcomes, retry state, reset/preserve semantics, and recovery entry rules.

## Types / Structures

```c
typedef enum JPRecoveryDecision {
    JP_RECOVERY_ENTER = 0,
    JP_RECOVERY_RETRY,
    JP_RECOVERY_TERMINAL_HALT
} JPRecoveryDecision;

typedef struct JPBootRecoveryState {
    uint64_t current_attempt_id;
    uint64_t previous_attempt_id;
    uint32_t retry_count;
    uint32_t maximum_retry_count;
    bool exhausted;
    JPRecoveryDecision decision;
} JPBootRecoveryState;
```

The retry counter is monotonic within the recovery sequence and must not wrap. A new boot attempt receives a new attempt identity while retaining the previous attempt identity and diagnostic history where storage permits.

## Constants / Enums

`JPRecoveryDecision` defines the only recovery outcomes. Retry limits and exhaustion behavior are governed by the Retry Contract.

## Public API

```c
JPBootResult jp_boot_recovery_decide(const JPBootContext *context, JPRecoveryDecision *decision);
_Noreturn void jp_boot_recovery_enter(JPBootContext *context, JPRecoveryDecision decision);
```

The contract represents `ENTER_RECOVERY`, `RETRY_BOOT`, and `TERMINAL_HALT`, retry count/exhaustion, preserved diagnostic state, and the allowed recovery transition.

`jp_boot_recovery_enter()` is the Stage 3 entry point to the external recovery-transition boundary. It validates and submits the preserved request to the Stage 4 Boot Manager through `jp_boot_manager_request_recovery()`; it does not select a firmware provider, perform Stage 1 acquisition, or construct a new provider context. A `JP_BOOT_TRANSITION_REJECTED` or `JP_BOOT_TRANSITION_TERMINAL` result is converted to the non-returning Stage 3 terminal path without mutating preserved diagnostics.

`jp_boot_recovery_enter()` with `JP_RECOVERY_ENTER` is accepted without a recorded error only while the status is `RUNNING` (recovery requested by the Boot Manager through the boot configuration); in every other state a recorded error is required.

## Internal API

Private retry-counter, attempt-allocation, reset/preserve, provider-selection, and terminal-fallback helpers may be used only by `recovery.c`.

## Algorithm

Read immutable error and recovery state; reject panic-terminal or halted state; return `TERMINAL_HALT` for non-recoverable/exhausted failures; return `RETRY_BOOT` only below the limit; otherwise return `ENTER_RECOVERY` when a recovery destination is defined (in Milestone 0, the Stage 4 recovery diagnostics state); otherwise halt. `jp_boot_recovery_enter()` with `JP_RECOVERY_ENTER` is accepted without an error record only while the status is `RUNNING`.

## State / Lifecycle

Recovery state is `IDLE → DECIDED → ENTERED` or `IDLE → DECIDED → RETRYING → NEW_ATTEMPT`; exhaustion transitions to `TERMINAL`. A decision is not an entry until the corresponding transition succeeds.

## Ownership / Lifetime

Recovery owns counters, attempt identities, and decision state through the retry sequence or terminal halt. Providers remain owned by their defining stage and are only invoked through an existing transition contract.

## Invariants

Retry count never decreases or wraps; attempt identities are never reused; every decision is one of the three enum values; Stage 1/Stage 2 ownership remains unchanged.

## Dependencies

`recovery.h` depends on `boot_types.h`, `boot_status.h`, and `error.h` for context, status, and failure classification state. It owns recovery metadata but does not own providers or Stage 1/Stage 2 resources.

## Boundary Rules

Recovery cannot create a provider, repair an earlier stage, depend on Stage 4 UI, or mutate kernel image, mappings, stack, heap ownership, or handoff history.

## Failure Semantics

`jp_boot_recovery_decide()` fails without mutating preserved diagnostics when the decision inputs are invalid. `RETRY_BOOT` is never selected after retry exhaustion; exhaustion yields `TERMINAL_HALT`. Rejection of the external recovery-transition request enters the non-returning Stage 3 terminal path without mutating preserved diagnostics. Recovery never creates a provider or mutates Stage 1/Stage 2 ownership.

## Implementation Freedom

Decision tables, counter storage, and provider dispatch are free if retry atomicity, reset/preserve semantics, exhaustion, and ownership boundaries remain exact.

## Verification

Verify each decision, retry atomicity, reset/preserve table, identity uniqueness, exhaustion, existing-provider entry, terminal fallback, and both architectures.

# 10. `src/boot/recovery.c`

## File

Recovery implementation.

## Purpose

Implement deterministic recovery decisions, retry limits, recovery entry, and terminal fallback.

## Responsibilities

Evaluate recovery decisions, commit retry transitions, and enter an existing recovery or terminal path.

## Types / Structures

No additional public types. Private state must implement `JPBootRecoveryState` and the reset/preserve contract.

## Constants / Enums

Consumes `JPRecoveryDecision` and the configured maximum retry count; the counter must not wrap.

## Public API

Implements exactly the public operations declared by `recovery.h`.

The implementation must call only `jp_boot_manager_request_recovery()` declared by `boot_manager.h`. If the request is rejected or returns a terminal result, `recovery.c` must preserve the current diagnostic state and enter the defined terminal path; it must not invent a fallback provider.

## Internal API

Private decision, counter, transition, and terminal-fallback helpers only.

## Algorithm

Validate state, decide, and commit atomically using the recovery and reset/preserve contracts. A failed commit leaves the old attempt current and does not increment the counter.

## State / Lifecycle

Publish `DECIDED` before entry; publish `NEW_ATTEMPT` only after all preserved and reset state is committed.

## Ownership / Lifetime

Own recovery metadata; borrow status, error, and log views; never take ownership of provider or Stage 1/Stage 2 resources.

## Invariants

No infinite retry, no counter wrap, no identity reuse, no partial reset, and no ownership mutation.

## Dependencies

`recovery.h`, status/error/log state, and the Stage 4 recovery-transition boundary; it must not create a new provider or alter Stage 1/Stage 2 ownership.

## Boundary Rules

No Stage 4 UI or newly invented boot provider; terminal fallback is available without graphics.

## Failure Semantics

No infinite retry, no new provider, no Stage 1/Stage 2 ownership mutation, and no Stage 4 UI dependency.

## Implementation Freedom

Private dispatch and storage are free within the decision and transition contract.

## Verification

Verify each decision, retry exhaustion, preserved diagnostics, terminal fallback, x86_64, and AArch64.

# 11. `include/justos/boot/diagnostic_output.h`

## File

Normalized diagnostic-output routing contract.

## Purpose

Define the single owner of framebuffer and non-graphical diagnostic output routing.

## Responsibilities

Validate messages, select permitted sinks, preserve fallback availability, and report sink outcome without invoking panic recursively.

## Types / Structures

```c
typedef struct JPBootDiagnosticMessage {
    JPDiagnosticStage stage;
    JPBootSourceComponent source_component;
    uint32_t diagnostic_context;
    const char *text;
    size_t length;
} JPBootDiagnosticMessage;
```

## Constants / Enums

No sink-specific public enum is required. Sink availability is derived from the validated Stage 1 framebuffer contract and the always-required non-graphical fallback.

## Public API

```c
JPBootResult jp_boot_diagnostic_output(
    JPBootContext *context,
    const JPBootDiagnosticMessage *message
);
```

## Internal API

Private message validation, framebuffer emission, fallback emission, and output-failure recording helpers remain in `diagnostic_output.c`.

## Algorithm

Validate message and context; attempt framebuffer output only when valid; attempt non-graphical fallback whenever framebuffer output is unavailable or fails; return success when at least one permitted sink accepts the message; record failure without recursive panic entry when all sinks fail.

## State / Lifecycle

Output routing is stateless with respect to ownership. Each call is independent and must be safe during normal entry and degraded failure entry.

## Ownership / Lifetime

The router borrows the message for the call and borrows sink state. It owns only output-failure diagnostic metadata.

## Invariants

No output call dereferences an invalid framebuffer; fallback remains available without heap or scheduler; routing never changes Stage 1/Stage 2 ownership.

## Dependencies

Depends on `boot_types.h` and the Stage 1 framebuffer contract. It does not own framebuffer memory, serial hardware, or terminal state.

## Boundary Rules

The router must not access firmware, allocate heap memory, invoke panic recursively, enter recovery, or depend on Stage 4.

## Failure Semantics

Malformed messages, unavailable sinks, and sink failures return failure without corrupting diagnostic records or protected state.

## Implementation Freedom

Framebuffer text layout and fallback transport are implementation-defined within the observable sink-selection and failure contract.

## Verification

Verify valid/invalid messages, valid/invalid framebuffer, fallback-only operation, both sinks, all-sinks failure, recursion prevention, x86_64, AArch64, and ownership preservation.

# 12. `src/boot/diagnostic_output.c`

## File

Diagnostic-output routing implementation.

## Purpose

Implement `diagnostic_output.h` without owning any output device.

## Responsibilities

Implement validation, sink selection, output attempts, and failure recording.

## Types / Structures

Private sink adapters and bounded failure metadata only.

## Constants / Enums

Consumes the normalized Stage 1 framebuffer validity state and the Stage 3 source registry.

## Public API

Implements exactly `jp_boot_diagnostic_output()`.

## Internal API

Private framebuffer, fallback, validation, and failure-record helpers only.

## Algorithm

Execute the `diagnostic_output_route` algorithm defined by Stage 3. A failed framebuffer write never suppresses fallback and never invokes panic recursively.

## State / Lifecycle

No persistent output lifecycle is published. Each operation either reports accepted output or deterministic failure.

## Ownership / Lifetime

Borrow message and sink state; do not acquire framebuffer or fallback ownership.

## Invariants

Output failure cannot rewrite result/error history or protected ownership state.

## Dependencies

`diagnostic_output.h`, the Stage 1 framebuffer contract, and the non-graphical fallback boundary; no firmware, heap, scheduler, or Stage 4 dependency.

## Boundary Rules

No firmware, heap, scheduler, Stage 4, panic, or recovery dependency.

## Failure Semantics

Return failure only after all permitted sinks fail; preserve the original diagnostic record in every case.

## Implementation Freedom

Sink adapters and transport details are free within the header contract.

## Verification

Verify routing, fallback, sink failure, recursion prevention, architecture state preservation, and both architectures.

# 13. `include/justos/boot/boot_report.h`

## File

Canonical Boot Report and diagnostic-finalization contract.

## Purpose

Own final diagnostic-state coordination and the read-only report exposed to Stage 4 or terminal halt.

## Responsibilities

Validate final state, finalize owner modules in order, publish one immutable report, and provide read-only access.

## Types / Structures

```c
typedef struct JPBootReport {
    uint64_t boot_attempt_id;
    JPBootStatusState status;
    JPBootError error;
    JPBootRecoveryState recovery;
    JPBootLogStatus log_status;
    JPArchitectureType architecture;
    bool finalized;
} JPBootReport;
```

The report contains copied summaries and read-only views of retained event history; it does not duplicate mutable owner storage.

## Constants / Enums

No new lifecycle enum. `finalized == true` is the publication state and is immutable.

## Public API

```c
JPBootResult jp_boot_report_finalize(JPBootContext *context);
JPBootResult jp_boot_report_get(const JPBootContext *context, JPBootReport *report);
```

## Internal API

Private consistency validation, owner-finalization, report assembly, and publication helpers.

## Algorithm

Validate lifecycle and destination; record final status/outcome; finalize log; freeze error/retry history; assemble the report; publish it atomically. Repeated finalize returns the existing report. Failed finalization publishes nothing.

## State / Lifecycle

`UNFINALIZED → FINALIZED` is the only report lifecycle. `FINALIZED` has no mutable transition.

## Ownership / Lifetime

`boot_report.c` owns the published report summary until terminal halt or Stage 4 handoff. Owner modules retain authority over their storage until finalization, then become immutable.

## Invariants

Final report status, original results, event order, truncation, recovery outcome, retry history, architecture, and ownership view are internally consistent and immutable.

## Dependencies

Depends on `boot_log.h`, `boot_status.h`, `error.h`, `recovery.h`, and `boot_types.h`. It does not depend on Stage 4 implementation.

## Boundary Rules

No firmware, heap, scheduler, provider creation, Stage 4 UI, or Stage 1/Stage 2 ownership mutation.

## Failure Semantics

Inconsistent owner state, unavailable report storage, or invalid lifecycle prevents publication and leaves pre-finalized diagnostic state intact.

## Implementation Freedom

Report storage layout and read-only view mechanism are free if atomic publication and immutability remain observable.

## Verification

Verify successful finalization, repeated finalization, inconsistent state, failed publication, read-only Stage 4 access, terminal consumption, both architectures, and ownership preservation.

# 14. `src/boot/boot_report.c`

## File

Boot Report and diagnostic-finalization implementation.

## Purpose

Implement the finalization and read-only report contract.

## Responsibilities

Coordinate owner modules without duplicating their state and publish one final report.

## Types / Structures

Private consistency and publication state only.

## Constants / Enums

Consumes owner lifecycle states and the finalized-report contract.

## Public API

Implements exactly the operations declared by `boot_report.h`.

## Internal API

Private consistency, finalize-order, report-copy, and atomic-publication helpers only.

## Algorithm

Run the Diagnostic Finalization Contract sequence; if a required step fails, preserve the old state and enter no partial finalized state.

## State / Lifecycle

Publish `FINALIZED` only after all owner records and counters pass consistency validation.

## Ownership / Lifetime

Borrow owner state before publication; own the published report summary; never take ownership of Stage 1/Stage 2 resources.

## Invariants

Finalize is idempotent, publication is atomic, and the report cannot be rewritten after publication.

## Dependencies

`boot_report.h`, all Stage 3 owner headers, and the transferred `JPBootContext`; no Stage 4 UI or firmware dependency.

## Boundary Rules

No firmware, heap, scheduler, Stage 4 implementation, provider creation, or protected-state mutation.

## Failure Semantics

Any failed consistency or publication step leaves the report unpublished and preserves diagnostic history.

## Implementation Freedom

Private publication and storage mechanisms are free within the immutable report contract.

## Verification

Verify finalization order, idempotence, failed publication, immutability, terminal/report consumers, both architectures, and ownership preservation.

For every file above, the common contract is shared across architectures; architecture-specific output and terminal mechanisms remain inside explicit x86_64/AArch64 boundaries. Every public operation requires success/failure semantics and verification for both architectures.

## 3 Stage 3 → Stage 4 Boundary

Stage 3 provides Stage 4 with:

```text
retained boot events
retained error records
recovery outcome
diagnostic history
terminal/recovery status
```

Stage 4 may provide the user-facing Boot Manager UI, but it must not be required for Stage 3 terminal diagnostics or safe recovery entry.

Stage 4 consumes the environment prepared by Stage 2 and the read-only Boot Report published by Stage 3. It provides Boot UI and boot orchestration without replacing, repeating, or bypassing Stage 2 initialization or Stage 3 diagnostics.

## 3 Specification Lock Criteria

- [x] Stage 3 scope is explicitly limited to diagnostics and recovery.
- [x] x86_64 and AArch64 common and architecture-specific behavior is defined.
- [x] Stage 3 normal and failure execution contexts are defined.
- [x] Boot attempt identity, attempt sequence, retry count, and event sequence are defined.
- [x] Boot status lifecycle and mutation authority are defined.
- [x] Boot event structure and bounded-storage policy are defined.
- [x] Event ordering and overflow retention policy are defined.
- [x] Error structure, severity, recoverability, and result preservation are defined.
- [x] Error classification and idempotence are defined.
- [x] Diagnostic output routing and non-graphical fallback are defined.
- [x] Panic presentation and non-graphical fallback are defined.
- [x] Panic lifecycle and recursive-panic behavior are defined.
- [x] Recovery decisions and retry exhaustion are defined.
- [x] Recovery transition ownership and reset/preservation semantics are defined.
- [x] Existing recovery-transition interface and Stage 3 → transition → new-attempt boundary are defined.
- [x] Ownership and lifetime of diagnostic records are defined.
- [x] Stage 2 failure path to Stage 3 is defined.
- [x] Stage 3 → Stage 4 boundary is defined.
- [x] Canonical Boot Report data and read-only Stage 4 consumption are defined.
- [x] Diagnostic finalization order, idempotence, failure behavior, and immutability are defined.
- [x] Every Stage 3 production file has a normative file specification.
- [x] No Stage 3 component owns Stage 1/Stage 2 resources.

## 3 Specification Traceability

| Requirement             | Specification Location                     | Verification                        |
| ----------------------- | ------------------------------------------ | ----------------------------------- |
| Boot events             | Boot Event Contract                        | Event verification                  |
| Boot status             | Boot Status Contract                       | Status verification                 |
| Error records           | Error Contract                             | Error verification                  |
| Diagnostic output       | Diagnostic Output Contract                 | Output verification                 |
| Panic presentation      | Panic and Diagnostic Presentation Contract | Panic verification                  |
| Recovery decision       | Recovery Contract                          | Recovery verification               |
| Recovery transition     | Recovery Transition Boundary               | Transition integration verification |
| Retry behavior          | Retry Contract                             | Retry verification                  |
| Boot attempt identity   | Boot Attempt and Event Identity            | Attempt verification                |
| Stage 2 failure path    | Stage 3 Execution Flow                     | Integration verification            |
| Stage 3 ownership       | Ownership and Lifetime                     | Ownership verification              |
| Stage 3 exit            | Stage 3 → Stage 4 Boundary                 | Boundary verification               |
| Boot Report             | Boot Report Contract                       | Report immutability verification    |
| Diagnostic finalization | Diagnostic Finalization Contract           | Finalization verification           |

## 3 Specification Completeness Verification

Stage 3 is complete as a specification only when every lock criterion has a normative definition, every production file is specified, both architectures have observable contracts, failure behavior is deterministic, and no recovery or diagnostic component requires an undocumented later stage.

The verification result shall be recorded as:

```text
Specification Completeness Verification: PASS
```

or:

```text
Specification Completeness Verification: FAIL
```

**Record of the verification (Stage 3).** `PASS`, 2026-11-21. The list of completeness criteria above was ticked after the contract was implemented and tested, against the finished text, so the ticks confirm that the text describes what was built: the "3 Decided rules" block holds the decisions that the older text did not define, and every Stage 3 production file (including the private files `diagnostic_font`, `diagnostic_hw`, and `diagnostic_hw_<arch>`) is described.

## 3 Specification Lock Decision

Stage 3 receives Specification Lock only when:

- all Stage 3 lock criteria pass;
- traceability is complete;
- completeness verification is `PASS`;
- x86_64 and AArch64 behavior is independently verifiable;
- Stage 2 failure handling has a defined destination;
- Stage 3 does not depend on Stage 4 other than the single recovery-transition boundary (`jp_boot_manager_request_recovery()`).

Otherwise:

```text
SPECIFICATION LOCK = REJECTED
```

**Record of the decision (Stage 3).** `SPECIFICATION LOCK = APPROVED`, signed by the project owner on 2026-11-21, after the contract was implemented and verified: the unit tests of all Stage 3 modules (gcc and clang) with mutation checks; the QEMU tests on x86_64 UEFI, x86_64 BIOS (both also under KVM), and AArch64 UEFI, in three variants each: the normal run, the run in which Stage 2 fails and Stage 3 enters recovery, and the run in which Stage 2 fails and the recovery transition is refused (`panic`). In the `panic` variant the test asks QEMU for a screen dump (`screendump`, the picture that the virtual display really shows) and compares the Stage 3 text area pixel by pixel with the expected text, and it reads the eight panic lines from the serial log. Two things are **not** covered by this decision: level M (nothing of Stage 3 ran on real hardware; the project owner runs the whole milestone by hand after the last stage), and the older text of Stage 3 (before the "3 Decided rules" block), which has not been rewritten yet for the junior-readability test. "Implementation Acceptance" below is a separate step and is not ticked by this decision.

## 3 Implementation Acceptance

Implementation Acceptance is separate from Specification Lock.

It requires:

- event recording verified;
- error preservation verified;
- panic presentation verified;
- graphical and non-graphical fallback verified;
- recovery decision verified;
- retry exhaustion verified;
- Stage 4 recovery-transition boundary integration verified;
- terminal halt verified;
- x86_64 and AArch64 paths verified separately;
- no Stage 1/Stage 2 ownership corruption;
- no unresolved Stage 3 symbols;
- no Stage 4 dependency other than the single recovery-transition boundary.

**Record of the acceptance (Stage 3).** All twelve items above are met; ticked on 2026-11-21 after the architecture review and the commit `Stage 3 — Boot Diagnostics & Recovery`, with this evidence: the unit tests of all Stage 3 modules (gcc and clang, 36 tests in all) and their mutation checks (the survivors are equivalent changes); the QEMU tests on x86_64 UEFI, x86_64 BIOS (both also under KVM), and AArch64 UEFI, in three variants each: the normal run (the lines `JP|stage3.*` pass through the real serial output), the run in which Stage 2 fails and Stage 3 enters recovery, and the run `panic` in which the recovery transition is refused (the eight panic lines in the serial log and a real screen dump compared pixel by pixel; a panic does not end with a test result); the retry and its undo only by unit tests, because the maximum is 0 in Milestone 0; a search of the Stage 3 sources for Stage 4 and Stage 2 references (only `jp_boot_manager_request_recovery`), `malloc`, `memcpy`, `memset`, `TODO`, mutable global data, and writes to the fields of Stage 1 and Stage 2 (none). **Not part of this acceptance:** level M (nothing of Stage 3 ran on a real machine; the project owner runs the whole milestone by hand after the last stage), and the Stage 4 stand-ins, which Stage 4 replaces.

## 3 Final Lock Statement

Stage 3 — Boot Diagnostics & Recovery is formally closed only after:

```text
Specification Completeness Verification = PASSED
Specification Traceability            = PASSED
Specification Lock Decision           = APPROVED
Implementation Acceptance             = PASSED
```

---

---

# Stage 4 — Boot manager & UI

## Stage 4 Definition

Stage 4 is the last step of Milestone 0 (the first milestone, which only has to start the computer and hand control to the operating system's kernel). The kernel is the core program of the operating system; its first function is called `kernel_main`.

The earlier stages did this:

- Stage 1 finds the hardware, loads the kernel into memory, and prepares the processor to run it.
- Stage 2 prepares the kernel's working environment (its memory map, its stack, which is the memory area that the code uses for its function calls, and its early heap, which is a pool of memory that can be handed out).
- Stage 3 records what happens during boot and explains what went wrong when something fails.

Stage 4 does four things:

1. It owns the **boot configuration**: the start options chosen for this start (see the Boot Configuration Contract).
2. It shows the **boot screens**: a menu where a developer picks the start options, and plain text screens that report what happened.
3. It collects what Stages 1 to 3 found and did, without changing any of it.
4. It **decides** what happens next (start the kernel, show a diagnostic screen and stop, or just stop), **freezes** the configuration (after this moment it cannot be changed), and hands control to the kernel.

Stage 4 works in two phases. The two phases share one record, called the **context** (`JPBootContext`: the single record that every boot step reads and writes; it is passed from step to step and is never copied) and one configuration, but they run in different environments.

```text
Phase A — Selection (runs in the boot program, while the firmware still works)
    Stage 1 has checked that everything needed is ready
        ↓
    Stage 1 reaches its "interaction point" (a moment where it calls a function that Stage 4 registered)
        ↓
    Stage 4 selection function runs (the menu)
        ↓
    the boot configuration is chosen
        ↓
    Stage 1 finishes its handoff work and gives up the firmware

Phase B — Completion (runs in the kernel program, after Stage 2)
    jp_kernel_stage2_complete(context)
        ↓
    Stage 3 starts (the normal start, or the failure start)
        ↓
    decision
        ├── CONTINUE_TO_KERNEL → report finalized → configuration frozen → kernel_main(context)
        ├── ENTER_RECOVERY     → recovery diagnostics state (never returns)
        └── TERMINAL_HALT      → Stage 3 stops the machine
```

Two words in this picture need an explanation.

- **Firmware** is the startup software built into the computer (UEFI or BIOS) that runs before anything else. It can read the keyboard and show things on the screen. Once Stage 1 gives it up, nothing in Milestone 0 can read the keyboard any more. This is why the menu exists only in phase A, and why phase B can only show text and cannot ask questions.
- The **boot program** (also called the boot image) is the code that runs first and loads the kernel. The **kernel program** (the kernel image) is the kernel's own code. Phase A runs inside the boot program; phase B runs inside the kernel program. Because of this, the same source files of `boot_config`, `boot_ui`, and `boot_info` are built into both programs.

Stage 4 uses the machine that Stage 2 prepared and the read-only Boot Report that Stage 3 published. It never replaces, repeats, or skips what Stage 2 initialized or what Stage 3 records.

Stage 4 also handles the recovery request that Stage 3 can send when boot cannot continue normally (`jp_boot_manager_request_recovery()`).

## 4 Specification Lock

A Specification Lock means that this document describes Stage 4 completely enough that a programmer can build it without guessing. Before any code is written, the document must define: the functions, the stored data, the allowed states, the order of events, who owns each piece of data, what happens on failure, and how each part is tested.

If a programmer finds that a decision is missing, they stop working on that part and report the gap. The document is then updated first, and only then does the work continue.

## 4 Architecture Target

Milestone 0 supports two processor families, and Stage 4 works on both:

```text
x86_64:  REQUIRED
AArch64: REQUIRED
```

Phase A runs on the Stage 1 firmware layers: UEFI on both families, and BIOS on x86_64 only. Phase B runs on the processor families that Stage 2 supports. The rules for the configuration, the decision, and the recovery request are the same on both families. Only two things differ: how Stage 1 reads the keyboard, and how Stage 3 stops the machine.

## 4 Execution Context

Stage 4 is entered in three ways. Each way is a different moment and a different caller.

**Selection entry (phase A).** The code that drives Stage 1 (the "caller") first calls `jp_boot_manager_prepare(context)`, which must happen before `jp_boot_prepare_handoff()`. Later, while `jp_boot_prepare_handoff(context)` runs, Stage 1 reaches its interaction point and calls the Stage 4 selection function:

```text
Stage 1 caller
    ↓
jp_boot_manager_prepare(context)
    ↓
jp_boot_prepare_handoff(context)
    ↓
jp_boot_manager_select(context, NULL)      (called by Stage 1 itself)
```

**Completion entry (phase B).** Stage 2 reaches the function `jp_kernel_stage2_complete(context)` in two ways: directly when Stage 2 succeeded, or through `jp_kernel_stage2_failure()` when Stage 2 failed.

```text
entry stub → jp_kernel_init(context) → success ─────────────┐
                                                            ▼
                     jp_kernel_stage2_failure(context, r) → jp_kernel_stage2_complete(context)
```

(The entry stub is the tiny first piece of kernel code that calls `jp_kernel_init`.)

**Recovery entry (from Stage 3).** When boot cannot continue normally, Stage 3 calls:

```text
jp_boot_recovery_enter(context, decision)
    ↓
jp_boot_manager_request_recovery(context, recovery)
```

Where the code runs: phase A runs in the boot program, while the firmware services (including the keyboard) still work. Phase B runs in the kernel program, after the "platform transition" (the moment when the boot program gives up the firmware and the kernel's own environment takes over). The sources of `boot_config`, `boot_ui`, and `boot_info` are built into both programs. They keep no global data that changes (Rule 11 of `rules.md`); they work only on the context that is passed to them.

The function that Stage 1 stored during phase A (the registered hook) is only a record after the transition. Phase B never calls it.

In phase B there is no firmware, no keyboard, no timer, no scheduler, and no heap. Phase B may use only things whose setup is recorded as finished in the context, and it shows text only through `jp_boot_diagnostic_output()`. That is Stage 3's text output function: it writes to the screen when a valid screen memory exists and otherwise falls back to a plain text channel.

## 4 Stage 4 Responsibilities

Stage 4 is responsible for:

1. The boot configuration: its default values, its checks, applying a new one, freezing it, and reading it (`JPBootConfig`).
2. The selection function and the interactive menu of phase A.
3. The Stage 4 area of the context and its life cycle.
4. Collecting information from Stages 1 to 4 into a read-only summary, without searching for anything again and without rewriting anything.
5. Showing the summary, a failure, and the recovery diagnostics as plain text in phase B.
6. The boot decision: `CONTINUE_TO_KERNEL`, `RETRY_BOOT`, `ENTER_RECOVERY`, or `TERMINAL_HALT`.
7. Receiving the recovery request from Stage 3.
8. Providing `jp_kernel_stage2_complete()` and starting Stage 3 in the right way.
9. Freezing the configuration before the kernel starts.
10. Handing control to `kernel_main(context)`.

## 4 Stage 4 Non-Goals

Stage 4 is not responsible for:

- the Stage 1 firmware layers, the keyboard hardware, firmware access, or the platform transition;
- Stage 2 setup, memory ownership, paging, the stack, the heap, or the kernel file state;
- Stage 3 records, status, error classification, panic handling, or text output routing;
- understanding or running the configuration entries: in Milestone 0 the entries are opaque (they are stored and passed on, never interpreted);
- starting, stopping, or watching services or background programs (daemons);
- saving the configuration between boots;
- changing the configuration after the kernel has started;
- choosing the kernel, choosing the screen mode, a boot delay, or boot themes;
- reading the keyboard in phase B, repair tools, or alternative kernels;
- restarting the machine automatically;
- a command shell, a window manager, a desktop, or an application framework.

Left for later milestones (each must be fully specified before it is used): saving the configuration, restarting the machine and retrying automatically, changing the configuration after start (compare, restart, and protect against restart loops), interactive recovery, running configuration entries by a service manager, choosing the kernel, screen settings, and boot themes.

## 4 Folder Structure

```text
include/justos/boot/
├── boot_config.h
├── boot_info.h
├── boot_ui.h
└── boot_manager.h

src/boot/
├── boot_config.c
├── boot_info.c
├── boot_ui.c
└── boot_manager.c
```

What each module does:

- `boot_config` owns the boot configuration (`JPBootConfig`).
- `boot_info` collects information and only reads it.
- `boot_ui` owns the menu and the text screens.
- `boot_manager` runs the whole of Stage 4 and passes each piece of work to the module that owns it.

No assembly language file (assembly language is the lowest-level way of writing code) is needed for Stage 4.

Order of dependence in the code (a lower module never calls a higher one): `boot_config` → `boot_info` → `boot_ui` → `boot_manager`. The shared Stage 4 types (`JPBootStage4Area` and the lists of named values) are declared in `boot_manager.h`. The lower modules use them only so that the compiler (the tool that turns source code into a program) knows their size. Splitting headers to avoid circular `#include` lines is left to the programmer, as long as every type, function name, and signature in this document stays exactly as written.

## 4 Stage 4 Common Contract

Every Stage 4 module must follow these rules:

- It works only on the context that is passed in and on its `stage4` area.
- It keeps no global data that changes. Fixed read-only tables (such as the built-in font) are allowed.
- It never invents state. If information is missing, it reports exactly what the owner reported: `UNKNOWN`, `NOT DETECTED`, `NOT INITIALIZED`, or `FAILED`.
- It never rewrites anything that Stage 1, Stage 2, or Stage 3 produced.
- It never allocates heap memory. It never talks to the firmware, except for the phase A keyboard and screen use that Stage 1 defines.
- With the same input it always behaves the same way.
- It works on both processor families.
- When an operation fails, it passes on the original failure result (`JPBootResult`) unchanged.

## 4 Stage 4 Area of JPBootContext

The context contains one **area** (a fixed block of storage inside the context) for Stage 4. It is named `stage4` and has the type `JPBootStage4Area`. Stage 1 reserves the space but never reads or writes it.

```c
typedef struct JPBootStage4Area {
    JPBootConfig config;                   /* the boot configuration */
    JPBootManagerState state;              /* how far the Boot Manager has got */
    JPBootSelectionOutcome selection_outcome; /* what happened to the menu */
    JPBootDecision decision;               /* what the Boot Manager decided */
} JPBootStage4Area;
```

The area is filled with zeros until `jp_boot_manager_prepare()` sets it up. It lives exactly as long as the context. That is why it survives the platform transition: Stage 2 takes over the context, and the area comes with it. Stage 4 may change only this area, and the Stage 3 data through Stage 3's own functions. It may not change anything else.

## 4 Boot Configuration Contract

`JPBootConfig` is the one and only description of the start options of one boot. It holds data only.

```c
#define JP_BOOT_CONFIG_KERNEL_PARAMETERS_CAPACITY 256   /* bytes, including the end marker */
#define JP_BOOT_CONFIG_ENTRY_CAPACITY 16                /* most entries a configuration can hold */
#define JP_BOOT_CONFIG_ENTRY_ID_CAPACITY 32             /* bytes for an entry's name */
#define JP_BOOT_CONFIG_ENTRY_PARAMETERS_CAPACITY 64     /* bytes for an entry's text */

typedef enum JPBootConfigSource {        /* where the configuration came from */
    JP_BOOT_CONFIG_SOURCE_UNKNOWN = 0,
    JP_BOOT_CONFIG_SOURCE_DEFAULT,       /* built-in default values */
    JP_BOOT_CONFIG_SOURCE_INTERACTIVE    /* chosen in the menu */
} JPBootConfigSource;

typedef enum JPBootConfigState {         /* how far the configuration has got */
    JP_BOOT_CONFIG_STATE_UNSET = 0,      /* nothing has been set up yet */
    JP_BOOT_CONFIG_STATE_VALIDATED,      /* checked and still changeable */
    JP_BOOT_CONFIG_STATE_FROZEN          /* checked and locked */
} JPBootConfigState;

typedef struct JPBootConfigEntry {       /* one launch record */
    char id[JP_BOOT_CONFIG_ENTRY_ID_CAPACITY];
    char parameters[JP_BOOT_CONFIG_ENTRY_PARAMETERS_CAPACITY];
    bool enabled;
} JPBootConfigEntry;

typedef struct JPBootConfig {
    JPBootConfigState state;
    JPBootConfigSource source;
    char kernel_parameters[JP_BOOT_CONFIG_KERNEL_PARAMETERS_CAPACITY];
    bool boot_logging;                   /* show a summary of the boot */
    bool verbose_output;                 /* also show every recorded event */
    bool safe_mode;                      /* a flag handed to the kernel */
    bool recovery_requested;             /* ask for the recovery screen */
    size_t entry_count;                  /* how many entries are used */
    JPBootConfigEntry entries[JP_BOOT_CONFIG_ENTRY_CAPACITY];
} JPBootConfig;
```

What the fields mean:

- `kernel_parameters`: a text that is handed to the kernel unchanged. It ends with a zero byte (called the NUL byte; it marks where the text ends), so it holds at most 255 characters. Only printable ASCII characters are allowed (ASCII is the basic set of English letters, digits, punctuation, and the space).
- `boot_logging` and `verbose_output`: ask phase B to show text about the boot. `boot_logging` shows a summary. `verbose_output` also shows every event that was recorded.
- `safe_mode`: a flag that is passed to the kernel. Milestone 0 does not act on it.
- `recovery_requested`: asks for the recovery screen in phase B (see the Boot Manager Decision Contract).
- `entries`: launch records `{id, parameters, enabled}` that Milestone 0 only stores and passes on. It never interprets them. `entry_count` is at most `JP_BOOT_CONFIG_ENTRY_CAPACITY`, and unused entries are all zero. The default configuration and the Milestone 0 menu always produce `entry_count == 0`.

The default configuration:

```text
state = VALIDATED, source = DEFAULT
kernel_parameters = ""  (empty)
boot_logging = true, verbose_output = false
safe_mode = false, recovery_requested = false
entry_count = 0
```

A candidate configuration (a configuration offered for use) is valid only if all of the following are true:

- `kernel_parameters` ends with a NUL byte inside its capacity and contains only printable ASCII characters;
- `entry_count` is not larger than `JP_BOOT_CONFIG_ENTRY_CAPACITY`;
- every used entry has a non-empty `id` (ending with a NUL byte, made of printable ASCII characters without spaces) that differs from the `id` of every other entry, and a `parameters` text that ends with a NUL byte and has only printable ASCII characters;
- unused entries are all zero.

A candidate that is not valid is rejected completely. It is never applied in part.

The states, and the moves between them:

```text
UNSET
    ↓ jp_boot_config_initialize()
VALIDATED  (source DEFAULT)
    ↓ jp_boot_config_apply()  (with a valid candidate; the source becomes INTERACTIVE)
VALIDATED
    ↓ jp_boot_config_freeze()
FROZEN     (final; read-only)
```

If the configuration is missing or invalid:

- If the menu never ran, was skipped, or was discarded, the configuration stays at its default values. It is `VALIDATED` with source `DEFAULT`, and the reason is stored in `selection_outcome`.
- If `jp_boot_config_apply()` rejects a candidate, the active configuration is not changed.
- Stage 2 and the kernel never receive an `UNSET` configuration. Freezing fails for `UNSET`, and `jp_kernel_stage2_complete()` treats an `UNSET` configuration as a Stage 4 failure.

Changing it: the configuration can be changed only in the state `VALIDATED`. After `FROZEN`, no function in this document changes it. Milestone 0 enforces this through the functions only and checks it with tests. It does not protect the memory itself.

## 4 Selection Outcome and Manager State

```c
typedef enum JPBootSelectionOutcome {    /* what happened to the menu */
    JP_BOOT_SELECTION_NOT_RUN = 0,       /* the menu function has not run yet */
    JP_BOOT_SELECTION_SKIPPED_POLICY,    /* the build setting turned the menu off */
    JP_BOOT_SELECTION_SKIPPED_NO_FRAMEBUFFER, /* there is no usable screen */
    JP_BOOT_SELECTION_SKIPPED_NO_INPUT,  /* there is no keyboard */
    JP_BOOT_SELECTION_APPLIED,           /* the user pressed F10 and the choice was accepted */
    JP_BOOT_SELECTION_DISCARDED          /* the user pressed Esc */
} JPBootSelectionOutcome;

typedef enum JPBootManagerState {        /* how far the Boot Manager has got */
    JP_BOOT_MANAGER_UNINITIALIZED = 0,
    JP_BOOT_MANAGER_PREPARED,
    JP_BOOT_MANAGER_SELECTED,
    JP_BOOT_MANAGER_COMPLETING,
    JP_BOOT_MANAGER_RECOVERY_REQUESTED,
    JP_BOOT_MANAGER_DECIDED
} JPBootManagerState;

typedef enum JPBootDecision {            /* what the Boot Manager decided */
    JP_BOOT_DECISION_NONE = 0,
    JP_BOOT_DECISION_CONTINUE_TO_KERNEL,
    JP_BOOT_DECISION_RETRY_BOOT,
    JP_BOOT_DECISION_ENTER_RECOVERY,
    JP_BOOT_DECISION_TERMINAL_HALT
} JPBootDecision;
```

The Boot Manager moves through its states in this order:

```text
UNINITIALIZED → PREPARED → SELECTED → COMPLETING → DECIDED → RECOVERY_REQUESTED (used up; final)
```

- `PREPARED` means that the setup in phase A is done.
- `SELECTED` is entered after the menu function ran, including when the menu was skipped.
- `COMPLETING` is entered by `jp_kernel_stage2_complete()`.
- `DECIDED` is entered once the decision is recorded.
- `RECOVERY_REQUESTED` is entered from `DECIDED` when a recovery request is accepted. The state is never left.

Phase A stores what happened in `selection_outcome`, because Stage 3's record of events does not exist yet. Phase B adds this outcome to Stage 3's record as a Stage 4 event. In this way a skipped menu is never reported as if it had been used.

## 4 Selection Policy

```c
typedef enum JPBootSelectionPolicy {
    JP_BOOT_SELECTION_POLICY_DEFAULT_ONLY = 0,   /* never show the menu */
    JP_BOOT_SELECTION_POLICY_INTERACTIVE         /* show the menu */
} JPBootSelectionPolicy;
```

The policy is a constant `JP_BOOT_SELECTION_POLICY` that is chosen when the program is built. During development it is `INTERACTIVE`. In a release build it is `DEFAULT_ONLY`. With `DEFAULT_ONLY` the menu function returns `JP_BOOT_SUCCESS` at once, keeps the default configuration, and stores `SKIPPED_POLICY`. The policy is not stored in `JPBootConfig`, and it cannot be changed while the program runs.

## 4 Boot UI Contract

The "UI" (user interface) of Stage 4 is the set of screens it shows. There are two kinds: the interactive menu of phase A and the plain text screens of phase B.

**Phase A: the menu (`jp_boot_ui_select()`).**

- It needs a valid **framebuffer** (a block of memory whose contents appear on the screen; writing pixels, which are the single dots of the screen, into it draws on the screen) and a keyboard. The menu draws directly into the framebuffer that Stage 1 checked (`framebuffer.state == DETECTED`). It uses a built-in **font** (a fixed table that holds the picture of each character) stored as read-only data. The pixel layout is the one that Stage 1 recorded for the framebuffer.
- The keyboard is read as `JPBootInputEvent` values through `jp_boot_environment_read_input()`. The menu never waits inside the firmware: it keeps asking ("polling") until a key that ends the menu is pressed. Milestone 0 has no timers, so there is no timeout and no boot delay.
- The menu shows five fields, in this order: kernel parameters, boot logging, verbose output, safe mode, and recovery requested. It also shows an information panel taken from `boot_info`. Information that is not available is shown exactly as reported (`UNKNOWN`, `NOT DETECTED`, `NOT INITIALIZED`, or `FAILED`).
- The keys:

| Key | What it does |
| --- | ------------ |
| Up | go to the previous field; from the first field go to the last one (does nothing while editing the kernel parameters) |
| Down | go to the next field; from the last field go to the first one (does nothing while editing the kernel parameters) |
| Left, Right | change the current switch (does nothing on the kernel parameters field, and while editing) |
| Enter | on a switch: change it; on the kernel parameters field: start editing; while editing: stop editing |
| a character (while editing) | add it to the end of the kernel parameters; if the text is already full (255 characters), the character is ignored |
| Backspace (while editing) | remove the last character (nothing happens if the text is empty) |
| F10 | submit the edited values (works at any time, also while editing) |
| Esc | throw away the edits and keep the current configuration (works at any time, also while editing) |
| any other key | ignored |

- F10 builds a candidate from the edited values and calls `jp_boot_config_apply()`. If the candidate is rejected, the menu shows a message and stays open, and the active configuration is not changed. The message stays on the screen until the next key is pressed; that key press removes the message and is then handled as usual. Esc keeps the current configuration. The menu changes `JPBootConfig` only through `boot_config`.
- The menu writes only into the checked framebuffer region and into the `stage4` area (through `boot_config`).

**Phase B: text screens (`jp_boot_ui_present_summary()` and `jp_boot_ui_present_failure()`).**

- These screens cannot ask questions. They show lines of text. All text is sent through `jp_boot_diagnostic_output()`, which stays the only owner of the choice of where text goes (the screen or the fallback).
- A summary is shown only when `boot_logging` is true. The recorded events are also shown when `verbose_output` is true. A failure or recovery screen is always attempted. These screens are shown before the Boot Report is finalized, so they read the status, the recorded error, and the recorded events through the Stage 3 functions `jp_boot_status_get`, `jp_boot_error_get`, `jp_boot_log_get_status`, and `jp_boot_log_get`, not through `jp_boot_report_get`.
- If showing the text fails, the failure is recorded, and the decision does not change.

## 4 Boot Information Aggregation Contract

To aggregate means to gather. `boot_info` gathers information from the modules that own it, formats it for display, and offers it read-only. It never searches for hardware, never keeps its own copy, never guesses, and never changes anything.

```c
typedef struct JPBootSummary {
    JPArchitectureType architecture;          /* x86_64 or AArch64 */
    JPBootEnvironmentType environment;        /* UEFI or BIOS */
    JPDetectionState environment_state;
    JPDetectionState hardware_state;
    JPDetectionState memory_map_state;
    JPDetectionState framebuffer_state;
    JPKernelValidationState kernel_validation_state;
    JPKernelLoadState kernel_load_state;
    bool status_available;                    /* true only if Stage 3 reported a status */
    JPBootStatusState status;
    JPBootConfigState config_state;
    JPBootConfigSource config_source;
    JPBootSelectionOutcome selection_outcome;
} JPBootSummary;
```

The true sources of this information stay where they are: the context (the Stage 1 and Stage 2 data), the Stage 3 status and Boot Report, and the `stage4` area. `status_available` is true only if the Stage 3 status owner reported a status (that is, `jp_boot_status_get()` returned `JP_BOOT_SUCCESS`). Otherwise it is false, and `status` is cleared and must not be used. In phase A, Stage 3 has not started, so `status_available` is false there.

A summary is a copy of the values at the moment of the call. It contains no pointers into the context (a pointer is a memory address that refers to other data), and it is not a second source of truth. If a module reports that something is not available, the summary carries that state unchanged. `JPBootInfo` (a separate structure that earlier documents mention) is not defined or needed in Milestone 0.

## 4 Boot Manager Decision Contract

`jp_boot_manager_decide()` produces exactly one decision from this list:

```text
CONTINUE_TO_KERNEL   start the kernel
RETRY_BOOT           try the boot again
ENTER_RECOVERY       show the diagnostic (recovery) screen and wait
TERMINAL_HALT        stop the machine
```

It looks at three things: the result that Stage 2 recorded, the Stage 3 status and recovery state, and `JPBootConfig.recovery_requested`. It checks the rules in this order and uses the first one that matches:

1. If the result recorded by Stage 2 is not `JP_BOOT_SUCCESS`, or the Stage 3 status is `FAILED`, `RECOVERY`, or `HALTED`, the decision is the recovery decision that Stage 3 makes, converted one to one: `JP_RECOVERY_ENTER` becomes `ENTER_RECOVERY`, `JP_RECOVERY_RETRY` becomes `RETRY_BOOT`, and `JP_RECOVERY_TERMINAL_HALT` becomes `TERMINAL_HALT`.
2. Otherwise, if `recovery_requested` is true, the decision is `ENTER_RECOVERY`.
3. Otherwise the decision is `CONTINUE_TO_KERNEL`.

`CONTINUE_TO_KERNEL` exists only in Stage 4 and has no match in Stage 3. In Milestone 0 Stage 3's maximum retry count is zero, so `RETRY_BOOT` is never produced.

The decision is computed by `jp_boot_manager_decide()`, and `jp_kernel_stage2_complete()` records it once in the `stage4` area (state `DECIDED`). It is never computed again.

## 4 Recovery Transition Owner Contract

When boot cannot continue normally, Stage 3 decides what should happen and then asks Stage 4 to do it. The part of Stage 4 that accepts this request is the **owner** of the recovery transition.

| Part | Its job |
| ---- | ------- |
| Stage 3 `recovery.c` | decides the recovery action and keeps the recovery state |
| Stage 4 `boot_manager.c` | accepts the request and enters the chosen place |
| Stage 1 firmware layers | would carry out the platform-specific steps of a new boot attempt (not available in Milestone 0) |
| Stage 2 | would set up the kernel environment again in a new attempt (not available in Milestone 0) |

The request function is declared in `boot_manager.h`:

```c
typedef enum JPBootTransitionResult {
    JP_BOOT_TRANSITION_ACCEPTED,     /* the request was accepted and the destination started */
    JP_BOOT_TRANSITION_REJECTED,     /* the request was invalid; nothing changed */
    JP_BOOT_TRANSITION_TERMINAL      /* there is no safe destination */
} JPBootTransitionResult;

JPBootTransitionResult jp_boot_manager_request_recovery(
    JPBootContext *context,
    const JPBootRecoveryState *recovery
);
```

What it does depends on the decision stored in `recovery->decision`:

- `JP_RECOVERY_ENTER`: the request is used up exactly once (the manager state becomes `RECOVERY_REQUESTED`). The Boot Manager sets the Stage 3 status to `RECOVERY` (from `FAILED`, or from `RUNNING` when recovery was asked for through the configuration), shows the failure and recovery diagnostics, finalizes the Boot Report, freezes the configuration unless it is `UNSET`, and enters the **recovery diagnostics state**. This state is a final, non-interactive wait: the machine shows what went wrong and then waits with interrupts turned off (interrupts are signals from the hardware that pause the running code). It uses the routine that Stage 3 provides for stopping the machine. Only an operator restarting the machine leaves it. The call never returns to Stage 3 in a real build. The value `JP_BOOT_TRANSITION_ACCEPTED` can be seen only by a test harness (a test program) that replaces the destination.
- `JP_RECOVERY_RETRY`: Milestone 0 cannot restart the platform and cannot remember a retry count, so the result is `JP_BOOT_TRANSITION_TERMINAL`.
- `JP_RECOVERY_TERMINAL_HALT`: the result is `JP_BOOT_TRANSITION_TERMINAL`.
- `JP_BOOT_TRANSITION_REJECTED` is returned, without changing Stage 3's records, when an argument is null, the decision is not a defined value, the manager state is not `DECIDED`, the request was already used up, or finalizing the Boot Report fails.

Stage 3 turns `REJECTED` and `TERMINAL` into its own way of stopping the machine. Stage 3 depends on Stage 4 only through this one function.

## 4 Freeze and Kernel Handoff Contract

When the decision is `CONTINUE_TO_KERNEL`, `jp_kernel_stage2_complete()` does these steps, in this order:

```text
check that the Stage 3 records and the configuration agree
    ↓
jp_boot_info_summarize() and show the summary (if boot_logging is true)
    ↓
jp_boot_report_finalize(context)      (the Boot Report becomes read-only)
    ↓
jp_boot_config_freeze(context)        (the configuration is locked)
    ↓
kernel_main(context)
```

The decision `CONTINUE_TO_KERNEL` was recorded before these steps. The summary is shown before the report is finalized, so that finalizing is the last thing that touches the diagnostic records.

If finalizing the report or freezing fails, the failure is recorded as a Stage 3 error with the source `JP_BOOT_SOURCE_BOOT_MANAGER`, and the decision is worked out once more as a failure. The kernel is never started after a failed finalization or with a configuration that is not frozen.

The kernel receives the same `JPBootContext *`. It reads the frozen configuration with `jp_boot_config_get()` on `context->stage4.config`, and the configuration is read-only by contract. It reads the Boot Report with `jp_boot_report_get()`. After `kernel_main(context)` starts, the next running part of the system owns the context.

`kernel_main` is the kernel's entry function. It is defined outside Milestone 0. Stage 4 calls it with this declaration:

```c
_Noreturn void kernel_main(JPBootContext *context);
```

(`_Noreturn` means the function never gives control back to its caller.) Tests for Milestone 0 provide a test version of `kernel_main`.

## 4 Stage 4 Algorithms and State Rules

`jp_boot_manager_prepare(context)` runs in phase A, before `jp_boot_prepare_handoff()`:

```text
check the context; the manager state must be UNINITIALIZED
    ↓
jp_boot_config_initialize(context)          state VALIDATED, source DEFAULT
    ↓
selection_outcome = NOT_RUN; decision = NONE
    ↓
jp_boot_register_interaction(context, jp_boot_manager_select, NULL)
    ↓
manager state = PREPARED
```

If any step fails, the manager state stays `UNINITIALIZED`, no function is registered, and the failing result is returned unchanged. Phase B treats an `UNINITIALIZED` manager or an `UNSET` configuration as a Stage 4 failure.

`jp_boot_manager_select(context, user_data)` is the function that Stage 1 calls at its interaction point (a "hook"). Its type matches `JPBootInteractionHook`, and `user_data` is ignored.

```text
the manager state must be PREPARED, otherwise return JP_BOOT_FAILED
    ↓
policy is DEFAULT_ONLY?          → outcome SKIPPED_POLICY, return JP_BOOT_SUCCESS
no usable framebuffer?           → outcome SKIPPED_NO_FRAMEBUFFER, return JP_BOOT_SUCCESS
the keyboard is NOT_DETECTED?    → outcome SKIPPED_NO_INPUT, return JP_BOOT_SUCCESS
    ↓
jp_boot_ui_select(context, &outcome)
    ├── failure → return the failing result (the handoff then fails)
    └── success → selection_outcome = APPLIED or DISCARDED
    ↓
manager state = SELECTED; return JP_BOOT_SUCCESS
```

To find out whether a keyboard exists, the function makes one non-blocking call to `jp_boot_environment_read_input()`. The answer `JP_BOOT_SUCCESS` with `JP_BOOT_INPUT_NONE` means that a keyboard exists and no key is waiting. The answers `JP_BOOT_NOT_INITIALIZED` and `JP_BOOT_FAILED` mean the function fails. A skipped menu still sets the manager state to `SELECTED`.

`jp_kernel_stage2_complete(context)` runs in phase B:

1. Check the context. A null context cannot hold any state, so the function stops the machine with the Stage 3 stopping routine.
2. If the manager state is `UNINITIALIZED`, `PREPARED`, or `SELECTED`, set it to `COMPLETING`. (`UNINITIALIZED` becomes a Stage 4 failure in step 5, because the configuration is `UNSET`.) A second entry (state `COMPLETING`, `DECIDED`, or `RECOVERY_REQUESTED`) is not allowed: the function stops the machine with the Stage 3 stopping routine and changes no state.
3. Start Stage 3: call `jp_boot_log_initialize()`, then `jp_boot_status_initialize()`. If either fails, nothing can be recorded, so the function stops the machine with the Stage 3 stopping routine.
4. Add events in this order: the result recorded by Stage 2 (`context->stage2_result`, source `JP_BOOT_SOURCE_STAGE2`), then the menu outcome and the configuration state and source (source `JP_BOOT_SOURCE_BOOT_MANAGER`, stage `JP_DIAGNOSTIC_STAGE_STAGE4`).
5. Update the status. It starts as `BOOTING`. First ask for `JP_BOOT_STATUS_INITIALIZING` with the tag `JP_DIAGNOSTIC_STAGE_STAGE2` (a tag is a label that says which stage a record is about; Stage 4 does this on behalf of Stage 2, because the status module was not yet running while Stage 2 worked). Then:
   - if the Stage 2 result is `JP_BOOT_SUCCESS`, the configuration is not `UNSET`, and the manager state before step 2 was `PREPARED` or `SELECTED`, ask for `JP_BOOT_STATUS_RUNNING` with the tag `JP_DIAGNOSTIC_STAGE_STAGE3`;
   - otherwise create a Stage 3 error from what went wrong (the Stage 2 result, or `JP_BOOT_NOT_INITIALIZED` with the source `JP_BOOT_SOURCE_BOOT_MANAGER` for a Stage 4 failure) and ask for `JP_BOOT_STATUS_FAILED` with the tag of the stage that failed (`JP_DIAGNOSTIC_STAGE_STAGE2` for a Stage 2 result, `JP_DIAGNOSTIC_STAGE_STAGE4` for a Stage 4 failure).
6. Call `jp_boot_manager_decide()` and record the decision (state `DECIDED`).
7. Act on the decision:
   - `CONTINUE_TO_KERNEL`: do the steps of the Freeze and Kernel Handoff Contract.
   - `ENTER_RECOVERY`, `RETRY_BOOT`, `TERMINAL_HALT`: call `jp_boot_recovery_enter(context, decision)` with the matching `JPRecoveryDecision`. This call never returns.

When recovery was asked for through the configuration (no failure was recorded), step 7 calls `jp_boot_recovery_enter(context, JP_RECOVERY_ENTER)`, and Stage 3's recovery code then reaches `jp_boot_manager_request_recovery()`.

The manager states `DECIDED` and `RECOVERY_REQUESTED` are never left. No Stage 4 function turns interrupts on.

## 4 Ownership and Lifetime

"Owner" means the part that is allowed to change a piece of data. "Lifetime" is how long the data exists.

| Data | Made by | Used by | Owner | Lifetime | Who may change it |
| ---- | ------- | ------- | ----- | -------- | ----------------- |
| `JPBootConfig` | `boot_config`, the menu | the kernel, Stage 3 reports, `boot_info` | the Stage 4 area of the context | as long as the context | only `boot_config`, and only until `FROZEN` |
| Menu outcome | `boot_manager` | `boot_manager`, `boot_info` | the Stage 4 area | as long as the context | only `boot_manager` (the menu function) |
| Manager state and decision | `boot_manager` | `boot_info`, Stage 3 integration | the Stage 4 area | as long as the context | only `boot_manager` |
| `JPBootSummary` | `boot_info` | `boot_ui`, `boot_manager` | the caller (a copy) | one call | nobody; it is a copy |
| Registered function (hook) | `jp_boot_register_interaction()` | Stage 1 | the context (Stage 1's interaction state) | until the handoff | Stage 1 |

Stage 4 never owns or changes Stage 1 or Stage 2 memory, the page tables (the tables that map memory addresses), the kernel stack, the heap ownership, the kernel image, who owns the framebuffer, or the records of what Stage 1 found. It changes Stage 3 data only through Stage 3's own functions.

## 4 Stage 4 Execution Flow

Phase A:

```text
jp_boot_manager_prepare()  →  jp_boot_prepare_handoff()  →  [interaction point]
    →  jp_boot_manager_select()  →  platform handoff preparation  →  platform transition
```

Phase B, when everything worked:

```text
jp_kernel_stage2_complete  →  Stage 3 starts  →  status RUNNING  →  decide
    →  show summary (if boot_logging)  →  finalize report  →  freeze configuration  →  kernel_main
```

Phase B, when Stage 2 failed:

```text
jp_kernel_stage2_complete  →  Stage 3 starts  →  error recorded, status FAILED
    →  decide  →  jp_boot_recovery_enter  →  jp_boot_manager_request_recovery
        ├── ENTER    →  recovery diagnostics state (never returns)
        └── other    →  TERMINAL → Stage 3 stops the machine
```

Phase B, when recovery was asked for through the configuration:

```text
jp_kernel_stage2_complete  →  Stage 3 starts  →  status RUNNING  →  decide (ENTER_RECOVERY)
    →  jp_boot_recovery_enter(JP_RECOVERY_ENTER)  →  jp_boot_manager_request_recovery
    →  recovery diagnostics state (never returns)
```

## 4 Stage 4 → Kernel Boundary

At the moment control passes to `kernel_main(context)`, all of this is true:

- Stage 2 finished successfully and its data is unchanged;
- Stage 3's records are finalized, and the Boot Report is published read-only;
- `context->stage4.config` is `FROZEN`;
- the context pointer is the very same record that was received at the Stage 1 handoff;
- interrupts are still turned off, exactly as at the Stage 2 completion function (on x86_64, `IF = 0`; on AArch64, `PSTATE.DAIF = 1111`). Stage 4 never changes the interrupt state or any other processor state;
- the kernel does not need any hook, any screen of Stage 4, or any other Stage 4 resource to stay available.

The kernel may read the frozen configuration and the Boot Report. It must not depend on any Stage 4 function remaining callable.

## 4 Stage 4 Verification Matrix

This table says what must be tested and on which combination of processor family and firmware.

| What is tested | x86_64 / UEFI | x86_64 / BIOS | AArch64 / UEFI |
| -------------- | ------------- | ------------- | -------------- |
| The menu function runs at the right moment, exactly once | verified | verified | verified |
| The menu is skipped (policy, no screen, no keyboard) | verified | verified | verified |
| The menu: accepted choice, discarded choice, rejected candidate | verified | verified | verified |
| The configuration states and the checks of a candidate | verified | verified | verified |
| Freezing, and the read-only functions | verified | verified | verified |
| The summary shows each state exactly as its owner reported it | verified | verified | verified |
| The decision rules | verified | verified | verified |
| Recovery request: enter, retry, terminal halt, rejected | verified | verified | verified |
| A Stage 2 failure reaches the completion function | verified | verified | verified |
| State of the machine at `kernel_main` | `IF = 0` | `IF = 0` | `PSTATE.DAIF = 1111` |

AArch64 with BIOS is not supported. It is rejected before Stage 4 is reached.

## 4 File Specifications

Each Stage 4 file is described with the same sections as the files of Stages 1, 2, and 3. In these sections:

- the **public functions** are the functions that other code may call (the section called "Public API" lists them; API means the list of functions a file offers to others);
- the **internal functions** are helpers that only the file itself may call (listed under "Internal API");
- **Constants / Enums** lists fixed values and "enums", which are lists of named values;
- an **invariant** is a rule that must always be true;
- **implementation freedom** lists the details that the programmer may choose freely, because they do not change what the rest of the system can see;
- **verification** says what must be tested.

# 1. `include/justos/boot/boot_config.h`

## File

The header file that describes the boot configuration.

## Purpose

To define `JPBootConfig` and the functions that set it up, check it, apply a new one, freeze it, and read it.

## Responsibilities

To define the structure of the configuration, its constants, the lists of named values for its source and state, and the functions that work with it. To define when a configuration is valid.

## Types / Structures

`JPBootConfigEntry` and `JPBootConfig`, exactly as given in the Boot Configuration Contract.

## Constants / Enums

The constants `JP_BOOT_CONFIG_KERNEL_PARAMETERS_CAPACITY`, `JP_BOOT_CONFIG_ENTRY_CAPACITY`, `JP_BOOT_CONFIG_ENTRY_ID_CAPACITY`, and `JP_BOOT_CONFIG_ENTRY_PARAMETERS_CAPACITY`, and the lists of named values `JPBootConfigSource` and `JPBootConfigState`.

## Public API

```c
JPBootResult jp_boot_config_initialize(JPBootContext *context);
JPBootResult jp_boot_config_apply(JPBootContext *context, const JPBootConfig *candidate);
JPBootResult jp_boot_config_freeze(JPBootContext *context);
JPBootResult jp_boot_config_get(const JPBootContext *context, JPBootConfig *config);
```

## Internal API

None. The header offers no internal functions.

## Algorithm

Four functions work on the configuration that is stored in the `stage4` area of the context.

- `initialize` writes the default configuration and moves the state from `UNSET` to `VALIDATED`.
- `apply` checks a candidate against the rules of validity in this order: the kernel parameters, then the entry count, then each used entry (its name, its text, and whether the name is unique), then the unused entries. The first rule that fails rejects the whole candidate. A valid candidate replaces the editable contents of the active configuration, and the source becomes `INTERACTIVE`.
- `freeze` moves the state from `VALIDATED` to `FROZEN`. Calling it again changes nothing.
- `get` copies the active configuration to the caller.

## State / Lifecycle

The state goes `UNSET → VALIDATED → FROZEN`.

- `initialize` is allowed only in `UNSET`.
- `apply` is allowed only in `VALIDATED`.
- `freeze` is allowed in `VALIDATED`, and in `FROZEN` it changes nothing.
- A call that is not allowed fails and changes nothing.

## Ownership / Lifetime

The configuration lives in the `stage4` area of the context for as long as the context exists. A caller lends a candidate to the function for the duration of the call and remains its owner.

## Invariants

- A `FROZEN` configuration never changes.
- `entry_count` is never larger than `JP_BOOT_CONFIG_ENTRY_CAPACITY`.
- Every text ends with a NUL byte inside its capacity.
- A rejected candidate leaves the active configuration exactly as it was.

## Dependencies

It needs `boot_types.h` (for `JPBootContext` and `JPBootResult`) and the standard C headers. It has no dependency on UEFI, BIOS, or Stage 3.

## Boundary Rules

The module does not understand the entries, does no input or output, does not save anything, and does not read any other Stage 4 data.

## Failure Semantics

- A null argument returns `JP_BOOT_ERROR_INVALID_PARAM`.
- A candidate that is not valid returns `JP_BOOT_ERROR_INVALID_PARAM`.
- A call in a state where it is not allowed returns `JP_BOOT_FAILED`.
- `get` while the state is `UNSET` returns `JP_BOOT_NOT_INITIALIZED`.
- No failure changes any state.

## Implementation Freedom

The helper functions that check a candidate and handle texts are free, as long as the rules of validity, the state moves, and the "all or nothing" behavior are kept.

## Verification

Check the default values, every rule of validity, a successful apply and a rejected apply, that freezing twice changes nothing, that apply is refused after freezing, `get` in every state, and that a failed call changes nothing.

# 2. `src/boot/boot_config.c`

## File

The implementation of the boot configuration.

## Purpose

To implement the Boot Configuration Contract.

## Responsibilities

To check, apply, freeze, and read the configuration that is stored in the `stage4` area.

## Types / Structures

Private helper types only.

## Constants / Enums

It uses the constants of `boot_config.h`.

## Public API

It implements exactly the functions declared in `boot_config.h`, and no others.

## Internal API

Private helper functions that check a candidate and check a text.

## Algorithm

`jp_boot_config_initialize(context)`:

1. A null `context` returns `JP_BOOT_ERROR_INVALID_PARAM`.
2. A configuration state other than `UNSET` returns `JP_BOOT_FAILED`.
3. Write the defaults: state `VALIDATED`, source `DEFAULT`, `kernel_parameters` empty, `boot_logging` true, `verbose_output` false, `safe_mode` false, `recovery_requested` false, `entry_count` zero, and all entries set to zero.

`jp_boot_config_apply(context, candidate)`:

1. A null `context` or `candidate` returns `JP_BOOT_ERROR_INVALID_PARAM`.
2. A configuration state other than `VALIDATED` (that is, `UNSET` or `FROZEN`) returns `JP_BOOT_FAILED`.
3. Check the candidate without touching the active configuration: (a) `kernel_parameters` ends with a NUL byte inside its capacity and has only printable ASCII characters; (b) `entry_count` is not larger than the entry capacity; (c) every used entry has a non-empty `id` that ends with a NUL byte, has only printable ASCII characters without spaces, and differs from every earlier entry, and has a `parameters` text that ends with a NUL byte and has only printable ASCII characters; (d) every entry from `entry_count` onward is entirely zero. The first rule that fails returns `JP_BOOT_ERROR_INVALID_PARAM`, and nothing is written.
4. Copy `kernel_parameters`, the four switches, `entry_count`, and the entries from the candidate into the active configuration, and set the source to `INTERACTIVE`. The `state` and `source` fields of the candidate are ignored, and the state stays `VALIDATED`.

`jp_boot_config_freeze(context)`:

1. A null `context` returns `JP_BOOT_ERROR_INVALID_PARAM`.
2. The state `UNSET` returns `JP_BOOT_FAILED`.
3. The state `FROZEN` returns `JP_BOOT_SUCCESS` and changes nothing.
4. Set the state to `FROZEN`.

`jp_boot_config_get(context, config)`:

1. A null `context` or `config` returns `JP_BOOT_ERROR_INVALID_PARAM`.
2. The state `UNSET` returns `JP_BOOT_NOT_INITIALIZED`.
3. Copy the active configuration into `config`.

## State / Lifecycle

The state changes only as shown in this table. Any other combination fails and leaves the state unchanged.

| Function | Allowed when the state is | State afterwards |
| -------- | ------------------------- | ---------------- |
| `initialize` | `UNSET` | `VALIDATED` |
| `apply` | `VALIDATED` | `VALIDATED` |
| `freeze` | `VALIDATED` | `FROZEN` |
| `freeze` | `FROZEN` | `FROZEN` (no change) |
| `get` | `VALIDATED`, `FROZEN` | unchanged |

## Ownership / Lifetime

The function borrows the context for the call. It writes only the configuration inside the `stage4` area.

## Invariants

- A candidate is never applied in part.
- Nothing is written after `FROZEN`.
- The module keeps no global data that changes.

## Dependencies

`boot_config.h`.

## Boundary Rules

No heap, no firmware, no Stage 3 code, and no other Stage 4 module.

## Failure Semantics

The same as in `boot_config.h`.

## Implementation Freedom

How data is copied and compared inside the file is free.

## Verification

Run every check listed for `boot_config.h` on both processor families.

# 3. `include/justos/boot/boot_info.h`

## File

The header file that describes how information is gathered.

## Purpose

To offer a read-only copy of the state of Stages 1 to 4, and a way to turn a detection state into text.

## Responsibilities

To define `JPBootSummary` and the functions that build it and name states.

## Types / Structures

`JPBootSummary`, exactly as given in the Boot Information Aggregation Contract.

## Constants / Enums

None of its own. It uses the lists of named values of Stages 1 to 4.

## Public API

```c
JPBootResult jp_boot_info_summarize(const JPBootContext *context, JPBootSummary *summary);
const char *jp_boot_info_state_name(JPDetectionState state);
```

## Internal API

None. The header offers no internal functions.

## Algorithm

Each state is copied, without any change, from the module that owns it into the summary, and `status_available` is set from the result of `jp_boot_status_get()`. The name function returns the fixed text for a detection state: `"UNKNOWN"`, `"NOT DETECTED"`, `"NOT INITIALIZED"`, `"DETECTED"`, or `"FAILED"`. A value that is not in the list gives `"UNKNOWN"`.

## State / Lifecycle

The module keeps no state. Each call is independent.

## Ownership / Lifetime

The caller owns the summary. The summary holds no pointers into the context.

## Invariants

No state is guessed, replaced by a default, or swapped for another. A state that is not available is carried over exactly as reported.

## Dependencies

`boot_types.h`, `boot_config.h`, the types of the Stage 4 area from `boot_manager.h`, and the Stage 3 status types.

## Boundary Rules

The module does not search for anything, does not keep copies, does not change anything, and does not depend on `boot_ui.h`.

## Failure Semantics

A null argument returns `JP_BOOT_ERROR_INVALID_PARAM`. The module never reports a state that its owner did not report.

## Implementation Freedom

How the copying is done is free.

## Verification

Check that every field equals the value of its owner, including every "not available" state, and that nothing in the context is changed.

# 4. `src/boot/boot_info.c`

## File

The implementation of the information gathering.

## Purpose

To implement the Boot Information Aggregation Contract.

## Responsibilities

To build the summary and to name detection states.

## Types / Structures

None.

## Constants / Enums

A fixed table that gives each `JPDetectionState` value its display name: `UNKNOWN` is `"UNKNOWN"`, `NOT_DETECTED` is `"NOT DETECTED"`, `NOT_INITIALIZED` is `"NOT INITIALIZED"`, `DETECTED` is `"DETECTED"`, and `FAILED` is `"FAILED"`.

## Public API

It implements exactly the functions declared in `boot_info.h`.

## Internal API

None.

## Algorithm

`jp_boot_info_summarize(context, summary)`:

1. A null `context` or `summary` returns `JP_BOOT_ERROR_INVALID_PARAM`.
2. Clear the summary.
3. Copy, without any change, the architecture, the boot environment type and state, the hardware state, the memory map state, the framebuffer state, and the kernel validation and load states from the Stage 1 data in the context.
4. Call `jp_boot_status_get()`. If it returns `JP_BOOT_SUCCESS`, copy the status and set `status_available` to true. Otherwise leave the status cleared and set `status_available` to false.
5. Copy the configuration state and source and the menu outcome from the `stage4` area. A configuration that was never set up is therefore reported as `UNSET` with the source `UNKNOWN`.
6. Return `JP_BOOT_SUCCESS`.

`jp_boot_info_state_name(state)` looks the value up in the table and returns the fixed text. A value outside the table returns `"UNKNOWN"`.

## State / Lifecycle

The module keeps no state.

## Ownership / Lifetime

The function borrows the context for the call.

## Invariants

No state is invented, and nothing in the context is written.

## Dependencies

`boot_info.h`.

## Boundary Rules

No firmware, no heap, and no screen output.

## Failure Semantics

The same as in `boot_info.h`.

## Implementation Freedom

Free, as long as what other code can observe stays the same.

## Verification

Run every check listed for `boot_info.h` on both processor families.

# 5. `include/justos/boot/boot_ui.h`

## File

The header file that describes the boot screens.

## Purpose

To define the menu of phase A and the text screens of phase B.

## Responsibilities

To define the screens, the keys, and the functions that show information, as written in the Boot UI Contract.

## Types / Structures

None of its own. It uses types from other files.

## Constants / Enums

None.

## Public API

```c
JPBootResult jp_boot_ui_select(JPBootContext *context, JPBootSelectionOutcome *outcome);
JPBootResult jp_boot_ui_present_summary(JPBootContext *context, const JPBootSummary *summary);
JPBootResult jp_boot_ui_present_failure(JPBootContext *context);
```

## Internal API

None. The header offers no internal functions.

## Algorithm

`jp_boot_ui_select` (phase A):

1. Check that a menu is possible: the manager state is `PREPARED` and the framebuffer is valid.
2. Make a private copy of the active configuration. This copy is the candidate that the user edits. The active configuration is not touched until the user presses F10.
3. Repeat: draw the screen, read one input event, and act on it as the key table says.
4. Stop when the user presses F10 (submit) or Esc (discard).

The keys are listed in the Boot UI Contract. In short: Up and Down move through the five fields and wrap around at both ends; Left, Right, and Enter change a switch; Enter on the kernel parameters starts editing and Enter during editing stops it; characters and Backspace edit the text while editing; F10 submits and Esc discards at any time. A message about a rejected candidate stays until the next key press; that key press removes it and is then handled as usual.

The screen shows the editable fields with their current values, the system information from `boot_info`, a help line with the keys, and a message line that reports a rejected candidate.

`jp_boot_ui_present_summary` and `jp_boot_ui_present_failure` (phase B) turn data into lines of text and send each line through `jp_boot_diagnostic_output()`. The summary lists every field of `JPBootSummary`, and writes detection states with `jp_boot_info_state_name()`. The failure screen lists the status, the recorded error, and, when `verbose_output` is set, every recorded event.

## State / Lifecycle

`jp_boot_ui_select` may be used only in phase A (manager state `PREPARED`). The two text functions may be used only in phase B.

## Ownership / Lifetime

The screens borrow the context and keep no data between calls. The candidate that is being edited exists only inside one call.

## Invariants

- The menu changes the configuration only through `boot_config`.
- It writes only into the checked framebuffer region.
- It never waits inside the firmware, and it uses no timer.

## Dependencies

`boot_info.h`, `boot_config.h`, the types from `boot_manager.h`, `diagnostic_output.h`, and the Stage 1 definitions of keyboard input and of the framebuffer.

## Boundary Rules

The module uses no firmware except the keyboard input that Stage 1 provides. In phase B it draws nothing directly. It allocates no memory.

## Failure Semantics

`jp_boot_ui_select` returns `JP_BOOT_NOT_DETECTED` when there is no valid framebuffer. Otherwise it returns the failing result of the keyboard read unchanged. A failure never changes the configuration. The text functions return the result of the text output unchanged and never change the decision.

## Implementation Freedom

The layout, the colors, and the shape of the font are free, as long as the key table, the five fields, and the rules for unavailable states are kept.

## Verification

Check every key (including wrapping with Up and Down, starting and stopping editing with Enter, a character when the text is full, Backspace on empty text, and F10 and Esc during editing), the accepted and discarded paths, the display of a rejection and its removal at the next key, a missing framebuffer, a missing keyboard, the framebuffer limits, and the text screens of phase B through the text output function.

# 6. `src/boot/boot_ui.c`

## File

The implementation of the boot screens.

## Purpose

To implement the Boot UI Contract.

## Responsibilities

To draw the menu into the framebuffer, to handle the keys, and to show the phase B text through the text output function.

## Types / Structures

Private data that exists only during one call of `jp_boot_ui_select`: the number of the current field, a flag that says whether the kernel parameters are being edited, the candidate configuration, and the current message.

## Constants / Enums

The built-in font (one picture for each printable ASCII character) and the layout values (where each field goes, the size of a character cell, and the colors), all stored as read-only data.

## Public API

It implements exactly the functions declared in `boot_ui.h`.

## Internal API

Private helper functions for drawing, for the font, for moving between fields, and for editing the candidate.

## Algorithm

`jp_boot_ui_select(context, outcome)`:

1. A null argument returns `JP_BOOT_ERROR_INVALID_PARAM`. A manager state other than `PREPARED` returns `JP_BOOT_FAILED`.
2. A framebuffer that is not valid returns `JP_BOOT_NOT_DETECTED`, and nothing is changed.
3. Read the active configuration into the candidate with `jp_boot_config_get()`, and build a summary with `jp_boot_info_summarize()` for the information panel. Set the current field to the first field, editing to off, and the message to empty.
4. Repeat:
   - draw the whole screen into the framebuffer with the built-in font, writing only inside the framebuffer region (`height` lines of `pitch` bytes each, starting at `base_address`);
   - read one event with `jp_boot_environment_read_input()`. Any result other than `JP_BOOT_SUCCESS` is returned unchanged, and the configuration stays as it was;
   - if the event is of kind `NONE`, ask again;
   - otherwise, if a message is shown, remove it, and then handle the event as the key table in the Boot UI Contract says;
   - on F10, call `jp_boot_config_apply()` with the candidate. If it succeeds, set `*outcome` to `APPLIED` and return `JP_BOOT_SUCCESS`. If it returns `JP_BOOT_ERROR_INVALID_PARAM`, put a rejection text in the message and keep repeating;
   - on Esc, set `*outcome` to `DISCARDED` and return `JP_BOOT_SUCCESS`.

`jp_boot_ui_present_summary(context, summary)` and `jp_boot_ui_present_failure(context)` build one `JPBootDiagnosticMessage` for each line and call `jp_boot_diagnostic_output()` for every one. They try all lines and then return the first failing result, if any. A failure to show text never changes the decision.

## State / Lifecycle

The module keeps no state between calls.

## Ownership / Lifetime

The functions borrow the context and the framebuffer for the duration of the call.

## Invariants

No pixel is written outside the checked framebuffer region. The active configuration changes only through `boot_config`.

## Dependencies

`boot_ui.h`.

## Boundary Rules

No global data that changes, no heap, no firmware, and no Stage 3 code except the text output function.

## Failure Semantics

The same as in `boot_ui.h`.

## Implementation Freedom

How drawing and editing are done inside the file is free.

## Verification

Run every check listed for `boot_ui.h` on both processor families.

# 7. `include/justos/boot/boot_manager.h`

## File

The header file that describes the Boot Manager.

## Purpose

To define the part that runs Stage 4, the Stage 4 area, the decision, and the part that accepts recovery requests.

## Responsibilities

To declare the Stage 4 types, the functions that set up and select, the function that decides, and the recovery request.

## Types / Structures

`JPBootStage4Area`.

## Constants / Enums

The lists of named values `JPBootSelectionOutcome`, `JPBootManagerState`, `JPBootDecision`, `JPBootSelectionPolicy`, and `JPBootTransitionResult`, and the build-time constant `JP_BOOT_SELECTION_POLICY`.

## Public API

```c
JPBootResult jp_boot_manager_prepare(JPBootContext *context);
JPBootResult jp_boot_manager_select(JPBootContext *context, void *user_data);
JPBootResult jp_boot_manager_decide(const JPBootContext *context, JPBootDecision *decision);
JPBootTransitionResult jp_boot_manager_request_recovery(
    JPBootContext *context,
    const JPBootRecoveryState *recovery
);
_Noreturn void kernel_main(JPBootContext *context);
```

`kernel_main` is declared here only because Stage 4 calls it; the kernel defines it. `jp_kernel_stage2_complete()` is declared by Stage 2 in `init.h` and implemented in `boot_manager.c`.

## Internal API

None. The header offers no internal functions.

## Algorithm

The four functions of the Boot Manager:

- `jp_boot_manager_prepare` sets up the default configuration and registers the menu function with Stage 1. It runs in phase A, before the handoff.
- `jp_boot_manager_select` is the function that Stage 1 calls. It applies the selection policy, checks that a framebuffer and a keyboard exist, and runs the menu. It runs in phase A, at the interaction point.
- `jp_boot_manager_decide` works out the decision from the Stage 2 result, the Stage 3 status, and the configuration.
- `jp_boot_manager_request_recovery` accepts the recovery request from Stage 3. It either enters the recovery diagnostics state or reports that no safe destination exists.

The step-by-step sequences are in Stage 4 Algorithms and State Rules and in the Recovery Transition Owner Contract.

## State / Lifecycle

The state goes `UNINITIALIZED → PREPARED → SELECTED → COMPLETING → DECIDED`. From `DECIDED`, an accepted recovery request moves it to `RECOVERY_REQUESTED`, which is final.

## Ownership / Lifetime

The Boot Manager owns the manager state, the menu outcome, and the decision. They are stored in the `stage4` area and live as long as the context.

## Invariants

- The menu function runs at most once.
- The decision is recorded once.
- A recovery request is used up at most once.
- The kernel is never started unless the report is finalized and the configuration is `FROZEN`.

## Dependencies

`boot_config.h`, `boot_info.h`, `boot_ui.h`, the Stage 3 headers (`boot_log.h`, `boot_status.h`, `error.h`, `recovery.h`, `boot_report.h`, `diagnostic_output.h`), and `init.h`.

## Boundary Rules

The Boot Manager hands each piece of work to the module that owns it. It does not search for hardware, load kernels, manage memory, or understand the configuration entries, and it never turns interrupts on.

## Failure Semantics

`prepare` and `select` return the failing result unchanged. The completion function records every failure through Stage 3 error handling and never starts the kernel after a failure. A recovery request returns `REJECTED` or `TERMINAL` as the Recovery Transition Owner Contract says.

## Implementation Freedom

The helper functions that order the work are free, as long as the order and the state moves described in this document are kept.

## Verification

Check `prepare`, `select` (every outcome), `decide` (every rule), the recovery request (every decision and every reason for rejection), and the manager's state moves.

# 8. `src/boot/boot_manager.c`

## File

The implementation of the Boot Manager.

## Purpose

To implement the running of Stage 4, `jp_kernel_stage2_complete()`, the decision, and the acceptance of recovery requests.

## Responsibilities

To run the menu in phase A, to provide the completion function for both success and failure, to decide, to accept recovery requests, to freeze the configuration, and to start the kernel.

## Types / Structures

None.

## Constants / Enums

It uses the lists of named values of `boot_manager.h`.

## Public API

It implements exactly the functions declared in `boot_manager.h`, and it implements `jp_kernel_stage2_complete()`, which is declared in `init.h`.

## Internal API

Private helper functions for acting on a decision, for recording events, and for turning a failure into a Stage 3 error.

## Algorithm

The numbered sequences of `jp_boot_manager_prepare`, `jp_boot_manager_select`, and `jp_kernel_stage2_complete` are in Stage 4 Algorithms and State Rules. The other functions work as follows.

`jp_boot_manager_decide(context, decision)` returns the first decision whose condition matches:

| Condition | Decision |
| --------- | -------- |
| the Stage 2 result is not `JP_BOOT_SUCCESS`, or the Stage 3 status is `FAILED`, `RECOVERY`, or `HALTED` | the Stage 3 recovery decision, converted one to one |
| `recovery_requested` is true | `ENTER_RECOVERY` |
| none of the above | `CONTINUE_TO_KERNEL` |

`jp_boot_manager_request_recovery(context, recovery)`:

1. A null argument, a decision that is not a defined value, or a manager state other than `DECIDED` returns `JP_BOOT_TRANSITION_REJECTED`, and nothing is changed.
2. For `JP_RECOVERY_RETRY` and `JP_RECOVERY_TERMINAL_HALT`, return `JP_BOOT_TRANSITION_TERMINAL`.
3. For `JP_RECOVERY_ENTER`: set the manager state to `RECOVERY_REQUESTED`; ask for the status `RECOVERY`; show the failure and recovery diagnostics; finalize the Boot Report (if this fails, return `JP_BOOT_TRANSITION_REJECTED`); freeze the configuration unless it is `UNSET`; and then enter the recovery diagnostics state, which does not return in a real build.

## State / Lifecycle

The manager state changes only as shown in this table. Any other combination leaves the state unchanged.

| Call | Allowed when the state is | State afterwards | Otherwise |
| ---- | ------------------------- | ---------------- | --------- |
| `jp_boot_manager_prepare` | `UNINITIALIZED` | `PREPARED` | failure; state unchanged |
| `jp_boot_manager_select` | `PREPARED` | `SELECTED` | `JP_BOOT_FAILED`; state unchanged |
| `jp_kernel_stage2_complete` (first entry) | `UNINITIALIZED`, `PREPARED`, `SELECTED` | `COMPLETING` | second entry: the machine is stopped with the Stage 3 stopping routine |
| decision recorded | `COMPLETING` | `DECIDED` | cannot happen in any other state |
| `jp_boot_manager_request_recovery` (`ENTER`) | `DECIDED` | `RECOVERY_REQUESTED` | `JP_BOOT_TRANSITION_REJECTED` |

`DECIDED` and `RECOVERY_REQUESTED` are never left.

## Ownership / Lifetime

The functions borrow the context. They write only the `stage4` area, and the Stage 3 data through Stage 3's own functions.

## Invariants

The kernel is never started after a failed finalization or with an unfrozen configuration. The decision is never computed again. Interrupts are never turned on.

## Dependencies

`boot_manager.h`.

## Boundary Rules

In phase B there is no firmware, keyboard, or timer use. Phase A uses only the keyboard input that Stage 1 provides, through the registered function.

## Failure Semantics

The same as in `boot_manager.h`.

## Implementation Freedom

How the helper functions are split is free, as long as the order of the steps is kept.

## Verification

Run every check listed for `boot_manager.h` on both processor families. The tests need a test version of `kernel_main` and a test harness that replaces the recovery diagnostics state.

For every file above, the rules are the same on both processor families. Every public function needs a description of success and failure and tests for x86_64 and for AArch64.

## 4 Specification Lock Criteria

This list is the checklist for the Stage 4 lock. Each line must be fully described in this document.

- [ ] What Stage 4 does and does not do is stated: boot configuration, boot screens, collecting information, the decision, the recovery request, freezing, and starting the kernel.
- [ ] The behavior on x86_64 and on AArch64 is stated.
- [ ] The three ways Stage 4 is entered are stated.
- [ ] The Stage 4 area, who owns it, and how long it lives are stated.
- [ ] `JPBootConfig`, its default values, its rules of validity, its states, and what happens when it is missing or invalid are stated.
- [ ] The lists of named values for the menu outcome, the manager state, and the decision are stated.
- [ ] The selection policy, and the fact that it is chosen when the program is built, are stated.
- [ ] The phase A menu (framebuffer, keyboard, keys, accepting and discarding) and the phase B text screens are stated.
- [ ] The information gathering keeps "not available" states as they are, and defines no `JPBootInfo`.
- [ ] The decision rules and their one-to-one link to the Stage 3 recovery decisions are stated.
- [ ] The part that accepts recovery requests, its results, and the rule that a request is used up once are stated.
- [ ] The order of freezing and the handoff to the kernel are stated.
- [ ] What `jp_kernel_stage2_complete()` does after success and after failure is stated.
- [ ] The boundary between Stage 4 and the kernel, and the processor state at that moment, are stated.
- [ ] Every Stage 4 source file has its own description.
- [ ] The things left for later milestones are named and excluded.
- [ ] No Stage 4 module owns resources of Stage 1, Stage 2, or Stage 3.

## 4 Specification Traceability

Traceability means that every requirement can be found in one place of this document and has one way of being checked.

| Requirement | Where it is described | How it is checked |
| ----------- | --------------------- | ----------------- |
| Configuration | Boot Configuration Contract | Configuration tests |
| Menu function | Stage 4 Algorithms and State Rules | Menu function tests |
| Selection policy | Selection Policy | Policy tests |
| Boot screens | Boot UI Contract | Screen tests |
| Collecting information | Boot Information Aggregation Contract | Information tests |
| Decision | Boot Manager Decision Contract | Decision tests |
| Recovery request | Recovery Transition Owner Contract | Recovery request tests |
| Freezing and handing over to the kernel | Freeze and Kernel Handoff Contract | Freeze tests |
| Entry from Stage 2 | Stage 4 Algorithms and State Rules | Integration tests |
| The Stage 4 area | Stage 4 Area of JPBootContext | Ownership tests |
| The kernel boundary | Stage 4 → Kernel Boundary | Boundary tests |

## 4 Specification Completeness Verification

The Stage 4 specification is complete only when every lock criterion is described in this document, every source file has its own description, both processor families have behavior that can be observed, every failure behaves in a fixed way, and no part needs a later stage or a capability that was left for later.

The result is written down as:

```text
Specification Completeness Verification: PASS
```

or:

```text
Specification Completeness Verification: FAIL
```

Recorded result (2026-11-15):

```text
Specification Completeness Verification: PASS
```

## 4 Specification Lock Decision

Stage 4 receives its Specification Lock only when:

- all Stage 4 lock criteria pass;
- traceability is complete;
- the completeness verification is `PASS`;
- the behavior on x86_64 and on AArch64 can be checked separately;
- the Stage 1 interaction point, the Stage 2 completion function, the Stage 3 recovery boundary, and the Stage 4 contracts agree with each other;
- Stage 4 depends on no capability that this document leaves for later.

Otherwise:

```text
SPECIFICATION LOCK = REJECTED
```

Recorded decision (2026-11-15):

```text
SPECIFICATION LOCK = APPROVED
```

This decision locks the Stage 4 specification only. It does not say that the code is finished and tested, and it does not replace the lock decisions of Stages 1 to 3.

## 4 Implementation Acceptance

Implementation Acceptance is separate from the Specification Lock. It answers a different question: does the finished code behave as this document says? It requires:

- the default configuration, the checks, applying, freezing, and the read-only behavior are tested;
- the menu function, the skipped menu, the interactive menu, and a rejected candidate are tested on each supported firmware layer;
- both selection policies, `DEFAULT_ONLY` and `INTERACTIVE`, are tested;
- the summary keeps every state as its owner reported it, including every "not available" state;
- the decision rules are tested;
- the recovery request (enter, retry, terminal halt, and rejected) is tested with a test harness;
- the paths for success and for failure of Stage 2 through `jp_kernel_stage2_complete()` are tested;
- freezing before the kernel starts, and refusing to start the kernel after a failure, are tested;
- the state of the machine at `kernel_main` is tested separately for x86_64 and AArch64;
- no resource of Stage 1, Stage 2, or Stage 3 was damaged;
- no Stage 4 function is missing at link time;
- nothing that was left for later was used.

## 4 Final Lock Statement

Stage 4 — Boot manager & UI, and with it Milestone 0, is closed only after all four of these hold:

```text
Specification Completeness Verification = PASSED
Specification Traceability            = PASSED
Specification Lock Decision           = APPROVED
Implementation Acceptance             = PASSED
```

---

# Test Kernel

The **test kernel** is a small kernel that exists only to prove that the boot works. The boot hands control to its `kernel_main(context)`. It is test code: it lives in `tests/qemu/test_kernel/`, is never part of the boot code, and no production file includes it. Its data collector is its own file; the Stage 4 module `boot_info` is not changed and not used for this.

## Test Kernel Folder Structure

```text
tests/qemu/test_kernel/
├── platform.h          what the test kernel needs from the machine (declarations)
├── report.h            the data collector (declarations)
├── report.c            the data collector
├── font.h              the built-in font (declarations)
├── font8x16.c          the glyph bitmaps (generated by scripts/gen_font.py)
├── screen.h            the on-screen list (declarations)
├── screen.c            the on-screen list
├── wait.h              the 40 seconds / ESC wait (declarations)
├── wait_common.c       the waiting loop shared by both architectures
├── wait_x86_64.c       the wait on x86_64
├── wait_aarch64.c      the wait on AArch64
├── serial_x86_64.c     serial output and ending of the run on x86_64
├── serial_aarch64.c    serial output and ending of the run on AArch64
├── kernel_main.c       the entry function
├── kernel_x86_64.ld    link layout for x86_64
└── kernel_aarch64.ld   link layout for AArch64
```

## Test Kernel File Specifications

**`report.h` and `report.c` (the data collector).** The one function, `jp_test_report_emit(context, sink)`, reads the received `JPBootContext` and produces every item of the list described under "The proof that the milestone works". It follows the aggregation rule (Rule 34): it only reads, it copies nothing and keeps nothing, and it never invents a value; a value that is not available is written with the state its owner reported. It produces the list as text lines and writes each character through the `sink` (a function that the caller supplies), so the same list can go to the serial port, to the screen, or to a buffer in a host test. It also decides `PASS` or `FAIL` by the rules written under the serial protocol, and it returns that decision. It uses no libc and no heap.

**`serial_x86_64.c` and `serial_aarch64.c`.** `jp_test_serial_put(c)` writes one character to the serial port (the COM1 port at I/O port `0x3F8` on x86_64, the PL011 serial device of the QEMU `virt` machine on AArch64). `jp_test_exit(pass)` ends the run: x86_64 writes `0x10` (pass) or `0x11` (fail) to the `isa-debug-exit` device at I/O port `0xF4`; AArch64 asks QEMU to leave through semihosting (the interface that lets a guest ask the host for services) with exit code 0 or 1. If the machine does not stop, the function halts the processor forever.

**`screen.h` and `screen.c` (the on-screen list).** `jp_test_screen_show(context, pixels)` draws the same list on the framebuffer that `context->framebuffer` describes; `pixels` is the writable memory of the framebuffer (the caller turns `base_address` into a pointer; in Milestone 0 the framebuffer is reached at its physical address). It returns the number of text lines that it drew (0 if it drew nothing). It uses its own built-in font (each character is a bitmap of 8 by 16 pixels, only the printable ASCII characters), so it needs no font from any other file. The glyph bitmaps in `font8x16.c` are generated, not drawn by hand: `scripts/gen_font.py` renders the printable ASCII characters from a system TrueType font (by default DejaVu Sans Mono, which has a free license) into 16 bytes per character, one byte per pixel row, the most significant bit being the leftmost pixel. The generated file is kept in the repository, so building the test kernel needs neither Python nor the font; `scripts/gen_font.py --check` tells whether the file still matches the script. It fills the screen column by column (each column is 80 characters wide); lines that do not fit are not drawn, and the last drawn line then says how many lines were left out (they are always complete on the serial port). It writes pixels only inside the framebuffer region, in the format of the framebuffer (`RGB_888` or `BGR_888`, 4 bytes per pixel). If the framebuffer state is not `DETECTED`, nothing is drawn.

**`wait.h`, `wait_common.c`, `wait_x86_64.c`, `wait_aarch64.c` (40 seconds or ESC).** `jp_test_wait_for_escape(seconds)` waits until `seconds` seconds have passed or the person presses ESC, whichever comes first, and returns which one happened. It does not sleep: it repeatedly checks the clock and the keyboard (this is called polling). The loop itself is in `wait_common.c`: `jp_test_wait_poll(source, seconds)` takes a *source* (a clock function, the clock frequency, and an ESC-check function) and repeats these steps: if ESC is pressed, return "escape"; if the elapsed time has reached `seconds`, return "timeout"; otherwise go round again (a source with frequency 0 returns "timeout" at once, so a broken clock can never wait forever). The two architecture files only supply the source. Because the loop takes a source, it can be tested on the host with a fake clock. The clock on x86_64 is the PIT (the programmable interval timer, a chip with a counter that runs at 1,193,182 ticks per second, read through I/O ports `0x40` and `0x43`); the clock on AArch64 is the generic timer (the registers `CNTPCT_EL0` for the count and `CNTFRQ_EL0` for its frequency). ESC is detected in two ways: on x86_64 by the PS/2 keyboard controller (status port `0x64`, data port `0x60`, the "make" code `0x01` of ESC); on both architectures by the byte `0x1B` arriving at the serial port. (AArch64 has no PS/2 keyboard, and a USB keyboard would need a driver, which the test kernel does not have.)

**`kernel_main.c`.** `kernel_main(context)` does these steps in this order: (1) a null context ends the run with `FAIL`; (2) write the whole list to the serial port with `jp_test_report_emit()`; (3) draw the list on the screen with `jp_test_screen_show()`; (4) unless the kernel was built as the *automatic* variant, call `jp_test_wait_for_escape(40)`; (5) end the run with `jp_test_exit(pass)`.

**Two builds of the same sources.** The *automatic* variant is built with the macro `JP_TEST_KERNEL_NO_WAIT` and skips step 4; the automatic QEMU tests use it, because nobody presses ESC there. The *interactive* variant is built without the macro; it is the one that a person starts with `scripts/run_qemu.sh` (without `--nographic`) or in a virtual machine manager and reads.

## Test Kernel Boundary Rules

The test kernel only reads the context. It never changes `JPBootContext`. It does not use Stage 3 or Stage 4 code, the Boot Report, or the start options: lines for them are added to the list when those stages define the types that carry them (see the serial protocol). Its pixel writes stay inside the framebuffer region that the context describes.

---

# Security Integration

Security here means making sure that the system starts only what it is supposed to start, in a valid state. This section lists the security parts that this milestone adds or extends. The records come from the security documents in `docs/security/` (`security_system.md` section 8.1, `security_architecture.md`, `security_engine.md`, and `security_philosophy.md`). The right-hand column of the table says what this milestone gives.

## Boot Security

### Responsibility

Boot Security is the first layer of the Security System (called Layer 0), because it works before the kernel starts. As the security documents put it, its job is to protect the system before the kernel runs, to establish that the system is intact (its "integrity") before the operating system begins, and to create the first trusted state on which every higher security layer relies. Its one authoritative responsibility is boot integrity: making sure that what is started is whole and valid.

### Provides

| What the security documents list | What Milestone 0 gives |
| -------------------------------- | ---------------------- |
| Boot Manager validation and integrity (checking that the Boot Manager itself is genuine and unchanged) | Not provided with cryptography. Stage 4 checks the boot configuration (its rules of validity, rejection of invalid input, and freezing before the kernel starts). This specification defines no check that the Boot Manager program itself is unchanged. |
| Kernel loading and kernel verification (loading the kernel and checking it) | Stage 1 loads the kernel and checks that it is well formed: the file format fields, the parts of the file that are loaded, the address where it starts, the memory it uses, the memory mappings, and that it does not overlap memory that the firmware reserved. It does not check any signature. |
| BootInfo generation and validation (building and checking the record that is handed to the kernel) | `JPBootInfo` is not defined. The kernel receives the checked context (`JPBootContext`, together with `JPHandoffInfo`) and, from Stage 4, the frozen configuration and the finalized Boot Report. |
| Boot chain integrity and verification (each step checking the next, so that a fake step is noticed) | Not provided with cryptography. When a required check fails, boot stops in a fixed way before the system is declared ready (`READY`). |
| Secure Boot integration (a firmware feature that refuses to run boot code that is not signed), optional | Not provided. The security documents list it as a planned technology. |

### Notes

- The security philosophy calls "Boot Manager modification" and "Kernel modification" critical operations. The threat model lists "boot configuration" among the configuration that needs protection. In Milestone 0 the frozen configuration is read-only only because the functions refuse to change it; the memory itself is not protected. Nothing detects that the Boot Manager or the kernel was changed.
- Checking is not the same as proving who made something. Checking that a file is well formed (structural validation) does not prove where it came from (cryptographic authentication). An everyday comparison: a passport that is correctly printed is not proof that it belongs to the person holding it. The security philosophy states that cryptography supports trust and does not replace the evaluation of trust. Milestone 0 must not claim that a kernel image is genuine unless checking signatures is specified and built separately.
- Not provided by this milestone: the Identity Engine (who or what something is), the Cryptographic Engine, Authentication (proving an identity), Authorization (deciding what an identity may do), the Protection Manager, the Index Engine, the Security Engine, the Trust Engine, Runtime Protection (watching the running system), the Response Engine, the Audit Engine (keeping a log of security events), and the Security System's Recovery Engine. Boot recovery in Stages 3 and 4 is a way of coping with a failed boot. It is not the Security System's Recovery Engine.

---

# Checklist

A checklist is a list of things to tick off when they are done. A box marked `[x]` is done; an empty box `[ ]` is not done yet.

## Documentation

- [ ] The design (Specification Lock) of Stage 1 is recorded as approved.
- [ ] The Stage 2 review list (Part 1 of [milestone0_pre_lock.md](milestone0_pre_lock.md)) is fully passed, and the design of Stage 2 is recorded as approved.
- [ ] The design of Stage 3 is recorded as approved.
- [x] The design of Stage 4 is recorded as approved (2026-11-15).
- [ ] `boot.md` agrees with this document.
- [ ] `progress.md` and `roadmap.md` are updated when the milestone is finished.

## Stage Completion

A stage counts as finished only after the Stage Verification Workflow (Rule 26 in `rules.md`): the Stage Report is written, the architecture review is passed, and the work is committed. The questions below ask, for each stage, whether the finished code does what this document says it must do. Each question is answered by running the tests of that stage on every supported combination: x86_64 with UEFI, x86_64 with BIOS, and AArch64 with UEFI. The detailed list of every function and test is in [milestone0_pre_lock.md](milestone0_pre_lock.md).

### Stage 1 — Boot Pipeline Foundation

- [ ] Stage 1 is built, reviewed, and committed.
- [ ] The system recognizes whether it was started by UEFI or by BIOS, and it refuses the combination AArch64 with BIOS.
- [ ] It finds the processor, the firmware, and the boot device, and anything it cannot find is reported as not found (it never invents a value).
- [ ] It reads the memory map, checks it, and rejects a map whose regions overlap or overflow.
- [ ] It finds and checks the screen memory (the framebuffer), and reports clearly when there is none.
- [ ] It reads the kernel file, checks that the file is well formed and made for the running processor family, and rejects a damaged or wrong file.
- [ ] It copies the parts of the kernel to the right memory, fills the empty tails with zeros, and maps each part with the correct permissions (readable, writable, executable).
- [ ] It sets up the first page tables, the kernel stack, and the processor state that the kernel needs to start.
- [ ] It calls the Stage 4 menu function exactly once at the right moment, and a failing menu stops the boot.
- [ ] It declares the system ready (`READY`) only when all of the above really happened, and it gives up the firmware correctly.

### Stage 2 — Platform Initialization

- [ ] Stage 2 is built, reviewed, and committed.
- [ ] The kernel starts with the shared record (`JPBootContext`) and with the processor in the promised state (interrupts off).
- [ ] The processor, memory, paging, stack, heap, and kernel file state are set up in this order, and each step starts only after the previous one succeeded.
- [ ] Only one part of the code owns the physical memory, and nobody else hands out memory.
- [ ] The early heap hands out memory in the promised sizes, returns nothing for a request of zero bytes, and refuses to grow.
- [ ] The first failure stops the setup, and the original error is passed on and recorded unchanged.
- [ ] When everything succeeds, Stage 2 hands over to the completion function of Stage 4.

### Stage 3 — Boot Diagnostics & Recovery

- [ ] Stage 3 is built, reviewed, and committed.
- [ ] Boot events are recorded in a log of fixed size; error records are never overwritten, and a full log follows the rule for what to drop.
- [ ] The boot status moves only along the allowed moves, and a final failure is never replaced by success.
- [ ] Errors are classified without changing the original result.
- [ ] Text goes to the screen when screen memory is valid and to the plain-text fallback otherwise, and no heap memory is needed.
- [ ] A panic is shown once, never loops, and stops the machine with interrupts off.
- [ ] The recovery decision follows the rules; a retry is refused in Milestone 0; a recovery request goes to the Boot Manager; a refusal leads to stopping the machine.
- [ ] The Boot Report is finalized once and is read-only afterwards.

### Stage 4 — Boot manager & UI

- [ ] Stage 4 is built, reviewed, and committed.
- [ ] The default configuration is valid, an invalid candidate is rejected completely, and a frozen configuration cannot be changed.
- [ ] The menu appears only when the policy is interactive and a screen and a keyboard exist; every key behaves as described; the user can accept or discard the choice.
- [ ] The summary shows each state exactly as its owner reported it, and missing information is shown as missing.
- [ ] The decision follows its rules; the recovery request behaves as described for each decision; the recovery screen shows what went wrong and then waits.
- [ ] The kernel is started only after the Boot Report is finalized and the configuration is frozen, and it receives the same shared record.

## Engineering Deliverables

- [ ] Firmware abstraction, UEFI layer, and BIOS layer (deliverables 1–3).
- [ ] Hardware, memory, and framebuffer discovery and checking (deliverables 4–6).
- [ ] Kernel file discovery, checking, loading, and mapping (deliverables 7–10).
- [ ] Early processor setup, virtual memory, kernel stack, and heap (deliverables 11–14).
- [ ] Boot diagnostics, status, errors, panic, and recovery (deliverables 15–19).
- [ ] Boot-information infrastructure, Boot Manager, Boot Manager screens, and Boot Log (deliverables 20–23).
- [ ] Kernel handoff, kernel entry stub, and Boot Report (deliverables 24–26).
- [ ] The test kernel prints the list of all received data, keeps it for 40 seconds or until ESC, and then ends the run, on x86_64 with UEFI, x86_64 with BIOS, and AArch64 with UEFI.
- [ ] The finished code passes acceptance on x86_64 with UEFI, x86_64 with BIOS, and AArch64 with UEFI; AArch64 with BIOS is rejected.
- [ ] The project builds without warnings and all tests pass.
- [ ] An annotated Git tag is created and a GitHub Release is published where applicable (Rules 24 and 25).

---

# Progress

Status: Planned.

Completion: no code has been written yet (0 of 4 stages built).

Design: the design of Stage 4 is finished and recorded as approved (2026-11-15). The designs of Stages 1, 2, and 3 are written, but their approval is not recorded in this document, and the review list of Stage 2 is still open.

Readiness: according to Rule 39, a stage whose design is not recorded as approved is not yet stable enough to build on. The checklists in this document describe what must be done and tested; they are not proof that anything has been built.

---

# Expected Result

When this milestone is finished, the computer can be switched on and the system will start on the supported machines (x86_64 with UEFI, x86_64 with BIOS, and AArch64 with UEFI):

- it identifies the machine and finds and checks its hardware, memory, and screen memory (the framebuffer);
- it loads the kernel, checks it, maps it into memory, and gives it a valid stack and a valid processor state to start from;
- it prepares the kernel's working environment (processor setup, memory, paging, stack, early heap, and the kernel file state);
- it records boot events, errors, and failures, explains them on the screen, and, when boot cannot go on, shows a diagnostic screen and waits;
- it lets the developer choose the start options, checks them, and locks them;
- it hands control to `kernel_main(context)` together with the shared record (`JPBootContext`), the locked start options, and the finalized Boot Report.

## The proof that the milestone works: the data list of the test kernel

The real measure of success of Milestone 0 is what the test kernel shows when it has started successfully. The test kernel is a small kernel that exists only to prove the boot (it provides the `kernel_main` that the boot hands control to). When `kernel_main(context)` has started, it prints **a list of all the data it has received** and keeps it on the screen so that a person can read it:

- the list is plain text, one item per line: a name and a value (for example the machine type, the boot environment, the processor information, every memory region with its start, length, and type, the framebuffer size and format, every kernel segment, the entry address, the page-table root, the stack pointer, the locked start options, and the Boot Report summary);
- an item whose value is not known is printed with the state the owner reported (`UNKNOWN`, `NOT DETECTED`, `NOT INITIALIZED`, or `FAILED`); the test kernel never prints a value that it did not receive (Rule 30);
- the list stays on the screen for **40 seconds** so that it can be read, or until the person presses **ESC**, whichever comes first;
- after that the test kernel ends the run (the program leaves): in the automatic QEMU tests the run ends with the exit code defined by the testing strategy, and on real hardware the machine stops.

If this list appears on x86_64 with UEFI, x86_64 with BIOS, and AArch64 with UEFI, and every item agrees with the machine, then the whole chain (the machine is found, the kernel is loaded, the handoff works, and the context really arrives) works.

### How an automatic test reads the list (the serial protocol)

A person reads the list on the screen. An automatic test (a QEMU test, level Q) cannot read a screen, so the test kernel writes the same list to the **serial port** (a plain text channel that QEMU can save to a file), one item per line, in this exact form:

```text
JP|<key>=<value>
```

The keys, in this order (a key whose data is not available is still written, with the state the owner reported):

| Key | Value |
|-----|-------|
| `machine` | `x86_64` or `aarch64` (from `handoff.architecture`), or `UNKNOWN` |
| `environment` | `UEFI`, `BIOS`, or `UNKNOWN` |
| `current_stage` | the name of `JPBootStage`, for example `READY` |
| `stage2_result` | the name of `JPBootResult`, for example `SUCCESS` |
| `cpu.state`, `firmware.state`, `boot_device.state`, `hardware.state` | the name of `JPDetectionState` |
| `cpu.logical_cores` | decimal number |
| `memory.state` | the name of `JPDetectionState` |
| `memory.regions` | decimal number of regions |
| `memory.region.<i>` | `0x<base>,0x<length>,<type>` for region number `<i>` (counted from 0) |
| `framebuffer.state` | the name of `JPDetectionState` |
| `framebuffer.mode` | `<width>x<height>,pitch=<pitch>,<format>,base=0x<base>` (only when the framebuffer state is `DETECTED`) |
| `kernel.validation`, `kernel.load` | the name of `JPKernelValidationState` and `JPKernelLoadState` |
| `kernel.segments` | decimal number of segments |
| `kernel.segment.<i>` | `0x<virtual>,0x<physical>,0x<file size>,0x<memory size>,<flags>` where flags is three characters, `R` or `-`, `W` or `-`, `X` or `-` |
| `kernel.entry` | `0x<entry address>` |
| `handoff.state` | `UNINITIALIZED`, `PREREQUISITES_VALID`, `PLATFORM_PREPARED`, `READY`, or `FAILED` |
| `handoff.page_table_root`, `handoff.stack_pointer` | `0x<address>` |
| `handoff.paging_enabled`, `handoff.interrupts_disabled`, `handoff.ownership_transferred` | `yes` or `no` |
| `interaction.state` | the name of `JPBootInteractionState` |
| `RESULT` | `PASS` or `FAIL\|<key>` (the last line) |

Hexadecimal numbers are written in lower case after `0x`, without leading zeros. The lines for the Boot Report and the start options are added when Stage 3 and Stage 4 define the types that carry them.

`RESULT` is decided by the test kernel from the data it received, never from anything else. It is `PASS` only if all of these hold: `handoff.state` is `READY`, `current_stage` is `READY`, `stage2_result` is `SUCCESS`, `hardware.state` and `memory.state` are `DETECTED`, `kernel.load` is `LOADED`, and `handoff.paging_enabled`, `handoff.interrupts_disabled`, and `handoff.ownership_transferred` are `yes`. Otherwise it is `FAIL|<key>`, where `<key>` is the first key in the table order whose condition was not met.

**Ending the run.** After the last line the test kernel ends the run. On x86_64 it writes to the `isa-debug-exit` device (QEMU leaves with exit code 33 for `PASS` and 35 for `FAIL`); on AArch64 it uses semihosting (QEMU leaves with exit code 0 for `PASS` and 1 for `FAIL`). The start script `scripts/run_qemu.sh` turns these into one convention: exit code 0 for `PASS`, 1 for `FAIL`, 124 for a timeout (no end of run within the time limit), and 2 for any other QEMU exit.

**Checking the lines.** `tests/qemu/check_serial.sh <platform> <serial-log>` compares the log with `tests/qemu/expected/<platform>.txt`. Every line of the expectation file that is not empty and does not start with `#` is an extended regular expression: it must match at least one `JP|` line of the log. A line that starts with `!` must match none. The last `JP|` line of the log must be `JP|RESULT=PASS`. The platforms are `x86_64-uefi`, `x86_64-bios`, and `aarch64-uefi`.

**Where the run happens.** The automatic test runs QEMU in software emulation on every platform. On an x86_64 machine the x86_64 platforms can also run with KVM (hardware acceleration, the real host processor); AArch64 cannot, because KVM runs only guests of the host architecture. A person can also start the same disk image in a virtual machine manager (virt-manager) and read the data list on the screen; the recipe is in `scripts/virt_manager_vm.md`.

---

# Future Extensions

Things that are deliberately left for later milestones:

- saving the start options so that they survive a restart;
- restarting the machine automatically and trying the boot again, with a saved retry count;
- changing the start options after the system has started (compare, restart, and protection against endless restarts);
- an interactive recovery screen and repair tools;
- understanding the launch entries and running them with a service manager (a program that starts and watches other programs);
- choosing between several kernels, screen settings, a boot delay, and boot themes;
- a separate record for the kernel (`BootInfo`) that has its own maker, user, layout, owner, and lifetime;
- Secure Boot, checking the kernel with cryptography, and signed and encrypted start options;
- several boot profiles, and an automatic fallback to a working profile;
- measuring boot speed, boot telemetry (automatic reports about boots), hardware compatibility checks, and remote diagnostics;
- boot diagnostics helped by artificial intelligence;
- support for more kinds of firmware, and a live boot mode (starting from removable media without installing);
- more processor families, each through a separately approved extension.
