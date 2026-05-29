# Upgrade Validation Runner

`tools/run_upgrade_validation.py` is the top-level local runner for upgrade
branches. It orchestrates the existing smoke, unit, regression, and optional
benchmark tools without replacing their detailed reports.

## Default Checks

By default, the runner performs:

- serial smoke test through `tools/run_smoke_tests.py`;
- CMake configure/build for `nerdss_unit_tests`;
- CTest execution for the unit suite;
- default regression suite through `tests/regression/run_regression.py`;
- benchmark step marked as skipped unless explicitly requested.

It writes command logs and a machine-readable `validation_report.json` under
the selected output directory.

## Usage

Run the standard validation stack and build through the smoke runner:

```sh
python3 tools/run_upgrade_validation.py --output-dir /tmp/nerdss-upgrade-validation
```

Reuse an existing serial executable:

```sh
python3 tools/run_upgrade_validation.py \
  --skip-build \
  --binary bin/nerdss \
  --output-dir /tmp/nerdss-upgrade-validation
```

Run one or more benchmark cases after correctness checks:

```sh
python3 tools/run_upgrade_validation.py \
  --skip-build \
  --binary bin/nerdss \
  --benchmarks \
  --benchmark-case small_homotrimer \
  --benchmark-case medium_michaelis_menten \
  --output-dir /tmp/nerdss-upgrade-validation
```

Stop after the first failed step:

```sh
python3 tools/run_upgrade_validation.py --fail-fast
```

## Report Layout

Each step writes separate stdout and stderr logs:

```text
smoke.stdout.txt
smoke.stderr.txt
unit_configure.stdout.txt
unit_configure.stderr.txt
unit_build.stdout.txt
unit_build.stderr.txt
unit_ctest.stdout.txt
unit_ctest.stderr.txt
regression.stdout.txt
regression.stderr.txt
regression_report.json
benchmarks.stdout.txt
benchmarks.stderr.txt
validation_report.json
```

The JSON report includes:

- git branch, commit, and dirty status;
- step command, working directory, runtime, exit code, skipped flag, and log
  paths;
- aggregate `passed` or `failed` status.

When regression validation runs, `regression_report.json` contains the detailed
case-level pass/fail report from `tests/regression/run_regression.py`.

Keep generated validation output directories out of commits. Attach them as CI
artifacts or local run artifacts when reviewing behavior-sensitive changes.
