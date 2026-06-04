# Stack 21 Validation - 2026-06-04

## Scope

Stack 21 integrates two focused slices on top of
`codex/validation-integration-stack-20`:

- `codex/parser-molecule-bngl-diagnostics-stack21-slice`, which routes
  malformed reaction molecule BNGL syntax failures through structured parser
  diagnostics.
- `codex/probability-engine-stack21-slice`, which moves association contact
  geometry normalization behind `nerdss::core::ProbabilityEngine`.

Accepted parser behavior, stochastic event selection, reweighting, and mutable
molecule probability-vector updates remain in the legacy wrappers.

## Pull Requests

- Parser diagnostics slice: PR #33.
- ProbabilityEngine association contact slice: PR #34.

## Local Validation

All commands below were run on `codex/validation-integration-stack-21` at
`60193eb97ecf0d14a673a9b50825a35ee7bf1286`. The only dirty worktree entry
during the validation-suite runs was this validation note.

- `git diff --check HEAD`: passed.
- `cmake -S . -B build-stack21`: passed.
- `cmake --build build-stack21 --target nerdss_unit_tests --parallel 4`:
  passed.
- `ctest --test-dir build-stack21 --output-on-failure`: passed, 3/3 tests
  passed in 0.63 seconds.
- `tools/run_static_analysis.sh include/core/probability_engine.hpp include/parser/parser_diagnostics.hpp src/parser/parse_molecule_bngl.cpp src/reactions/determine_2D_bimolecular_reaction_probability.cpp src/reactions/determine_3D_bimolecular_reaction_probability.cpp src/reactions/determine_3D_implicitlipid_reaction_probability.cpp tests/unit/test_diagnostics.cpp tests/unit/test_vector_coord.cpp`:
  exited 0 with existing clang-tidy warnings in legacy signatures, stream
  flushing, and redundant test declarations.
- `make serial -j4`: passed and rebuilt `bin/nerdss`.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --output-dir /tmp/nerdss-stack21-validation`:
  passed. Smoke took 0.30 seconds, unit configure 0.93 seconds, unit build
  15.43 seconds, unit CTest 0.63 seconds, and regression 1.50 seconds.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --skip-smoke --skip-unit --skip-regression --benchmarks --benchmark-case small_homotrimer --output-dir /tmp/nerdss-stack21-small-benchmark`:
  passed. The `small_homotrimer` benchmark took 9.23 seconds.

## Follow-Up Notes

- Remaining parser diagnostics work is concentrated in `parse_input*.cpp`,
  `parse_molFile.cpp`, restart parsing, and MPI-aware error paths.
- Remaining MathEngine/ProbabilityEngine work should continue extracting pure
  setup and mathematical kernels before moving any mutable topology behavior.
