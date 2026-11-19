#!/usr/bin/env bash
# Self-test of check_serial.sh: a good log passes, and every kind of broken log
# is rejected. Runs on the host and needs no QEMU.

set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
check="${here}/check_serial.sh"
good="${here}/samples/pass_x86_64_uefi.log"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

expect_ok() {
	"$check" x86_64-uefi "$1" > /dev/null || { echo "expected ok: $2"; exit 1; }
}
expect_fail() {
	if "$check" x86_64-uefi "$1" > /dev/null; then
		echo "expected failure: $2"
		exit 1
	fi
}

expect_ok "$good" "good log"

# CRLF line endings of a serial console.
sed 's/$/\r/' "$good" > "$work/crlf.log"
expect_ok "$work/crlf.log" "CRLF log"

# RESULT is FAIL.
sed 's/^JP|RESULT=PASS$/JP|RESULT=FAIL|memory.state/' "$good" > "$work/fail.log"
expect_fail "$work/fail.log" "RESULT=FAIL"

# RESULT line missing.
grep -v '^JP|RESULT=' "$good" > "$work/noresult.log"
expect_fail "$work/noresult.log" "no RESULT line"

# RESULT is not the last JP line.
{ grep -v '^JP|RESULT=' "$good"; echo 'JP|RESULT=PASS'; echo 'JP|interaction.state=COMPLETED'; } > "$work/notlast.log"
expect_fail "$work/notlast.log" "RESULT not last"

# A required line is missing.
grep -v '^JP|kernel.entry=' "$good" > "$work/noentry.log"
expect_fail "$work/noentry.log" "missing kernel.entry"

# No usable memory region.
sed 's/,USABLE$/,RESERVED/' "$good" > "$work/nousable.log"
expect_fail "$work/nousable.log" "no usable memory"

# A forbidden state is present.
sed 's/^JP|cpu.state=DETECTED$/JP|cpu.state=FAILED/' "$good" > "$work/failedstate.log"
expect_fail "$work/failedstate.log" "FAILED state"
sed 's/^JP|boot_device.state=DETECTED$/JP|boot_device.state=UNKNOWN/' "$good" > "$work/unknown.log"
expect_fail "$work/unknown.log" "UNKNOWN state"

# Wrong platform data: an AArch64 log is not accepted for x86_64.
sed 's/^JP|machine=x86_64$/JP|machine=aarch64/' "$good" > "$work/wrongmachine.log"
expect_fail "$work/wrongmachine.log" "wrong machine"

# Empty log and a log without any JP line.
: > "$work/empty.log"
expect_fail "$work/empty.log" "empty log"
echo "nothing here" > "$work/nojp.log"
expect_fail "$work/nojp.log" "no JP lines"

# The entry must be a high address.
sed 's/^JP|kernel.entry=.*/JP|kernel.entry=0x100000/' "$good" > "$work/lowentry.log"
expect_fail "$work/lowentry.log" "low entry"

# Stack pointer 0 is not a stack.
sed 's/^JP|handoff.stack_pointer=.*/JP|handoff.stack_pointer=0x0/' "$good" > "$work/nostack.log"
expect_fail "$work/nostack.log" "zero stack"

# Missing files: exit status 2.
set +e
"$check" x86_64-uefi "$work/does-not-exist.log" > /dev/null 2>&1
[ "$?" -eq 2 ] || { echo "missing log must give status 2"; exit 1; }
"$check" nosuch-platform "$good" > /dev/null 2>&1
[ "$?" -eq 2 ] || { echo "unknown platform must give status 2"; exit 1; }
set -e

echo "check_serial self-test passed"
