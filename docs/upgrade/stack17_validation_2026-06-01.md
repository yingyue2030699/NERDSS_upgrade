# Stack 17 Validation - 2026-06-01

This stack starts from validated stack 16 and integrates two focused branches:

- `codex/setup-geometry-diagnostics-slice`
- `codex/probability-engine-poisson-event-slice`

## Changes

- Added structured setup diagnostics for implicit lipid molecule ordering,
  sphere/compartment incompatibility, and compartment water-box clearance
  failures.
- Routed the duplicated serial setup checks in `EXEs/nerdss.cpp` through shared
  setup diagnostic helpers while preserving the same setup invariants.
- Added a pure `nerdss::core::ProbabilityEngine::PoissonEventProbability`
  helper for the legacy `1 - exp(-lambda)` event-probability formula.
- Routed unimolecular, dissociation, state-change, and loop-closure probability
  formulas through the new helper while leaving RNG draws, counters, topology
  mutation, warnings, and cancellation in the legacy reaction paths.
- Extended diagnostics and ProbabilityEngine unit coverage, and updated the
  error-model and core computation refactor notes.

## Validation

- `git diff --check HEAD`: passed.
- `cmake -S . -B build-stack17`: passed with the existing CMake compatibility
  warning.
- `cmake --build build-stack17 --target nerdss_unit_tests --parallel 4`:
  passed.
- `ctest --test-dir build-stack17 --output-on-failure`: 3/3 passed.
- `make serial -j4`: passed, rebuilding `bin/nerdss`.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --output-dir /tmp/nerdss-stack17-validation`:
  smoke, unit configure/build/CTest, and regression passed.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --skip-smoke --skip-unit --skip-regression --benchmarks --benchmark-case small_homotrimer --output-dir /tmp/nerdss-stack17-small-benchmark`:
  passed.

## Benchmark Snapshot

`small_homotrimer` completed with status `passed`, wall time
`8.904265207936987` seconds, CPU time `8.664897` seconds, 24 output files, and
8,828,274 output bytes.

## Static Analysis Notes

- `tools/run_static_analysis.sh EXEs/nerdss.cpp src/reactions/check_dissociation.cpp src/reactions/check_for_unimolecular_reactions.cpp src/reactions/check_for_unimolstatechange_reactions.cpp include/system_setup/setup_diagnostics.hpp include/core/probability_engine.hpp`:
  exited 0.
- The run reported existing legacy warnings in `EXEs/nerdss.cpp` and touched
  reaction paths, including missing braces, `std::endl`, redundant boolean
  comparisons, narrowing conversions, dead stores, and easily-swappable
  parameters.

## GitHub Checks

- PR #21 (`codex/setup-geometry-diagnostics-slice`) completed successfully in
  GitHub Actions: both duplicate workflow runs passed unit coverage and the
  build/unit/smoke/regression job.
- PR #22 (`codex/probability-engine-poisson-event-slice`) completed
  successfully in GitHub Actions: both duplicate workflow runs passed unit
  coverage and the build/unit/smoke/regression job.
