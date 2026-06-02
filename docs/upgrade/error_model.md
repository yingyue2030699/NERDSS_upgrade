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

The setup coordinate-file loader now uses the same file-open diagnostic for
missing `--coordinate` / `-c` files. This replaces the previous plain stderr
message and silent continuation with a structured `file_io` diagnostic and
`ExitCode::file_io`.

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

## Parser Numeric Array Diagnostics

The shared numeric array reader now uses
`nerdss::parser::MakeInvalidNumericArrayTokenDiagnostic` and
`ExitWithInvalidNumericArrayTokenDiagnostic` when a value cannot be parsed as a
number and is not one of the legacy aliases `pi`, `m_pi`, or `nan`. This
replaces the previous ad hoc thrown string path with a structured `input`
diagnostic while preserving the accepted token set and `ExitCode::input`.

## Parser Molecule Copy-Number Diagnostics

The main input and add-file `startMolecules` sections now use
`nerdss::parser::MakeInvalidMoleculeCountDiagnostic` and
`ExitWithInvalidMoleculeCountDiagnostic` for malformed molecule copy-number
syntax. Lines without the required `:` separator now produce a structured
`input` diagnostic instead of printing help text and exiting successfully.
Invalid characters and malformed state count tokens in `parse_number_bngl`
also report the same stable diagnostic while preserving accepted legacy count
syntax.

## Setup State Diagnostics

The state setup path now uses
`nerdss::setup::MakeInvalidStateCharacterDiagnostic` and
`ExitWithInvalidStateCharacterDiagnostic` when a starting copy-number state
expression contains a character outside the accepted alphanumeric token,
`~`, and `,` grammar. This migrates the previous direct `std::cerr` plus
`exit(1)` path in `initialize_states` to a structured `input` diagnostic,
includes the offending character, state expression, and molecule name, and
exits with `ExitCode::input`.

Implicit molecule state setup also uses
`nerdss::setup::MakeImplicitMoleculeInterfaceCountDiagnostic` and
`ExitWithImplicitMoleculeInterfaceCountDiagnostic` when an implicit molecule
starting state lists more than one interface. This migrates the previous
direct `std::cerr` plus `exit(1)` path in `initialize_states` to the shared
structured formatter, includes the molecule name and state expression, and
exits with `ExitCode::input`.

## Parser Reaction Semantic Diagnostics

The reaction parser now uses
`nerdss::parser::MakeTooManyReactionReactantsDiagnostic` and
`ExitWithTooManyReactionReactantsDiagnostic` when a reaction has more than two
reacting molecules. This preserves the existing semantic validation boundary
while moving the previous direct `std::cerr` plus `exit(1)` path to the shared
structured `input` diagnostic formatter and `ExitCode::input`.

Additional reaction parse boundary failures now use structured diagnostics for
missing reaction arrows, null-to-null reactions whose type cannot be inferred,
unknown reaction parameter keywords, and incomplete assembled reactions. The
legacy parser behavior is preserved for accepted reactions, while rejected
reactions now report stable `input` diagnostics through the shared formatter
instead of direct `std::cerr`/`exit(1)` calls.

## Parser Observable Diagnostics

The observable parser now uses
`nerdss::parser::MakeUnknownObservableTypeDiagnostic` and
`ExitWithUnknownObservableTypeDiagnostic` when an observable type is not one of
`molecule` or `complex`. This migrates the previous direct `std::cerr` plus
`exit(1)` path in `parse_observable` to a structured `input` diagnostic and
`ExitCode::input`.

## Parser State Declaration Diagnostics

Molecule state declarations now use
`nerdss::parser::MakeUnknownStateInterfaceDiagnostic` and
`ExitWithUnknownStateInterfaceDiagnostic` when a state line references an
interface that is not declared on the molecule template. This migrates the
previous direct `std::cout` plus `exit(1)` path in `parse_states` to a
structured `input` diagnostic and `ExitCode::input` while preserving accepted
state declarations and state-token normalization.

## Parser Reaction State Validation Diagnostics

Reaction molecule state validation now uses structured parser diagnostics for
hard semantic failures in `check_for_valid_states`: unknown molecule templates,
unknown interfaces on a declared molecule template, and unknown states on a
declared template interface. This migrates the previous direct `std::cerr` or
`std::cout` plus `exit(...)` paths to shared `input` diagnostics and
`ExitCode::input` while preserving accepted reaction molecules, interfaces,
states, and legacy informational parsing output.

## Setup Geometry Diagnostics

Top-level serial setup validation now uses structured setup diagnostics for
implicit lipid molecule ordering, sphere/compartment incompatibility, and
compartment water-box clearance failures. The two duplicated setup paths in
`EXEs/nerdss.cpp` share the same diagnostic helpers. Water-box clearance still
reports every failing dimension before exiting, but now each message includes
the category, stable input exit code, dimension, compartment radius, and
`rMaxLimit`.

## Optional I/O Artifact Diagnostics

The serial PDB and bonded-complex JSON artifact writers now use
`nerdss::io::MakeArtifactWriteOpenDiagnostic` and
`WriteArtifactWriteOpenDiagnostic` when their output file cannot be opened.
These failures report category `file_io` and render through the shared
diagnostic formatter while preserving the legacy human-readable message text
inside the diagnostic payload. The writers still return to the caller instead
of exiting, matching the previous optional-artifact behavior.

## Remaining Traceback Gaps

- Diagnostics unit coverage is now part of the normal CMake/CTest path through
  the `nerdss_diagnostics_tests` and `nerdss_io_diagnostics_tests` targets and
  CTest entries. The older standalone
  `g++ -std=c++11 -Iinclude tests/unit/test_diagnostics.cpp ...` check is no
  longer required for routine validation.
- Parser failures that still call `exit(...)`, `error(...)`, throw ad hoc
  exceptions, or write direct `std::cerr` messages include remaining paths in
  `src/parser/parse_input.cpp` outside the migrated file-open, molecule
  keyword, section-order, and molecule copy-number checks,
  `src/parser/parse_molecule_bngl.cpp` outside the migrated starting molecule
  copy-number parser,
  `src/parser/parse_input_for_a_new_simulation.cpp`,
  `src/parser/parse_input_for_a_restart_simulation.cpp`, and
  `src/parser/parse_input_for_add_file.cpp`. Other `parse_molFile` semantic
  validation failures, such as malformed coordinate, state, bond, or numeric
  values, are not yet fully migrated.
- Setup failures and warnings with direct text/exit behavior remain in
  `EXEs/nerdss.cpp` outside the migrated setup geometry paths,
  `src/system_setup/initialize_states.cpp` outside the migrated invalid
  starting state character check,
  `src/system_setup/generate_coordinates.cpp` outside the migrated coordinate
  file-open check, and
  `src/system_setup/determine_shape_molecule.cpp`.
- File I/O failures with direct text/exit behavior remain in restart and other
  artifact paths, notably `src/io/read_restart.cpp` and MPI output writers.
- MPI-aware errors still rely on legacy rank text and have not been migrated to
  structured diagnostics.
- Core simulation loops should remain compile-time gated through
  `NERDSS_TRACE_SCOPE`; this slice does not enable hot-loop trace collection.
