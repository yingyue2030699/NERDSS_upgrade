# Stack 29 Serial Restart Diagnostics - 2026-06-08

## Scope

This diagnostics slice extends restart trajectory diagnostics to the serial
executable's separate restart setup block while preserving its existing
control-flow decisions.

- Keeps missing restart trajectory permissive, but emits a structured warning.
- Keeps restart/trajectory iteration mismatch fatal, but emits a structured
  input diagnostic instead of a plain `ERROR:` string.
- Converts malformed serial restart trajectory iteration headers from an
  uncaught conversion path into a structured fatal input diagnostic.
- Keeps MPI-aware rank text conditional so serial diagnostics do not print the
  legacy `rank=-1` sentinel.

## Local Validation

Commands run on `codex/serial-restart-diagnostics-stack29-slice`:

- `git diff --check HEAD`: passed.
- `tools/run_static_analysis.sh include/parser/parser_diagnostics.hpp EXEs/nerdss.cpp tests/unit/test_diagnostics.cpp`:
  exited 0 with existing legacy warnings in `EXEs/nerdss.cpp`.
- `cmake -S . -B build-serial-restart-diagnostics`: passed.
- `cmake --build build-serial-restart-diagnostics --target nerdss_unit_tests --parallel 4`:
  passed.
- `ctest --test-dir build-serial-restart-diagnostics --output-on-failure`:
  passed, 3/3 tests.
- `make serial -j4`: passed and rebuilt `bin/nerdss`.
- `make mpi`: failed before compiling NERDSS code because `/opt/anaconda3/bin/mpicxx`
  still points to missing `x86_64-apple-darwin13.4.0-clang++`.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --output-dir /tmp/nerdss-serial-restart-diagnostics-validation`:
  passed smoke, unit configure/build/ctest, and regression; benchmarks were
  skipped by the validation runner.
- `./bin/nerdss -r /tmp/definitely_missing_restart.dat`: exited 9 with the
  existing serial structured file-I/O diagnostic and no serial rank sentinel.

## Follow-Up Notes

- The MPI restart setup parser already received rank-aware restart diagnostics
  in stack 28.
- Additional parser warning normalization remains outside restart setup, most
  notably molecule parser warnings and remaining legacy parser warning strings.
