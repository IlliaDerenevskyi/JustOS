#!/usr/bin/env bash
# Builds the boot code and the test kernel for the given platforms and runs the
# QEMU tests (level Q) for them.
#
# Usage: CI/qemu_tests.sh <platform>...
#   platform is x86_64-uefi, x86_64-bios, or aarch64-uefi.
#
# Environment (read by scripts/run_qemu.sh): JP_OVMF_CODE, JP_OVMF_VARS,
# JP_AAVMF_CODE. CI/install_qemu_deps.sh sets them on a CI runner.
#
# The KVM tests (label kvm) are left out: a CI runner does not promise
# /dev/kvm, so level Q-KVM stays a local check. Any failure is a failure: there
# is no "build incomplete" here, because these tests exist only for finished
# code.

set -euo pipefail

if [ "$#" -lt 1 ]; then
	echo "usage: $0 <platform>..." >&2
	exit 2
fi

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${root}/build/ci-qemu"
platforms="$(IFS=';'; echo "$*")"

cd "$root"

echo "::group::Configure (${platforms})"
cmake -S . -B "$build_dir" -DCMAKE_C_COMPILER=clang \
	-DJP_ENABLE_QEMU_TESTS=ON "-DJP_QEMU_PLATFORMS=${platforms}"
echo "::endgroup::"

echo "::group::Build (${platforms})"
cmake --build "$build_dir" --parallel
echo "::endgroup::"

echo "::group::QEMU tests (${platforms})"
status=0
ctest --test-dir "$build_dir" -L qemu -LE kvm --output-on-failure || status=$?
if [ "$status" -ne 0 ]; then
	# The run scripts keep the output of QEMU itself (its errors) in run*.log; show it, because CI cannot open the files.
	for log in "$build_dir"/qemu/*/run*.log; do
		[ -f "$log" ] || continue
		echo "::group::QEMU output: ${log#"$build_dir"/qemu/}"
		tail -n 30 "$log"
		echo "::endgroup::"
	done
fi
echo "::endgroup::"
exit "$status"
