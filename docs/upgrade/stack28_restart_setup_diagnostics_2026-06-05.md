# Stack 28 Restart Setup Diagnostics - 2026-06-05

## Scope

This diagnostics slice improves restart setup boundary reporting without
changing restart simulation semantics.

- Converts missing restart-file setup failure from the legacy `error(...)`
  banner to a structured file-I/O diagnostic that includes the MPI rank.
- Adds structured warning messages for missing restart trajectory files,
  trajectory/restart iteration mismatches, and malformed trajectory iteration
  headers.
- Keeps trajectory checks permissive: missing, mismatched, or malformed
  trajectory metadata still warns and allows the restart file to proceed.

## Local Validation

Commands run on `codex/restart-setup-diagnostics-stack28-slice`:

- `git diff --check HEAD`: passed.
- `tools/run_static_analysis.sh include/parser/parser_diagnostics.hpp src/parser/parse_input_for_a_restart_simulation.cpp tests/unit/test_diagnostics.cpp`:
  exited 0 with existing legacy restart setup warnings.
- `cmake -S . -B build-restart-setup-diagnostics`: passed.
- `cmake --build build-restart-setup-diagnostics --target nerdss_unit_tests --parallel 4`:
  passed.
- `ctest --test-dir build-restart-setup-diagnostics --output-on-failure`:
  passed, 3/3 tests.
- `make serial -j4`: passed and rebuilt `bin/nerdss`.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --output-dir /tmp/nerdss-restart-setup-diagnostics-validation`:
- passed smoke, unit configure/build/ctest, and regression; benchmarks were
  skipped by the validation runner.
- `./bin/nerdss -r /tmp/definitely_missing_restart.dat`: exited 9 with the
  existing serial structured file-I/O diagnostic for the missing restart file.

## Follow-Up Notes

- MPI source rebuild remains dependent on the local `mpicxx` wrapper being
  repaired.
- Additional parser diagnostics remain in molecule `.mol` parsing, BNGL
  molecule expression setup, and broader parser warning normalization.
