# Full Benchmark Suite Report - 2026-05-31

This report records a clean full-stack validation and benchmark run from
`codex/full-benchmark-suite-report`, based on
`personal/codex/validation-integration-stack-3` commit
`6039689352266026fcc296e217715cd19395c19e`.

## Run Context

| Field | Value |
| --- | --- |
| Local date | 2026-05-31 |
| Created at | 2026-05-31T23:31:11+00:00 |
| Branch | `codex/full-benchmark-suite-report` |
| Commit under test | `6039689352266026fcc296e217715cd19395c19e` |
| Worktree | `/Users/yueying/Documents/NERDSS upgrade/worktrees/full-benchmark-suite-report` |
| Validation artifacts | `/private/tmp/nerdss-full-benchmark-suite-20260531/` |
| Benchmark JSON | `/private/tmp/nerdss-full-benchmark-suite-20260531/benchmarks/benchmark_results.json` |
| Benchmark CSV | `/private/tmp/nerdss-full-benchmark-suite-20260531/benchmarks/benchmark_results.csv` |
| Platform | macOS 14.5 arm64, Darwin 23.5.0 |
| CPU / memory | 10 logical CPUs, 34,359,738,368 bytes RAM |
| Python | 3.13.3 |

Command:

```sh
python3 -B tools/run_upgrade_validation.py \
  --benchmarks \
  --output-dir /tmp/nerdss-full-benchmark-suite-20260531
```

The repository was clean when the validation and benchmark reports were written.
Generated build, validation, and benchmark artifacts were kept outside the
commit or in ignored local build paths.

## Validation Results

The broadest practical local suite passed: serial smoke build/run, CMake unit
configuration/build/CTest, the default seven-case regression harness, and all
four benchmark manifest cases.

| Step | Status | Runtime (s) | Notes |
| --- | --- | ---: | --- |
| Smoke | Passed | 148.169 | Built serial NERDSS and ran the generated minimal smoke simulation. |
| Unit configure | Passed | 0.789 | Configured `build-upgrade-validation`. |
| Unit build | Passed | 14.029 | Built `nerdss_unit_tests`. |
| Unit CTest | Passed | 0.342 | `ctest --output-on-failure` passed. |
| Regression | Passed | 1.621 | Seven default regression cases passed. |
| Benchmarks | Passed | 13.803 | All benchmark manifest cases passed. |

Regression cases covered:

| Case | Type | Status |
| --- | --- | --- |
| `create_destroy_fresh_small` | deterministic | Passed |
| `create_destroy_stochastic_seed_set` | stochastic ensemble | Passed |
| `michaelis_menten_fresh_small` | deterministic | Passed |
| `implicit_lipid_fresh_small` | deterministic | Passed |
| `sphere_fresh_small` | deterministic | Passed |
| `trimer_het_fresh_small` | deterministic | Passed |
| `homo_trimer_restart_from_1000` | deterministic restart | Passed |

## Benchmark Results

All benchmark cases used seed `12345` and the serial executable at
`bin/nerdss`. Peak RSS was not reported by this run; the benchmark runner's
resource and polling paths both left `max_rss_bytes` empty for these cases.

| Case | Status | Wall (s) | CPU (s) | User CPU (s) | System CPU (s) | Output files | Output bytes | Stdout bytes | Stderr bytes |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `small_homotrimer` | Passed | 9.174 | 9.026 | 8.912 | 0.114 | 24 | 8,828,274 | 7,317 | 0 |
| `medium_michaelis_menten` | Passed | 1.031 | 0.886 | 0.869 | 0.017 | 15 | 164,072 | 10,225 | 0 |
| `representative_implicit_lipid` | Passed | 2.074 | 1.977 | 1.946 | 0.031 | 24 | 1,510,622 | 9,154 | 0 |
| `large_clathrin_short` | Passed | 0.419 | 0.260 | 0.245 | 0.015 | 15 | 442,369 | 18,972 | 0 |

## Baseline Comparison

The comparison baseline is the numeric table in
`docs/upgrade/benchmark_baseline.md`, collected on 2026-05-28 from
`codex/validation-integration` with the same benchmark manifest and seed.

| Case | Baseline wall (s) | Current wall (s) | Wall delta | Baseline CPU (s) | Current CPU (s) | CPU delta | Output byte delta |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `small_homotrimer` | 8.668 | 9.174 | +0.506 (+5.8%) | 8.560 | 9.026 | +0.466 (+5.4%) | -80 |
| `medium_michaelis_menten` | 1.036 | 1.031 | -0.005 (-0.5%) | 0.871 | 0.886 | +0.015 (+1.8%) | 0 |
| `representative_implicit_lipid` | 2.068 | 2.074 | +0.006 (+0.3%) | 1.979 | 1.977 | -0.002 (-0.1%) | -80 |
| `large_clathrin_short` | 0.419 | 0.419 | +0.000 (+0.1%) | 0.256 | 0.260 | +0.004 (+1.8%) | -8 |

Recent docs contain additional `small_homotrimer` benchmark-mode timings from
upgrade slices. Across those available entries, wall time ranges from `8.060s`
to `9.384s`, CPU time ranges from `7.849s` to `8.982s`, and the median is
`8.675s` wall / `8.518s` CPU. This run's `small_homotrimer` wall time is within
that observed wall-time range and its CPU time is slightly above the prior
documented CPU range by `0.044s`.

## Remaining Gaps

- No benchmark runner bugfix was required.
- The benchmark harness records timing and output volume only; scientific output
  equivalence remains covered by the separate regression harness.
- Peak RSS was unavailable in this local run.
- MPI benchmarks, sanitizer builds, static analysis, and privileged profiling
  were outside this benchmark-suite run.
