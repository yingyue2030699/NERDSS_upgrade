# Stack 23 Validation - 2026-06-04

## Scope

Stack 23 integrates three focused diagnostics slices on top of
`codex/validation-integration-stack-22`:

- `codex/restart-reaction-diagnostics-stack23-slice`, which routes invalid
  restart reaction-type sentinels in `read_restart` through structured restart
  diagnostics.
- `codex/restart-malformed-diagnostics-stack23-slice`, which replaces the
  final malformed restart catch handlers in `read_restart` with structured
  diagnostics.
- `codex/simulvolume-mpi-diagnostics-stack23-slice`, which adds MPI-aware
  structured diagnostics for molecule subcell assignment and membrane placement
  failures in `SimulVolume::update_memberMolLists`.

Accepted restart records, molecule placement, stochastic event selection,
topology mutation, and MPI finalization policy remain in the legacy code paths.
The SimulVolume slice adds a negative-bin guard so impossible MPI subcell
assignments diagnose before indexing the subcell array.

## Pull Requests

- Restart reaction diagnostics slice: PR #40.
- Malformed restart diagnostics slice: PR #41.
- SimulVolume MPI diagnostics slice: PR #42.

## Local Validation

All commands below were run on `codex/validation-integration-stack-23` at
`93c2ff6254ea4c0901748985e6e1f63251c3929f`. The worktree was clean during the
validation-suite and benchmark runs.

- `git diff --check HEAD`: passed.
- `cmake -S . -B build-stack23`: passed.
- `cmake --build build-stack23 --target nerdss_unit_tests --parallel 4`:
  passed.
- `ctest --test-dir build-stack23 --output-on-failure`: passed, 3/3 tests
  passed.
- `tools/run_static_analysis.sh include/parser/parser_diagnostics.hpp include/error/error_diagnostics.hpp src/io/read_restart.cpp src/classes/class_SimulVolume.cpp tests/unit/test_diagnostics.cpp`:
  exited 0 with existing clang-tidy warnings in legacy stream usage,
  signatures, braces, narrowing conversions, and analyzer warnings in legacy
  restart parsing.
- `make serial -j4`: passed and rebuilt `bin/nerdss`.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --output-dir /tmp/nerdss-stack23-validation`:
  passed. Smoke took 0.31 seconds, unit configure 0.80 seconds, unit build
  15.61 seconds, unit CTest 0.70 seconds, and regression 1.50 seconds.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --output-dir /tmp/nerdss-stack23-benchmark --benchmarks --benchmark-case small_homotrimer`:
  passed. The `small_homotrimer` benchmark took 9.06 seconds.

## Follow-Up Notes

- MPI rebuild validation is still blocked by the local MPI compiler wrapper:
  `/opt/anaconda3/bin/mpicxx` invokes the missing compiler
  `x86_64-apple-darwin13.4.0-clang++`.
- Restart parsing still has additional unchecked stream reads and file-open
  context that should be migrated in a future slice.
- Broader parser hardening remains for `parse_input*.cpp`,
  `parse_molecule_bngl.cpp`, and semantic `.mol` validation paths not covered
  by earlier diagnostics slices.
- Further MathEngine/ProbabilityEngine extraction should continue with pure
  kernels such as 3D association probabilities, 2D diffusion-table lookups,
  diffusion/Rmax precomputation, implicit lipid probabilities, and spherical
  geometry helpers before touching stochastic selection or topology mutation.
