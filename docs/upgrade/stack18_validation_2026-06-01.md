# Stack 18 Validation - 2026-06-01

This stack starts from validated stack 17 and integrates two focused branches:

- `codex/probability-engine-loop-correction-slice`
- `codex/parser-molecule-count-diagnostics-slice`

## Changes

- Added `nerdss::core::ProbabilityEngine::LoopDissociationCorrectionRatio`
  helpers for the legacy closed-loop dissociation correction formula.
- Routed the scalar loop correction math in `break_interaction` through
  `ProbabilityEngine` while leaving RNG draws, topology updates, cancellation,
  and warning behavior in the legacy reaction path.
- Added parity tests for typical, large, and zero-lambda loop correction
  behavior. The zero-lambda `NaN` result is intentionally preserved for
  compatibility.
- Routed malformed molecule copy-count and `startMolecules` parser failures
  through structured `ERROR [input]` diagnostics.
- Extended diagnostic unit coverage and updated the error-model and core
  computation refactor notes.

## Validation

- `git diff --check HEAD`: passed.
- `cmake -S . -B build-stack18`: passed with the existing CMake compatibility
  warning.
- `cmake --build build-stack18 --target nerdss_unit_tests --parallel 4`:
  passed.
- `ctest --test-dir build-stack18 --output-on-failure`: 3/3 passed.
- `make serial -j4`: passed, rebuilding `bin/nerdss`.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --output-dir /tmp/nerdss-stack18-validation`:
  smoke, unit configure/build/CTest, and regression passed.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --skip-smoke --skip-unit --skip-regression --benchmarks --benchmark-case small_homotrimer --output-dir /tmp/nerdss-stack18-small-benchmark`:
  passed.

## Benchmark Snapshot

`small_homotrimer` completed with status `passed`, wall time
`9.084144374821335` seconds, CPU time `8.887637` seconds, 24 output files, and
8,828,274 output bytes.

## Static Analysis Notes

- `tools/run_static_analysis.sh include/core/probability_engine.hpp src/reactions/break_interaction.cpp include/parser/parser_diagnostics.hpp include/parser/parser_functions.hpp src/parser/parse_input.cpp src/parser/parse_molecule_bngl.cpp tests/unit/test_diagnostics.cpp tests/unit/test_vector_coord.cpp`:
  exited 0.
- The run reported existing legacy warnings in the touched parser, reaction, and
  unit-test paths, including missing braces, `std::endl`, redundant boolean
  comparisons, narrowing conversions, redundant declarations, and suspicious
  test helper argument naming.

## GitHub Checks

- PR #24 (`codex/probability-engine-loop-correction-slice`) completed
  successfully in GitHub Actions: both duplicate workflow runs passed unit
  coverage and the build/unit/smoke/regression job.
- PR #25 (`codex/parser-molecule-count-diagnostics-slice`) completed
  successfully in GitHub Actions: both duplicate workflow runs passed unit
  coverage and the build/unit/smoke/regression job.
