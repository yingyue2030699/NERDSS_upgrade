# Stack 15 Validation - 2026-06-01

This stack starts from validated stack 14 and integrates two focused branches:

- `codex/parser-state-diagnostic-slice`
- `codex/probability-engine-next-kernel-slice`

## Changes

- Replaced the invalid molecule-state interface path in `parse_states` with a
  structured parser diagnostic while preserving accepted state declarations and
  state-token normalization.
- Added parser diagnostic unit coverage and updated the error-model traceback
  migration notes.
- Added pure `nerdss::core::ProbabilityEngine` helpers for association-rate
  conversion:
  - 1D area scaling with the legacy asymmetric half-rate normalization.
  - 2D length scaling with the legacy symmetric doubling.
  - 3D surface/symmetric multipliers with the existing fiber exception.
  - 3D-to-surface association-rate doubling.
- Routed legacy reaction probability callers through the new helpers while
  leaving mutation, table allocation, warnings, RNG, and I/O in existing
  reaction paths.

## Validation

- `git diff --check HEAD`: passed.
- `cmake -S . -B build-stack15`: passed with the existing CMake compatibility
  warning.
- `cmake --build build-stack15 --target nerdss_unit_tests --parallel 4`:
  passed.
- `ctest --test-dir build-stack15 --output-on-failure`: 3/3 passed.
- `make serial -j4`: passed, rebuilding `bin/nerdss`.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --output-dir /tmp/nerdss-stack15-validation`:
  smoke, unit configure/build/CTest, and regression passed.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --skip-smoke --skip-unit --skip-regression --benchmarks --benchmark-case small_homotrimer --output-dir /tmp/nerdss-stack15-small-benchmark`:
  passed.

## Benchmark Snapshot

`small_homotrimer` completed with status `passed`, wall time
`9.01856758305803` seconds, CPU time `8.830512` seconds, 24 output files, and
8,828,274 output bytes.

## Static Analysis Notes

- `tools/run_static_analysis.sh src/parser/parse_states.cpp`: exited 0; only
  non-user warnings were suppressed.
- `tools/run_static_analysis.sh` over the touched reaction probability callers:
  exited 0; reported existing legacy warnings in reaction entry points,
  including easily-swappable parameters, redundant boolean comparisons, dead
  stores, missing braces, narrowing conversions, and `std::endl`.

## GitHub Checks

- PR #15 (`codex/parser-state-diagnostic-slice`) completed successfully in
  GitHub Actions: both duplicate workflow runs passed unit coverage and the
  build/unit/smoke/regression job.
- PR #16 (`codex/probability-engine-next-kernel-slice`) completed successfully
  in GitHub Actions: both duplicate workflow runs passed unit coverage and the
  build/unit/smoke/regression job.
