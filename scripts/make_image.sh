#!/usr/bin/env bash
# Builds a bootable disk image for QEMU or a USB stick (no root rights needed).
#
# UEFI (x86_64-uefi, aarch64-uefi):
#   scripts/make_image.sh <platform> --boot-app FILE --kernel FILE --out FILE
#   One FAT file system (written with mtools) with
#     /EFI/BOOT/BOOTX64.EFI (or BOOTAA64.EFI)   the boot application
#     /EFI/JustOS/kernel.elf                    the kernel
#
# BIOS (x86_64-bios):
#   scripts/make_image.sh x86_64-bios --boot-sector FILE --loader FILE --kernel FILE --out FILE
#   A fixed sequence of 512-byte sectors (see the BIOS disk image layout in
#   milestone0.md): sector 0 the boot sector (512 bytes, ends with 0x55 0xAA),
#   sector 1 the image header, sectors 2.. the loader, sector 2048 and on the kernel.
#
# Other option: --size-mib N (UEFI image size, default 64).

set -euo pipefail

usage() {
	sed -n '2,/^$/p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//' >&2
	exit 2
}

if [ "$#" -lt 1 ]; then
	usage
fi

platform="$1"
shift
boot_app=""
boot_sector=""
loader=""
kernel=""
out=""
size_mib=64

while [ "$#" -gt 0 ]; do
	case "$1" in
	--boot-app) boot_app="$2"; shift 2 ;;
	--boot-sector) boot_sector="$2"; shift 2 ;;
	--loader) loader="$2"; shift 2 ;;
	--kernel) kernel="$2"; shift 2 ;;
	--out) out="$2"; shift 2 ;;
	--size-mib) size_mib="$2"; shift 2 ;;
	*) echo "unknown option: $1" >&2; usage ;;
	esac
done

require_file() {
	if [ -z "$1" ] || [ ! -f "$1" ]; then
		echo "missing input file for $2: '$1'" >&2
		exit 2
	fi
}

if [ -z "$out" ]; then
	echo "give --out" >&2
	exit 2
fi

sector=512
kernel_lba=2048
loader_lba=2

make_uefi() {
	local boot_name="$1"

	require_file "$boot_app" --boot-app
	require_file "$kernel" --kernel
	mkdir -p "$(dirname "$out")"
	rm -f "$out"
	truncate -s "${size_mib}M" "$out"
	mformat -i "$out" -F ::
	mmd -i "$out" ::/EFI ::/EFI/BOOT ::/EFI/JustOS
	mcopy -i "$out" "$boot_app" "::/EFI/BOOT/${boot_name}"
	mcopy -i "$out" "$kernel" ::/EFI/JustOS/kernel.elf
}

make_bios() {
	local loader_size loader_sectors kernel_size total_sectors

	require_file "$boot_sector" --boot-sector
	require_file "$loader" --loader
	require_file "$kernel" --kernel

	if [ "$(stat -c %s "$boot_sector")" -ne "$sector" ]; then
		echo "the boot sector file must be exactly ${sector} bytes" >&2
		exit 2
	fi
	if [ "$(tail -c 2 "$boot_sector" | od -An -tx1 | tr -d ' \n')" != "55aa" ]; then
		echo "the boot sector must end with the bytes 0x55 0xAA" >&2
		exit 2
	fi
	loader_size="$(stat -c %s "$loader")"
	loader_sectors=$(((loader_size + sector - 1) / sector))
	kernel_size="$(stat -c %s "$kernel")"
	if [ "$kernel_size" -eq 0 ]; then
		echo "the kernel file is empty" >&2
		exit 2
	fi
	if [ "$loader_sectors" -gt 112 ]; then
		echo "the loader is larger than 56 KiB (it must end before the boot data block at 0x10000)" >&2
		exit 2
	fi
	if [ $((loader_lba + loader_sectors)) -gt "$kernel_lba" ]; then
		echo "the loader does not fit before sector ${kernel_lba}" >&2
		exit 2
	fi

	mkdir -p "$(dirname "$out")"
	rm -f "$out"
	total_sectors=$((kernel_lba + (kernel_size + sector - 1) / sector))
	# Round the image up to whole mebibytes (at least 16).
	total_sectors=$(((total_sectors * sector + 1048575) / 1048576 * 1048576 / sector))
	if [ "$total_sectors" -lt $((16 * 1048576 / sector)) ]; then
		total_sectors=$((16 * 1048576 / sector))
	fi
	truncate -s $((total_sectors * sector)) "$out"

	dd if="$boot_sector" of="$out" bs=$sector seek=0 conv=notrunc status=none
	python3 -I - "$out" "$loader_lba" "$loader_sectors" "$kernel_lba" "$kernel_size" "$sector" <<'PY'
import struct, sys
out, loader_lba, loader_sectors, kernel_lba, kernel_size, sector = sys.argv[1], *map(int, sys.argv[2:])
header = b"JUSTBIOS" + struct.pack("<IIIQQ", 1, loader_lba, loader_sectors, kernel_lba, kernel_size)
header = header.ljust(sector, b"\0")
with open(out, "r+b") as f:
	f.seek(sector)
	f.write(header)
PY
	dd if="$loader" of="$out" bs=$sector seek=$loader_lba conv=notrunc status=none
	dd if="$kernel" of="$out" bs=$sector seek=$kernel_lba conv=notrunc status=none
}

case "$platform" in
x86_64-uefi) make_uefi "BOOTX64.EFI" ;;
aarch64-uefi) make_uefi "BOOTAA64.EFI" ;;
x86_64-bios) make_bios ;;
*) echo "unknown platform: ${platform}" >&2; usage ;;
esac
echo "image written: ${out}"
