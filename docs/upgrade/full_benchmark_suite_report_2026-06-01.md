# Full Benchmark Suite Report - 2026-06-01

This report records a clean full-stack validation and benchmark rerun for
stack 4. The report commit lives on `codex/stack4-full-benchmark-rerun`; the
benchmark execution used a detached worktree at stack-4 commit
`40995f47a6830437e6b5f535bd1f2e31908a8140`, which is
`personal/codex/validation-integration-stack-4`.

## Run Context

| Field | Value |
| --- | --- |
| Local date | 2026-06-01 |
| Created at | 2026-06-01T04:36:35+00:00 |
| Report branch | `codex/stack4-full-benchmark-rerun` |
| Commit under test | `40995f47a6830437e6b5f535bd1f2e31908a8140` |
| Execution worktree | `/private/tmp/nerdss-stack4-full-benchmark-clean-20260601` |
| Validation artifacts | `/private/tmp/nerdss-stack4-full-benchmark-clean-20260601-results/` |
| Benchmark JSON | `/private/tmp/nerdss-stack4-full-benchmark-clean-20260601-results/benchmarks/benchmark_results.json` |
| Benchmark CSV | `/private/tmp/nerdss-stack4-full-benchmark-clean-20260601-results/benchmarks/benchmark_results.csv` |
| Platform | macOS 14.5 arm64, Darwin 23.5.0 |
| CPU / memory | 10 logical CPUs, 34,359,738,368 bytes RAM |
| Python | 3.13.3 |

Command:

```sh
python3 -B tools/run_upgrade_validation.py \
  --benchmarks \
  --output-dir /private/tmp/nerdss-stack4-full-benchmark-clean-20260601-results
```

The execution worktree was clean when `validation_report.json` and
`benchmark_results.json` were written. A first in-place run on
`codex/stack4-full-benchmark-rerun` also passed, but the main checkout contained
preexisting/unrelated local work (`validation_results/` and later concurrent
parser-diagnostic edits), so the clean detached rerun above is the one used for
this report.

## Validation Results

The local stack passed the serial smoke build/run, CMake unit
configuration/build/CTest, the default seven-case regression harness, and all
four benchmark manifest cases.

| Step | Status | Runtime (s) | Notes |
| --- | --- | ---: | --- |
| Smoke | Passed | 151.694 | Built serial NERDSS and ran the generated minimal smoke simulation. |
| Unit configure | Passed | 0.792 | Configured `build-upgrade-validation`. |
| Unit build | Passed | 13.078 | Built `nerdss_unit_tests`. |
| Unit CTest | Passed | 0.292 | `ctest --output-on-failure` passed. |
| Regression | Passed | 1.503 | Seven default regression cases passed. |
| Benchmarks | Passed | 13.145 | All benchmark manifest cases passed. |

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
`bin/nerdss`. The clean rerun did not report peak RSS for any benchmark case;
`max_rss_bytes` and `max_rss_source` were empty in the benchmark JSON.

| Case | Status | Wall (s) | CPU (s) | User CPU (s) | System CPU (s) | Output files | Output bytes | Stdout bytes | Stderr bytes |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `small_homotrimer` | Passed | 8.578 | 8.504 | 8.409 | 0.095 | 24 | 8,828,274 | 7,296 | 0 |
| `medium_michaelis_menten` | Passed | 1.094 | 0.881 | 0.862 | 0.019 | 15 | 164,072 | 10,204 | 0 |
| `representative_implicit_lipid` | Passed | 2.094 | 1.963 | 1.932 | 0.031 | 24 | 1,510,622 | 9,133 | 0 |
| `large_clathrin_short` | Passed | 0.427 | 0.258 | 0.243 | 0.015 | 15 | 442,369 | 18,951 | 0 |

## Previous Full-Suite Comparison

The previous full-suite report is
`docs/upgrade/full_benchmark_suite_report_2026-05-31.md`, collected from
`codex/full-benchmark-suite-report` at stack-3 commit
`6039689352266026fcc296e217715cd19395c19e`.

| Case | Previous wall (s) | Stack-4 wall (s) | Wall delta | Previous CPU (s) | Stack-4 CPU (s) | CPU delta | Output byte delta |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `small_homotrimer` | 9.174 | 8.578 | -0.596 (-6.5%) | 9.026 | 8.504 | -0.522 (-5.8%) | 0 |
| `medium_michaelis_menten` | 1.031 | 1.094 | +0.063 (+6.2%) | 0.886 | 0.881 | -0.005 (-0.6%) | 0 |
| `representative_implicit_lipid` | 2.074 | 2.094 | +0.020 (+1.0%) | 1.977 | 1.963 | -0.014 (-0.7%) | 0 |
| `large_clathrin_short` | 0.419 | 0.427 | +0.008 (+2.0%) | 0.260 | 0.258 | -0.002 (-0.9%) | 0 |

Against the 2026-05-28 numeric baseline in
`docs/upgrade/benchmark_baseline.md`, stack 4 is within a few percent for all
manifest cases. The largest wall-time movement is
`medium_michaelis_menten` at `+0.058s` (`+5.6%`); the largest CPU-time movement
is `small_homotrimer` at `-0.056s` (`-0.7%`). Output byte totals match the
2026-05-31 full-suite report exactly.

## Recent `small_homotrimer` Stack Results

Recent upgrade-slice docs list `small_homotrimer` benchmark-mode wall times from
`8.060s` to `9.384s`, CPU times from `7.849s` to `8.982s`, and a median of
`8.675s` wall / `8.518s` CPU. This stack-4 run's `8.578s` wall and `8.504s`
CPU are inside those observed ranges and slightly below those documented
medians.

Compared with the 2026-05-28 baseline row (`8.668s` wall / `8.560s` CPU),
stack 4 is `0.090s` faster on wall time and `0.056s` faster on CPU time. The
`small_homotrimer` output volume remains `80` bytes below that baseline and
matches the 2026-05-31 full-suite report.

## Remaining Gaps

- No benchmark runner bugfix was required.
- The benchmark harness records timing and output volume only; scientific output
  equivalence remains covered by the separate regression harness.
- Peak RSS was unavailable in this clean rerun.
- Benchmarks were serial only. MPI benchmarks, sanitizer builds, static
  analysis, and privileged profiling were outside this run.
- Timings are local single-run measurements on one macOS arm64 host; they are
  useful for stack-to-stack smoke comparisons, not a controlled performance
  study.
