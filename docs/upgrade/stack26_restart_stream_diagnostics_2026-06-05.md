# Stack 26 Restart Stream Diagnostics - 2026-06-05

## Scope

This diagnostics slice keeps restart behavior unchanged while aligning the
factored restart parser helper with the structured error model.

- Migrates missing restart input files in
  `parse_input_for_a_restart_simulation` from legacy `error(...)` handling to
  `nerdss::parser::ExitWithFileOpenDiagnostic`.
- Checks serial restart artifact output streams before the immediate restart
  rewrite, scheduled restart write, checkpoint restart write, and final restart
  write.
- Preserves the existing nonfatal trajectory mismatch and missing-trajectory
  warnings because those paths intentionally continue execution.
- Leaves MPI-specific restart output-open handling for a follow-up slice because
  local MPI rebuild remains blocked by the `mpicxx` wrapper configuration.

## Local Validation

Commands run on `codex/restart-stream-diagnostics-stack26-slice`:

- `git diff --check HEAD`: passed.
- `tools/run_static_analysis.sh EXEs/nerdss.cpp src/parser/parse_input_for_a_restart_simulation.cpp`:
  exited 0 with existing legacy executable/parser warnings.
- `cmake -S . -B build-restart-stream-diagnostics`: passed.
- `cmake --build build-restart-stream-diagnostics --target nerdss_unit_tests --parallel 4`:
  passed.
- `ctest --test-dir build-restart-stream-diagnostics --output-on-failure`:
  passed, 3/3 tests passed.
- `make serial -j4`: passed and rebuilt `bin/nerdss`.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --output-dir /tmp/nerdss-restart-stream-diagnostics-validation`:
  passed. Benchmarks were skipped for this narrow diagnostics slice.

## Follow-Up Notes

- Add explicit regression coverage for restart parser helper failures once a
  harness target can call the factored parser entry point with small fixtures.
- Continue MPI-aware output-open diagnostics in `EXEs/nerdss_mpi.cpp` and
  `src/io_mpi/write_output.cpp` after the MPI wrapper is repaired.
