# JustOS Project Handoff

> **Purpose:** resume engineering work after a project pause
> **Status:** Working handoff, documentation-led
> **Last reviewed:** 2026-10-14

## Project Identity

JustOS is a documentation-driven engineering system for a complete Linux distribution/ecosystem, including boot infrastructure, kernel-side runtime, compositor/desktop foundations, services, JSL, applications, and security architecture. It is not only a desktop environment or compositor.

The project prioritizes:

- explicit contracts;
- deterministic behavior;
- explicit ownership and lifetime;
- architecture and backend independence;
- documentation before implementation;
- no fabricated success state;
- stable boundaries between stages and subsystems.

Milestone 0 is limited to boot infrastructure. It excludes the desktop environment, desktop services, JSL, applications, package management, AI services, and distribution infrastructure.

## Authority

Use this authority order when documents disagree:

```text
Project Vision
    -> Roadmap
    -> Architecture
    -> Rules
    -> Milestones
    -> Stages
    -> Technical Specifications
    -> Implementation
```

Primary Milestone 0 sources:

- [milestone0.md](milestones/milestone0/milestone0.md): normative Stage 1-3 contracts;
- [boot.md](milestones/milestone0/boot.md): high-level boot architecture and Boot Manager model;
- [milestone0_pre_lock.md](milestones/milestone0/milestone0_pre_lock.md): non-normative Milestone 0 verification gate (Stage 2 pre-lock corrections and the detailed function and test checks of all stages);
- [rules.md](../rules.md): engineering and specification authority rules;
- [project_vision.md](project_vision.md), [roadmap.md](roadmap.md), and [progress.md](progress.md): project direction and status.

When a specification is incomplete, implementation must stop at that decision. Do not silently invent architecture-significant behavior.

## Current Status

The repository is not documented as complete:

- `README.md` reports development status and Milestone 1 as the current milestone;
- `docs/progress.md` leaves Milestone 0 stages unchecked;
- `docs/roadmap.md` marks Milestone 0 as planned;
- `docs/milestones/milestone0/milestone0.md` remains planned.

Therefore no implementation, build, test, or acceptance completion should be assumed from the documentation alone. Lock checklists are requirements and verification plans unless explicitly recorded as passed by executable evidence.

The current documentation work has substantially specified Stages 1-3. Stage 3 is conditionally specification-ready, but its final lock depends on resolving the lifecycle relationship with the proposed pre-kernel Stage 4 model.

## Milestone 0 Architecture

The intended high-level system is:

```text
Firmware
    |
    v
Boot Manager / configuration
    |
    v
Stage 1: Boot Pipeline Foundation
    |
    v
Stage 2: Platform Initialization
    |
    v
Stage 3: Diagnostics / Recovery
    |
    v
Stage 4: final boot aggregation and configuration freeze
    |
    v
kernel_main(context)
```

This is the proposed final flow. The existing normative `milestone0.md` still describes Stage 2 entry followed by `kernel_main()` and Stage 3 in the initialized kernel runtime. That contradiction must be resolved before the new Stage 4 model becomes normative.

## Stage 1

Stage 1 transforms the firmware/platform environment into a validated kernel-entry environment.

It owns:

- UEFI and x86_64 BIOS providers;
- hardware, CPU, firmware, and boot-device discovery;
- memory-map acquisition, normalization, and validation;
- framebuffer acquisition and validation;
- kernel image acquisition;
- ELF64 and `PT_LOAD` validation;
- segment loading, zero-fill, mappings, and permissions;
- initial paging and kernel stack;
- required CPU entry state;
- final handoff preparation and ownership transfer.

Required targets:

```text
x86_64 / UEFI:  supported
x86_64 / BIOS:  supported
AArch64 / UEFI: supported
AArch64 / BIOS: not supported
```

The Stage 1 -> Stage 2 contract uses the same `JPBootContext *`, normalized state, `JPHandoffInfo`, and explicit ownership-transfer state. `JPBootInfo` is not a Stage 1 dependency.

Required entry state includes:

```text
x86_64:  long mode, paging enabled, valid CR3/RSP, IF = 0
AArch64: EL1, MMU/TTBR state, 4 KiB granule, 48-bit VA regime,
         valid SP, PSTATE.DAIF = 1111
```

## Stage 2

Stage 2 is kernel-side platform initialization after the Stage 1 handoff.

The required order is serial:

```text
Stage 1 handoff
    -> architecture entry stub
    -> jp_kernel_init(context)
    -> CPU
    -> Memory
    -> Paging
    -> Stack
    -> Heap
    -> ELF
    -> kernel_main(context)
```

Responsibilities are split among `cpu`, `memory`, `paging`, `stack`, `heap`, `elf`, and `init`.

Important contracts:

- `memory.c` is the sole physical-memory ownership authority;
- paging, stack, and heap consume typed ownership and do not create independent allocators;
- interrupts remain masked during early initialization;
- early heap alignment and granularity are 16 bytes;
- zero-size allocation returns `NULL`;
- Stage 2 heap growth and additional physical-page requests are forbidden;
- Stage 2 validates the already-loaded ELF state and does not load or relocate it;
- `init` stops at the first mandatory failure and propagates the original result;
- Stage 2 has no dependency on Stage 3 or Stage 4.

`kernel_main()` is the first main C-level kernel runtime entry. It does not load the kernel. Stage 1 loads/prepares the kernel, Stage 2 prepares its runtime environment, and the entry stub calls `kernel_main()` only after `jp_kernel_init()` succeeds.

## Stage 3

Stage 3 is currently specified as diagnostics and recovery around the initialized kernel runtime.

It owns:

- boot event recording;
- boot status and error normalization;
- bounded log retention;
- diagnostic output routing;
- panic presentation;
- retry accounting;
- recovery decisions;
- Boot Report generation and finalization.

It must preserve original failure results and must never fabricate success or repair Stage 1/Stage 2 state.

The Stage 3 contract includes:

- `JPDiagnosticStage` and `JPBootSourceComponent` vocabularies;
- explicit severity/recoverability override values;
- fixed `JPBootLogStorage` with capacity 256;
- a named `JPBootContext.stage3` runtime area;
- owner modules for log, status, error, diagnostic output, Boot Report, panic, and recovery;
- x86_64/AArch64 observable verification matrices;
- retry reset/preserve rules;
- diagnostic finalization and immutable report publication;
- an external recovery-transition boundary.

Stage 3 failure handling must remain usable without heap, scheduler, framebuffer, Stage 4, or user space. Non-graphical fallback is mandatory.

## Proposed Stage 4

The proposed Stage 4 model is pre-kernel and closes Milestone 0 around boot configuration and final handoff. It is described in this document's `Proposed Stage 4` section and is not yet a normative lock.

Proposed modules:

```text
include/justos/boot/
├── boot_info.h
├── boot_manager.h
└── boot_ui.h

src/boot/
├── boot_info.c
├── boot_manager.c
└── boot_ui.c
```

Responsibilities:

- load and validate boot configuration;
- select kernel/boot target/options;
- aggregate Stage 1-3 results without rediscovering or rewriting them;
- present boot, recovery, diagnostic, and critical-failure UI;
- choose continue/retry/recovery/halt;
- freeze configuration before kernel entry;
- publish the final handoff toward `kernel_main(context)`.

`boot_info` is an aggregator only. It must preserve `UNKNOWN`, `NOT DETECTED`, `NOT INITIALIZED`, and `FAILED` states. `boot_manager` is the Milestone 0 orchestrator and delegates actual work to the owning stages. `boot_ui` is boot-time UI, not a shell, compositor, desktop UI, or application framework.

Proposed decisions:

```text
CONTINUE_TO_KERNEL
RETRY_BOOT
ENTER_RECOVERY
TERMINAL_HALT
```

The Stage 4 configuration is mutable before freeze and read-only afterward. It must reach the kernel through an explicitly defined extension of the existing `JPBootContext + JPHandoffInfo` contract; it must not revive `JPBootInfo` as an undocumented replacement.

## Open Architectural Decisions

Stage 4 Specification Lock was recorded as APPROVED on 2026-11-15 (specification only; no implementation exists). Status of the decisions that were resolved for it:

1. **Resolved** (`milestone0.md`): Stage 3 is pre-kernel. Flow: Stage 1 -> Stage 2 -> Stage 2 completion continuation -> Stage 3 (normal entry) -> Stage 4 -> `kernel_main(context)`; Stage 2 failure -> `jp_kernel_stage2_failure` -> Stage 3 (failure entry).
2. **Resolved** (`milestone0.md`): Stage 4 `boot_manager` owns the recovery transition through `jp_boot_manager_request_recovery(JPBootContext *, const JPBootRecoveryState *)` returning `JPBootTransitionResult` (`ACCEPTED`, `REJECTED`, `TERMINAL`). Stage 4 must still specify its concrete types and behavior.
3. **Resolved** (`milestone0.md` Stage 4): boot configuration is `JPBootConfig` in the `stage4` area of `JPBootContext`, chosen at every start through the Stage 1 pre-transition interaction point (build-time selection policy: `INTERACTIVE` for development, `DEFAULT_ONLY` for release), frozen before `kernel_main`. Entries are opaque and empty in Milestone 0. Persistence, post-start changes, platform reset, interactive recovery, and entry interpretation are deferred.
4. **Resolved** (`milestone0.md` Stage 4): `boot_info` is a by-value read-only summary (`JPBootSummary`) over `JPBootContext`, the Stage 3 status and Boot Report, and the `stage4` area; it is never a second source of truth.
5. **Resolved** by decision 1: Stage 3 moves entirely before `kernel_main()`.
6. **Resolved:** the interactive Boot Manager layout in `boot.md` is descriptive and forward-looking; `milestone0.md` is normative where they conflict.
7. Open: resolve documentation status mismatch: README says Milestone 1 while progress/roadmap/Milestone 0 say Milestone 0 planned.

## Security and Architecture Constraints

- Never fabricate hardware, memory, status, configuration, or recovery success.
- Preserve explicit ownership and lifetime for every persistent resource.
- Keep x86_64 and AArch64 behavior independently verifiable.
- Reject AArch64/BIOS explicitly.
- Keep structural validation, trust, authentication, and authorization distinct.
- Do not claim cryptographic authenticity from structural validation.
- Do not introduce undocumented architecture-significant state or ABI.
- Keep failure handling deterministic and explainable.
- Keep Stage 4 from taking ownership of Stage 1/Stage 2 physical resources merely because it aggregates metadata.

## Documentation and Implementation Status

The repository contains substantial design documentation and incomplete prototype C/CMake code. The Markdown documents do not prove successful compilation, linking, boot execution, or architecture acceptance.

Before implementation work resumes:

- inspect the actual source and CMake state;
- run the narrowest relevant build/check;
- do not mark documentation checkboxes as implementation acceptance;
- preserve unrelated user changes in a dirty worktree;
- update the normative specification before implementing a missing architecture-significant decision.

## Next Step

1. Review this handoff and approve the pre-kernel Stage 4 direction.
2. Resolve the seven open architectural decisions above, beginning with the Stage 3/Stage 4 order.
3. Update `milestone0.md` and `boot.md` to one authoritative boot flow.
4. Replace the deleted Stage 4 proposal with normative Stage 4 file specifications only after the flow is approved.
5. Define and lock the kernel-facing frozen configuration/handoff structure.
6. Define the owner-side recovery transition.
7. Run specification completeness and traceability checks for Milestones 0 Stages 1-4.
8. Only then implement Stage 4 and add executable tests for x86_64 and AArch64.
