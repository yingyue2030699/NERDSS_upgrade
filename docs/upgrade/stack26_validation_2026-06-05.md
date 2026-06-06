# Stack 26 Validation - 2026-06-05

## Scope

Stack 26 builds on the Stack 25 `ReactionTable2DService` extraction.

- Moved the 2D reaction-table matrix-fill algorithms into
  `nerdss::core::ReactionTable2DService`: combined table fill, survival table
  fill, free-diffusion normalization table fill, and irreversible PIR table
  fill.
- Kept the legacy `create_DDMatrices`, `create_survMatrix`,
  `create_normMatrix`, and `create_pirMatrix` functions as forwarding wrappers.
- Linked those legacy table-fill wrappers into the CMake unit-test target so
  service-vs-wrapper matrix-fill coverage can run under CTest.
- Preserved existing GSL integration tolerances, GSL error-handler behavior,
  matrix orientation, and caller-owned matrix allocation.

## Local Validation

Commands run on `codex/reaction-table-fill-service-stack26-slice`:

- `git diff --check HEAD`: passed.
- `cmake -S . -B build-reaction-table-fill-service`: passed.
- `tools/run_static_analysis.sh include/core/probability/reaction_table_2d_service.hpp src/reactions/create_DDMatrices.cpp src/reactions/create_survMatrix.cpp src/reactions/create_normMatrix.cpp src/reactions/create_pirMatrix.cpp tests/unit/test_vector_coord.cpp`:
  exited 0 with existing legacy/test warnings.
- `cmake --build build-reaction-table-fill-service --target nerdss_unit_tests --parallel 4`:
  passed.
- `ctest --test-dir build-reaction-table-fill-service --output-on-failure`:
  passed, 3/3 tests passed.
- `make serial -j4`: passed and rebuilt `bin/nerdss`.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --output-dir /tmp/nerdss-reaction-table-fill-service-validation`:
  passed. Benchmarks were skipped for this narrow extraction slice.

## Follow-Up Notes

- Move 2D table allocation/cache ownership into an explicit table-cache service.
- Continue output file-open diagnostics in parser setup and MPI entry points.
- MPI rebuild remains locally dependent on a working MPI compiler wrapper.
