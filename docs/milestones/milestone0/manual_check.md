# Manual check of Milestone 0 (level M, under KVM or on a machine)

This is the list of runs a person does by hand after the last stage. The automatic tests (`ctest`) already pass these scenarios in QEMU; here the same scenarios are run with hardware acceleration (KVM) and, for UEFI, from a USB stick. Nothing here is a new requirement; it is a checklist.

## 1. Build the images

```bash
cmake -S . -B build/q -DJP_ENABLE_QEMU_TESTS=ON
cmake --build build/q -j"$(nproc)"
scripts/make_manual_images.sh build/q        # writes build/q/manual/<platform>/<scenario>.img
```

## 2. Run one image

With a window (the keyboard goes to the guest; the serial output goes to the terminal):

```bash
scripts/run_qemu.sh x86_64-uefi --kvm --image build/q/manual/x86_64-uefi/normal.img
scripts/run_qemu.sh x86_64-bios --kvm --image build/q/manual/x86_64-bios/menu.img
scripts/run_qemu.sh aarch64-uefi   --image build/q/manual/aarch64-uefi/panic.img     # AArch64: no KVM on an x86_64 host
```

Without a window, only the serial output (the menu is skipped, because it needs a screen):

```bash
scripts/run_qemu.sh x86_64-uefi --kvm --nographic --timeout 20 --image build/q/manual/x86_64-uefi/normal.img
```

In virt-manager: see `scripts/virt_manager_vm.md` (import the same `.img` as a raw disk; Firmware = UEFI or BIOS; add a serial device that writes to a file).

## 3. The scenarios and what must be seen

| Scenario | What happens | What must be seen |
|---|---|---|
| `normal` | no menu, the kernel starts | the data list on the screen and on the serial port; the last serial line is `JP|RESULT=PASS`; `JP|stage4.decision=CONTINUE_TO_KERNEL`, `JP|stage4.config.source=DEFAULT`; the kernel then waits 40 seconds or until ESC |
| `menu` | the menu with five fields | the screen "JustOS boot menu"; Up/Down change the field; Enter edits the kernel parameters (type text, Enter to finish) or toggles a switch; F10 applies; Esc keeps the defaults. After F10 the data list shows your values (`JP|stage4.config.source=INTERACTIVE`, `JP|stage4.config.kernel_parameters=<your text>`, ...) |
| `recovery` | Stage 2 fails with a recoverable result | the screen "JustOS boot diagnostics" with `Status: RECOVERY` and `Error: JP_BOOT_FAILED in STAGE2`; the machine stops (nothing else happens) |
| `panic` | Stage 2 fails with a non-recoverable result | the screen "JustOS boot failure" with `result: JP_BOOT_ERROR_INVALID_PARAM`, `severity: FATAL`, `recoverability: NON_RECOVERABLE`; the machine stops |

Platforms: `x86_64-uefi` and `x86_64-bios` (both with `--kvm`), and `aarch64-uefi` (QEMU software emulation only).

Keys in the menu on BIOS: letters, digits, space, and the unshifted punctuation `- = [ ] ; ' \` , . /`. Shift is not tracked, so upper-case letters cannot be typed there.

## 4. Extra variations worth a look

- Different memory sizes: add `-m 128`, `-m 2048`, `-m 8192` (edit `run_qemu.sh` or run QEMU by hand); the memory lines of the data list must follow, and `JP|RESULT` must stay `PASS`.
- More processors and another screen size (virt-manager: Video = VGA with a different resolution): the framebuffer lines and the text area must follow.
- The menu without a framebuffer or without a keyboard (virt-manager: remove the display or the keyboard): the menu must be skipped, `JP|stage4.selection=SKIPPED_NO_FRAMEBUFFER` or `SKIPPED_NO_INPUT`, and the boot goes on.
- The `.img` of a UEFI scenario written to a USB stick (`dd if=... of=/dev/sdX bs=4M conv=fsync`; check the device name twice, `dd` destroys the target) and booted on a real machine with Secure Boot off.

## 5. Limits

- libvirt has no `isa-debug-exit` device, so in virt-manager the machine does not exit with a PASS/FAIL code; read `JP|RESULT=` on the serial file.
- With KVM the guest sees the real host processor; results can differ from software emulation. A difference is a finding, not necessarily a defect: write it down.
