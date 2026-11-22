#!/usr/bin/env bash
# Builds the disk images for a manual check (KVM, virt-manager, or a USB stick).
#
# Usage: scripts/make_manual_images.sh [BUILD_DIR]     (default: build/q, a build with -DJP_ENABLE_QEMU_TESTS=ON)
#
# For every platform (x86_64-uefi, x86_64-bios, aarch64-uefi) it writes
# BUILD_DIR/manual/<platform>/<scenario>.img:
#   normal     the boot program without a menu (policy DEFAULT_ONLY) and the test kernel, which shows its data
#              list and waits 40 seconds or ESC
#   menu       the boot program with the menu (policy INTERACTIVE) and the same test kernel
#   recovery   Stage 2 fails on purpose with a result that can be recovered from: the recovery diagnostics screen
#   panic      Stage 2 fails on purpose with a result that cannot be recovered from: the panic screen
# What each scenario must show is in docs/milestones/milestone0/manual_check.md.

set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="${1:-${root}/build/q}"

make_one() {
	local platform="$1" scenario="$2" boot="$3" kernel="$4"
	local dir="${build}/qemu/${platform}"
	local out="${build}/manual/${platform}/${scenario}.img"
	mkdir -p "$(dirname "$out")"
	case "$platform" in
	x86_64-bios)
		"${root}/scripts/make_image.sh" "$platform" --boot-sector "${dir}/boot_sector.bin" \
			--loader "${dir}/${boot}.bin" --kernel "${dir}/${kernel}" --out "$out" > /dev/null ;;
	*)
		"${root}/scripts/make_image.sh" "$platform" --boot-app "${dir}/${boot}.efi" \
			--kernel "${dir}/${kernel}" --out "$out" > /dev/null ;;
	esac
	echo "$out"
}

for platform in x86_64-uefi x86_64-bios aarch64-uefi; do
	if [ "$platform" = x86_64-bios ]; then plain=loader; menu=loader_interactive; else plain=boot; menu=boot_interactive; fi
	make_one "$platform" normal "$plain" test_kernel_manual.elf
	make_one "$platform" menu "$menu" test_kernel_manual.elf
	make_one "$platform" recovery "$plain" test_kernel_recovery.elf
	make_one "$platform" panic "$plain" test_kernel_panic.elf
done
