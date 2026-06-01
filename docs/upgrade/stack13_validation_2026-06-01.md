# Stack 13 Validation - 2026-06-01

This stack starts from validated stack 12 and integrates two focused branches:

- `codex/observable-type-diagnostic`
- `codex/probability-engine-diffusion-slice`

## Changes

- Replaced the unknown observable-type exit path with the structured parser
  diagnostic helper and added smoke-runner coverage for malformed observable
  input.
- Moved rotational diffusion contribution math behind
  `nerdss::core::ProbabilityEngine` while preserving the legacy reaction
  probability call sites as forwarding users.
- Extended unit coverage for the new ProbabilityEngine rotational diffusion
  helper.

## Validation

- `git diff --check HEAD`: passed.
- `cmake -S . -B build-stack13`: passed with the existing CMake compatibility
  warning.
- `cmake --build build-stack13 --target nerdss_unit_tests --parallel 4`:
  passed.
- `ctest --test-dir build-stack13 --output-on-failure`: 3/3 passed.
- `make serial -j4`: passed, rebuilding `bin/nerdss`.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --output-dir /tmp/nerdss-stack13-validation`:
  smoke, unit configure/build/CTest, and regression passed.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --skip-smoke --skip-unit --skip-regression --benchmarks --benchmark-case small_homotrimer --output-dir /tmp/nerdss-stack13-small-benchmark`:
  passed.

## Benchmark Snapshot

`small_homotrimer` completed with status `passed`, wall time
`9.09292283304967` seconds, CPU time `8.924515` seconds, 24 output files, and
8,828,274 output bytes.

## Static Analysis Notes

- `tools/run_static_analysis.sh src/parser/parse_observable.cpp`: exited 0;
  only non-user warnings were suppressed by the configured clang-tidy filter.
- `tools/run_static_analysis.sh src/reactions/determine_2D_bimolecular_reaction_probability.cpp src/reactions/determine_2D_implicitlipid_reaction_probability.cpp src/reactions/determine_3D_bimolecular_reaction_probability.cpp src/reactions/determine_3D_implicitlipid_reaction_probability.cpp`:
  exited 0; reported existing legacy warnings in reaction entry points,
  including easily-swappable parameters, boolean simplifications, dead stores,
  `std::endl`, braces, and narrowing conversions.
