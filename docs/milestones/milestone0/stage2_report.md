# Stage 2 Report: Platform Initialization (for the architecture review, Rule 26)

Status: **the Specification Lock of Stage 2 is signed (2026-11-20); all the code of Stage 2 is written and passes its tests: 7 new host tests (29 in all, gcc and clang, also under AddressSanitizer and UBSan) and the QEMU tests (8 in all: each platform has the normal run and a `stage2_failure` run that makes the ELF check fail on purpose, to run the failure path of the entry stub) on x86_64 UEFI, x86_64 BIOS (both also under KVM), and AArch64 UEFI (also with `virtualization=on`, the EL2 to EL1 path). The architecture review was done and the stage was committed by the project owner; the "Implementation Acceptance" of `milestone0.md` (Stage 2) was ticked on 2026-11-21 (the QEMU level; level M is left to the project owner). Nothing has run on real hardware.**

How the QEMU tests prove Stage 2: the test kernel now starts through `just_kernel_entry_<arch>` (its ELF entry), so it reaches `kernel_main` only if `jp_kernel_init` returned success and the stub called `jp_kernel_stage2_complete`. That symbol is Stage 4's; the test kernel provides a stand-in in `tests/qemu/test_kernel/fake_stage2_complete.c` that calls `kernel_main`. Before this report the QEMU tests found three real mistakes that the host tests could not see (section 4).

## 1. Completed tasks

Public headers (`include/justos/kernel/`): `cpu.h`, `memory.h`, `paging.h`, `stack.h`, `heap.h`, `elf.h`, `init.h`; `boot_types.h` got `JPBootStage2Area` (the ownership table, inside `JPBootContext`), `JP_KERNEL_MEMORY_MAX_ENTRIES`, and `JP_ELF_PF_{X,W,R}`.

Implementation (`src/kernel/`):

| Module | What it does |
|--------|--------------|
| `memory.c` | the ownership table (256 entries in `context->stage2`): built from the Stage 1 map plus the kernel segments, stack, table pool, framebuffer and context; `reserve`, `allocate`, `query`, `release`, `validate_ownership` |
| `cpu.c` | checks long mode / EL1, interrupts masked, TCR/MAIR; sets `EFER.NXE` and `CR0.WP` on x86_64; reads everything back |
| `paging.c` | builds a new W^X identity map from the unused pages of the Stage 1 pool, switches CR3 / TTBR0, sets WXN on AArch64, and `jp_kernel_paging_validate` walks the active tables |
| `stack.c` | checks the Stage 1 stack: exact range in the table, alignment, pointer inside, writable |
| `heap.c` | first-fit free-list heap, all state inside its 1 MiB region |
| `elf.c` | checks the loaded image: header, segments, ownership, entry point, mappings |
| `init.c` | entry check, the six steps in order, completion check; `jp_kernel_stage2_failure` |
| `entry_x86_64.S`, `entry_aarch64.S` | the entry stubs |
| `cpu_regs_*.c`, `paging_hw_*.c`, `stack_hw_*.c`, `halt_hw_*.c` | the instructions that C cannot write (no logic); private headers `cpu_regs.h`, `paging_hw.h`, `stack_hw.h`, `halt_hw.h`, `arch_dispatch.h` |

Tests (written before the code, Rule 40): `tests/unit/kernel/test_{memory,cpu,paging,stack,heap,elf,init}.c`. The register functions are replaced by fakes in the tests; the real ones run in QEMU.

## 2. How the code was checked

- Warnings are errors (same flags as Stage 1), gcc and clang; every source also compiles for `x86_64-unknown-none-elf` and `aarch64-unknown-none-elf` with `-DJP_KERNEL_ONLY_<ARCH>` (the other architecture's code is removed).
- **Mutation checks** (about 25 to 290 deliberately changed copies per module; the survivors were read one by one). Survivors left: changes in comments; loops that read one zero slot after the last entry; overflow checks at the exact limit (unreachable inputs); and checks that another check already makes redundant. They are not hidden: the scripts are ad-hoc (not in the repository).
- Fault injection in the `paging` test (every table access fails in turn) found a null-pointer write that the other tests missed; it is fixed.
- Self-check against `rules.md`: tabs, `jp_` prefix, private helpers `static`, no `TODO`, no `malloc`, no mutable global data in `src/kernel`, no `memcpy`/`memset` (the kernel must be built with `-fno-builtin`).

## 3. Decisions taken beyond the text of the original specification (please review)

All are in `milestone0.md` ("Decided rules ..." blocks). Items 1 to 8 were asked of the project owner; the others are *derived* (a consequence of the text) and listed so that the reviewer can disagree.

1. AArch64 `TTBR0_EL1` is the identity map: first built by Stage 1, replaced by Stage 2 (same addresses, W^X).
2. The whole Stage 1 table pool is `PAGE_TABLES`; new tables come from its unused pages.
3. The ownership table lives in a new `stage2` area of `JPBootContext`.
4. Initial owners come from context fields by a fixed table.
5. Stage 2 replaces the Stage 1 identity map with a W^X map at the same addresses (needs `EFER.NXE`, `CR0.WP`, WXN).
6. Early heap: 1 MiB (`JP_KERNEL_EARLY_HEAP_SIZE`), the lowest available range at or above 1 MiB.
7. Mandatory CPU state: mode, NX/WXN support, interrupts masked; SSE/FPU are not required.
8. `jp_kernel_init` checks the handoff first; `jp_kernel_stage2_failure(NULL, ...)` stops the processor.
9. *Derived:* stack alignment (top multiple of 16; pointer multiple of 8 on x86_64, 16 on AArch64), checked through `jp_kernel_paging_validate`.
10. *Derived:* the operations of `memory` (`reserve` and `release` rules; `release` only for the heap; `allocate` is first fit at or above 1 MiB; `query` gives `NOT_DETECTED` for mixed or missing ownership).
11. *Derived:* the framebuffer has three cases in the table (outside: skipped; inside reserved: cut out; otherwise a broken handoff).
12. *Derived:* `elf` accepts only `ET_EXEC` (what Stage 1 accepts) and requires the entry point to be the same in three places and inside an executable segment.
13. *Derived:* regions are mapped rounded outwards to 2 MiB blocks (found by the AArch64 QEMU run: the serial port is not in the firmware map and was reached through the neighbouring block in Stage 1); page 0 is mapped when its block is touched; two regions with different kinds of memory in one block are refused.
14. *Derived:* a failure is never recorded as success (`stage2_result` becomes `JP_BOOT_FAILED` if the sink is called with `JP_BOOT_SUCCESS`).
15. The `init` completion check is `memory_validate_ownership` then `paging_validate`.

## 4. What the QEMU runs found

1. x86_64: `pushfq` in the register reader writes below the stack pointer; without `-mno-red-zone` it corrupted the reader's locals, so `cpu` failed. The kernel (and the test kernel) must be built with `-mno-red-zone`.
2. AArch64: the new map left the serial port unmapped (item 13 above) and the core took an unhandled data abort.
3. AArch64: a hang in that state is silent; no exception handler exists in Milestone 0, so any such mistake shows as a timeout (this is how the test reported it, it never reported a fake success).

## 5. Known limitations and gaps (for the reviewer's attention)

1. **A framebuffer that lies in a firmware region** (the map says `MMIO`, `ACPI` or `FIRMWARE` for it) stays owned by the firmware (decided with the project owner after the first version of this report; item 11 has four cases now). It is tested on the host only: QEMU's framebuffers are outside the map, so no QEMU run took this path. A framebuffer that touches available or unavailable memory still stops the boot with `JP_BOOT_ERROR_CORRUPTED_DATA`.
2. **The table pool (128 pages) was never filled in the QEMU runs** (512 MiB of RAM). Machines with a lot of memory or many regions may need more; paging then returns `JP_BOOT_FAILED` cleanly (tested), but the number was not measured.
3. Page 0 is mapped (item 13), as in Stage 1.
4. The ownership table has 256 entries; `reserve`/`allocate` fail cleanly when it is full.
5. The heap has no tests with real hardware and no corruption detection beyond the block chain checks.
6. `jp_kernel_stage2_complete` is only a test-kernel stand-in; Stage 4 must provide the real one before the stage chain works outside the tests.
7. The AArch64 QEMU test runs with the Cortex-A72 model only; x86_64 only with QEMU's default CPU and the host CPU (KVM).
8. `memory.c` zero-fills the table with a loop; the compiler may turn it into a `memset` call if the kernel is built without `-fno-builtin` (the test kernel build has it).
9. Nothing was run on real hardware; the level M checks remain.
10. The Specification Lock Decision of Stage 2 was signed by the project owner on 2026-11-20 (Part 1 of `milestone0_pre_lock.md`, Completeness, Traceability, and the Lock Decision are ticked; one item of Part 1 was reworded: "Stage 2 never enables interrupts"). "Implementation Acceptance" of Stage 2 was ticked on 2026-11-21 (see the record in `milestone0.md`).

## 6. Conclusion for the review

The code does what the specification (with the decided rules) says, on the three supported platforms in QEMU. Items 1 and 2 of section 5 are the ones to decide before real hardware.
