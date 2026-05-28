# Benchmark Harness Baseline

This document records the Phase 6 benchmark harness for the NERDSS upgrade.
It is a harness baseline, not a performance baseline: current `master` may not
build in every agent worktree until the build-fix branch is merged.

## Scope

- Runner: `benchmarks/run_benchmarks.py`
- Manifest: `benchmarks/benchmark_manifest.json`
- Generated results: `benchmark_results/<timestamp>/benchmark_results.json`
  and `benchmark_results/<timestamp>/benchmark_results.csv`

The benchmark runner is independent from the regression harness. It does not
compare scientific outputs or enforce tolerances. Its job is to collect timing,
resource, output-volume, git, and system metadata for later performance work.

## Benchmark Cases

The manifest currently defines four serial cases:

| Case | Size | Source input | Purpose |
| --- | --- | --- | --- |
| `small_homotrimer` | small | `sample_inputs/VALIDATE_SUITE/homoTrimer/parmTri6.inp` | Small 3D association workload. |
| `medium_michaelis_menten` | medium | `sample_inputs/VALIDATE_SUITE/michaelis_menten/michaelis.inp` | Moderate reaction-network workload. |
| `representative_implicit_lipid` | representative | `sample_inputs/VALIDATE_SUITE/implicit_lipid/parms.inp` | Implicit-lipid binding workload. |
| `large_clathrin_short` | large | `sample_inputs/VALIDATE_SUITE/clathrin/parms_clath_kon1uM.inp` | Larger model run at short benchmark iteration count. |

Each case copies its source directory into an isolated benchmark work directory
and applies benchmark-only `start parameters` overrides there. The original
sample inputs are not modified.

## Usage

List benchmark cases:

```sh
python3 benchmarks/run_benchmarks.py --list
```

Run all cases with the repository serial executable:

```sh
python3 benchmarks/run_benchmarks.py --nerdss ./bin/nerdss
```

Build first, then run all cases:

```sh
python3 benchmarks/run_benchmarks.py --build
```

Run a single case:

```sh
python3 benchmarks/run_benchmarks.py --nerdss ./bin/nerdss --case small_homotrimer
```

If current `master` cannot build locally, use a known-good executable from
Agent B's build branch once that branch is merged or checked out separately:

```sh
python3 benchmarks/run_benchmarks.py --nerdss /path/to/nerdss
```

## Collected Fields

The JSON result includes:

- Git branch, commit, and dirty status.
- Platform, hostname, CPU count, Python version, and total memory when the OS
  exposes it.
- Optional build command, build exit status, and build logs.
- Per-case command, seed, timeout, wall time, CPU time when available, peak RSS
  when available, RSS source, exit status, timeout status, stdout/stderr log
  paths and byte counts, and produced output file count/bytes.

The CSV result flattens the most useful per-case fields for spreadsheets and
quick trend comparisons.

## Baseline Notes

The first numeric local baseline was collected on 2026-05-28 from
`codex/validation-integration` after the serial build fix, smoke runner,
regression harnesses, crash hardening, benchmark harness, and profile command
guide were merged locally.

| Case | Seed | Wall time (s) | CPU time (s) | Output files | Output bytes |
| --- | ---: | ---: | ---: | ---: | ---: |
| `small_homotrimer` | 12345 | 8.668 | 8.560 | 24 | 8,828,354 |
| `medium_michaelis_menten` | 12345 | 1.036 | 0.871 | 15 | 164,072 |
| `representative_implicit_lipid` | 12345 | 2.068 | 1.979 | 24 | 1,510,702 |
| `large_clathrin_short` | 12345 | 0.419 | 0.256 | 15 | 442,377 |

Raw artifacts were kept outside the repository under `/tmp`:

- `/tmp/nerdss-benchmark-baseline/`
- `/tmp/nerdss-benchmark-representative/`
- `/tmp/nerdss-benchmark-large/`

The benchmark result metadata marked the repository dirty only because of
generated local build directories. Avoid committing large raw output
directories; keep detailed benchmark JSON/CSV as CI artifacts or external
run artifacts.
