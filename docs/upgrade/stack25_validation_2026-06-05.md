# Stack 25 Validation - 2026-06-05

## Scope

Stack 25 continues the ProbabilityEngine service decomposition on top of
`codex/validation-integration-stack-24`.

- Added `nerdss::core::ReactionTable2DService` for pure 2D reaction-table
  kernels: survival and irreversible integrands, free-diffusion probability
  helpers, semi-infinite integration retry/fallback, table sizing, matrix-row
  interpolation, PIR table lookup, and table rebinding-ratio calculation.
- Kept `nerdss::core::ProbabilityEngine` as the stable facade and kept the
  legacy free-function wrappers unchanged.
- Left stochastic event selection, topology mutation, GSL matrix allocation,
  table-cache ownership, and reaction-table generation orchestration in the
  legacy wrappers for later service/cache slices.

## Local Validation

Commands run on `codex/reaction-table-2d-service-stack25-slice`:

- `git diff --check HEAD`: passed.
- `cmake -S . -B build-reaction-table-2d-service`: passed.
- `tools/run_static_analysis.sh include/core/probability/reaction_table_2d_service.hpp include/core/probability_engine.hpp tests/unit/test_vector_coord.cpp`:
  exited 0. The output retains existing `test_vector_coord.cpp`
  redundant-declaration and test-main warnings.
- `cmake --build build-reaction-table-2d-service --target nerdss_unit_tests --parallel 4`:
  passed.
- `ctest --test-dir build-reaction-table-2d-service --output-on-failure`:
  passed, 3/3 tests passed.
- `make serial -j4`: passed and rebuilt `bin/nerdss`.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --output-dir /tmp/nerdss-reaction-table-2d-service-validation`:
  passed. Smoke took 0.26 seconds, unit configure 0.66 seconds, unit build
  13.35 seconds, unit CTest 0.64 seconds, and regression 1.41 seconds.
  Benchmarks were skipped for this narrow extraction slice.

## Follow-Up Notes

- Continue ProbabilityEngine decomposition by moving 2D table allocation/cache
  ownership toward an explicit table-cache service.
- Continue parser diagnostics migration in `parse_molFile.cpp`,
  `parse_input*.cpp`, `parse_molecule_bngl.cpp`, and restart semantic
  diagnostics.
- MPI rebuild remains a local toolchain-wrapper issue unless the build system
  grows an explicit MPI compiler override.
