# Stack 12 Validation - 2026-06-01

This stack starts from validated stack 11 and integrates three focused
branches:

- `codex/ci-node24-actions`
- `codex/coordinate-file-diagnostic`
- `codex/probability-engine-radius-slice`

## Changes

- Opted the GitHub Actions workflow into Node 24 with
  `FORCE_JAVASCRIPT_ACTIONS_TO_NODE24` and updated official action versions to
  `actions/checkout@v6.0.2` and `actions/upload-artifact@v7.0.1` to address
  the Node 20 runner warning without changing the build or validation commands.
- Replaced the missing coordinate-file exit path with the structured file I/O
  diagnostic helper and added smoke-runner coverage for
  `--coordinate missing_coords.pdb`.
- Added `nerdss::core::ProbabilityEngine` helpers for the legacy 1D, 2D, and
  3D reaction search-radius/RMax formulas and forwarded existing callers
  through them.
- Extended unit coverage for the new ProbabilityEngine radius helpers.

## Validation

- `git diff --check HEAD`: passed.
- `python3 -m py_compile tools/run_smoke_tests.py`: passed.
- Workflow YAML parse check: passed.
- `cmake -S . -B build-stack12`: passed with the existing CMake compatibility
  warning.
- `cmake --build build-stack12 --target nerdss_unit_tests --parallel 4`:
  passed.
- `ctest --test-dir build-stack12 --output-on-failure`: 3/3 passed.
- `make serial -j4`: passed, rebuilding `bin/nerdss`.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --output-dir /tmp/nerdss-stack12-validation`:
  smoke, unit configure/build/CTest, and regression passed.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --skip-smoke --skip-unit --skip-regression --benchmarks --benchmark-case small_homotrimer --output-dir /tmp/nerdss-stack12-small-benchmark`:
  passed.

The first pushed `codex/validation-integration-stack-12` CI run completed
successfully: unit coverage and the full build/unit/smoke regression job both
passed. GitHub still annotated the v4 actions as Node 20 actions forced onto
Node 24, so this stack was updated again to use the latest checked official
action releases.

## Benchmark Snapshot

`small_homotrimer` completed with status `passed`, wall time
`8.688902291003615` seconds, CPU time `8.516135` seconds, 24 output files, and
8,828,274 output bytes.

## Static Analysis Notes

- `tools/run_static_analysis.sh src/system_setup/generate_coordinates.cpp`:
  exited 0; reported existing legacy readability/performance warnings in the
  setup file.
- `tools/run_static_analysis.sh src/reactions/check_compartment_reaction.cpp src/reactions/determine_1D_bimolecular_reaction_probability.cpp src/reactions/determine_2D_bimolecular_reaction_probability.cpp src/reactions/determine_3D_bimolecular_reaction_probability.cpp src/reactions/determine_3D_implicitlipid_reaction_probability.cpp`:
  exited 0; reported existing legacy warnings in the reaction entry points.
- Running two static-analysis script invocations concurrently is unsafe because
  they share `build/static-analysis`; sequential runs passed.
