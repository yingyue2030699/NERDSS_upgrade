# Stack 22 Validation - 2026-06-04

## Scope

Stack 22 integrates three focused slices on top of
`codex/validation-integration-stack-21`:

- `codex/parser-input-diagnostics-stack22-slice`, which routes invalid
  boundary value parsing through structured parser diagnostics.
- `codex/molfile-mpi-diagnostics-stack22-slice`, which adds structured `.mol`
  bond-count diagnostics and rank-aware MPI diagnostics for legacy error
  helpers.
- `codex/mathengine-rotation-policy-stack22-slice`, which moves association
  rotation diffusion-policy selection behind `nerdss::core::MathEngine`.

Accepted parser syntax, stochastic event selection, coordinate mutation,
topology mutation, and MPI abort/finalization policy remain in legacy call
paths.

## Pull Requests

- Parser input diagnostics slice: PR #36.
- `.mol` and MPI diagnostics slice: PR #37.
- MathEngine association rotation-policy slice: PR #38.

## Local Validation

All commands below were run on `codex/validation-integration-stack-22` at
`31998184ea052c1efa704d2dedcfb64a05737e97`. The only dirty worktree entry
during the validation-suite runs was this validation note.

- `git diff --check HEAD`: passed.
- `cmake -S . -B build-stack22`: passed.
- `cmake --build build-stack22 --target nerdss_unit_tests --parallel 4`:
  passed.
- `ctest --test-dir build-stack22 --output-on-failure`: passed, 3/3 tests
  passed in 0.63 seconds.
- `tools/run_static_analysis.sh include/parser/parser_diagnostics.hpp include/error/error_diagnostics.hpp include/core/math_engine.hpp src/parser/parse_input.cpp src/parser/parse_molFile.cpp src/error/error.cpp src/reactions/determine_rotation_angles.cpp tests/unit/test_diagnostics.cpp tests/unit/test_vector_coord.cpp`:
  exited 0 with existing clang-tidy warnings in legacy stream usage,
  signatures, braces, narrowing conversions, and redundant test declarations.
- `make serial -j4`: passed and rebuilt `bin/nerdss`.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --output-dir /tmp/nerdss-stack22-validation`:
  passed. Smoke took 0.31 seconds, unit configure 0.71 seconds, unit build
  15.05 seconds, unit CTest 0.66 seconds, and regression 1.50 seconds.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --skip-smoke --skip-unit --skip-regression --benchmarks --benchmark-case small_homotrimer --output-dir /tmp/nerdss-stack22-small-benchmark`:
  passed. The `small_homotrimer` benchmark took 8.81 seconds.
- `make mpi -j4` was attempted on the mol-file/MPI diagnostics slice and
  failed before project code because `/opt/anaconda3/bin/mpicxx` invokes the
  missing compiler `x86_64-apple-darwin13.4.0-clang++`.

## Follow-Up Notes

- Restart reaction-type diagnostics in `src/io/read_restart.cpp` remain the
  next low-risk restart parsing target.
- Broader parser hardening still includes blank/comment line handling in
  `parse_input*.cpp` and `parse_molFile.cpp`.
- MPI binary verification needs a working local `mpicxx` wrapper; the local
  wrapper currently points to a missing compiler.
- Further MathEngine/ProbabilityEngine work should continue with pure kernels
  before moving stochastic event selection or topology mutation.
