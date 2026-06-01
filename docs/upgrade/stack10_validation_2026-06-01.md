# Stack 10 Validation - 2026-06-01

This stack starts from validated stack 9 and adds the
`codex/probability-engine-integrator-slice` branch.

## Changes Integrated

- Moved the semi-infinite GSL integration retry/fallback algorithm used by 2D
  reaction-table generation into `nerdss::core::ProbabilityEngine`.
- Kept the legacy `integrator` function as a forwarding wrapper.
- Added CMake unit-test linkage for `src/reactions/integrator.cpp`.
- Added unit coverage comparing the engine and legacy wrapper against a known
  exponential integral.

## Local Slice Validation

- `cmake -S . -B build-integrator-slice`: passed with the existing CMake
  compatibility deprecation warning.
- `cmake --build build-integrator-slice --target nerdss_unit_tests --parallel 4`:
  passed.
- `ctest --test-dir build-integrator-slice --output-on-failure`: 3/3 passed.
- `make serial -j4`: passed and rebuilt `bin/nerdss`.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --output-dir /tmp/nerdss-integrator-slice-validation`:
  smoke, unit configure/build/CTest, and regression passed.
- `small_homotrimer` benchmark: passed in 9.0394 seconds wall time, 8.7727
  seconds CPU time, with 24 output artifacts.

## Integrated Stack Validation

- `git diff --check`: passed.
- `cmake -S . -B build-upgrade-validation-stack10`: passed with the existing
  CMake compatibility deprecation warning.
- `cmake --build build-upgrade-validation-stack10 --target nerdss_unit_tests --parallel 4`:
  passed.
- `ctest --test-dir build-upgrade-validation-stack10 --output-on-failure`: 3/3
  passed.
- `make serial -j4`: passed and rebuilt `bin/nerdss`.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --output-dir /tmp/nerdss-stack10-validation`:
  smoke, unit configure/build/CTest, and regression passed.
- `small_homotrimer` benchmark: passed in 8.8139 seconds wall time, 8.5573
  seconds CPU time, with 24 output artifacts.

## Remaining Follow-Up

- Continue moving small probability/math kernels behind engine methods in
  focused slices with wrapper parity tests.
- Keep larger mutable reaction/topology paths out of the engine boundary until
  they have direct regression coverage.
