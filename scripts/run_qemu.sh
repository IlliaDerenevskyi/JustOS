#!/usr/bin/env bash
# Starts a JustOS disk image in QEMU.
#
# Usage: scripts/run_qemu.sh <platform> [options]
#   <platform>        x86_64-uefi, x86_64-bios, or aarch64-uefi
#
# Options:
#   --image FILE      disk image (default: build/qemu/<platform>/disk.img)
#   --nographic       no window: the serial output goes to the serial log file
#                     and is printed when the run ends; the boot menu is skipped
#                     (it needs a screen and a keyboard). This is the mode of the
#                     automatic tests.
#   --serial-log FILE where --nographic writes the serial output
#                     (default: build/qemu/<platform>/serial.log)
#   --timeout SEC     stop the run after SEC seconds (default: no limit; the
#                     automatic tests use 60)
#   --qmp SOCKET      also open a QMP control socket (a unix socket file); the panic test uses it to
#                     take a screen dump (screendump) of what the guest shows
#   --gdb             start stopped and wait for gdb on tcp::1234
#   --kvm             hardware acceleration (KVM) with the real host CPU
#                     (-accel kvm -cpu host). Only for x86_64-uefi and
#                     x86_64-bios: KVM runs only guests of the host
#                     architecture, so aarch64-uefi is refused.
#
# Exit status (one convention for all platforms):
#   0    the test kernel reported PASS (isa-debug-exit 33, or semihosting exit 0)
#   1    the test kernel reported FAIL (isa-debug-exit 35, or semihosting exit 1)
#   124  the time limit was reached
#   2    any other QEMU exit, or a usage error, or a missing file
#   Without a test kernel (a normal interactive run) the status of QEMU itself
#   is passed through the same table.
#
# Firmware files can be overridden by the environment variables
# JP_OVMF_CODE, JP_OVMF_VARS (x86_64 UEFI) and JP_AAVMF_CODE (AArch64 UEFI).

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

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
out_dir="${root}/build/qemu/${platform}"
image="${out_dir}/disk.img"
serial_log="${out_dir}/serial.log"
nographic=false
gdb=false
qmp_socket=""
kvm=false
timeout_s=0

while [ "$#" -gt 0 ]; do
	case "$1" in
	--image) image="$2"; shift 2 ;;
	--serial-log) serial_log="$2"; shift 2 ;;
	--timeout) timeout_s="$2"; shift 2 ;;
	--nographic) nographic=true; shift ;;
	--qmp) qmp_socket="$2"; shift 2 ;;
	--gdb) gdb=true; shift ;;
	--kvm) kvm=true; shift ;;
	-h | --help) usage ;;
	*) echo "unknown option: $1" >&2; usage ;;
	esac
done

require_file() {
	if [ ! -f "$1" ]; then
		echo "missing file: $1 ($2)" >&2
		exit 2
	fi
}

require_file "$image" "build the disk image first (scripts/make_image.sh)"
mkdir -p "$(dirname "$serial_log")"

args=()
case "$platform" in
x86_64-uefi)
	code="${JP_OVMF_CODE:-/usr/share/edk2/x64/OVMF_CODE.4m.fd}"
	vars_template="${JP_OVMF_VARS:-/usr/share/edk2/x64/OVMF_VARS.4m.fd}"
	require_file "$code" "UEFI firmware; set JP_OVMF_CODE"
	require_file "$vars_template" "UEFI variables; set JP_OVMF_VARS"
	vars="$(mktemp)"
	trap 'rm -f "$vars"' EXIT
	cp "$vars_template" "$vars"
	qemu=qemu-system-x86_64
	args+=(-machine q35 -m 512
		-drive "if=pflash,format=raw,readonly=on,file=${code}"
		-drive "if=pflash,format=raw,file=${vars}"
		-drive "format=raw,file=${image}"
		-device isa-debug-exit,iobase=0xf4,iosize=0x04)
	;;
x86_64-bios)
	qemu=qemu-system-x86_64
	args+=(-machine q35 -m 512 -boot c
		-drive "format=raw,file=${image}"
		-device isa-debug-exit,iobase=0xf4,iosize=0x04)
	;;
aarch64-uefi)
	code="${JP_AAVMF_CODE:-/usr/share/edk2/aarch64/QEMU_EFI.fd}"
	require_file "$code" "AArch64 UEFI firmware; set JP_AAVMF_CODE"
	qemu=qemu-system-aarch64
	args+=(-machine virt -cpu cortex-a72 -m 512
		-bios "$code"
		-drive "if=none,id=disk0,format=raw,file=${image}"
		-device virtio-blk-pci,drive=disk0
		-device ramfb -device qemu-xhci -device usb-kbd
		-semihosting-config enable=on,target=native)
	;;
*)
	echo "unknown platform: ${platform}" >&2
	usage
	;;
esac

if "$kvm"; then
	case "$platform" in
	x86_64-*) ;;
	*)
		echo "--kvm is not possible for ${platform}: KVM runs only guests of the host architecture (x86_64)" >&2
		exit 2
		;;
	esac
	if [ ! -r /dev/kvm ] || [ ! -w /dev/kvm ]; then
		echo "--kvm: /dev/kvm is not accessible (is the kvm module loaded, and is your user in the kvm group?)" >&2
		exit 2
	fi
	args+=(-accel kvm -cpu host)
fi

if "$nographic"; then
	: > "$serial_log"
	args+=(-display none -monitor none -serial "file:${serial_log}")
else
	args+=(-serial stdio)
fi
if [ -n "$qmp_socket" ]; then
	args+=(-qmp "unix:${qmp_socket},server,nowait")
fi
if "$gdb"; then
	args+=(-S -gdb tcp::1234)
fi

command=("$qemu" "${args[@]}")
if [ "$timeout_s" != "0" ]; then
	command=(timeout --signal=KILL "$timeout_s" "${command[@]}")
fi

set +e
"${command[@]}"
status=$?
set -e

if "$nographic"; then
	cat "$serial_log"
fi

case "$platform" in
x86_64-*)
	case "$status" in
	33) exit 0 ;;
	35) exit 1 ;;
	esac
	;;
aarch64-*)
	case "$status" in
	0) exit 0 ;;
	1) exit 1 ;;
	esac
	;;
esac
# timeout --signal=KILL ends with 137; plain timeout with 124.
if [ "$status" -eq 124 ] || [ "$status" -eq 137 ]; then
	exit 124
fi
exit 2
