#!/usr/bin/env bash
# One automatic QEMU test (level Q) for one platform: builds the disk image,
# runs it in QEMU without a window, and checks the serial log.
#
# Usage: tests/qemu/run_test.sh <platform> <build-dir> [<variant>]
#   <variant> (optional): runs test_kernel_<variant>.elf and checks tests/qemu/expected/<platform>.<variant>.txt
#   (for example stage2_failure: Stage 2 fails on purpose and the run must end with the failed result).
#
# Needs in <build-dir>/qemu/<platform>/ :
#   boot.efi                         the boot application (UEFI platforms)
#   boot_sector.bin and loader.bin   the boot sector and the loader (x86_64-bios)
#   test_kernel.elf                  the test kernel (the automatic variant)
# A missing file means that the part is not built yet: the test fails with a
# message that says which part is missing.
#
# Set JP_QEMU_KVM=1 to run with KVM acceleration (x86_64 platforms only).
#
# Exit status: 0 the test passed; 1 it failed or timed out; 2 a tool or file is missing.

set -euo pipefail

if [ "$#" -lt 2 ] || [ "$#" -gt 3 ]; then
	echo "usage: $0 <platform> <build-dir> [<variant>]" >&2
	exit 2
fi

platform="$1"
build_dir="$2"
variant="${3:-}"
kernel_name="test_kernel.elf"
image_name="disk.img"
if [ -n "$variant" ]; then
	kernel_name="test_kernel_${variant}.elf"
	image_name="disk_${variant}.img"
fi
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
dir="${build_dir}/qemu/${platform}"
log="${dir}/serial${variant:+_${variant}}.log"
timeout_s="${JP_QEMU_TEST_TIMEOUT:-60}"

case "$platform" in
x86_64-bios)
	parts=(boot_sector.bin loader.bin "$kernel_name")
	;;
*)
	parts=(boot.efi "$kernel_name")
	;;
esac
for part in "${parts[@]}"; do
	if [ ! -f "${dir}/${part}" ]; then
		echo "not built yet: ${dir}/${part}" >&2
		exit 2
	fi
done

if [ "$platform" = "x86_64-bios" ]; then
	"${root}/scripts/make_image.sh" "$platform" --boot-sector "${dir}/boot_sector.bin" \
		--loader "${dir}/loader.bin" --kernel "${dir}/${kernel_name}" --out "${dir}/${image_name}"
else
	"${root}/scripts/make_image.sh" "$platform" \
		--boot-app "${dir}/boot.efi" --kernel "${dir}/${kernel_name}" --out "${dir}/${image_name}"
fi

extra=()
if [ "${JP_QEMU_KVM:-0}" = "1" ]; then
	extra+=(--kvm)
fi

set +e
"${root}/scripts/run_qemu.sh" "$platform" --image "${dir}/${image_name}" \
	--nographic --timeout "$timeout_s" --serial-log "$log" "${extra[@]}" > "${dir}/run${variant:+_${variant}}.log" 2>&1
status=$?
set -e

case "$status" in
0 | 1) ;;
124) echo "timeout: the test kernel did not end the run in ${timeout_s} s"; tail -n 20 "$log"; exit 1 ;;
*) echo "QEMU ended with an unexpected status (${status})"; tail -n 20 "${dir}/run${variant:+_${variant}}.log"; exit 2 ;;
esac

"${root}/tests/qemu/check_serial.sh" "$platform" "$log" ${variant:+"$variant"}
