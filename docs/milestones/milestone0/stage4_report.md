# Stage 4 Report: Boot Manager and Boot UI (for the architecture review, Rule 26)

Status: the code is written and tested. The Stage 4 Specification Lock was approved on 2026-11-15 (before the code). The
Implementation Acceptance is not ticked. Nothing was committed.

## 1. Completed tasks

| File | What it does |
| ---- | ------------ |
| `include/justos/boot/stage4_types.h` | all Stage 4 types and the Stage 4 area (included in the middle of `boot_types.h`, like `diagnostic_types.h`); the temporary byte block is gone |
| `boot_config.{h,c}` | the configuration: defaults, validity rules, apply (all or nothing), freeze, get |
| `boot_info.{h,c}` | the read-only summary; names of detection states |
| `boot_ui.{h,c}`, `boot_ui_font.{h,c}` | phase A: the menu (key table, five fields, information panel, rejection message); phase B: the summary and the failure screen as lines of text |
| `boot_manager.{h,c}` | phase A: `prepare`, `select`; phase B: `decide`, `request_recovery`, `jp_kernel_stage2_complete` |
| `uefi_entry.c`, `bios_entry.c` | call `jp_boot_manager_prepare()` before `jp_boot_run_pipeline()` |
| `handoff.c`, `bios.c` | Stage 1 changes found during the review (see section 4, items 1 and 2): the hook runs before the architecture operations; the BIOS keyboard map has the space and the unshifted punctuation |
| `scripts/gen_font.py` | writes a third copy of the font (`boot_ui_font.c`); `--check` checks all three |
| `CMakeLists.txt` | the boot programs get Stage 4 phase A (both policies: `boot.efi`/`loader.bin` DEFAULT_ONLY, `boot_interactive.efi`/`loader_interactive.bin` INTERACTIVE); the kernel gets phase B; the test kernel runs the real Stage 4 |

Tests: `tests/unit/stage4/test_boot_{config,info,ui,ui_text,manager,manager_select}.c`; QEMU: `tests/qemu/run_observed_test.py`.
Rule 40 order was followed for every module (documentation, API, tests, code); the headers were written first.

## 2. How the code was checked

- Host: 43 tests (gcc and clang), warnings are errors. The Stage 4 tests also pass under AddressSanitizer and UBSan.
- **Mutation checks** (`mut.py`, comments skipped). Survivors that are left are equivalent or cannot be reached: the one-past-the-end reads of loops, the check of the line buffer of 80 characters, `records_agree` (a defensive check of three conditions that `jp_kernel_stage2_complete` itself has just made true), and the policy branches that the other build of the same test covers.
- The menu test reads the screen back with the independent font of the test kernel (glyph, place, and colour), and checks that no byte outside the framebuffer is written (guard bytes, padding of every line, small and wide screens, an exact row height, an exact pitch).
- QEMU (level Q), on x86_64 UEFI, x86_64 BIOS (both also under KVM for the automatic run), and AArch64 UEFI, with the real Stage 3 and Stage 4:
  - the normal run (the menu is skipped by the policy; the kernel receives the frozen default configuration; the summary goes out through the real text output);
  - `recovery`: Stage 2 fails with a recoverable result, Stage 4 enters the recovery diagnostics state (a real screen dump is compared pixel by pixel with the expected text);
  - `panic`: Stage 2 fails with a result that cannot be recovered, Stage 4 decides TERMINAL_HALT, Stage 3 shows the panic screen (the same screen check);
  - the menu (x86_64 UEFI, x86_64 BIOS, and AArch64 UEFI): keys are sent through the QEMU control socket; the screen of the menu is checked before and after the keys (`menu_apply`: kernel parameters, three switches, F10; the kernel reports exactly that configuration, frozen; `menu_recovery`: "Recovery requested", F10; the recovery diagnostics state).

## 3. Decisions taken beyond the text of the specification (all in "4 Decided rules")

Decided with the project owner: the phase macros and the link layout; the entry code calls `prepare`; the one-time replacement of the stored decision after a failure while the kernel is being started, and the meaning of "records agree"; the policy constant and the QEMU test of the menu; the two events of phase B, the usable-framebuffer rule, and the third copy of the font.

Derived by me while writing the code (please review): the exact lines of the phase B screens; the summary also lists the events when `verbose_output` is on (the contract says so, the file specification does not); a character that is not printable ASCII is ignored in the menu; a failure of `apply` other than a rejection is returned and the menu is left; `decide` returns the result of `jp_boot_recovery_decide` unchanged when it fails, and `jp_kernel_stage2_complete` then halts; a failure to show a screen adds one WARNING event; the manager state `RECOVERY_REQUESTED` is set only after the report is final, so a rejected request is not "used up"; the menu redraws after a key, not on every poll; a framebuffer line is cut at 80 columns.

## 4. Known limitations and gaps (for the reviewer's attention)

1. **Found and fixed during the review: the menu did not get keys on AArch64 UEFI.** The cause was in Stage 1: `jp_boot_prepare_handoff()` ran the interaction hook after `prepare_entry`, which masks the processor interrupts. The AArch64 firmware drives its USB and serial keyboards from timer interrupts, so `ReadKeyStroke` always answered NOT_READY (x86_64 reads the PS/2 controller directly, so it did not show). Experiment: with the mask removed for the time of the menu, the keys worked at once. Decision of the project owner: the hook runs before the architecture operations (after the stack is reserved). Changed (documentation first): the section "Pre-Transition Interaction Point", the algorithm and the decided rules of `handoff.c` in `milestone0.md`; `src/boot/handoff.c`; `tests/unit/boot/test_handoff.c` (the hook sees `UNINITIALIZED`, zero architecture calls, the interrupts not yet masked, the stack already reserved; the damage test of the final check now damages the handoff in the platform step through the new `on_prepare_handoff` of the fake environment); mutation check: the old order makes `test_handoff` fail. The AArch64 tests now run `menu_apply` and `menu_recovery` with real keys (the temporary `menu_screen` is removed).
2. **Found and fixed: the BIOS keyboard map of Stage 1 had only the letter and digit rows**, so a space and punctuation could not be typed. Decision of the project owner: extend it. The scan code table in `milestone0.md` (`bios.c`, "Input") now lists the unshifted punctuation keys and the space (`-` `=` `[` `]` `;` `'` `` ` `` `\` `,` `.` `/`); Shift is not tracked, so upper-case letters and the upper-half symbols are not available in Milestone 0. `test_bios` covers every new key and the neighbours that stay unmapped; the QEMU `menu_apply` test types `quiet debug=1-x`.
3. **Found and fixed during the manual check under KVM (virt-manager): the menu stayed on the screen under the recovery screen.** Stage 3 clears only the lines it writes, so after "menu, then Recovery requested, then F10" the old menu showed through below the five diagnostic lines. The specification did not say who clears the screen; the project owner decided that the menu does (small rule 6 of the menu in "4 Decided rules"): `jp_boot_ui_select` fills the whole framebuffer with black when it ends after it was drawn. Unit test `test_screen_is_cleared_on_exit` (Esc, F10, keyboard failures; the line padding and guards stay untouched) with a mutation check, and the QEMU test `menu_recovery` now also requires that nothing is lit outside the Stage 3 lines (checked to fail without the fix). Not covered by this decision: text of the firmware left under Stage 3 screens in runs without the menu (only the lines Stage 3 writes are cleared there).
4. **Found and fixed during the manual check in virt-manager: AArch64 hung on the processor `max` (the libvirt default).** The cause was in Stage 1: `jp_aarch64_jump_to_kernel` wrote `MAIR_EL1`, `TCR_EL1`, and `TTBR` while the MMU of the firmware was still on (firmware at EL1). On a processor with `LPA2` the firmware sets `TCR_EL1.DS`, and the new `TCR_EL1` made the processor read the live firmware tables in the other format; the next instruction fetch failed with a permission fault and the exception handler looped (traced with the QEMU exception log; the faulting address was the instruction right after `msr tcr_el1`). The automatic tests did not see it because they use `cortex-a72`. Decision of the project owner: at EL1 the jump switches the MMU off first (step 1a of "The jump" in `milestone0.md`, with the reason). New QEMU tests `qemu_aarch64-uefi_cpu_max` (`JP_QEMU_CPU=max`; failed before the fix) and `qemu_aarch64-uefi_el2` (`JP_QEMU_MACHINE=virt,virtualization=on`, the EL2 path, which was only run by hand before). Not checked: a real processor with LPA2 (level M), and the state of the data cache when the MMU is switched off (the function uses only registers in that short window).
5. `records_agree` and the failure path of `freeze` cannot be reached from `jp_kernel_stage2_complete` and are covered only by reading.
6. Retry never runs in Milestone 0 (the maximum is 0); the conversion `RETRY_BOOT` is tested with a larger limit only.
7. The recovery diagnostics state is the Stage 3 stopping routine; `JP_BOOT_TRANSITION_ACCEPTED` cannot be seen in a real run (as the specification says).
8. The older text of Stages 1 to 3 was not rewritten for a junior reader.
9. Nothing was run on real hardware (level M). The menu was driven through QEMU's key injection and, by hand, in virt-manager under KVM (x86_64 UEFI scenarios `normal`, `menu`, `recovery`, `panic`; x86_64 BIOS; AArch64 UEFI).
10. The Specification Lock of Stage 4 was signed before the code; this stage added the "4 Decided rules" block to the specification, so the owner should confirm that it is part of the lock.

## 5. The list of fakes

All Stage 4 stand-ins are gone (`fake_stage2_complete.c`, `fake_boot_manager*.c`, `fake_stage3_lines.*`). What is left is on the list in the project memory (`fake_elf_failure.c` and the inline fakes of unit tests).
