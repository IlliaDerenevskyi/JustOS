#!/usr/bin/env bash
# Checks the serial log of a test-kernel run against the expectations of a
# platform (the "serial protocol" of milestone0.md).
#
# Usage: tests/qemu/check_serial.sh <platform> <serial-log> [<variant>]
#   <platform> is x86_64-uefi, x86_64-bios, or aarch64-uefi.
#   <variant> (optional) selects tests/qemu/expected/<platform>.<variant>.txt, for a run that
#   must end in a failure on purpose (for example stage2_failure).
#
# Rules:
#   - only lines that start with "JP|" are looked at (firmware and kernel
#     messages in the same log are ignored);
#   - the last "JP|" line must be "JP|RESULT=PASS", or, when the expectation file has a line
#     "@result <text>", exactly <text> (a run that must fail on purpose);
#   - every non-empty line of tests/qemu/expected/<platform>.txt that does not
#     start with '#' is an extended regular expression that must match at least
#     one "JP|" line; a line that starts with '!' must match none.
#
# Exit status: 0 all checks passed; 1 a check failed (the reason is printed);
# 2 wrong usage or a missing file.

set -euo pipefail

if [ "$#" -lt 2 ] || [ "$#" -gt 3 ]; then
	echo "usage: $0 <platform> <serial-log> [<variant>]" >&2
	exit 2
fi

platform="$1"
log="$2"
variant="${3:-}"
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
if [ -n "$variant" ]; then
	expected="${root}/tests/qemu/expected/${platform}.${variant}.txt"
else
	expected="${root}/tests/qemu/expected/${platform}.txt"
fi

if [ ! -f "$expected" ]; then
	echo "no expectation file for platform '${platform}': ${expected}" >&2
	exit 2
fi
if [ ! -f "$log" ]; then
	echo "no serial log: ${log}" >&2
	exit 2
fi

lines="$(mktemp)"
trap 'rm -f "$lines"' EXIT
# The serial console may add carriage returns.
tr -d '\r' < "$log" | grep -a '^JP|' > "$lines" || true

failures=0

if [ ! -s "$lines" ]; then
	echo "FAIL: the log contains no JP| line (the test kernel did not report)"
	exit 1
fi

required_result="JP|RESULT=PASS"
wanted="$(grep -a '^@result ' "$expected" | head -n 1 || true)"
if [ -n "$wanted" ]; then
	required_result="${wanted#@result }"
fi
last="$(tail -n 1 "$lines")"
if [ "$last" != "$required_result" ]; then
	echo "FAIL: the last JP| line is '${last}', expected '${required_result}'"
	failures=$((failures + 1))
fi

while IFS= read -r pattern || [ -n "$pattern" ]; do
	case "$pattern" in
	'' | '#'* | '@'*) continue ;;
	'!'*)
		forbidden="${pattern#!}"
		if grep -aEq -- "$forbidden" "$lines"; then
			echo "FAIL: forbidden line present: ${forbidden}"
			grep -aE -- "$forbidden" "$lines" | head -n 3 | sed 's/^/        /'
			failures=$((failures + 1))
		fi
		;;
	*)
		if ! grep -aEq -- "$pattern" "$lines"; then
			echo "FAIL: no line matches: ${pattern}"
			failures=$((failures + 1))
		fi
		;;
	esac
done < "$expected"

if [ "$failures" -ne 0 ]; then
	echo "${failures} check(s) failed for ${platform}"
	exit 1
fi
echo "ok: ${platform}"
