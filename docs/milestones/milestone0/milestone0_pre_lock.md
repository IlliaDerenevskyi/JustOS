# Milestone 0 Pre-Lock Verification Gate

> **Status:** Verification gate
> **Normative source:** [milestone0.md](milestone0.md)

This file is a review checklist only. The normative definitions live in `milestone0.md` and `boot.md`. A checklist item passes only when the corresponding rule is present, consistent, and verifiable in those authoritative documents.

Part 1 below is the review list for Stage 2. Stage 2 stayed **NOT LOCKED** until every item of Part 1 passed. All items of Part 1 passed and the Specification Lock Decision was signed by the project owner on 2026-11-20 (see "Specification Lock Decision" in `milestone0.md`). Part 2 is the detailed list of the functions and tests of all four stages; it is used when the code is checked.

# Part 1 — Stage 2 pre-lock corrections

## Cross-Stage Corrections

- [x] x86_64 and AArch64 are both required Milestone 0 architectures.
- [x] UEFI is required for both architectures.
- [x] BIOS is required only for x86_64.
- [x] Architecture-specific Stage 1 and Stage 2 contracts are complete for both architectures.
- [x] x86_64 handoff and Stage 2 entry define `IF = 0`.
- [x] AArch64 interrupt mask state is explicitly defined as `PSTATE.DAIF = 1111`.
- [x] AArch64 CPU state defines EL1, SCTLR_EL1 boundary, and TCR_EL1/MAIR_EL1 preservation for paging.
- [x] AArch64 paging defines 4 KiB granules, 48-bit virtual addresses, TTBR1_EL1, descriptor classes, permission bits, and TLBI/DSB/ISB ordering.
- [x] Stage 2 never enables interrupts (x86_64 `IF = 0`, AArch64 `DAIF = 1111` stay unchanged through the exit to `jp_kernel_stage2_complete`); enabling them is outside Stage 2 and is not specified in Milestone 0.
- [x] The sole path is Stage 1 handoff -> architecture-specific Stage 2 entry stub -> `jp_kernel_init(context)` -> CPU -> Memory -> Paging -> Stack -> Heap -> ELF -> `jp_kernel_stage2_complete(context)` (then Stage 3, Stage 4, and `kernel_main(context)`).
- [x] The Stage 2 entry stub, `jp_kernel_init()`, completion point, transfer owner, calling conventions, and machine state are defined.
- [x] `jp_kernel_stage2_failure()` has a defined symbol, declaration, implementation location, calling convention, and non-returning behavior for both architectures.
- [x] The Stage 2 memory subsystem is the single authoritative physical-memory owner.
- [x] Kernel image, stack, page tables, heap, framebuffer, firmware-reserved, and runtime regions have explicit ownership.
- [x] General allocation cannot consume reserved memory.
- [x] Memory ownership operations are defined conceptually: reserve, allocate, query, release/reclaim where permitted, typed-region request, validation, and conflict rejection.
- [x] `paging.c`, `stack.c`, and `heap.c` have no independent physical allocators.
- [x] Heap alignment is 16 bytes, allocation granularity is 16 bytes, zero-size allocation returns `NULL`, free is public and required, and Stage 2 heap growth is forbidden.
- [x] Boot Security is explicitly limited to boot integrity and execution-state validation.
- [x] Validation, trust, and authentication are distinct; no cryptographic-authenticity claim is implied.
- [x] `JPBootInfo` is excluded from Stage 1 and Stage 2.
- [x] `JPBootContext + JPHandoffInfo + ownership-transfer state` is the authoritative Stage 1 -> Stage 2 contract.
- [x] `JPBootContext` ownership transfers to Stage 2 at handoff; Stage 1 has no access afterward; the same pointer reaches `kernel_main(context)`.
- [x] `kernel_loader.h` is included in the canonical Stage 1 file structure and specified.
- [x] AArch64 CPU, paging descriptor, stack, memory-range, ELF, and init contracts are concrete in their owning file specifications.
- [x] Stage 1 acceptance has separate x86_64/UEFI, x86_64/BIOS, and AArch64/UEFI cases, with AArch64/BIOS explicitly rejected.
- [x] `boot.md`, Stage 1, Stage 2, and later Milestone 0 documents use the same BootInfo lifecycle.

## Verification Sequence

1. Run Specification Completeness Verification in `milestone0.md`.
2. Run Specification Traceability in `milestone0.md`.
3. Review cross-document consistency between `milestone0.md` and `boot.md`.
4. Record the Specification Lock Decision in `milestone0.md`.
5. Perform Implementation Acceptance only after both x86_64 and AArch64 implementations exist.

A checklist pass does not itself create a Specification Lock. The authoritative specification must be updated first.

# Part 2 — Function and verification checks for every stage

All boxes of this part were ticked on 2026-11-21, after the four stages were committed. Every function named here has a definition in `src/` (checked by a search), and every `Tested:` line is covered by the unit tests of the stage and by the QEMU tests on x86_64 UEFI, x86_64 BIOS, and AArch64 UEFI (see the Implementation Acceptance records in `milestone0.md`).

This list contains, for every file of every stage, each function that other code may call and the tests that file requires. A box is ticked only when the function has been built and the tests have passed on every supported processor family and firmware layer. The general, shorter checklist is in the "Checklist" section of [milestone0.md](milestone0.md).

### Stage 1 — function and verification checks

- [x] Stage 1 (Boot Pipeline Foundation) is built, reviewed (Stage Report and architecture review, Rule 26), and committed.
- [x] Every function listed below behaves as described, both when it succeeds and when it fails, and this was tested on every supported processor family and firmware layer.

**`include/justos/boot/boot_types.h`**

- [x] Tested: all required normalized fields exist; no provider-native dependency exists; all Stage 1 constants are centralized; `JPBootInfo` is absent; the input and interaction types are present and contain no provider-native dependency; every architecture-significant Stage 1 state is representable.

**`include/justos/boot/boot_environment.h`**

- [x] `jp_boot_environment_initialize()`
- [x] `jp_boot_environment_get_memory_map()`
- [x] `jp_boot_environment_get_framebuffer()`
- [x] `jp_boot_environment_get_kernel_image()`
- [x] `jp_boot_environment_prepare_handoff()`
- [x] `jp_boot_environment_read_input()`
- [x] Tested: generic code contains no provider-native structures; provider ownership is explicit; supported providers expose equivalent normalized operations; failures propagate deterministically; input dispatch returns provider results unchanged and is rejected outside the permitted states.

**`src/boot/boot_environment.c`**

- [x] Tested: provider selection and state transitions using both supported providers, and input dispatch (`NONE`, event, `NOT_DETECTED`, `NOT_INITIALIZED` outside permitted states) for each provider.

**`include/justos/boot/platform/uefi.h`**

- [x] `jp_uefi_initialize()`
- [x] `jp_uefi_get_memory_map()`
- [x] `jp_uefi_get_framebuffer()`
- [x] `jp_uefi_get_kernel_image()`
- [x] `jp_uefi_prepare_handoff()`
- [x] `jp_uefi_read_input()`
- [x] Tested: memory-map acquisition; final key preservation; GOP normalization; kernel acquisition; `ExitBootServices` transition semantics; input: no pending input, a mapped key, a character, an unmappable key, no input device, and rejection from `FINAL_MAP_ACQUIRED` onward, on both x86_64 and AArch64.

**`src/boot/platform/uefi.c`**

- [x] Tested: Test both normal and failure paths, especially final memory-map changes and transition failure, and the input path (no input, event, no device, rejection after the final memory map is acquired).

**`include/justos/boot/platform/bios.h`**

- [x] `jp_bios_initialize()`
- [x] `jp_bios_get_memory_map()`
- [x] `jp_bios_get_framebuffer()`
- [x] `jp_bios_get_kernel_image()`
- [x] `jp_bios_prepare_handoff()`
- [x] `jp_bios_read_input()`
- [x] Tested: provider behavior against the same normalized Stage 1 contract required of UEFI, including the input path (no input, key, character, unmappable key, no keyboard service, rejection after handoff preparation).

**`src/boot/platform/bios.c`**

- [x] Tested: the provider against the normalized UEFI-equivalent Stage 1 behavior, including input.

**`include/justos/boot/hardware.h`**

- [x] `jp_boot_discover_hardware()`
- [x] Tested: CPU, firmware, and boot-device discovery independently and in aggregate.

**`src/boot/hardware.c`**

- [x] Tested: real discovery and failure propagation.

**`include/justos/boot/memory_map.h`**

- [x] `jp_boot_discover_memory()`
- [x] `jp_boot_validate_memory_map()`
- [x] Tested: valid maps, malformed maps, overflow, overlap, and unavailable memory.

**`src/boot/memory_map.c`**

- [x] Tested: overflow, overlap, invalid type, invalid length, and maximum region count.

**`include/justos/boot/framebuffer.h`**

- [x] `jp_boot_initialize_framebuffer()`
- [x] `jp_boot_validate_framebuffer()`
- [x] Tested: RGB/BGR representations, pitch semantics, dimensions, and unsupported formats.

**`src/boot/framebuffer.c`**

- [x] Tested: real framebuffer data and invalid configurations.

**`include/justos/boot/kernel_image.h`**

- [x] `jp_boot_acquire_kernel_image()`
- [x] Tested: valid image, unavailable image, truncated image, and invalid storage.

**`src/boot/kernel_image.c`**

- [x] Tested: actual kernel image acquisition and lifetime.

**`include/justos/boot/kernel_loader.h`**

- [x] `jp_boot_validate_kernel()`
- [x] `jp_boot_load_kernel()`
- [x] Tested: that the header declarations match `src/boot/kernel_loader.c` and that the header compiles without provider-specific dependencies.

**`src/boot/kernel_loader.c`**

- [x] Tested: valid ELF; invalid magic; invalid class; invalid architecture; invalid type; malformed program-header table; invalid `PT_LOAD`; overflow; invalid entry; allocation failure; mapping failure; permission failure; successful complete load.

**`include/justos/boot/handoff.h`**

- [x] `jp_boot_register_interaction()`
- [x] `jp_boot_prepare_handoff()`
- [x] Tested: successful and failed handoff preparation and hook registration (valid, null, duplicate, late).

**`src/boot/handoff.c`**

- [x] Tested: all success and failure combinations required by the handoff contract, including: no hook registered, hook registered and succeeding, hook failing, duplicate registration, registration after handoff started, hook invoked exactly once, and no memory-map key held across the hook.

### Stage 2 — function and verification checks

- [x] Stage 2 (Platform Initialization) is built, reviewed (Stage Report and architecture review, Rule 26), and committed.
- [x] Every function listed below behaves as described, both when it succeeds and when it fails, and this was tested on every supported processor family and firmware layer.

**`src/kernel/entry_x86_64.S`**

- [x] Tested: both symbols, argument passing, calling conventions, success transfer, failure transfer, interrupt masking, context identity, and non-return behavior.

**`include/justos/kernel/cpu.h`**

- [x] `jp_kernel_cpu_initialize()`
- [x] Tested: ### Successful initialization Valid Stage 1 CPU state ↓ CPU initialization ↓ INITIALIZED ### Invalid inherited state Invalid Stage 1 state ↓ Initialization rejected ↓ FAILED ### Architecture mismatch Boot architecture ≠ Stage 2 target ↓ FAILED ### Mandatory feature unavailable Required CPU feature missing ↓ FAILED ### Final validation Verify that: UNINITIALIZED ≠ INITIALIZED FAILED ≠ INITIALIZED INITIALIZED only after complete CPU initialization

**`src/kernel/cpu.c`**

- [x] Tested: ### Architecture x86_64 → accepted AArch64 → accepted unknown → rejected ### Execution Mode long mode → accepted invalid execution mode → rejected ### Context valid context → accepted NULL context → rejected ### CPU State valid inherited CPU state → accepted missing required state → rejected invalid required state → rejected ### Interrupt State explicitly defined state → accepted undefined state → rejected ### Required CPU Features available → accepted required feature unavailable → rejected ### Final State Successful initialization must demonstrate that the resulting CPU state satisfies the Stage 2 CPU contract. Execution of initialization instructions without final state validation is insufficient.

**`include/justos/kernel/paging.h`**

- [x] `jp_kernel_paging_initialize()`
- [x] `jp_kernel_paging_validate()`
- [x] Tested: ### Architecture x86_64 → accepted AArch64 → accepted ### Page-Table Root valid architecture-specific root → accepted invalid architecture-specific root → rejected ### Paging State paging enabled → accepted paging disabled when Stage 2 requires it → rejected ### Kernel Mappings all required mappings present → accepted missing required mapping → rejected ### Permissions required permissions established → accepted incorrect permissions → rejected ### Address Validity canonical virtual address → accepted non-canonical virtual address → rejected ### Final State Successful initialization must prove that the active page-table environment satisfies the Stage 2 contract. Execution of page-table setup operations without final validation is insufficient.

**`src/kernel/paging.c`**

- [x] Tested: ### Architecture x86_64 → accepted AArch64 → accepted ### Inherited State valid Stage 1 paging state → accepted invalid Stage 1 paging state → rejected ### Architecture-Specific Root valid CR3 root on x86_64 or valid TTBR1_EL1 root on AArch64 → accepted invalid architecture-specific root → rejected ### Page-Table Memory valid aligned page-table memory → accepted invalid memory → rejected overlapping protected memory → rejected ### Kernel Entry canonical + mapped + executable → accepted non-canonical → rejected unmapped → rejected non-executable → rejected ### Kernel Segments all required segments mapped → accepted missing segment mapping → rejected ### Permissions ELF-derived permissions preserved → accepted unnecessary write permission → rejected unnecessary execute permission → rejected unjustified W+X mapping → rejected ### Mapping Conflicts compatible existing mapping → may be preserved different physical mapping → rejected incompatible permissions → rejected ### Stack required stack mapping exists → accepted missing stack mapping → rejected ### TLB required synchronization completed → accepted stale translation remains possible → rejected ### Final State Successful initialization must prove that the active x86_64 paging environment satisfies every required Stage 2 paging invariant. Page-table construction alone is insufficient.

**`include/justos/kernel/stack.h`**

- [x] `jp_kernel_stack_initialize()`
- [x] Tested: the header exposes the required stack initialization API; `JPBootContext` is available through the declared dependency; `JPBootResult` is available through the declared dependency; no firmware-specific dependency is introduced; no `JPBootInfo` dependency is introduced; no Stage 3 dependency is introduced; the public API matches the implementation in `stack.c`; the header exposes no undocumented internal stack functions; the header does not introduce unrelated stack-management responsibilities.

**`src/kernel/stack.c`**

- [x] Tested: successful initialization with a valid inherited stack; valid stack range; valid active stack pointer; valid stack mapping; valid stack permissions; correct selected-architecture stack alignment; invalid `JPBootContext`; missing Stage 1 handoff state; missing stack state; invalid stack range; zero-sized stack; inverted stack range; stack-range arithmetic overflow; stack pointer below the valid range; stack pointer above the valid range; unmapped stack; inaccessible stack memory; invalid stack permissions; incorrect stack alignment; conflicting stack mapping; preservation of the active stack; preservation of required stack mappings after paging initialization; absence of heap dependency; absence of firmware dependency; absence of `JPBootInfo` dependency; failure propagation to `init`; no silent replacement of an invalid stack; no silent stack-pointer adjustment; no unauthorized page-table replacement; deterministic initialization behavior.

**`include/justos/kernel/heap.h`**

- [x] `jp_kernel_heap_initialize()`
- [x] `jp_kernel_heap_alloc()`
- [x] `jp_kernel_heap_free()`
- [x] Tested: successful heap initialization; invalid `JPBootContext`; missing memory state; missing paging state; missing stack state; valid heap region; invalid heap region; zero-sized heap; overlapping heap and kernel image; overlapping heap and active stack; overlapping heap and page-table memory; overlapping heap and reserved memory; invalid heap mapping; successful allocation; zero-size allocation handling; allocation alignment; multiple allocations; allocation exhaustion; allocation-size overflow; alignment overflow; allocation boundary enforcement; allocation outside the heap region is impossible; preservation of existing allocations; invalid free handling; double-free handling; allocator metadata integrity; stack preservation; page-table preservation; memory ownership consistency; failure propagation to `init`; absence of firmware dependencies; absence of `JPBootInfo` dependencies; absence of Stage 3 dependencies; deterministic initialization; deterministic allocation behavior; safe behavior when the heap is exhausted.

**`src/kernel/heap.c`**

- [x] Tested: successful heap initialization; invalid `JPBootContext`; missing memory state; missing paging state; missing stack state; invalid heap region; zero-sized heap; inverted heap range; address arithmetic overflow; unmapped heap region; heap overlap with kernel image; heap overlap with active stack; heap overlap with page-table memory; heap overlap with reserved memory; successful allocation; allocation alignment; multiple allocations; allocation boundary enforcement; allocation exhaustion; allocation-size overflow; alignment overflow; metadata placement; metadata protection; preservation of existing allocations; allocation cannot escape heap boundaries; allocation cannot overlap protected memory; invalid free; double free; free of non-heap pointer; preservation of allocator metadata; preservation of active stack; preservation of page tables; preservation of kernel image; consistency with memory ownership; failure propagation to `init`; absence of firmware dependencies; absence of `JPBootInfo` dependencies; absence of Stage 3 dependencies; deterministic initialization; deterministic allocation behavior; safe behavior when the heap is exhausted.

**`include/justos/kernel/memory.h`**

- [x] `jp_kernel_memory_initialize()`
- [x] `jp_kernel_memory_reserve()`
- [x] `jp_kernel_memory_allocate()`
- [x] `jp_kernel_memory_query()`
- [x] `jp_kernel_memory_release()`
- [x] `jp_kernel_memory_validate_ownership()`
- [x] Tested: the header compiles independently; `JPBootContext` is referenced through the authoritative Stage 1 definition; `JPMemoryRegion` is not redefined; `JPMemoryMap` is not redefined; the public initialization API matches `memory.c`; no implementation-only structures are exposed; no firmware-specific types are exposed; no paging internals are exposed; no heap internals are exposed; no stack internals are exposed; no ELF-loader internals are exposed; no `JPBootInfo` dependency exists; no Stage 3 dependency exists; the header remains limited to the Stage 2 memory subsystem boundary.

**`src/kernel/memory.c`**

- [x] Tested: successful initialization from valid Stage 1 memory state; invalid `JPBootContext`; missing memory map; invalid memory-map metadata; excessive region count; invalid region base; invalid region size; zero-sized region handling; inverted ranges; address arithmetic overflow; prohibited overlapping regions; contradictory ownership; reserved memory preservation; kernel-memory preservation; Stage 2 runtime-memory accounting; paging-memory accounting; stack-memory accounting; heap-memory accounting; physical/virtual memory separation; deterministic classification; failure without partial state publication; preservation of Stage 1 memory state; absence of firmware dependencies; absence of `JPBootInfo` dependency; absence of Stage 3 dependency; compatibility with paging initialization; compatibility with stack initialization; compatibility with heap initialization; compatibility with ELF runtime validation.

**`include/justos/kernel/elf.h`**

- [x] `jp_kernel_elf_initialize()`
- [x] Tested: the header compiles independently; all public declarations match `elf.c`; Stage 1 ELF structures are reused; ELF constants are not duplicated; selected-architecture validation is enforced; invalid ELF state is rejected; invalid segment state is rejected; invalid entry point is rejected; loaded-state validation is correct; mapped-state validation is correct; permission-state validation is correct; kernel image memory is preserved; no kernel image is reloaded; no segment copying is performed; no firmware dependency exists; no `JPBootInfo` dependency exists; no Stage 3 dependency exists; the ELF subsystem remains compatible with the Stage 1 → Stage 2 boundary.

**`src/kernel/elf.c`**

- [x] Tested: successful initialization from valid Stage 1 ELF state; invalid `JPBootContext`; missing kernel ELF state; invalid ELF identification; unsupported ELF class; machine type inconsistent with the selected architecture; unsupported ELF type; invalid program-header state; invalid segment count; invalid segment ranges; segment arithmetic overflow; invalid `p_filesz` / `p_memsz` relationship; invalid alignment; invalid loaded state; invalid mapped state; invalid permission state; invalid entry point; entry point outside the kernel image; entry point in non-executable memory; inconsistency between ELF metadata and normalized segment state; preservation of kernel image contents; preservation of kernel image ownership; preservation of page-table state; preservation of stack state; absence of heap allocation; absence of firmware dependencies; absence of kernel reloading; absence of segment copying; absence of relocation; absence of `JPBootInfo` dependency; absence of Stage 3 dependency; deterministic validation; failure without partial state publication.

**`include/justos/kernel/init.h`**

- [x] `jp_kernel_init()`
- [x] `jp_kernel_stage2_failure()`
- [x] `jp_kernel_stage2_complete()`
- [x] Tested: the header compiles independently; the public declaration matches `init.c`; `JPBootContext` is reused rather than redefined; the Stage 2 initialization entry point is unique; no subsystem internals are exposed; no firmware types are exposed; no `JPBootInfo` dependency exists; no Stage 3 dependency exists; no Stage 4 behavioral dependency exists (only the declared symbol `jp_kernel_stage2_complete`); the initialization order is explicitly represented by the implementation; subsystem failures are propagated; dependent initialization stops after failure; `init` does not implement subsystem responsibilities; ownership remains with the individual subsystems; the interface remains limited to Stage 2 orchestration.

**`src/kernel/init.c`**

- [x] Tested: the implementation compiles independently; the public API matches `init.h`; the Stage 1 context is passed correctly; CPU is initialized first; memory is initialized only after CPU success; paging is initialized only after memory success; stack is initialized only after paging success; heap is initialized only after stack success; ELF is initialized only after heap success; failure stops the sequence; subsystem failure is propagated; later subsystems are not invoked after failure; Stage 2 success is reported only after all mandatory subsystems succeed; subsystem ownership is not modified by the orchestrator; no subsystem implementation is duplicated in `init.c`; no firmware dependency exists; no `JPBootInfo` dependency exists; no Stage 3 dependency exists; no Stage 4 behavioral dependency exists (only the declared symbol `jp_kernel_stage2_complete`); initialization is deterministic; initialization is serial; no undocumented rollback is performed; the Stage 2 completion boundary remains consistent with the specification.

### Stage 3 — function and verification checks

- [x] Stage 3 (Boot Diagnostics & Recovery) is built, reviewed (Stage Report and architecture review, Rule 26), and committed.
- [x] Every function listed below behaves as described, both when it succeeds and when it fails, and this was tested on every supported processor family and firmware layer.

**`include/justos/boot/boot_log.h`**

- [x] `jp_boot_log_initialize()`
- [x] `jp_boot_log_append()`
- [x] `jp_boot_log_get()`
- [x] `jp_boot_log_get_status()`
- [x] `jp_boot_log_finalize()`
- [x] Tested: initialization, append ordering, exact capacity behavior, mixed-severity overflow, retrieval bounds, finalization, and failed-operation atomicity.

**`src/boot/boot_log.c`**

- [x] Tested: valid append/retrieval, ordering, truncation, invalid event, full storage, both architectures, and ownership preservation.

**`include/justos/boot/boot_status.h`**

- [x] `jp_boot_status_initialize()`
- [x] `jp_boot_status_update()`
- [x] `jp_boot_status_get()`
- [x] Tested: every allowed edge, every rejected edge, terminal immutability, original-result preservation, retry re-entry, and architecture mismatch rejection.

**`src/boot/boot_status.c`**

- [x] Tested: valid transitions, invalid transitions, terminal state preservation, architecture mismatch, and failure propagation.

**`include/justos/boot/error.h`**

- [x] `jp_boot_error_create()`
- [x] `jp_boot_error_get()`
- [x] `jp_boot_error_clear()`
- [x] Tested: every default mapping, valid and invalid overrides, duplicate creation, result immutability, malformed input, and architecture mismatch.

**`src/boot/error.c`**

- [x] Tested: valid errors, malformed context, severity/recoverability classification, result preservation, and both architectures.

**`include/justos/boot/panic.h`**

- [x] `jp_boot_panic()`
- [x] Tested: first panic, recursion from every active state, framebuffer success/failure, fallback output, first-error preservation, interrupt state, and non-returning behavior on both architectures.

**`src/boot/panic.c`**

- [x] Tested: first panic, recursive panic, valid/invalid framebuffer, fallback output, x86_64, AArch64, and non-return behavior.

**`include/justos/boot/recovery.h`**

- [x] `jp_boot_recovery_decide()`
- [x] `jp_boot_recovery_enter()`
- [x] `jp_boot_manager_request_recovery()`
- [x] Tested: each decision, retry atomicity, reset/preserve table, identity uniqueness, exhaustion, existing-provider entry, terminal fallback, and both architectures.

**`src/boot/recovery.c`**

- [x] Tested: each decision, retry exhaustion, preserved diagnostics, terminal fallback, x86_64, and AArch64.

**`include/justos/boot/diagnostic_output.h`**

- [x] `jp_boot_diagnostic_output()`
- [x] Tested: valid/invalid messages, valid/invalid framebuffer, fallback-only operation, both sinks, all-sinks failure, recursion prevention, x86_64, AArch64, and ownership preservation.

**`src/boot/diagnostic_output.c`**

- [x] Tested: routing, fallback, sink failure, recursion prevention, architecture state preservation, and both architectures.

**`include/justos/boot/boot_report.h`**

- [x] `jp_boot_report_finalize()`
- [x] `jp_boot_report_get()`
- [x] Tested: successful finalization, repeated finalization, inconsistent state, failed publication, read-only Stage 4 access, terminal consumption, both architectures, and ownership preservation.

**`src/boot/boot_report.c`**

- [x] Tested: finalization order, idempotence, failed publication, immutability, terminal/report consumers, both architectures, and ownership preservation.

### Stage 4 — function and verification checks

- [x] Stage 4 (Boot manager & UI) is built, reviewed (Stage Report and architecture review, Rule 26), and committed.
- [x] Every function listed below behaves as described, both when it succeeds and when it fails, and this was tested on every supported processor family and firmware layer.

**`include/justos/boot/boot_config.h`**

- [x] `jp_boot_config_initialize()`
- [x] `jp_boot_config_apply()`
- [x] `jp_boot_config_freeze()`
- [x] `jp_boot_config_get()`
- [x] Tested: Check the default values, every rule of validity, a successful apply and a rejected apply, that freezing twice changes nothing, that apply is refused after freezing, `get` in every state, and that a failed call changes nothing.

**`src/boot/boot_config.c`**

- [x] Tested: Run every check listed for `boot_config.h` on both processor families.

**`include/justos/boot/boot_info.h`**

- [x] `jp_boot_info_summarize()`
- [x] `jp_boot_info_state_name()`
- [x] Tested: Check that every field equals the value of its owner, including every "not available" state, and that nothing in the context is changed.

**`src/boot/boot_info.c`**

- [x] Tested: Run every check listed for `boot_info.h` on both processor families.

**`include/justos/boot/boot_ui.h`**

- [x] `jp_boot_ui_select()`
- [x] `jp_boot_ui_present_summary()`
- [x] `jp_boot_ui_present_failure()`
- [x] Tested: Check every key (including wrapping with Up and Down, starting and stopping editing with Enter, a character when the text is full, Backspace on empty text, and F10 and Esc during editing), the accepted and discarded paths, the display of a rejection and its removal at the next key, a missing framebuffer, a missing keyboard, the framebuffer limits, and the text screens of phase B through the text output function.

**`src/boot/boot_ui.c`**

- [x] Tested: Run every check listed for `boot_ui.h` on both processor families.

**`include/justos/boot/boot_manager.h`**

- [x] `jp_boot_manager_prepare()`
- [x] `jp_boot_manager_select()`
- [x] `jp_boot_manager_decide()`
- [x] `jp_boot_manager_request_recovery()`
- [x] `jp_kernel_stage2_complete()`
- [x] Tested: Check `prepare`, `select` (every outcome), `decide` (every rule), the recovery request (every decision and every reason for rejection), and the manager's state moves.

**`src/boot/boot_manager.c`**

- [x] Tested: Run every check listed for `boot_manager.h` on both processor families. The tests need a test version of `kernel_main` and a test harness that replaces the recovery diagnostics state.
