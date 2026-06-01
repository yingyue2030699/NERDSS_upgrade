# Stack 14 Validation - 2026-06-01

This stack starts from validated stack 13 and integrates two focused branches:

- `codex/parser-array-diagnostic`
- `codex/mathengine-distance-kernel-slice`

## Changes

- Replaced the bad numeric-array token path in `parse_input_array` with a
  structured parser diagnostic while preserving legacy accepted values:
  numeric literals, `pi`, `m_pi`, and `nan`.
- Added unit coverage for the numeric-array diagnostic and updated the error
  model migration notes.
- Added pure `nerdss::core::MathEngine` distance kernels for squared
  coordinate distance, coordinate distance, planar XY distance,
  sphere-surface distance, and z-plane distance.
- Routed legacy interface-separation and implicit-lipid surface-distance
  callers through the new MathEngine kernels while leaving threshold checks,
  cross-list mutation, and reaction bookkeeping in the legacy reaction paths.

The parser diagnostic merge had one documentation conflict in the remaining
traceback-gap list. The resolution preserves both migrated sections and removes
`parse_observable.cpp` and `parse_input_array.cpp` from the remaining parser
gap list.

## Validation

- `git diff --check HEAD`: passed.
- `cmake -S . -B build-stack14`: passed with the existing CMake compatibility
  warning.
- `cmake --build build-stack14 --target nerdss_unit_tests --parallel 4`:
  passed.
- `ctest --test-dir build-stack14 --output-on-failure`: 3/3 passed.
- `make serial -j4`: passed, rebuilding `bin/nerdss`.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --output-dir /tmp/nerdss-stack14-validation`:
  smoke, unit configure/build/CTest, and regression passed.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --skip-smoke --skip-unit --skip-regression --benchmarks --benchmark-case small_homotrimer --output-dir /tmp/nerdss-stack14-small-benchmark`:
  passed.

## Benchmark Snapshot

`small_homotrimer` completed with status `passed`, wall time
`9.053917875047773` seconds, CPU time `8.944475` seconds, 24 output files, and
8,828,274 output bytes.

## Static Analysis Notes

- `tools/run_static_analysis.sh src/parser/parse_input_array.cpp`: exited 0;
  only non-user warnings were suppressed.
- `tools/run_static_analysis.sh src/reactions/get_distance.cpp src/reactions/get_distance_to_surface.cpp`:
  exited 0; reported existing legacy warnings in the reaction entry points,
  including easily-swappable integer parameters, redundant boolean comparisons,
  and repeated branch bodies.

## GitHub Checks

- PR #11 (`codex/validation-integration-stack-13`) completed successfully in
  GitHub Actions: both duplicate workflow runs passed unit coverage and the
  build/unit/smoke/regression job.
