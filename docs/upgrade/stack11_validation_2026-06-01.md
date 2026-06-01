# Stack 11 Validation - 2026-06-01

This stack starts from validated stack 10 and integrates three focused branches:

- `codex/mathengine-rotation-angle-slice`
- `codex/probability-engine-2d-diffusion-binning`
- `codex/restart-file-diagnostic`

## Changes

- Moved association rotation-angle partitioning behind
  `nerdss::core::MathEngine` while preserving the legacy
  `determine_rotation_angles` wrapper.
- Moved duplicated 2D diffusion table quantization into
  `nerdss::core::ProbabilityEngine::QuantizeDiffusionFor2DTable`.
- Replaced the serial missing restart-file exit with a structured file I/O
  diagnostic and added a smoke-runner CLI check for the behavior.
- Extended unit coverage for the new MathEngine and ProbabilityEngine helpers.

## Validation

- `git diff --check HEAD`: passed.
- `cmake -S . -B build-stack11`: passed with the existing CMake compatibility
  warning.
- `cmake --build build-stack11 --target nerdss_unit_tests --parallel 4`:
  passed.
- `ctest --test-dir build-stack11 --output-on-failure`: 3/3 passed.
- `make serial -j4`: passed, rebuilding `bin/nerdss`.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --output-dir /tmp/nerdss-stack11-validation`:
  smoke, unit configure/build/CTest, and regression passed.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --skip-smoke --skip-unit --skip-regression --benchmarks --benchmark-case small_homotrimer --output-dir /tmp/nerdss-stack11-small-benchmark`:
  passed.

## Benchmark Snapshot

`small_homotrimer` completed with status `passed`, wall time
`8.685741666005924` seconds, CPU time `8.466295` seconds, 24 output files, and
8,828,274 output bytes.

## Static Analysis Notes

- `tools/run_static_analysis.sh src/reactions/determine_rotation_angles.cpp`:
  exited 0; only the preserved legacy output-reference signature warning
  remained after removing a local dead store.
- `tools/run_static_analysis.sh src/reactions/determine_2D_bimolecular_reaction_probability.cpp src/reactions/determine_2D_implicitlipid_reaction_probability.cpp`:
  exited 0; reported existing legacy style/dead-store warnings outside the
  extracted helper.
- `tools/run_static_analysis.sh EXEs/nerdss.cpp`: exited 0; reported existing
  legacy warnings in the large serial entry point.
