# Structured Error Model

This document defines the first Phase 5 target for structured NERDSS errors.
It is a planning and compatibility layer only: existing simulation behavior,
stderr formatting, MPI handling, and process exits are not changed by this
slice.

## Goals

- Give future parser, setup, simulation, and I/O errors stable categories.
- Reserve process exit codes so command-line callers can distinguish failure
  classes without parsing human-readable messages.
- Preserve current behavior during migration. Existing `error(...)` calls and
  direct `exit(...)` calls remain valid until their owning code paths are
  intentionally migrated.
- Keep rank-local context available for MPI diagnostics without requiring this
  first slice to change MPI finalization or collective behavior.

## Non-goals

- No broad source rewiring in Phase 5 slice 1.
- No exception-policy change for core simulation loops.
- No change to existing output text, logs, restart files, random seeds, or
  trajectory behavior.
- No requirement that all legacy exit codes immediately match the new table.

## Error Categories

`include/error/error_codes.hpp` defines `nerdss::error::ErrorCategory` as the
common vocabulary for future structured diagnostics:

| Category | Intended use |
| --- | --- |
| `success` | Completed successfully. |
| `input` | Invalid command-line argument, input file syntax, or semantic input validation. |
| `file_io` | Missing, unreadable, unwritable, or malformed external file. |
| `reaction` | Invalid reaction definition or reaction consistency failure. |
| `state` | Invalid molecule, complex, interface, observable, or restart state. |
| `numeric` | Numerical integration, probability, geometry, or floating-point failure. |
| `dependency` | Required external library, runtime feature, or environment dependency is unavailable. |
| `mpi` | MPI rank, communication, decomposition, or parallel consistency failure. |
| `internal` | Failed internal invariant or implementation bug. |
| `unsupported` | Recognized feature or option that is not implemented in this build or mode. |
| `interrupted` | User or system interruption. |
| `unknown` | Fallback when no more specific category is known. |

## Exit Codes

The first structured exit-code set keeps values close to existing NERDSS
conventions where they are already present. Future migrations should prefer
these values over new ad hoc integer literals.

| Exit code | Name | Category | Notes |
| --- | --- | --- | --- |
| 0 | `success` | `success` | Normal completion. |
| 1 | `general` | `unknown` | Legacy fallback used by `error(...)` and many direct exits. |
| 2 | `input` | `input` | General input parse or validation failure. |
| 3 | `coordinate_input` | `file_io` | Coordinate-specific read failure, matching a legacy helper. |
| 4 | `invalid_container_value` | `state` | Required value missing from an internal container. |
| 5 | `unbalanced_reaction` | `reaction` | Reaction definition is not balanced. |
| 6 | `invalid_reaction` | `reaction` | Invalid reaction rule or parameterization. |
| 7 | `dependency` | `dependency` | Missing external dependency or unavailable runtime capability. |
| 8 | `numeric` | `numeric` | Numerical failure. |
| 9 | `file_io` | `file_io` | General file I/O failure. |
| 10 | `unsupported` | `unsupported` | Unsupported but recognized feature or mode. |
| 11 | `internal` | `internal` | Internal bug or invariant failure. |
| 12 | `invariant` | `internal` | Legacy involvement/invariant error. |
| 13 | `mpi` | `mpi` | MPI or parallel consistency failure. |
| 120 | `parser_state` | `input` | Preserves the current state-parser special exit. |
| 125 | `unknown` | `unknown` | Structured fallback when a legacy code is not appropriate. |
| 130 | `interrupted` | `interrupted` | Conventional Ctrl-C/SIGINT-style termination. |

## Migration Rules

1. Add structured metadata at module boundaries first, such as parser entry
   points, file readers, setup validation, and top-level executable handling.
2. Do not change stochastic, geometry, reaction, or scheduler behavior while
   converting error reporting.
3. Keep legacy messages stable unless a migration explicitly updates tests and
   caller documentation.
4. When replacing a direct `exit(...)`, choose the closest `ExitCode` value and
   document any intentional compatibility change in the owning pull request.
5. MPI call sites should retain rank information. Any future collective abort
   policy should be introduced separately from category assignment.

## Current Implementation Surface

`include/error/error_codes.hpp` is header-only and has no side effects. It
provides:

- `ErrorCategory`
- `ExitCode`
- `to_exit_status(ExitCode)`
- `to_string(ErrorCategory)`
- `to_string(ExitCode)`
- `default_exit_code(ErrorCategory)`

The error-code header is still mostly a compatibility vocabulary. The
diagnostics boundary below is the first production use of those categories in
human-facing error text.

## Diagnostics Formatting Boundary

`include/core/diagnostics.hpp` now also provides `FormatDiagnostic(...)`, which
formats a structured diagnostic as:

```text
ERROR [category] (exit_code=name(status)): message
Trace:
#0 function (file:line): detail
```

Trace output is omitted when the diagnostic has no trace string. The formatter
does not run in hot loops; it is intended for parser, setup, file I/O, and
other boundary failures after an error has already been detected.
`WriteDiagnostic(...)` emits the same stable rendering to a caller-provided
stream, and `ExitWithDiagnostic(...)` writes the diagnostic before exiting with
the structured status.

The first production call site is command-line argument validation in
`src/parser/parse_command.cpp`. Usage errors now retain the existing input exit
status (`2`) and usage text while adding a single trace frame for the
command-line parser boundary.

## Parser File-Open Diagnostics

The parser file-open boundary now uses `nerdss::parser::MakeFileOpenDiagnostic`
and `ExitWithFileOpenDiagnostic` for the main input file, add input file, and
molecule config file open failures. These failures report category `file_io`,
render through the shared diagnostic formatter, include the failed path and
file role, and exit with `ExitCode::file_io`.

## Parser Invalid-Keyword Diagnostics

The molecule config parser now uses
`nerdss::parser::MakeInvalidKeywordDiagnostic` and
`ExitWithInvalidKeywordDiagnostic` for unknown top-level `.mol` keywords. This
replaces the previous stdout plus `exit(1)` path with a structured `input`
diagnostic, includes the invalid keyword and molecule config path, and exits
with `ExitCode::input`.

## Parser Section-Order Diagnostics

The main input parser now uses
`nerdss::parser::MakeSectionOrderDiagnostic` and
`ExitWithSectionOrderDiagnostic` when a `startReactions` block appears before
the required `startMolecules` block. This migrates a semantic parser failure
from direct `std::cerr` plus `exit(1)` to a structured `input` diagnostic,
includes the input path and section names, and exits with `ExitCode::input`.

## Parser Boolean Diagnostics

The shared parser boolean reader now uses
`nerdss::parser::MakeInvalidBooleanDiagnostic` and
`ExitWithInvalidBooleanDiagnostic` when a normalized value is not one of `0`,
`1`, `false`, or `true`. This migrates the previous direct `std::cerr` plus
`exit(1)` path to a structured `input` diagnostic and exits with
`ExitCode::input`.

## Remaining Traceback Gaps

- Diagnostics unit coverage is now part of the normal CMake/CTest path through
  the `nerdss_diagnostics_tests` target and CTest entry. The older standalone
  `g++ -std=c++11 -Iinclude tests/unit/test_diagnostics.cpp ...` check is no
  longer required for routine validation.
- Parser failures that still call `exit(...)`, `error(...)`, throw ad hoc
  exceptions, or write direct `std::cerr` messages include remaining paths in
  `src/parser/parse_input.cpp` outside the migrated file-open, molecule
  keyword, and section-order checks, `src/parser/parse_reaction.cpp`,
  `src/parser/parse_states.cpp`,
  `src/parser/parse_observable.cpp`, `src/parser/parse_molecule_bngl.cpp`,
  `src/parser/check_for_valid_states.cpp`, `src/parser/parse_input_array.cpp`,
  `src/parser/parse_input_for_a_new_simulation.cpp`,
  `src/parser/parse_input_for_a_restart_simulation.cpp`, and
  `src/parser/parse_input_for_add_file.cpp`. Other `parse_molFile` semantic
  validation failures, such as malformed coordinate, state, bond, or numeric
  values, are not yet fully migrated.
- Setup failures and warnings with direct text/exit behavior remain in
  `src/system_setup/initialize_states.cpp`,
  `src/system_setup/generate_coordinates.cpp`, and
  `src/system_setup/determine_shape_molecule.cpp`.
- File I/O failures with direct text/exit behavior remain in restart and
  optional artifact writers, notably `src/io/read_restart.cpp`,
  `src/io/write_pdb.cpp`, and `src/io/write_bonded_complex_json.cpp`.
- MPI-aware errors still rely on legacy rank text and have not been migrated to
  structured diagnostics.
- Core simulation loops should remain compile-time gated through
  `NERDSS_TRACE_SCOPE`; this slice does not enable hot-loop trace collection.
