# Validation Progress - 2026-05-28

This note records the validation, benchmark, coverage, and agent status from
the local integration workflow on `codex/validation-integration`.

## Integration Context

Local branch:

```text
codex/validation-integration
```

Integrated slices at the time of this run:

- Smoke runner and serial build fixes.
- Deterministic regression harness.
- Stochastic seed-set validation harness.
- Minimal CTest unit test target.
- Create/destroy crash hardening.
- Legacy restart crash hardening.
- Benchmark harness.
- Profiling command guide.

The local worktree was dirty only because of generated build directories:

```text
?? build-coverage/
?? build-unit-validation/
```

## Validation Results

| Check | Result | Notes |
| --- | --- | --- |
| Serial smoke runner | Passed | Built serial NERDSS and completed the generated smoke simulation. |
| CTest unit target | Passed | `1/1` tests passed. |
| Regression manifest | Passed | All three selected validation cases passed. |
| Stochastic seed set | Passed | `create_destroy_stochastic_seed_set` passed seeds `31001` through `31005`. |
| Restart regression | Passed | `homo_trimer_restart_from_1000` passed with legacy restart input. |

The restart regression depends on the legacy `RNGwrite` parser hardening. The
create/destroy seed-set regression depends on skipping destroyed molecules in
the overlap-check loop.

## Coverage Snapshot

Coverage was collected after smoke and the full validation manifest using the
Xcode `llvm-cov` toolchain.

| Metric | Coverage |
| --- | ---: |
| Lines | 17.43% |
| Functions | 20.25% |
| Branches | 16.32% |

This is an early integration coverage snapshot, not a target threshold. It is
useful for trend tracking and for identifying large untested regions before
deeper refactoring.

## Benchmark Snapshot

All benchmark manifest cases completed with exit status `0` using seed `12345`.

| Case | Wall time (s) | CPU time (s) | Output files | Output bytes |
| --- | ---: | ---: | ---: | ---: |
| `small_homotrimer` | 8.668 | 8.560 | 24 | 8,828,354 |
| `medium_michaelis_menten` | 1.036 | 0.871 | 15 | 164,072 |
| `representative_implicit_lipid` | 2.068 | 1.979 | 24 | 1,510,702 |
| `large_clathrin_short` | 0.419 | 0.256 | 15 | 442,377 |

Benchmark artifacts were written outside the repository:

```text
/tmp/nerdss-benchmark-baseline/
/tmp/nerdss-benchmark-representative/
/tmp/nerdss-benchmark-large/
```

## Profiling Status

`tools/profile_commands.sh` generated the expected macOS `sample` command
block for the homotrimer case. A live attempt to attach `sample` to the running
NERDSS process was blocked by macOS permissions:

```text
sample cannot examine process ...; try running with `sudo`.
```

The same homotrimer case was rerun successfully while trying to collect timing
data with `/usr/bin/time -lp`, but the sandbox blocked the kernel clock query
after the simulation completed. The benchmark runner timings above remain the
current reliable baseline. A full hotspot report still needs an elevated
macOS Instruments or `sample` run, or a Linux `perf`/`gprof` profiling run.

## Agent Status

| Agent slice | Branch | Status |
| --- | --- | --- |
| Main loop first slice | `codex/upgrade-main-loop-first-slice` | Docs-only slice committed and pushed to `personal`. |
| Run manifest writer | `codex/upgrade-run-manifest-writer` | Implemented, validated, committed, and pushed to `personal`. |
| CI regression workflow | `codex/upgrade-ci-regression` | Implemented and validated locally; push blocked because the OAuth credential lacks GitHub `workflow` scope. |
| Expanded validation | `codex/upgrade-expanded-validation` | Implemented, validated, committed, and pushed to `personal`. |

## Integration Update

The local integration branch was advanced after the initial note by merging the
completed non-core slices for baseline policy, architecture mapping, style
tooling, sanitizer/static-analysis tooling, input schema conversion, run
manifest writing, main-loop extraction planning, and expanded regression
validation.

A small CLI error-handling slice was also added locally:

- `parse_command` now reports missing values for flags such as `-f` and `-s`
  instead of indexing past `argv`.
- `--help` prints usage and exits successfully.
- Running without `-f` or `-r` now exits with the structured input error code.
- The smoke runner now includes negative CLI checks for `--help`, missing `-f`
  value, and missing `-s` value.

Post-merge validation:

| Check | Result | Notes |
| --- | --- | --- |
| Python syntax checks | Passed | Smoke, regression, benchmark, input converter, and manifest inspector scripts compiled with `py_compile`. |
| JSON schema syntax | Passed | Input and run-manifest schemas loaded with `python3 -m json.tool`. |
| Serial build | Passed | Incremental `make serial` rebuilt `bin/nerdss`. |
| Smoke runner | Passed | `--skip-build` smoke passed and CLI checks passed. |
| Expanded regression suite | Passed | `PASS: 7 regression case(s)`. |
| Upgrade validation runner | Passed | Smoke, unit configure/build/CTest, and regression passed; benchmark mode also passed with `medium_michaelis_menten`. |

## Validation Runner Update

Added `tools/run_upgrade_validation.py` as a top-level orchestrator for the
upgrade workflow. It runs the existing smoke runner, CMake unit tests, CTest,
the regression harness, and optional benchmark cases, then writes a single
`validation_report.json` plus per-step stdout/stderr logs.

This runner is intended as the standard local command before behavior-sensitive
refactors and as the future CI entry point once workflow pushes are available.

## Workflow Follow-Up

The first unified validation run exposed nondeterministic `DATA/restart.dat`
output in `implicit_lipid_fresh_small`: the serialized `membrane` line could
write an uninitialized `Membrane::No_protein` value. The `Membrane` primitive
fields are now default-initialized so fresh fixed-seed runs produce stable
restart metadata.

The regression harness also now creates the parent directory passed through
`--tmp-root`, and the unified validation runner asks the regression harness to
write `regression_report.json` alongside the top-level
`validation_report.json`.

## Core Refactor Start

Started the core-computation refactor track with
`docs/upgrade/core_computation_refactor_plan.md` and the first
`nerdss::core::MathEngine` CPU scalar facade. This is a boundary-only slice:
existing `Coord`, `Vector`, and matrix algorithms remain the implementation of
record, while the new facade gives future probability, trajectory, and
GPU-oriented work a clear place to attach.

Validation for the first slice:

| Check | Result | Notes |
| --- | --- | --- |
| CTest unit suite | Passed | Added coverage for the `MathEngine` facade. |
| Unified validation runner | Passed | Smoke, unit configure/build/CTest, and 7-case regression passed. |
| Benchmark mode | Passed | `small_homotrimer` wall time `8.738s`, CPU time `8.559s`. |

## Diagnostics Boundary Start

Added the first low-cost diagnostics boundary in `include/core/diagnostics.hpp`.
It provides fixed-capacity trace stacks, scoped frames, and structured
diagnostics that map to the existing error category and exit-code model. The
header does not allocate or format strings unless a diagnostic is explicitly
formatted, and trace scopes remain compile-time gated for future hot-loop use.

## Probability Engine Start

Started moving pure probability kernels behind core service interfaces by adding
`include/core/probability_engine.hpp`. The 3D and 1D association probability
formulas now live behind `nerdss::core::ProbabilityEngine`, while the legacy
`passocF` and `passocF_1D` functions remain as forwarding wrappers so existing
reaction call sites and validation baselines stay unchanged.

Validation for this slice:

| Check | Result | Notes |
| --- | --- | --- |
| CTest unit suite | Passed | Added facade-vs-wrapper checks for 1D and 3D association probability. |
| Unified validation runner | Passed | Smoke, unit configure/build/CTest, and 7-case regression passed. |
| Benchmark mode | Passed | `small_homotrimer` wall time `8.655s`, CPU time `8.518s`. |
