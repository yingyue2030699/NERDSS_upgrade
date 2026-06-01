# Stack 16 Validation - 2026-06-01

This stack starts from validated stack 15 and integrates two focused branches:

- `codex/parser-valid-states-diagnostic-slice`
- `codex/core-next-kernel-slice`

## Changes

- Replaced hard reaction-state validation failures in
  `check_for_valid_states` with structured parser diagnostics for unknown
  molecule templates, interfaces, and interface states.
- Added unit coverage for those reaction-state diagnostics and updated the
  traceback migration notes for the remaining parser error-model work.
- Added pure `nerdss::core::ProbabilityEngine` helpers for translational
  diffusion displacement and scaled displacement-limit thresholds.
- Routed the displacement-threshold math in `measure_complex_displacement`
  through `ProbabilityEngine` while preserving legacy coordinate mutation,
  cancellation, warning, and bookkeeping behavior in the existing reaction
  path.

## Validation

- `git diff --check HEAD`: passed.
- `cmake -S . -B build-stack16`: passed with the existing CMake compatibility
  warning.
- `cmake --build build-stack16 --target nerdss_unit_tests --parallel 4`:
  passed.
- `ctest --test-dir build-stack16 --output-on-failure`: 3/3 passed.
- `make serial -j4`: passed, rebuilding `bin/nerdss`.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --output-dir /tmp/nerdss-stack16-validation`:
  smoke, unit configure/build/CTest, and regression passed.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --skip-smoke --skip-unit --skip-regression --benchmarks --benchmark-case small_homotrimer --output-dir /tmp/nerdss-stack16-small-benchmark`:
  passed.

## Benchmark Snapshot

`small_homotrimer` completed with status `passed`, wall time
`9.047629250213504` seconds, CPU time `8.888924000000001` seconds, 24 output
files, and 8,828,274 output bytes.

## Static Analysis Notes

- `tools/run_static_analysis.sh src/parser/check_for_valid_states.cpp`:
  exited 0; only non-user warnings were suppressed.
- `tools/run_static_analysis.sh src/reactions/measure_complex_displacement.cpp`:
  exited 0; only non-user warnings were suppressed.

## GitHub Checks

- PR #18 (`codex/core-next-kernel-slice`) completed successfully in GitHub
  Actions: both duplicate workflow runs passed unit coverage and the
  build/unit/smoke/regression job.
- PR #19 (`codex/parser-valid-states-diagnostic-slice`) completed successfully
  in GitHub Actions: both duplicate workflow runs passed unit coverage and the
  build/unit/smoke/regression job.
