# CI Coverage Workflow Push Status

Status recorded on 2026-05-31.

The CI coverage workflow branch `codex/ci-coverage-actions-unblock` points at
commit `922afa6ca2a5bfcdae1cf347721d838e1ecf25ab` (`Add upgrade validation CI
and coverage reporting`). That commit adds:

- `.github/workflows/upgrade-validation.yml`
- `docs/upgrade/ci_coverage.md`
- updates to `docs/upgrade/validation_runner.md`
- `tools/collect_gcov_coverage.py`

Local validation on macOS arm64 passed:

- `python3 -m py_compile tools/collect_gcov_coverage.py tools/run_upgrade_validation.py tools/run_smoke_tests.py tests/regression/run_regression.py`
- Ruby YAML parsing for `.github/workflows/upgrade-validation.yml`
- `git diff --check`
- coverage configure/build/CTest for `nerdss_unit_tests`
- `tools/collect_gcov_coverage.py`, reporting 46.77% project line coverage
  (`918/1963` lines under `src/` and `include/`)

Pushing `codex/ci-coverage-actions-unblock` to remote `personal` was blocked by
GitHub because the available OAuth token does not have `workflow` scope:

```text
refusing to allow an OAuth App to create or update workflow
`.github/workflows/upgrade-validation.yml` without `workflow` scope
```

Required user intervention: push the workflow branch with credentials that have
GitHub `workflow` scope, or grant that scope to the token used by this
environment and retry:

```sh
git push personal codex/ci-coverage-actions-unblock
```
