#!/usr/bin/env bash
# Decides whether a failed build failed only because code is missing.
#
# Usage: CI/classify_build_log.sh <build-log>
#
# Inside a milestone a stage may depend on functions, types, or headers that a
# later stage of the same milestone defines. Such a build cannot succeed until
# the later stage is written. This script reads the compiler and linker output
# and answers one question: is every error of that kind?
#
# Errors that count as "missing code":
#   - undefined reference or undefined symbol (a function or variable that is
#     declared but not defined);
#   - implicit declaration, undeclared function, or undeclared identifier;
#   - unknown type name, incomplete type, or unknown structure size;
#   - a header file that does not exist.
#
# Exit status:
#   0  every error is "missing code" and there is at least one; the missing
#      items are printed to standard output, one per line;
#   1  there is any other error, or no recognizable error at all; the reason
#      is printed to standard error.
#
# This is a heuristic based on the text of compiler messages. It is not a
# guarantee, which is why a build on the main branch is always strict.

set -euo pipefail

if [ "$#" -ne 1 ] || [ ! -f "$1" ]; then
	echo "usage: $0 <build-log>" >&2
	exit 1
fi

log="$1"

# Messages of compilers and linkers that mean "something is not defined yet".
missing='undefined reference to|undefined symbol|error: unknown type name|error: implicit declaration of function|error: call to undeclared function|error: use of undeclared identifier|error: .* undeclared \(first use|has incomplete type|incomplete definition of type|invalid use of incomplete typedef|invalid use of undefined type|storage size of .* isn.t known|dereferencing pointer to incomplete type|fatal error: .*(No such file or directory|file not found)'

# Summary lines that only repeat that something failed.
summary='collect2: error: ld returned|clang: error: linker command failed|error: linker command failed'

# Every diagnostic line: compiler errors and linker errors.
diagnostics="$(mktemp)"
trap 'rm -f "$diagnostics"' EXIT
grep -E '(error|fatal error):|undefined reference to|undefined symbol' "$log" > "$diagnostics" || true

other="$(grep -Ev "$missing" "$diagnostics" | grep -Ev "$summary" || true)"
found="$(grep -E "$missing" "$diagnostics" || true)"

if [ -n "$other" ]; then
	echo "errors that are not missing code:" >&2
	echo "$other" | sort -u | head -n 20 >&2
	exit 1
fi

if [ -z "$found" ]; then
	echo "the build failed, but no missing-code error was found in the output" >&2
	exit 1
fi

# Print what is missing: the message without the file and line prefix.
echo "$found" \
	| sed -E 's/^.*(undefined reference to|undefined symbol)/\1/; s/^[^ ]*: (fatal )?error: //' \
	| sort -u \
	| head -n 50
