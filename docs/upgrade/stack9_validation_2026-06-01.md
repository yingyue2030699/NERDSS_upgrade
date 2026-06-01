# Stack 9 Validation - 2026-06-01

This stack integrates the stack 8 parser/setup/I/O diagnostics and
MathEngine/ProbabilityEngine extraction work with CI coverage workflow tooling
and a macOS clang-tidy runner fix.

## Branches Integrated

- `codex/ci-workflow-stack8-publish`
- `codex/static-analysis-macos-clang-tidy`

## Blocker Checks

- `gh auth status -h github.com` succeeds through the host keyring for
  `yingyue2030699` with `repo` and `workflow` scopes.
- `tools/run_static_analysis.sh EXEs/nerdss.cpp` finds Homebrew LLVM
  clang-tidy and runs to completion with macOS SDK/libc++ include arguments.

## Local Validation

- `ruby -e "require 'yaml'; YAML.load_file(...)"`: passed.
- `python3 -B -m py_compile tools/collect_gcov_coverage.py`: passed.
- `git diff --check HEAD~2..HEAD`: passed.
- `python3 tests/io/run_standard_format_tests.py`: passed.
- `cmake -S . -B build-upgrade-validation-stack9`: passed with the existing
  CMake compatibility deprecation warning.
- `cmake --build build-upgrade-validation-stack9 --target nerdss_unit_tests --parallel 4`:
  passed.
- `ctest --test-dir build-upgrade-validation-stack9 --output-on-failure`: 3/3
  tests passed.
- `make serial -j4`: passed and rebuilt `bin/nerdss`.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --output-dir /tmp/nerdss-stack9-validation`:
  smoke, unit configure/build/CTest, and regression passed.
- Coverage build and collector: passed with 55.24% project line coverage
  (1329/2406 lines) for files compiled into `nerdss_unit_tests`.
- `small_homotrimer` benchmark: passed in 8.7958 seconds wall time, 8.5616
  seconds CPU time, with 24 output artifacts.

## Remaining Follow-Up

- The static-analysis runner is unblocked, but the legacy warning backlog is
  large. Start focused cleanup slices before making warnings a CI gate.
- The workflow coverage job is report-only. Add `--fail-under` after a stable
  baseline and a policy threshold are agreed.
- The CMake policy-floor warning remains and should be handled in the build
  environment policy work.
- MPI validation was not included in this stack; keep it as a separate branch
  because it has different toolchain/environment assumptions.
