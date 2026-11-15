#!/usr/bin/env bash
# Configures, builds, and tests the project with one compiler.
#
# Usage: ci/build_and_test.sh <compiler>
#   <compiler> is the C compiler command, for example gcc or clang.
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
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${root}/build/ci-${compiler}"

cd "$root"

echo "::group::Configure (${compiler})"
cmake -S . -B "$build_dir" -DCMAKE_C_COMPILER="$compiler"
echo "::endgroup::"

echo "::group::Build (${compiler})"
cmake --build "$build_dir" --parallel
echo "::endgroup::"

echo "::group::Test (${compiler})"
# ctest exits successfully when the project defines no tests, so the output is
# checked to make that case visible instead of silently green.
test_log="$(mktemp)"
trap 'rm -f "$test_log"' EXIT
ctest --test-dir "$build_dir" --output-on-failure 2>&1 | tee "$test_log"
if grep -q "No tests were found" "$test_log"; then
	echo "::notice::The project defines no tests yet; the test step checked nothing."
fi
echo "::endgroup::"
