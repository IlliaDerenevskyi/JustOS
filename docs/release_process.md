# Release process: from a finished stage to a tagged milestone

This is the full sequence, in order. It follows `rules.md` (Rules 16, 19, 24, 25, 26, 40, 42). The commands are for a person at the terminal; replace `N` with the stage number and `X.Y.Z` with the version.

## 1. Before every push to `develop`

1. **Documentation first** (Rule 40): the specification and the stage report describe what the code does. A decision the specification does not make is taken with the project owner and written into the specification before the code.
2. **Build and test on the host, with both compilers.** From a build directory made with `cmake -S . -B build/<name>`: `make -j"$(nproc)"` and `ctest`. Every warning is an error (`-Werror`).
3. **QEMU and KVM** (when the boot code or the test kernel changed): build with `-DJP_ENABLE_QEMU_TESTS=ON`, then `ctest -L qemu`, and on an x86_64 host `JP_QEMU_KVM=1 ctest -L kvm`.
4. **The small checks:** `python3 scripts/gen_font.py --check` and `bash tests/qemu/test_check_serial.sh`.
5. **Check what Git will really contain.** CI sees only committed files. A file that `.gitignore` hides (for example a `*.log`) works on your machine and fails on CI. Make a clean copy of the commit and run the CI script on it:
   ```bash
   rm -rf /tmp/ci && mkdir /tmp/ci && git archive HEAD | tar -x -C /tmp/ci
   (cd /tmp/ci && CI/build_and_test.sh gcc && CI/build_and_test.sh clang)
   # the QEMU level, exactly as the two QEMU jobs of CI run it (needs the firmware, see requirements.txt):
   (cd /tmp/ci && CI/qemu_tests.sh x86_64-uefi x86_64-bios && CI/qemu_tests.sh aarch64-uefi)
   ```
   Also run `git status` and look at every `??` line: each one is either a file that must be committed, or a file that is meant to stay out.
6. **Stage Report and review** (Rule 26): write the report, make the diff since the previous `Stage <N-1>:` commit (`git diff <previous commit>`), and pass the architecture review.
7. **Commit** the finished stage as one commit whose message starts with `Stage N:`. Working commits are allowed in `develop`, but only a reviewed stage gets this prefix.
8. **Push `develop`, then watch CI:** `gh run list --branch develop --limit 3`, or `gh run watch`. A red CI is fixed before anything else (Rule 18: a warning or a failure is a bug).

## 2. Before the merge into `main` (the milestone is finished)

A milestone is finished only when all of Rule 25 holds: code, build, tests, documentation, tag, release.

1. **Every box of the milestone is ticked** in `milestoneN.md` (the Specification Lock Criteria, the Implementation Acceptance records, the Checklist) and in `milestoneN_pre_lock.md`. A box that stays empty has a written reason.
2. **Level M is done** (the manual check described in `docs/milestones/milestone0/manual_check.md` for Milestone 0).
3. **The other documents agree:** `progress.md` (the stages and the milestone are ticked), `roadmap.md` (the status of the milestone), and the descriptive documents of the milestone (`boot.md` for Milestone 0).
4. **CI is green on `develop`** for the last commit (`gh run list`).
5. **Write the tag notes `docs/tags/vX.Y.Z.md`** (Rule 24): the version and a short title on the first line, one sentence of description, then the summary of the completed work, the problems found and fixed during the final check, and what is not included. This file is the text of the tag and of the GitHub Release. It is the last file written before the merge, so it can name the exact state.
6. Commit the notes and the last document changes in `develop`, push, and wait for CI to be green again.

## 3. The merge, the tag, the release

1. **Merge `develop` into `main`:** a pull request (`gh pr create --base main --head develop`), CI green on the pull request, then merge. On `main` every failure is a failure (`CI_STRICT=true`); there is no "build incomplete".
2. **Update the local `main` and tag it** (an annotated tag, never moved or rewritten after publication):
   ```bash
   git checkout main && git pull
   git tag -a vX.Y.Z -F docs/tags/vX.Y.Z.md
   git push origin vX.Y.Z
   ```
3. **Publish the GitHub Release** with the same text: `gh release create vX.Y.Z --target main --title "<first line of the notes>" --notes-file docs/tags/vX.Y.Z.md` (add `--prerelease` while the version is below 1.0.0 and you want it marked so).
4. **Go back to work:** `git checkout develop && git merge main` (so `develop` has the merge commit), and tick the last boxes (tag and release) in the checklist.

## 4. The numbers of the tags

- `v0.0.x`: documentation and repository states (before any milestone is finished).
- `v0.N+1.0`: the finished Milestone N (Rule 24: `v0.x.0` is a completed milestone). Milestone 0 is `v0.1.0`, Milestone 1 is `v0.2.0`, and so on up to Milestone 7 (`v0.8.0`).
- `v1.0.0`: the first public release, decided after the last milestone.
