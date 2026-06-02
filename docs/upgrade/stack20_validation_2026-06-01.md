# Stack 20 Validation - 2026-06-01

## Scope

Stack 20 integrates two focused slices on top of
`codex/validation-integration-stack-19`:

- `codex/parser-reaction-diagnostics-stack19-slice`, which routes additional
  `parse_reaction.cpp` hard failure boundaries through structured parser
  diagnostics.
- `codex/probability-engine-compartment-transmission-slice`, which moves the
  shared compartment entry/exit transmission probability setup behind
  `nerdss::core::ProbabilityEngine`.

The integration preserves accepted legacy parser behavior and does not alter
the core compartment probability kernels. Legacy wrappers still own molecule
probability-vector mutation, warnings, and calls into the existing probability
functions.

## Local Validation

All local validation passed:

- `git diff --check HEAD`: passed.
- `cmake -S . -B build-stack20`: passed with the existing CMake minimum-version
  deprecation warning.
- `cmake --build build-stack20 --target nerdss_unit_tests --parallel 4`:
  passed.
- `ctest --test-dir build-stack20 --output-on-failure`: passed, 3/3 tests.
- `tools/run_static_analysis.sh include/core/probability_engine.hpp include/parser/parser_diagnostics.hpp src/parser/parse_reaction.cpp src/reactions/determine_entering_compartment_probability.cpp src/reactions/determine_exiting_compartment_probability.cpp tests/unit/test_diagnostics.cpp tests/unit/test_vector_coord.cpp`:
  exited 0 with existing legacy clang-tidy warnings in `parse_reaction.cpp`,
  the compartment probability wrappers, and `tests/unit/test_vector_coord.cpp`.
- `make serial -j4`: passed and produced `bin/nerdss`.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --output-dir /tmp/nerdss-stack20-validation`:
  passed smoke, unit configure/build/CTest, and regression. Benchmarks were
  skipped in this run.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --skip-smoke --skip-unit --skip-regression --benchmarks --benchmark-case small_homotrimer --output-dir /tmp/nerdss-stack20-small-benchmark`:
  passed the `small_homotrimer` benchmark in 9.231883 seconds.

Validation reports:

- `/private/tmp/nerdss-stack20-validation/validation_report.json`
- `/private/tmp/nerdss-stack20-small-benchmark/validation_report.json`

## Remote Slice CI

- PR #30, `codex/parser-reaction-diagnostics-stack19-slice`: GitHub Actions
  build/unit/smoke/regression and coverage checks passed.
- PR #31, `codex/probability-engine-compartment-transmission-slice`: GitHub
  Actions build/unit/smoke/regression and coverage checks passed.

## Follow-Up Notes

- Remaining parser diagnostics work is concentrated in `parse_input*.cpp`,
  `parse_molecule_bngl.cpp`, `parse_molFile.cpp`, restart parsing, and
  MPI-aware error paths.
- Remaining MathEngine/ProbabilityEngine work should continue extracting pure
  setup and mathematical kernels without changing stochastic event selection or
  mutable topology operations.
