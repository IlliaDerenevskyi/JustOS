#!/usr/bin/env bash
# Decides whether the build and test steps of CI have anything to build.
#
# CMakeLists.txt lists the source files of every library. While none of those
# files exist yet (the project is still documentation only), configuring the
# project would fail, so CI skips the build and tells the reader why. As soon
# as at least one listed source file exists, the build runs and CMake reports
# any file that is still missing.
#
# Output: prints a short report and, when run by GitHub Actions, writes
# "has_sources=true" or "has_sources=false" to the step outputs.

set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

if [ ! -f CMakeLists.txt ]; then
	echo "::error::CMakeLists.txt is missing"
	exit 1
fi

# Source files that CMakeLists.txt refers to (C and assembly).
mapfile -t referenced < <(grep -oE '(src|tests)/[A-Za-z0-9_./-]+\.(c|S)' CMakeLists.txt | sort -u)

existing=0
for file in "${referenced[@]}"; do
	if [ -f "$file" ]; then
		existing=$((existing + 1))
	fi
done

echo "Source files listed in CMakeLists.txt: ${#referenced[@]}"
echo "Of them present in the repository:     ${existing}"

if [ "$existing" -gt 0 ]; then
	result=true
else
	result=false
fi

if [ -n "${GITHUB_OUTPUT:-}" ]; then
	echo "has_sources=${result}" >> "$GITHUB_OUTPUT"
fi
echo "has_sources=${result}"
