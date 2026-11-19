#!/usr/bin/env bash
# Configures, builds, and tests the project with one compiler.
#
# Usage: CI/build_and_test.sh <compiler>
#   <compiler> is the C compiler command, for example gcc or clang.
#
# Environment:
#   CI_STRICT=true  any build failure is a failure. GitHub Actions sets it for
#                   the main branch. Without it, a build that fails only
#                   because code of later stages is missing (see
#                   CI/classify_build_log.sh) is reported as a warning
#                   ("build incomplete") and the script succeeds.
#
# The warning options (including -Werror) come from CMakeLists.txt, so any
# warning makes the build fail (Rule 18 of rules.md). The same script is used
# by GitHub Actions and can be run locally.

set -euo pipefail

if [ "$#" -ne 1 ]; then
	echo "usage: $0 <compiler>" >&2
	exit 2
fi

compiler="$1"
strict="${CI_STRICT:-false}"
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${root}/build/ci-${compiler}"

cd "$root"

build_log="$(mktemp)"
test_log="$(mktemp)"
trap 'rm -f "$build_log" "$test_log"' EXIT

echo "::group::Configure (${compiler})"
cmake -S . -B "$build_dir" -DCMAKE_C_COMPILER="$compiler"
echo "::endgroup::"

echo "::group::Build (${compiler})"
set +e
cmake --build "$build_dir" --parallel 2>&1 | tee "$build_log"
build_status="${PIPESTATUS[0]}"
set -e
echo "::endgroup::"

if [ "$build_status" -ne 0 ]; then
	if [ "$strict" = "true" ]; then
		echo "::error::The build failed (strict mode: an incomplete build is also a failure)."
		exit "$build_status"
	fi
	if missing_items="$("$root/CI/classify_build_log.sh" "$build_log")"; then
		echo "::group::Missing code (${compiler})"
		echo "$missing_items"
		echo "::endgroup::"
		echo "::warning title=Build incomplete (${compiler})::The build failed only because code that is not written yet is missing (functions, types, or headers of later stages). Tests were not run."
		exit 0
	fi
	echo "::error::The build failed with errors that are not missing code."
	exit "$build_status"
fi

echo "::group::Test (${compiler})"
# ctest exits successfully when the project defines no tests, so the output is
# checked to make that case visible instead of silently green.
ctest --test-dir "$build_dir" --output-on-failure 2>&1 | tee "$test_log"
test_status="${PIPESTATUS[0]}"
if grep -q "No tests were found" "$test_log"; then
	echo "::notice::The project defines no tests yet; the test step checked nothing."
fi
echo "::endgroup::"

exit "$test_status"
