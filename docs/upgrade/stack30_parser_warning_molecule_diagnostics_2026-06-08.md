# Stack 30 Parser Warning And Molecule Diagnostics - 2026-06-08

## Scope

This diagnostics slice normalizes a small set of parser warnings and hardens
edge cases in molecule parsing without changing accepted valid inputs.

- Replaces legacy ignored boundary keyword warnings with structured parser
  warning diagnostics in both main input and add-file parsing.
- Replaces the invalid molecule bond warning with a structured warning while
  preserving the existing behavior: clear parsed bonds and continue.
- Guards stripped empty lines in `.mol` parsing before indexing `line[0]`.
- Avoids dereferencing a failed molecule keyword lookup while detecting the
  coordinate block.
- Hardens BNGL molecule parsing for dangling state markers, malformed bond
  markers, unterminated bond tokens, full-token bond index parsing, and
  non-negative/full-token copy-number parsing.

## Local Validation

Commands run on `codex/parser-warning-molecule-diagnostics-stack30-slice`:

- `git diff --check HEAD`: passed.
- `tools/run_static_analysis.sh include/parser/parser_diagnostics.hpp src/parser/parse_input.cpp src/parser/parse_molFile.cpp src/parser/read_bonds.cpp src/parser/parse_molecule_bngl.cpp tests/unit/test_diagnostics.cpp`:
  passed with existing legacy parser readability/style warnings.
- `cmake -S . -B build-parser-warning-molecule-diagnostics`: passed.
- `cmake --build build-parser-warning-molecule-diagnostics --target nerdss_diagnostics_tests --parallel 4`:
  passed.
- `cmake --build build-parser-warning-molecule-diagnostics --target nerdss_unit_tests --parallel 4`:
  passed.
- `ctest --test-dir build-parser-warning-molecule-diagnostics --output-on-failure`:
  passed, 3/3 tests.
- `make serial -j4`: passed and rebuilt `bin/nerdss`.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --output-dir /tmp/nerdss-parser-warning-molecule-diagnostics-validation`:
  passed smoke, unit configure/build/ctest, and regression; benchmarks skipped.

## Follow-Up Notes

- Broader reaction-rate and simulation-time warnings remain outside this parser
  warning normalization slice.
- Future parser tests can add process-level negative-input fixtures for the
  newly diagnosed BNGL edge cases.
