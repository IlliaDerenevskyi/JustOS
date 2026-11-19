# Stage 1 Report: generic part (for the architecture review, Rule 26)

Status: **all the code of Stage 1 is written and passes its tests (host tests and the QEMU test on x86_64 UEFI, AArch64 UEFI, and x86_64 BIOS), but the stage is not finished until the architecture review is done** (Rule 26): it must not be recorded as the commit `Stage 1: ...` before that. Working commits in `develop` are allowed.

The diff for this review is `build/review/stage1_generic.diff` (changes since the tag `v0.0.3`: tracked changes plus the new files; about 9,800 lines, 80 files). `build/` is ignored by git, so the file is local.

## 1. Completed tasks

Public headers (`include/justos/boot/`): `boot_types.h`, `boot_environment.h`, `hardware.h`, `memory_map.h`, `framebuffer.h`, `kernel_image.h`, `kernel_loader.h`, `handoff.h`, `platform/{uefi,bios}.h`, `architecture/{x86_64,aarch64}.h` (declarations only).

Generic implementation (`src/boot/`, 8 files; the memory reservation, the final memory map, and the pipeline were added after the first version of this report):

| Module | Public functions |
|--------|------------------|
| `boot_environment.c` | `jp_boot_environment_{initialize,get_memory_map,get_framebuffer,get_kernel_image,prepare_handoff,read_input}` |
| `hardware.c` | `jp_boot_discover_hardware` |
| `memory_map.c` | `jp_boot_discover_memory`, `jp_boot_validate_memory_map` |
| `framebuffer.c` | `jp_boot_initialize_framebuffer`, `jp_boot_validate_framebuffer` |
| `kernel_image.c` | `jp_boot_acquire_kernel_image` |
| `kernel_loader.c` | `jp_boot_validate_kernel`, `jp_boot_load_kernel` |
| `handoff.c` | `jp_boot_register_interaction`, `jp_boot_prepare_handoff` |
| `boot_pipeline.c` | `jp_boot_run_pipeline` (nine steps in order, stops at the first failure) |

Tests (Rule 17, written before the code, Rule 40): 16 `ctest` tests, all pass with gcc and clang, also in strict mode (`CI_STRICT=true`, as on `main`). Test support: ELF image builder, fake providers, fake environment layer, fake architecture operations (all in `tests/support/`, separate libraries so a test cannot link a fake instead of a missing module).

Test kernel and QEMU level (written, **not yet run in QEMU**): serial protocol, `report`, `screen` with a generated 8x16 font, `wait_*`, `serial_*`, `kernel_main`, `scripts/run_qemu.sh` (with `--kvm`), `scripts/make_image.sh` (UEFI and BIOS layouts), `tests/qemu/check_serial.sh` with expectations for the three platforms, KVM and virt-manager notes.

Infrastructure: root `CMakeLists.txt` (strict flags, `jp_add_unit_test`, QEMU tests behind `-DJP_ENABLE_QEMU_TESTS=ON`), CI scripts in `CI/` with the "build incomplete" classification, `requirements.txt`.

## 2. How the code was checked

- Warnings are errors (`-Wall -Wextra -Wpedantic -Werror -Wstrict-prototypes -Wmissing-prototypes -Wshadow -Wconversion -Wundef`), gcc and clang, no warnings.
- Self-check against `rules.md`: tab indentation, `jp_` prefix on public functions, private helpers `static`, unique include guards, no `TODO`, no `malloc`/`free`, no mutable global data in `src/`.
- **Mutation checks** on every module: about 37 deliberately broken copies per module were run against its tests. The first rounds exposed real gaps in my tests (rules that hid each other, one-sided overlap check, wrong test image size); tests were added until the survivors were only equivalent mutants (changes that cannot be observed, or that another rule already catches).

## 3. Decisions taken beyond the text of the original specification (please review)

All are recorded in `milestone0.md` ("Decided rules ..." blocks), each after a question to the project owner, unless marked *derived*.

1. Provider choice: the caller sets `environment.type`, `environment.platform_context`, and `handoff.architecture`; nothing is detected inside Stage 1.
2. `JPBootEnvironment.lifecycle` and `JPBootEnvironmentLifecycle` were added (derived: tests need the provider lifecycle; the structure had three fields).
3. `JPHandoffInfo` fields and `JPHandoffState` names (the specification gave only a list of content).
4. Architecture files (sections 20 to 23): seven operations per architecture (`validate_entry_state`, `setup_paging`, `prepare_entry`, `validate_elf_machine`, `discover_cpu`, `map_segment`, `phys_access`). This is a draft that did not exist in the original text.
5. Memory map, framebuffer, kernel image, ELF, loading, and handoff rules (strict policy: ET_EXEC only, W xor X, page alignment, entry inside an executable segment, 4 bytes per pixel, and so on).
6. Placing segments and the stack: usable memory in map order, above 1 MiB, page aligned, not on the kernel image; the stack is 16 KiB, behind the highest segment, identity-mapped (derived).
7. `JPDetectionState` has no "acquired" and "validated" values, so `DETECTED` means acquired and a successful validation call means validated (derived).
8. `JPBootStage3Area` and `JPBootStage4Area` are **temporary byte blocks** (65,536 and 8,192 bytes) in `boot_types.h` until Stage 3 and Stage 4 define their types.
9. Per-segment lifecycle: the field `JPKernelSegmentState state` in `JPKernelSegment` (the specification left the representation free); the final handoff check lists its fields in `milestone0.md`.
10. UEFI kernel path `\EFI\JustOS\kernel.elf`; BIOS disk layout with the `JUSTBIOS` header and the kernel at sector 2048.

## 3a. Added after the first version of this report

- `jp_boot_environment_reserve_memory` (and `jp_{uefi,bios}_reserve_memory` declarations): the loader reserves every kernel segment right after placing it, and `jp_boot_prepare_handoff` reserves the stack; a failing reservation is `JP_BOOT_FAILED`.
- The final memory map: `jp_boot_environment_prepare_handoff` publishes it into `context->memory_map` after the provider's transition, and `jp_boot_prepare_handoff` validates it again (derived decision, see `milestone0.md`).
- `jp_boot_run_pipeline`: the nine steps in order; a missing framebuffer does not stop it.
- `current_stage` ownership wording was corrected in the specification (the public pipeline functions each write their own stage value).
- Specification for the architecture operation `enter_kernel`, the BIOS boot chain and block, the UEFI definitions and entry, and the assembly parts (sections 24 to 32 of `milestone0.md`); the code for all of them is described in sections 3b to 3f.
- The fakes of the providers carry a tag (`called_by`, `prepare_called_by`), so that a dispatch to the wrong provider is detected by the tests.

## 3b. The UEFI provider (host-tested)

`src/boot/platform/uefi_defs.h` (hand-written UEFI structures with static assertions of every offset; checked for gcc, clang, and the real UEFI targets `x86_64-pc-windows-msvc` and `aarch64-pc-windows-msvc`) and `src/boot/platform/uefi.c` (the provider: memory map with the normalization table and merging, the graphics output, the kernel file, input, reservation, and the transition with the stale-key retry) are written and tested against a fake firmware that uses the real calling convention (`test_uefi`: 17 groups of checks; 62 mutants, the five that survive cannot be observed through the public API). `uefi.c` compiles with `-O0` and `-O2` for both real targets with no undefined symbol (no libc needed). **Not yet run on a real firmware (QEMU with OVMF or AAVMF).** `uefi_entry.c`, the architecture modules, and the assembly are still to be written.

## 3c. The x86_64 architecture module (host-tested)

`src/boot/architecture/x86_64.c` and `x86_64_enter.S` are written. The C code (the table memory pool, kernel segment mapping with W xor X, the identity map, the framebuffer mapping, the descriptor table, CPU discovery, entry-state checks, and `enter_kernel`) is tested on the host (`test_x86_64`) against a host buffer that stands for physical memory and a page-table walker that reads the tables back; the four assembly functions (`cli`, reading `RFLAGS/CR0/CR4/EFER`, the jump, and the park loop) are replaced by fakes there. The code was built for `x86_64-pc-windows-msvc` and `x86_64-unknown-none-elf` with `-O0` and `-O2` with only the expected undefined symbols; the assembly assembles for both formats and its disassembly was checked against the algorithm. Mutation checks (74 mutants) found a real bug (a large-page check at the wrong table level), now fixed; the nine survivors cannot be observed on the host (CPUID, redundant guards, large pages that are always identity pages). **The assembly and the whole jump have never run on a processor.** `JPHandoffInfo` gained `page_table_memory_base`, `page_table_memory_size`, and `page_table_memory_used` (ownership facts for Stage 2).

## 3d. First run on a real firmware (x86_64 UEFI)

`uefi_entry.c` and the CMake target `boot_app_x86_64-uefi` build `boot.efi` (a 14 KB PE32+ EFI application; clang and lld only; no libc symbols). With `-DJP_ENABLE_QEMU_TESTS=ON`, `ctest -R qemu_x86_64-uefi` builds the boot application and the test kernel, makes the FAT image, boots it in QEMU with OVMF, and checks the serial protocol: **it passes, in software emulation and with KVM**. The test kernel receives the context, prints the data list (32 memory regions, the framebuffer, the kernel segment, the handoff fields), and ends with `JP|RESULT=PASS`; `check_serial.sh x86_64-uefi` reports `ok`. The run found four real defects that the host tests could not show, all now fixed and covered by host tests: (1) the generic modules needed both architectures at link time (`arch_dispatch.h`, one-architecture builds); (2) `boot_environment.c` needed the BIOS provider (`JP_BOOT_WITHOUT_BIOS`); (3) the table pool, placed at the top of the highest usable region, collided with firmware allocations made after the early memory map (reservation retry with lower candidates); (4) the identity map tried to cover a reserved address range near 1 TiB and used up the table pool (the map now follows the regions and skips reserved ranges of 1 GiB or more). Not yet done: AArch64, the BIOS chain, the on-screen list and the 40-second wait on a screen (the screen code ran in the automatic variant only up to drawing, without anyone looking at it).

## 3e. AArch64 UEFI (host-tested and run in QEMU)

`src/boot/architecture/aarch64.c` and `aarch64_enter.S` are written like the x86_64 module: two translation trees (`TTBR1_EL1` for the kernel half and `TTBR0_EL1` for the identity map) in one table pool, W xor X page descriptors, 2 MiB identity blocks that follow the regions (memory-mapped I/O as device memory; reserved or MMIO ranges of 1 GiB or more skipped), the framebuffer as device memory, entry-state checks, and a jump that programs `MAIR_EL1`, `TCR_EL1` (fixed part `0xB5103510`, `IPS` from the processor), and `SCTLR_EL1`, and drops from EL2 to EL1 when needed. `test_aarch64` is plain C (the four system-register functions are faked) and runs on any host; 80 mutants were checked and none survives (the checker was verified with a no-change mutant). The assembly assembles for ELF and COFF and its disassembly matches the specification. `boot.efi` for AArch64 (13 KB) and the test kernel run in QEMU with AAVMF: `ctest -R qemu_aarch64-uefi` passes (about 6 s in software emulation, semihosting exit), and the same image also passes with `-machine virt,virtualization=on`, where the firmware runs at EL2, so the drop to EL1 ran as well (the test kernel does not print the exception level, so this is shown by the machine setup, not by a printed value). Decisions that differ from x86_64 and are written in `milestone0.md`: `WXN` is not set (the identity map is writable and executable); `logical_cores` is 1 (only the boot processor is counted; AArch64 has no instruction for the number of processors); MMIO regions are mapped as device memory.

## 3f. x86_64 BIOS (host-tested and run in QEMU)

The BIOS chain is written: `bios.c` (the provider that only reads the boot data block and the PS/2 controller), `bios_io.h` (the one port read, faked by the test), `bios_entry.c` (the 64-bit entry `jp_bios_main`), `bios_boot_sector.S` (512 bytes) and `bios_loader.S` (16-bit part padded to 4 KiB, a 32-bit step, and the 64-bit start), with the linker scripts `bios_boot_sector.ld` and `bios_loader.ld`. The loader file is one flat binary (16 KB: the 16-bit part, then all the 64-bit Stage 1 code, running in place at `0x9000`); the linker script refuses writable static data and a size above 56 KiB. `test_bios` runs `bios.c` on the host (the block is built by hand; 141 mutants, the 14 survivors are equivalent mutants: ties, `mark_active` that no public call can tell from `INITIALIZED`, and loop bounds that read one element more). The test found a real bug in the first version of the cut-out loop (an endless loop when two cuts overlapped), now fixed. A new macro `JP_BOOT_WITHOUT_UEFI` (and the test `test_boot_environment_no_uefi`) keeps the UEFI provider out of the loader. In QEMU with SeaBIOS `ctest -R qemu_x86_64-bios` passes, in software emulation and with KVM: the loader reads E820, picks the 1024 x 768 VBE mode, copies the kernel to 2 MiB, enters long mode, and the pipeline reaches `READY`; the test kernel prints the data list and `JP|RESULT=PASS`. Also run by hand: 32 MiB and 6 GiB of memory pass (memory above 4 GiB is not offered to the kernel); 16 MiB stops in the loader with the text `JustOS loader: at least 16 MiB of memory are needed`. Real defects found by the run: the compiler used SSE instructions, which are off in the state the loader leaves (`-mgeneral-regs-only` now for the loader and the x86_64 test kernel); the first `L` of the loader shared a line with the first line of the test kernel (the loader now writes `L` and a line break). Decisions written in `milestone0.md`: one flat loader file with the 64-bit code in place (56 KiB limit); memory from 4 GiB upward is cut out as `RESERVED`; bit 5 of the PS/2 status marks mouse data, a status of `0xFF` means no controller; the private header `bios_io.h`; `jp_bios_context_size/setup`.

**Status of the three platforms:** x86_64 UEFI, AArch64 UEFI, and x86_64 BIOS all pass the QEMU test (x86_64 also with KVM). Nothing has run on real hardware, and the test kernel's screen has not been looked at by a person (the Boot UI itself is Stage 4).

## 3g. Self-review before the architecture review

Checked on the code: no TODO, tab indentation, no libc calls or heap in `src/`, no writable global state (the BIOS loader link enforces it), every non-static function has the `jp_` prefix, every header has its source. Fixed in the specification: two places in the BIOS provider sections still said "BIOS keyboard services" (the rule is the PS/2 controller), and the two linker scripts were not mentioned. Not a Stage 1 item: the on-screen list with the 40-second wait and ESC belongs to the Boot UI of Stage 4; in Stage 1 only the test kernel has such a screen, and it is not part of the contract.

## 4. Known limitations and gaps (for the reviewer's attention)

1. ~~Stage 1 is incomplete.~~ **Closed for the code:** every file of sections 1 to 32 is written, and the three platforms pass the QEMU test. What is not done: running on real hardware, the manual virt-manager run, and the review itself.
2. ~~Per-segment lifecycle was not tracked.~~ **Closed.** Each kernel segment now has `JPKernelSegmentState state` (`VALIDATED` -> `ALLOCATED` -> `LOADED` -> `MAPPED` -> `PERMISSIONED`, and a distinct `FAILED`). Validation writes `VALIDATED`; loading advances the state and refuses a segment that is not `VALIDATED`; the handoff requires every segment to be `PERMISSIONED`. `map_segment` creates the mapping together with its rights, so after one successful call the loader writes `PERMISSIONED` directly. The intermediate states are observed by the fake architecture in the tests.
3. ~~Step 12 of the handoff algorithm was not implemented.~~ **Closed.** After the platform step, `jp_boot_prepare_handoff` checks the normalized handoff once more (architecture, entry address, page-table root, paging, interrupts, stack pointer and its alignment) before `READY`; seven kinds of damage injected from the hook are rejected in the tests.
4. The ELF checks do not look at `EI_DATA` (byte order) or `EI_VERSION`; the specification does not list them.
5. The segment placement rule is implemented twice (`kernel_loader.c` and the stack placement in `handoff.c`) because the specified headers expose no shared helper.
6. The hook's restriction ("must not modify any other context state") is not enforced.
7. The text of Stage 1 to Stage 3 does not yet pass the "junior can understand every word" test (it applies to the whole specification).
8. The QEMU level, the on-screen list, the PIT and generic-timer waiting, and the ESC detection have never run on a machine. Device access from the test kernel (the PL011 serial port, framebuffer mapping) depends on a mapping that the specification does not define.
9. Repository state outside the code: the directory `ci/` was renamed to `CI/` in the working tree, and git still tracks the old `ci/` files; `docs/jsl/` is an untracked directory that is not part of this work.

## 5. Conclusion for the review

The generic Stage 1 logic matches the written contract and its tests; the review is asked to confirm the decisions in section 3 (especially 2, 3, 4, and 8) and to rule on the gaps in section 4 (items 2 and 3 in particular) before the provider and architecture code is written.
