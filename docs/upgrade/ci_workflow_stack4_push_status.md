# CI Workflow Stack 4 Push Status

Status recorded on 2026-06-01.

The stack-4 CI workflow branch `codex/ci-workflow-stack4-publish` points at
commit `abb4965d85e847ba8299faa03267d4850f67fb6f` (`Add upgrade validation CI
and coverage reporting`). It is based on
`personal/codex/validation-integration-stack-4` commit
`40995f47a6830437e6b5f535bd1f2e31908a8140`.

That workflow commit changes only the CI/coverage surface:

- `.github/workflows/upgrade-validation.yml`
- `docs/upgrade/ci_coverage.md`
- `docs/upgrade/validation_runner.md`
- `tools/collect_gcov_coverage.py`

Local validation on macOS passed:

- Ruby YAML parsing for `.github/workflows/upgrade-validation.yml`
- `python3 -B -m py_compile tools/collect_gcov_coverage.py`
- `python3 -B tools/collect_gcov_coverage.py --help`
- synthetic gcov collector smoke test, reporting 50.00% coverage
  (`2/4` lines) for `src/sample.cpp`

The first YAML parse attempt with Python was not usable because PyYAML is not
installed in this environment:

```text
ModuleNotFoundError: No module named 'yaml'
```

Pushing `codex/ci-workflow-stack4-publish` to remote `personal` was blocked by
GitHub because the available OAuth token does not have `workflow` scope:

```text
 ! [remote rejected] codex/ci-workflow-stack4-publish -> codex/ci-workflow-stack4-publish (refusing to allow an OAuth App to create or update workflow `.github/workflows/upgrade-validation.yml` without `workflow` scope)
error: failed to push some refs to 'https://github.com/yingyue2030699/NERDSS_upgrade.git'
```

Required user action: push the workflow branch with credentials that have
GitHub `workflow` scope, or grant `workflow` scope to the token used by this
environment and retry:

```sh
git push personal codex/ci-workflow-stack4-publish
```
