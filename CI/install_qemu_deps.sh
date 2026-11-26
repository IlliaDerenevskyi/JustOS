#!/usr/bin/env bash
# Installs what the QEMU tests need on a Debian/Ubuntu CI runner and tells the
# next steps where the UEFI firmware is.
#
# Usage: CI/install_qemu_deps.sh <x86_64|aarch64>
#   x86_64   QEMU for x86_64, OVMF (UEFI), SeaBIOS (comes with QEMU).
#   aarch64  QEMU for AArch64 and its UEFI firmware (AAVMF).
#
# Both kinds also get the tools that build the images: clang, lld, mtools,
# dosfstools, and Python with PIL (for scripts/gen_font.py --check).
#
# The firmware paths are found with `find`, because their names differ between
# Ubuntu releases. They are written to $GITHUB_ENV (when it is set) as
# JP_OVMF_CODE, JP_OVMF_VARS, JP_AAVMF_CODE, which scripts/run_qemu.sh reads.
# Without $GITHUB_ENV they are only printed.

set -euo pipefail

if [ "$#" -ne 1 ]; then
	echo "usage: $0 <x86_64|aarch64>" >&2
	exit 2
fi
arch="$1"

common=(clang lld mtools dosfstools python3 python3-pil fonts-dejavu-core)
case "$arch" in
x86_64) packages=("${common[@]}" qemu-system-x86 ovmf) ;;
aarch64) packages=("${common[@]}" qemu-system-arm qemu-efi-aarch64) ;;
*)
	echo "unknown architecture: $arch" >&2
	exit 2
	;;
esac

sudo apt-get update -qq
sudo apt-get install -y --no-install-recommends "${packages[@]}"

export_variable() {
	echo "$1=$2"
	if [ -n "${GITHUB_ENV:-}" ]; then
		echo "$1=$2" >> "$GITHUB_ENV"
	fi
}

# find_first <directory> <file name pattern>: the first match, or an error.
find_first() {
	local found
	found="$(find "$1" -name "$2" 2>/dev/null | sort | head -n 1)"
	if [ -z "$found" ]; then
		echo "::error::no file '$2' under $1 after installing ${packages[*]}" >&2
		exit 1
	fi
	echo "$found"
}

if [ "$arch" = "x86_64" ]; then
	export_variable JP_OVMF_CODE "$(find_first /usr/share OVMF_CODE_4M.fd)"
	export_variable JP_OVMF_VARS "$(find_first /usr/share OVMF_VARS_4M.fd)"
else
	export_variable JP_AAVMF_CODE "$(find_first /usr/share QEMU_EFI.fd)"
fi
