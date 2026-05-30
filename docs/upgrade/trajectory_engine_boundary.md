# Trajectory Engine Boundary Extraction

Branch: `codex/trajectory-engine-boundary`

## Scope

This slice moves the sphere/box dispatch for trajectory overlap sweeps behind
`nerdss::core::TrajectoryEngine` while preserving the legacy entry points as
forwarding wrappers.

Extracted dispatchers:

- `sweep_separation_complex_rot`
- `sweep_separation_complex_rot_memtest`
- `sweep_separation_complex_rot_memtest_cluster`

The selected sphere/box implementations, overlap loops, reflection calls,
resampling behavior, and RNG draw order are unchanged.

## Validation

| Command | Result |
| --- | --- |
| `cmake --build build-upgrade-validation --target nerdss_unit_tests` | Passed |
| `ctest --test-dir build-upgrade-validation --output-on-failure` | Passed |
| `make serial` | Passed |
| `python3 tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --output-dir /tmp/nerdss-trajectory-engine-boundary-validation` | Passed smoke, unit configure/build/ctest, and regression |
| `python3 tools/run_upgrade_validation.py --skip-smoke --skip-unit --skip-regression --benchmarks --benchmark-case small_homotrimer --skip-build --binary bin/nerdss --output-dir /tmp/nerdss-trajectory-engine-boundary-benchmark` | Passed |

Benchmark result for `small_homotrimer`: wall time `8.861864667152986s`, CPU
time `8.757227s`.

## Follow-Ups

- Add targeted tests that invoke the sweep dispatchers once a compact synthetic
  molecule/complex fixture exists.
- Extract deterministic coordinate-update helpers before moving mutation-heavy
  reflection and resampling logic.
- Keep trajectory RNG draws inside the existing selected implementations until
  parity checks cover each path directly.
