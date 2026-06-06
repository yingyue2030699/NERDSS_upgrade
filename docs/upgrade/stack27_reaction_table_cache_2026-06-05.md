# Stack 27 Reaction Table Cache - 2026-06-05

## Scope

This ProbabilityEngine slice moves 2D reaction table lookup and matrix ownership
behind a service-style cache while preserving the existing stochastic reaction
flow.

- Adds `nerdss::core::ReactionTable2DCache` as the owner of survival, norm, and
  irreversible GSL matrices.
- Preserves the legacy table key tolerances: `1e-8` for association rate and
  `1e-4` for total diffusion.
- Preserves the current max-table fatal behavior when the cache reaches
  `params.max2DRxns`.
- Passes one cache object through the serial/MPI 2D bimolecular reaction call
  chain instead of raw `tableIDs`, `DDTableIndex`, and three matrix vectors.
- Keeps stochastic event selection, molecule probability mutation, reweighting,
  and topology mutation in the legacy reaction functions.

## Local Validation

Commands run on `codex/reaction-table-cache-service-stack27-slice`:

- `git diff --check HEAD`: passed.
- `tools/run_static_analysis.sh include/core/probability/reaction_table_2d_cache.hpp src/reactions/determine_2D_bimolecular_reaction_probability.cpp src/reactions/check_bimolecular_reactions.cpp src/reactions/measure_separations_to_identify_possible_reactions.cpp tests/unit/test_vector_coord.cpp`:
  exited 0 with existing legacy readability, identifier-length, and magic-number
  warnings in the reaction/test sources.
- `cmake -S . -B build-reaction-table-cache-service`: passed.
- `cmake --build build-reaction-table-cache-service --target nerdss_unit_tests --parallel 4`:
  passed.
- `ctest --test-dir build-reaction-table-cache-service --output-on-failure`:
  passed, 3/3 tests.
- `make serial -j4`: passed and rebuilt `bin/nerdss`.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --output-dir /tmp/nerdss-reaction-table-cache-service-validation`:
  passed smoke, unit configure/build/ctest, and regression; benchmarks were
  skipped by the validation runner.

## Follow-Up Notes

- The cache is intentionally still GSL-backed; later GPU-oriented work can add a
  backend-neutral table storage interface once callers no longer own GSL
  pointers.
- MPI source signatures are updated, but full MPI rebuild remains dependent on
  the local `mpicxx` wrapper being repaired.
