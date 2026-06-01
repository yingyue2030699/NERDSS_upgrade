# Stack 19 Validation - 2026-06-01

This stack starts from validated stack 18 and integrates two focused branches:

- `codex/probability-engine-3d-association-parameters-slice`
- `codex/setup-implicit-interface-diagnostics-slice`

## Changes

- Added a `nerdss::core::ProbabilityEngine::AssociationParametersFor3D`
  helper that exposes the pure 3D bimolecular association parameter bundle:
  diffusion-limited rate, intrinsic rate, alpha, and association-probability
  coefficient.
- Routed `determine_3D_bimolecular_reaction_probability` through the helper
  while preserving pair search, reweighting, RNG, warnings, and probability
  vector mutation in the legacy caller.
- Added unit coverage for 3D `kdiff`, surface/symmetric/fiber `kact` scaling,
  alpha, and the `passocF` coefficient.
- Added structured setup diagnostics for the implicit-molecule
  interface-count invariant in `initialize_states`.
- Extended diagnostics unit coverage and updated the error-model and core
  computation refactor notes.

## Validation

- `git diff --check HEAD`: passed.
- `cmake -S . -B build-stack19`: passed with the existing CMake compatibility
  warning.
- `cmake --build build-stack19 --target nerdss_unit_tests --parallel 4`:
  passed.
- `ctest --test-dir build-stack19 --output-on-failure`: 3/3 passed.
- `make serial -j4`: passed, rebuilding `bin/nerdss`.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --output-dir /tmp/nerdss-stack19-validation`:
  smoke, unit configure/build/CTest, and regression passed.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --skip-smoke --skip-unit --skip-regression --benchmarks --benchmark-case small_homotrimer --output-dir /tmp/nerdss-stack19-small-benchmark`:
  passed.

## Benchmark Snapshot

`small_homotrimer` completed with status `passed`, wall time
`9.02420041593723` seconds, CPU time `8.839443` seconds, maximum resident set
size 6,438,912 bytes, 24 output files, and 8,828,274 output bytes.

## Static Analysis Notes

- `tools/run_static_analysis.sh include/core/probability_engine.hpp src/reactions/determine_3D_bimolecular_reaction_probability.cpp include/system_setup/setup_diagnostics.hpp src/system_setup/initialize_states.cpp tests/unit/test_vector_coord.cpp tests/unit/test_diagnostics.cpp`:
  exited 0.
- The run reported existing legacy warnings in the touched reaction, setup, and
  unit-test paths, including easily-swappable parameters, redundant boolean
  comparisons, dead stores, narrowing conversions, missing braces, redundant
  declarations, and exception escape from the unit-test `main`.

## GitHub Checks

- PR #27 (`codex/probability-engine-3d-association-parameters-slice`)
  completed successfully in GitHub Actions: both duplicate workflow runs passed
  unit coverage and the build/unit/smoke/regression job.
- PR #28 (`codex/setup-implicit-interface-diagnostics-slice`) completed
  successfully in GitHub Actions: both duplicate workflow runs passed unit
  coverage and the build/unit/smoke/regression job.
