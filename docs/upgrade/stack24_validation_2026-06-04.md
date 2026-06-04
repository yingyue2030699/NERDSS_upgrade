# Stack 24 Validation - 2026-06-04

## Scope

Stack 24 integrates two focused MathEngine/ProbabilityEngine slices on top of
`codex/validation-integration-stack-23`:

- `codex/association-probability-service-stack24-slice`, which moves pure
  1D/3D association and rebinding probability kernels into
  `nerdss::core::AssociationProbabilityService` while keeping
  `ProbabilityEngine` and legacy free functions as forwarding wrappers.
- `codex/probability-engine-rmax-precompute-stack24-slice`, which moves scalar
  setup Rmax arithmetic into `ProbabilityEngine` helpers while keeping
  `set_rMaxLimit` responsible for MolTemplate/ForwardRxn lookup, restart-added
  interface resolution, parameter mutation, and legacy output.

Stochastic event selection, topology mutation, neighbor-search policy, accepted
probability formulas, and the legacy `set_rMaxLimit` promoter branch structure
remain unchanged.

## Pull Requests

- Association probability service slice: PR #44.
- Rmax precompute helper slice: PR #45.

## Local Validation

All commands below were run on `codex/validation-integration-stack-24` at
`37006a29f6201141a96fc3c926d277d09a741d87`. The worktree was clean during the
validation-suite and benchmark runs.

- `git diff --check HEAD`: passed.
- `cmake -S . -B build-stack24`: passed.
- `cmake --build build-stack24 --target nerdss_unit_tests --parallel 4`:
  passed.
- `ctest --test-dir build-stack24 --output-on-failure`: passed, 3/3 tests
  passed.
- `tools/run_static_analysis.sh include/core/probability/association_probability_service.hpp include/core/probability_engine.hpp src/system_setup/set_rMaxLimit.cpp tests/unit/test_vector_coord.cpp`:
  exited 0 with existing clang-tidy warnings in legacy setup/test code. The
  `set_rMaxLimit` reactant-1 promoter dead-store warning is preserved legacy
  branch behavior and was not normalized in this slice.
- `make serial -j4`: passed and rebuilt `bin/nerdss`.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --output-dir /tmp/nerdss-stack24-validation`:
  passed. Smoke took 0.28 seconds, unit configure 0.76 seconds, unit build
  14.46 seconds, unit CTest 0.64 seconds, and regression 1.47 seconds.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --output-dir /tmp/nerdss-stack24-benchmark --benchmarks --benchmark-case small_homotrimer`:
  passed. The `small_homotrimer` benchmark took 8.50 seconds.

## Follow-Up Notes

- More `ProbabilityEngine` decomposition remains for 2D table ownership and
  broader service organization.
- Broader parser diagnostics still remain in `parse_input*.cpp`,
  `parse_molecule_bngl.cpp`, `parse_molFile.cpp` semantic paths, and restart
  file-open/stream-field handling.
- MPI rebuild validation remains blocked by the local MPI compiler wrapper:
  `/opt/anaconda3/bin/mpicxx` invokes the missing compiler
  `x86_64-apple-darwin13.4.0-clang++`.
