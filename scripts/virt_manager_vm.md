# Running a JustOS disk image in virt-manager (manual check)

This is the manual way to check the boot on a virtual machine with KVM
acceleration and to read the data list that the test kernel shows. The
automatic tests do not use virt-manager; they use `scripts/run_qemu.sh`
(see `tests/qemu/run_test.sh`).

KVM runs only guests of the host architecture. On an x86_64 host this covers
x86_64 with UEFI and x86_64 with BIOS. AArch64 with UEFI can only run in QEMU
software emulation (`scripts/run_qemu.sh aarch64-uefi`).

## What you need

- Packages from `requirements.txt` (optional section): `libvirt`, `virt-manager`.
- The libvirt service running (`systemctl status libvirtd`) and your user in the
  `libvirt` and `kvm` groups.
- A disk image, for example `build/qemu/x86_64-uefi/disk.img`, made by
  `scripts/make_image.sh` (a boot application and a kernel inside a FAT file system).

## x86_64 with UEFI

1. In virt-manager choose "Create a new virtual machine" and "Import existing disk image".
2. Select `disk.img`. Operating system: "Generic or unknown".
3. Memory 512 MiB, 1 or more CPUs. On the last page tick "Customize configuration before install".
4. Overview: Chipset = Q35, Firmware = UEFI (the OVMF file without Secure Boot).
5. Disk: format `raw`. Add Hardware, Serial: device type "File" (or "Pseudo TTY") so
   that the `JP|...` lines can be saved. Begin Installation.
6. The data list of the test kernel appears on the screen and on the serial output.
   Check that the last serial line is `JP|RESULT=PASS`.

Command line form (the same machine without clicking):

```bash
virt-install --name justos-test --memory 512 --vcpus 2 \
  --import --disk path=build/qemu/x86_64-uefi/disk.img,format=raw,bus=sata \
  --boot uefi --machine q35 --os-variant generic \
  --serial file,path=/tmp/justos-serial.log --graphics spice --noautoconsole
virsh console justos-test        # or open it in virt-manager
virsh destroy justos-test; virsh undefine justos-test --nvram   # clean up
```

Compare the file with the expectations:

```bash
tests/qemu/check_serial.sh x86_64-uefi /tmp/justos-serial.log
```

## x86_64 with BIOS

The same steps with Firmware = BIOS (SeaBIOS) and the BIOS disk image
(`scripts/make_image.sh x86_64-bios ...`; the layout is in `milestone0.md`, in the
specification of `src/boot/platform/bios.c`). Import it as a raw disk.

## Limits

- libvirt has no ready `isa-debug-exit` device, so the machine does not leave with
  a PASS/FAIL exit code here. Read `JP|RESULT=...` instead. The automatic exit-code
  check stays in `scripts/run_qemu.sh` (with `--kvm` for hardware acceleration).
- With KVM the guest sees the real host processor. This is useful (it finds code
  that assumes a particular CPU) but results can differ from software emulation.
