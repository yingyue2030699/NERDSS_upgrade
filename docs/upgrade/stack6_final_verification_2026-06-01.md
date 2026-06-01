# Stack 6 Final Verification - 2026-06-01

This report records the final local verification for
`codex/validation-integration-stack-6` at commit
`d17da342a3afd33dcc9e20e166e97650f53ff338`.

## Integrated Branches

- `codex/stack5-sanitizer-static-mpi-report`
  (`fa8d5dd1e7bf1a616d8c585dad60e52f60d4b5e7`)
- `codex/diagnostics-next-parser-slice`
  (`83834b60632292bf32725106ae84d0062973d140`)
- `codex/mathengine-probability-service-slice`
  (`15eea69f0f65e774404629c23ea707879eda597c`)
- `codex/standard-format-runtime-closure`
  (`2e2ac905712be2c34cd064e2423fe6078a318c6f`)

The integration merge completed without conflicts and was pushed to
`personal/codex/validation-integration-stack-6`.

## Serial Validation

Command:

```sh
python3 -B tools/run_upgrade_validation.py \
  --skip-build \
  --binary bin/nerdss \
  --output-dir /tmp/nerdss-stack6-full-benchmark \
  --benchmarks
```

Result: passed.

| Step | Result |
| --- | --- |
| Smoke | passed |
| Unit configure | passed |
| Unit build | passed |
| Unit CTest | passed |
| Regression | passed |
| Benchmarks | passed |

## Benchmark Results

| Case | Wall time (s) | CPU time (s) | Output files | Output bytes |
| --- | ---: | ---: | ---: | ---: |
| `small_homotrimer` | 7.861 | 7.766 | 24 | 8,828,274 |
| `medium_michaelis_menten` | 0.831 | 0.804 | 15 | 164,072 |
| `representative_implicit_lipid` | 1.865 | 1.855 | 24 | 1,510,622 |
| `large_clathrin_short` | 0.418 | 0.242 | 15 | 442,369 |

The benchmark harness reports serial runtime and output sizes. Scientific
output equivalence continues to be covered by the regression suite rather than
by the benchmark timing harness.

## Additional Quality Gates

The stack 5 quality-gate report was merged into stack 6. The checks were also
reconfirmed locally against stack 6 where feasible:

- Static analysis command: `tools/run_static_analysis.sh`
  - Result: blocked.
  - Reason: `clang-tidy` was not found on `PATH`.
- MPI build command: `make mpi`
  - Result: failed before compiling NERDSS MPI objects.
  - Reason: `/opt/anaconda3/bin/mpicxx` delegates to missing
    `x86_64-apple-darwin13.4.0-clang++`.

These are environment/toolchain blockers rather than known source regressions.

## CI Workflow Publication

The workflow branch remains blocked by GitHub credentials. GitHub rejects
updates to `.github/workflows/upgrade-validation.yml` from the available OAuth
token because it lacks `workflow` scope. The status branch documenting this
blocker is merged into stack 5 and therefore present in stack 6.

## Remaining Work

- Push the CI workflow branch with credentials that include GitHub `workflow`
  scope, then run GitHub Actions.
- Install or expose `clang-tidy`, then rerun static analysis.
- Repair or replace the local MPI compiler wrapper, then rerun `make mpi` and a
  NERDSS MPI smoke test.
- Continue incremental parser/setup/I/O diagnostic migrations.
- Continue MathEngine and ProbabilityEngine extraction for larger coupled
  kernels.
