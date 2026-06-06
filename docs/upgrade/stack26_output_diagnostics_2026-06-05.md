# Stack 26 Output Diagnostics - 2026-06-05

## Scope

This diagnostics slice adds structured file-open checks for new-simulation
setup output streams on top of `codex/validation-integration-stack-24`.

- Checks the observables output stream before writing the initial observables
  header.
- Checks the trajectory output stream before writing the initial trajectory.
- Checks the transition output stream before writing the initial transition
  matrix.
- Uses `nerdss::io::MakeArtifactWriteOpenDiagnostic` and
  `nerdss::core::ExitWithDiagnostic` so failures report `file_io` and exit with
  `ExitCode::file_io`.

## Local Validation

Commands run on `codex/parser-output-open-diagnostics-stack26-slice`:

- `git diff --check HEAD`: passed.
- `tools/run_static_analysis.sh src/parser/parse_input_for_a_new_simulation.cpp`:
  exited 0 with existing legacy parser-entry warnings.
- `cmake -S . -B build-parser-output-open-diagnostics`: passed.
- `cmake --build build-parser-output-open-diagnostics --target nerdss_unit_tests --parallel 4`:
  passed.
- `ctest --test-dir build-parser-output-open-diagnostics --output-on-failure`:
  passed, 3/3 tests passed.
- `make serial -j4`: passed and rebuilt `bin/nerdss`.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --output-dir /tmp/nerdss-parser-output-open-diagnostics-validation`:
  passed. Benchmarks were skipped for this narrow diagnostics slice.

## Follow-Up Notes

- Continue output-open diagnostics in MPI entry points and selected core I/O
  writers.
- Keep warning-and-continue restart trajectory handling separate because the
  current structured diagnostics are error-level.
