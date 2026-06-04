#include "core/diagnostics.hpp"
#include "error/error_diagnostics.hpp"
#include "parser/parser_diagnostics.hpp"
#include "system_setup/setup_diagnostics.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void require_contains(const std::string& text, const std::string& needle,
                      const std::string& label) {
  if (text.find(needle) == std::string::npos) {
    std::cerr << label << ": expected to find '" << needle << "' in '" << text
              << "'\n";
    std::exit(1);
  }
}

void require_true(bool condition, const std::string& label) {
  if (!condition) {
    std::cerr << label << '\n';
    std::exit(1);
  }
}

void test_diagnostic_format_with_trace() {
  nerdss::core::TraceStack<4> stack;
  stack.Push({"parser_entry", "src/parser/example.cpp", 42,
              "reading command-line input"});

  const nerdss::core::Diagnostic diagnostic = nerdss::core::MakeDiagnostic(
      nerdss::error::ErrorCategory::input, "missing -f", stack.Format());
  const std::string formatted = nerdss::core::FormatDiagnostic(diagnostic);

  require_contains(formatted, "ERROR [input]",
                   "formatted diagnostic includes category");
  require_contains(formatted, "exit_code=input(2)",
                   "formatted diagnostic includes exit code");
  require_contains(formatted, "missing -f",
                   "formatted diagnostic includes message");
  require_contains(formatted, "Trace:",
                   "formatted diagnostic includes trace heading");
  require_contains(formatted, "parser_entry",
                   "formatted diagnostic includes trace function");
  require_contains(formatted, "src/parser/example.cpp:42",
                   "formatted diagnostic includes trace location");
  require_contains(formatted, "reading command-line input",
                   "formatted diagnostic includes trace detail");
}

void test_diagnostic_format_without_trace() {
  const nerdss::core::Diagnostic diagnostic = nerdss::core::MakeDiagnostic(
      nerdss::error::ErrorCategory::file_io, "cannot open input", "");
  const std::string formatted = nerdss::core::FormatDiagnostic(diagnostic);

  require_contains(formatted, "ERROR [file_io]",
                   "formatted file I/O diagnostic includes category");
  require_contains(formatted, "exit_code=file_io(9)",
                   "formatted file I/O diagnostic includes exit code");
  require_contains(formatted, "cannot open input",
                   "formatted file I/O diagnostic includes message");
  require_true(formatted.find("Trace:") == std::string::npos,
               "diagnostic without trace should omit trace heading");
}

void test_parser_invalid_keyword_diagnostic() {
  const nerdss::core::Diagnostic diagnostic =
      nerdss::parser::MakeInvalidKeywordDiagnostic("diffusionx",
                                                   "molecule config", "A.mol");
  const std::string formatted = nerdss::core::FormatDiagnostic(diagnostic);

  require_true(diagnostic.category == nerdss::error::ErrorCategory::input,
               "invalid parser keyword should use input category");
  require_true(diagnostic.exit_code == nerdss::error::ExitCode::input,
               "invalid parser keyword should use input exit code");
  require_contains(diagnostic.message,
                   "invalid molecule config keyword 'diffusionx' in 'A.mol'",
                   "invalid parser keyword message includes context and path");
  require_contains(formatted, "ERROR [input]",
                   "invalid parser keyword rendering includes category");
  require_contains(formatted, "exit_code=input(2)",
                   "invalid parser keyword rendering includes exit code");
}

void test_parser_section_order_diagnostic() {
  const nerdss::core::Diagnostic diagnostic =
      nerdss::parser::MakeSectionOrderDiagnostic("bad_order.inp",
                                                 "startReactions",
                                                 "startMolecules");
  const std::string formatted = nerdss::core::FormatDiagnostic(diagnostic);

  require_true(diagnostic.category == nerdss::error::ErrorCategory::input,
               "parser section order should use input category");
  require_true(diagnostic.exit_code == nerdss::error::ExitCode::input,
               "parser section order should use input exit code");
  require_contains(
      diagnostic.message,
      "invalid input section order in 'bad_order.inp': 'startReactions' "
      "requires 'startMolecules' first",
      "parser section order message includes path and section names");
  require_contains(formatted, "ERROR [input]",
                   "parser section order rendering includes category");
  require_contains(formatted, "exit_code=input(2)",
                   "parser section order rendering includes exit code");
}

void test_parser_invalid_boolean_diagnostic() {
  const nerdss::core::Diagnostic diagnostic =
      nerdss::parser::MakeInvalidBooleanDiagnostic("maybe");
  const std::string formatted = nerdss::core::FormatDiagnostic(diagnostic);

  require_true(diagnostic.category == nerdss::error::ErrorCategory::input,
               "invalid parser boolean should use input category");
  require_true(diagnostic.exit_code == nerdss::error::ExitCode::input,
               "invalid parser boolean should use input exit code");
  require_contains(diagnostic.message,
                   "cannot read boolean value 'maybe': expected one of 0, 1, "
                   "false, or true",
                   "invalid parser boolean message includes accepted values");
  require_contains(formatted, "ERROR [input]",
                   "invalid parser boolean rendering includes category");
  require_contains(formatted, "exit_code=input(2)",
                   "invalid parser boolean rendering includes exit code");
}

void test_parser_invalid_numeric_array_token_diagnostic() {
  const nerdss::core::Diagnostic diagnostic =
      nerdss::parser::MakeInvalidNumericArrayTokenDiagnostic("abc",
                                                             "[1, abc, 3]");
  const std::string formatted = nerdss::core::FormatDiagnostic(diagnostic);

  require_true(diagnostic.category == nerdss::error::ErrorCategory::input,
               "invalid numeric array token should use input category");
  require_true(diagnostic.exit_code == nerdss::error::ExitCode::input,
               "invalid numeric array token should use input exit code");
  require_contains(
      diagnostic.message,
      "cannot read numeric array token 'abc' in '[1, abc, 3]': expected a "
      "number, pi, m_pi, or nan",
      "invalid numeric array token message includes token and accepted values");
  require_contains(formatted, "ERROR [input]",
                   "invalid numeric array token rendering includes category");
  require_contains(formatted, "exit_code=input(2)",
                   "invalid numeric array token rendering includes exit code");
}

void test_parser_invalid_boundary_value_diagnostic() {
  const nerdss::core::Diagnostic diagnostic =
      nerdss::parser::MakeInvalidBoundaryValueDiagnostic(
          "sphereR", "wide", "stod");
  const std::string formatted = nerdss::core::FormatDiagnostic(diagnostic);

  require_true(diagnostic.category == nerdss::error::ErrorCategory::input,
               "invalid boundary value should use input category");
  require_true(diagnostic.exit_code == nerdss::error::ExitCode::input,
               "invalid boundary value should use input exit code");
  require_contains(diagnostic.message,
                   "invalid boundary value for 'sphereR' 'wide': stod",
                   "invalid boundary value message includes keyword and "
                   "value");
  require_contains(formatted, "ERROR [input]",
                   "invalid boundary value rendering includes category");
  require_contains(formatted, "exit_code=input(2)",
                   "invalid boundary value rendering includes exit code");
}

void test_parser_invalid_molecule_count_diagnostic() {
  const nerdss::core::Diagnostic diagnostic =
      nerdss::parser::MakeInvalidMoleculeCountDiagnostic(
          "Kinase", "100(site~P)@bad", "invalid character '@'");
  const std::string formatted = nerdss::core::FormatDiagnostic(diagnostic);

  require_true(diagnostic.category == nerdss::error::ErrorCategory::input,
               "invalid molecule count should use input category");
  require_true(diagnostic.exit_code == nerdss::error::ExitCode::input,
               "invalid molecule count should use input exit code");
  require_contains(
      diagnostic.message,
      "invalid molecule copy-number for molecule 'Kinase' in "
      "'100(site~P)@bad': invalid character '@'",
      "invalid molecule count message includes molecule and bad expression");
  require_contains(
      diagnostic.message,
      "expected 'molName:100' or state counts like "
      "'molName:100(interface~state)'",
      "invalid molecule count message includes expected format");
  require_contains(formatted, "ERROR [input]",
                   "invalid molecule count rendering includes category");
  require_contains(formatted, "exit_code=input(2)",
                   "invalid molecule count rendering includes exit code");
}

void test_parser_invalid_molecule_bond_count_diagnostic() {
  const nerdss::core::Diagnostic diagnostic =
      nerdss::parser::MakeInvalidMoleculeBondCountDiagnostic(
          "Kinase.mol", "two", "stoi");
  const std::string formatted = nerdss::core::FormatDiagnostic(diagnostic);

  require_true(diagnostic.category == nerdss::error::ErrorCategory::input,
               "invalid molecule bond count should use input category");
  require_true(diagnostic.exit_code == nerdss::error::ExitCode::input,
               "invalid molecule bond count should use input exit code");
  require_contains(
      diagnostic.message,
      "invalid molecule bond count in 'Kinase.mol' 'two': stoi",
      "invalid molecule bond count message includes path and value");
  require_contains(diagnostic.message,
                   "expected a non-negative integer after 'bonds ='",
                   "invalid molecule bond count message includes expected "
                   "format");
  require_contains(formatted, "ERROR [input]",
                   "invalid molecule bond count rendering includes category");
  require_contains(formatted, "exit_code=input(2)",
                   "invalid molecule bond count rendering includes exit code");
}

void test_error_mpi_rank_diagnostic() {
  const nerdss::core::Diagnostic diagnostic =
      nerdss::error::MakeMpiRankDiagnostic(3, "missing input");
  const std::string formatted = nerdss::core::FormatDiagnostic(diagnostic);

  require_true(diagnostic.category == nerdss::error::ErrorCategory::mpi,
               "MPI rank diagnostic should use mpi category");
  require_true(diagnostic.exit_code == nerdss::error::ExitCode::mpi,
               "MPI rank diagnostic should use mpi exit code");
  require_contains(diagnostic.message, "missing input (rank=3)",
                   "MPI rank diagnostic message includes rank");
  require_contains(formatted, "ERROR [mpi]",
                   "MPI rank diagnostic rendering includes category");
  require_contains(formatted, "exit_code=mpi(13)",
                   "MPI rank diagnostic rendering includes exit code");
}

void test_error_mpi_molecule_diagnostic() {
  const nerdss::core::Diagnostic diagnostic =
      nerdss::error::MakeMpiMoleculeDiagnostic(4, "bad molecule", 9, 2, 12, 1,
                                               99);
  const std::string formatted = nerdss::core::FormatDiagnostic(diagnostic);

  require_true(diagnostic.category == nerdss::error::ErrorCategory::mpi,
               "MPI molecule diagnostic should use mpi category");
  require_true(diagnostic.exit_code == nerdss::error::ExitCode::mpi,
               "MPI molecule diagnostic should use mpi exit code");
  require_contains(diagnostic.message,
                   "bad molecule (rank=4, mol.id=9, mol.index=2",
                   "MPI molecule diagnostic message includes molecule ids");
  require_contains(diagnostic.message, "moleculeList.size()=12",
                   "MPI molecule diagnostic message includes list size");
  require_contains(diagnostic.message, "mol.complex.id=99",
                   "MPI molecule diagnostic message includes complex id");
  require_contains(formatted, "ERROR [mpi]",
                   "MPI molecule diagnostic rendering includes category");
  require_contains(formatted, "exit_code=mpi(13)",
                   "MPI molecule diagnostic rendering includes exit code");
}

void test_error_mpi_subcell_assignment_diagnostic() {
  const nerdss::core::Diagnostic diagnostic =
      nerdss::error::MakeMpiSubcellAssignmentDiagnostic(
          2, 8, 1250, 42, 7, 3, "Kinase", 1.5, -2.0, 0.25, -1, 4, 0, -5, 10,
          12, 3, 360, 6);
  const std::string formatted = nerdss::core::FormatDiagnostic(diagnostic);

  require_true(diagnostic.category == nerdss::error::ErrorCategory::mpi,
               "MPI subcell assignment should use mpi category");
  require_true(diagnostic.exit_code == nerdss::error::ExitCode::mpi,
               "MPI subcell assignment should use mpi exit code");
  require_contains(
      diagnostic.message,
      "invalid MPI subcell assignment (rank=2, nprocs=8, simItr=1250",
      "MPI subcell assignment message includes rank and iteration");
  require_contains(diagnostic.message,
                   "mol.id=42, mol.index=7, mol.type.index=3",
                   "MPI subcell assignment message includes molecule ids");
  require_contains(diagnostic.message, "mol.type.name=Kinase",
                   "MPI subcell assignment message includes molecule type");
  require_contains(diagnostic.message,
                   "xItr=-1, yItr=4, zItr=0, currBin=-5",
                   "MPI subcell assignment message includes bin indices");
  require_contains(diagnostic.message, "numSubCells=[10, 12, 3]",
                   "MPI subcell assignment message includes subcell shape");
  require_contains(diagnostic.message, "mpi.xOffset=6",
                   "MPI subcell assignment message includes rank offset");
  require_contains(formatted, "ERROR [mpi]",
                   "MPI subcell assignment rendering includes category");
  require_contains(formatted, "exit_code=mpi(13)",
                   "MPI subcell assignment rendering includes exit code");
}

void test_error_mpi_membrane_placement_diagnostic() {
  const nerdss::core::Diagnostic diagnostic =
      nerdss::error::MakeMpiMembranePlacementDiagnostic(
          1, 4, 25, 11, 5, 2, "Lipid", 0.1, 0.2, 9.0, -10.0, 1.5,
          "error_coord_dump.xyz");
  const std::string formatted = nerdss::core::FormatDiagnostic(diagnostic);

  require_true(diagnostic.category == nerdss::error::ErrorCategory::mpi,
               "MPI membrane placement should use mpi category");
  require_true(diagnostic.exit_code == nerdss::error::ExitCode::mpi,
               "MPI membrane placement should use mpi exit code");
  require_contains(
      diagnostic.message,
      "MPI molecule is off membrane (rank=1, nprocs=4, simItr=25",
      "MPI membrane placement message includes rank and iteration");
  require_contains(diagnostic.message, "mol.type.name=Lipid",
                   "MPI membrane placement message includes molecule type");
  require_contains(diagnostic.message,
                   "membrane.min.z=-10, RS3Dinput=1.5",
                   "MPI membrane placement message includes membrane context");
  require_contains(diagnostic.message,
                   "coordinate_dump='error_coord_dump.xyz'",
                   "MPI membrane placement message includes dump path");
  require_contains(formatted, "ERROR [mpi]",
                   "MPI membrane placement rendering includes category");
  require_contains(formatted, "exit_code=mpi(13)",
                   "MPI membrane placement rendering includes exit code");
}

void test_setup_invalid_state_character_diagnostic() {
  const nerdss::core::Diagnostic diagnostic =
      nerdss::setup::MakeInvalidStateCharacterDiagnostic('@', "Lipid",
                                                         "site@A");
  const std::string formatted = nerdss::core::FormatDiagnostic(diagnostic);

  require_true(diagnostic.category == nerdss::error::ErrorCategory::input,
               "setup invalid state character should use input category");
  require_true(diagnostic.exit_code == nerdss::error::ExitCode::input,
               "setup invalid state character should use input exit code");
  require_contains(diagnostic.message,
                   "invalid starting copy-number state character '@' in "
                   "'site@A' for molecule 'Lipid'",
                   "setup invalid state character message includes context");
  require_contains(
      diagnostic.message,
      "expected alphanumeric interface/state tokens separated by '~' and ','",
      "setup invalid state character message includes accepted separators");
  require_contains(formatted, "ERROR [input]",
                   "setup invalid state character rendering includes category");
  require_contains(formatted, "exit_code=input(2)",
                   "setup invalid state character rendering includes exit code");
}

void test_setup_implicit_lipid_ordering_diagnostic() {
  const nerdss::core::Diagnostic diagnostic =
      nerdss::setup::MakeImplicitLipidOrderingDiagnostic("Membrane", 2);
  const std::string formatted = nerdss::core::FormatDiagnostic(diagnostic);

  require_true(diagnostic.category == nerdss::error::ErrorCategory::input,
               "implicit lipid ordering should use input category");
  require_true(diagnostic.exit_code == nerdss::error::ExitCode::input,
               "implicit lipid ordering should use input exit code");
  require_contains(
      diagnostic.message,
      "invalid implicit lipid molecule ordering: molecule 'Membrane' has type "
      "index 2, but implicit lipid must be molecule type index 0",
      "implicit lipid ordering message includes molecule and index");
  require_contains(formatted, "ERROR [input]",
                   "implicit lipid ordering rendering includes category");
  require_contains(formatted, "exit_code=input(2)",
                   "implicit lipid ordering rendering includes exit code");
}

void test_setup_implicit_molecule_interface_count_diagnostic() {
  const nerdss::core::Diagnostic diagnostic =
      nerdss::setup::MakeImplicitMoleculeInterfaceCountDiagnostic(
          "Membrane", "site~A,tail~B");
  const std::string formatted = nerdss::core::FormatDiagnostic(diagnostic);

  require_true(diagnostic.category == nerdss::error::ErrorCategory::input,
               "implicit molecule interface count should use input category");
  require_true(diagnostic.exit_code == nerdss::error::ExitCode::input,
               "implicit molecule interface count should use input exit code");
  require_contains(
      diagnostic.message,
      "invalid implicit molecule starting state 'site~A,tail~B' for molecule "
      "'Membrane': implicit molecules can only have one interface",
      "implicit molecule interface count message includes state context");
  require_contains(formatted, "ERROR [input]",
                   "implicit molecule interface count rendering includes "
                   "category");
  require_contains(formatted, "exit_code=input(2)",
                   "implicit molecule interface count rendering includes exit "
                   "code");
}

void test_setup_sphere_compartment_conflict_diagnostic() {
  const nerdss::core::Diagnostic diagnostic =
      nerdss::setup::MakeSphereCompartmentConflictDiagnostic();
  const std::string formatted = nerdss::core::FormatDiagnostic(diagnostic);

  require_true(diagnostic.category == nerdss::error::ErrorCategory::input,
               "sphere compartment conflict should use input category");
  require_true(diagnostic.exit_code == nerdss::error::ExitCode::input,
               "sphere compartment conflict should use input exit code");
  require_contains(
      diagnostic.message,
      "invalid boundary setup: compartment cannot be used with a sphere system",
      "sphere compartment conflict message describes invalid combination");
  require_contains(formatted, "ERROR [input]",
                   "sphere compartment conflict rendering includes category");
  require_contains(formatted, "exit_code=input(2)",
                   "sphere compartment conflict rendering includes exit code");
}

void test_setup_compartment_water_box_clearance_diagnostic() {
  const nerdss::core::Diagnostic diagnostic =
      nerdss::setup::MakeCompartmentWaterBoxClearanceDiagnostic('x', 50.0,
                                                               20.0, 4.0);
  const std::string formatted = nerdss::core::FormatDiagnostic(diagnostic);

  require_true(diagnostic.category == nerdss::error::ErrorCategory::input,
               "compartment clearance should use input category");
  require_true(diagnostic.exit_code == nerdss::error::ExitCode::input,
               "compartment clearance should use input exit code");
  require_contains(
      diagnostic.message,
      "invalid compartment setup: water box x dimension 50 is too small for "
      "compartment radius 20 and rMaxLimit 4; require half box length minus "
      "compartment radius >= 8",
      "compartment clearance message includes dimension and required clearance");
  require_contains(formatted, "ERROR [input]",
                   "compartment clearance rendering includes category");
  require_contains(formatted, "exit_code=input(2)",
                   "compartment clearance rendering includes exit code");
}

void test_parser_too_many_reaction_reactants_diagnostic() {
  const nerdss::core::Diagnostic diagnostic =
      nerdss::parser::MakeTooManyReactionReactantsDiagnostic("A+B+C->D", 3);
  const std::string formatted = nerdss::core::FormatDiagnostic(diagnostic);

  require_true(diagnostic.category == nerdss::error::ErrorCategory::input,
               "too many reaction reactants should use input category");
  require_true(diagnostic.exit_code == nerdss::error::ExitCode::input,
               "too many reaction reactants should use input exit code");
  require_contains(
      diagnostic.message,
      "invalid reaction 'A+B+C->D': has 3 reacting molecules; expected at most "
      "2",
      "too many reaction reactants message includes reaction and count");
  require_contains(formatted, "ERROR [input]",
                   "too many reaction reactants rendering includes category");
  require_contains(formatted, "exit_code=input(2)",
                   "too many reaction reactants rendering includes exit code");
}

void test_parser_missing_reaction_arrow_diagnostic() {
  const nerdss::core::Diagnostic diagnostic =
      nerdss::parser::MakeMissingReactionArrowDiagnostic("A+B");
  const std::string formatted = nerdss::core::FormatDiagnostic(diagnostic);

  require_true(diagnostic.category == nerdss::error::ErrorCategory::input,
               "missing reaction arrow should use input category");
  require_true(diagnostic.exit_code == nerdss::error::ExitCode::input,
               "missing reaction arrow should use input exit code");
  require_contains(
      diagnostic.message,
      "invalid reaction 'A+B': missing reaction arrow; expected '->' or "
      "'<->'",
      "missing reaction arrow message includes accepted arrows");
  require_contains(formatted, "ERROR [input]",
                   "missing reaction arrow rendering includes category");
  require_contains(formatted, "exit_code=input(2)",
                   "missing reaction arrow rendering includes exit code");
}

void test_parser_ambiguous_reaction_type_diagnostic() {
  const nerdss::core::Diagnostic diagnostic =
      nerdss::parser::MakeAmbiguousReactionTypeDiagnostic("null->0");
  const std::string formatted = nerdss::core::FormatDiagnostic(diagnostic);

  require_true(diagnostic.category == nerdss::error::ErrorCategory::input,
               "ambiguous reaction type should use input category");
  require_true(diagnostic.exit_code == nerdss::error::ExitCode::input,
               "ambiguous reaction type should use input exit code");
  require_contains(
      diagnostic.message,
      "invalid reaction 'null->0': cannot determine reaction type when both "
      "sides are null",
      "ambiguous reaction type message includes null/null cause");
  require_contains(formatted, "ERROR [input]",
                   "ambiguous reaction type rendering includes category");
  require_contains(formatted, "exit_code=input(2)",
                   "ambiguous reaction type rendering includes exit code");
}

void test_parser_invalid_reaction_keyword_diagnostic() {
  const nerdss::core::Diagnostic diagnostic =
      nerdss::parser::MakeInvalidReactionKeywordDiagnostic("badkey",
                                                           "A(a)->A(b)");
  const std::string formatted = nerdss::core::FormatDiagnostic(diagnostic);

  require_true(diagnostic.category == nerdss::error::ErrorCategory::input,
               "invalid reaction keyword should use input category");
  require_true(diagnostic.exit_code == nerdss::error::ExitCode::input,
               "invalid reaction keyword should use input exit code");
  require_contains(
      diagnostic.message,
      "invalid reaction parameter keyword 'badkey' for reaction 'A(a)->A(b)'",
      "invalid reaction keyword message includes keyword and reaction");
  require_contains(formatted, "ERROR [input]",
                   "invalid reaction keyword rendering includes category");
  require_contains(formatted, "exit_code=input(2)",
                   "invalid reaction keyword rendering includes exit code");
}

void test_parser_incomplete_reaction_diagnostic() {
  const nerdss::core::Diagnostic diagnostic =
      nerdss::parser::MakeIncompleteReactionDiagnostic(
          "A(a)+B(b)->A(a!1).B(b!1)", "missing onrate3dka");
  const std::string formatted = nerdss::core::FormatDiagnostic(diagnostic);

  require_true(diagnostic.category == nerdss::error::ErrorCategory::input,
               "incomplete reaction should use input category");
  require_true(diagnostic.exit_code == nerdss::error::ExitCode::input,
               "incomplete reaction should use input exit code");
  require_contains(
      diagnostic.message,
      "invalid reaction 'A(a)+B(b)->A(a!1).B(b!1)': missing onrate3dka",
      "incomplete reaction message includes reaction and reason");
  require_contains(formatted, "ERROR [input]",
                   "incomplete reaction rendering includes category");
  require_contains(formatted, "exit_code=input(2)",
                   "incomplete reaction rendering includes exit code");
}

void test_parser_invalid_reaction_molecule_syntax_diagnostic() {
  const nerdss::core::Diagnostic diagnostic =
      nerdss::parser::MakeInvalidReactionMoleculeSyntaxDiagnostic(
          "A(site!1)", "indexed interactions are not allowed on the reactant "
                       "side; use wildcard bonds like '!*'");
  const std::string formatted = nerdss::core::FormatDiagnostic(diagnostic);

  require_true(diagnostic.category == nerdss::error::ErrorCategory::input,
               "invalid reaction molecule syntax should use input category");
  require_true(diagnostic.exit_code == nerdss::error::ExitCode::input,
               "invalid reaction molecule syntax should use input exit code");
  require_contains(
      diagnostic.message,
      "invalid reaction molecule 'A(site!1)': indexed interactions are not "
      "allowed on the reactant side",
      "invalid reaction molecule syntax message includes expression and "
      "reason");
  require_contains(
      diagnostic.message,
      "expected BNGL-like molecule syntax such as 'Mol(iface~state!*)' or "
      "'Mol(iface!1)'",
      "invalid reaction molecule syntax message includes expected grammar");
  require_contains(formatted, "ERROR [input]",
                   "invalid reaction molecule syntax rendering includes "
                   "category");
  require_contains(formatted, "exit_code=input(2)",
                   "invalid reaction molecule syntax rendering includes exit "
                   "code");
}

void test_parser_invalid_restart_reaction_type_diagnostic() {
  const nerdss::core::Diagnostic diagnostic =
      nerdss::parser::MakeInvalidRestartReactionTypeDiagnostic(
          12, "coupled reaction attached to reaction", -1);
  const std::string formatted = nerdss::core::FormatDiagnostic(diagnostic);

  require_true(diagnostic.category == nerdss::error::ErrorCategory::input,
               "invalid restart reaction type should use input category");
  require_true(diagnostic.exit_code == nerdss::error::ExitCode::input,
               "invalid restart reaction type should use input exit code");
  require_contains(
      diagnostic.message,
      "invalid restart reaction type -1 for coupled reaction attached to "
      "reaction 12",
      "invalid restart reaction type message includes context and index");
  require_contains(formatted, "ERROR [input]",
                   "invalid restart reaction type rendering includes "
                   "category");
  require_contains(formatted, "exit_code=input(2)",
                   "invalid restart reaction type rendering includes exit "
                   "code");
}

void test_parser_malformed_restart_diagnostic() {
  const nerdss::core::Diagnostic diagnostic =
      nerdss::parser::MakeMalformedRestartDiagnostic("template vectors",
                                                     "vector too long");
  const std::string formatted = nerdss::core::FormatDiagnostic(diagnostic);

  require_true(diagnostic.category == nerdss::error::ErrorCategory::input,
               "malformed restart should use input category");
  require_true(diagnostic.exit_code == nerdss::error::ExitCode::input,
               "malformed restart should use input exit code");
  require_contains(diagnostic.message,
                   "malformed restart file while reading template vectors: "
                   "vector too long",
                   "malformed restart message includes context and reason");
  require_contains(formatted, "ERROR [input]",
                   "malformed restart rendering includes category");
  require_contains(formatted, "exit_code=input(2)",
                   "malformed restart rendering includes exit code");
}

void test_parser_unknown_observable_type_diagnostic() {
  const nerdss::core::Diagnostic diagnostic =
      nerdss::parser::MakeUnknownObservableTypeDiagnostic("species");
  const std::string formatted = nerdss::core::FormatDiagnostic(diagnostic);

  require_true(diagnostic.category == nerdss::error::ErrorCategory::input,
               "unknown observable type should use input category");
  require_true(diagnostic.exit_code == nerdss::error::ExitCode::input,
               "unknown observable type should use input exit code");
  require_contains(diagnostic.message,
                   "invalid observable type 'species': expected molecule or "
                   "complex",
                   "unknown observable type message includes accepted values");
  require_contains(formatted, "ERROR [input]",
                   "unknown observable type rendering includes category");
  require_contains(formatted, "exit_code=input(2)",
                   "unknown observable type rendering includes exit code");
}

void test_parser_unknown_state_interface_diagnostic() {
  const nerdss::core::Diagnostic diagnostic =
      nerdss::parser::MakeUnknownStateInterfaceDiagnostic("siteB", "Kinase");
  const std::string formatted = nerdss::core::FormatDiagnostic(diagnostic);

  require_true(diagnostic.category == nerdss::error::ErrorCategory::input,
               "unknown state interface should use input category");
  require_true(diagnostic.exit_code == nerdss::error::ExitCode::input,
               "unknown state interface should use input exit code");
  require_contains(
      diagnostic.message,
      "invalid state declaration for molecule 'Kinase': interface 'siteB' is "
      "not declared on the molecule template",
      "unknown state interface message includes molecule and interface");
  require_contains(formatted, "ERROR [input]",
                   "unknown state interface rendering includes category");
  require_contains(formatted, "exit_code=input(2)",
                   "unknown state interface rendering includes exit code");
}

void test_parser_unknown_reaction_molecule_template_diagnostic() {
  const nerdss::core::Diagnostic diagnostic =
      nerdss::parser::MakeUnknownReactionMoleculeTemplateDiagnostic("Ghost");
  const std::string formatted = nerdss::core::FormatDiagnostic(diagnostic);

  require_true(diagnostic.category == nerdss::error::ErrorCategory::input,
               "unknown reaction molecule should use input category");
  require_true(diagnostic.exit_code == nerdss::error::ExitCode::input,
               "unknown reaction molecule should use input exit code");
  require_contains(
      diagnostic.message,
      "invalid reaction molecule 'Ghost': molecule template is not declared",
      "unknown reaction molecule message includes molecule name");
  require_contains(formatted, "ERROR [input]",
                   "unknown reaction molecule rendering includes category");
  require_contains(formatted, "exit_code=input(2)",
                   "unknown reaction molecule rendering includes exit code");
}

void test_parser_unknown_reaction_interface_diagnostic() {
  const nerdss::core::Diagnostic diagnostic =
      nerdss::parser::MakeUnknownReactionInterfaceDiagnostic("tail", "Lipid");
  const std::string formatted = nerdss::core::FormatDiagnostic(diagnostic);

  require_true(diagnostic.category == nerdss::error::ErrorCategory::input,
               "unknown reaction interface should use input category");
  require_true(diagnostic.exit_code == nerdss::error::ExitCode::input,
               "unknown reaction interface should use input exit code");
  require_contains(
      diagnostic.message,
      "invalid reaction interface 'tail' for molecule 'Lipid': interface is "
      "not declared on the molecule template",
      "unknown reaction interface message includes molecule and interface");
  require_contains(formatted, "ERROR [input]",
                   "unknown reaction interface rendering includes category");
  require_contains(formatted, "exit_code=input(2)",
                   "unknown reaction interface rendering includes exit code");
}

void test_parser_unknown_reaction_interface_state_diagnostic() {
  const nerdss::core::Diagnostic diagnostic =
      nerdss::parser::MakeUnknownReactionInterfaceStateDiagnostic("P", "site",
                                                                  "Kinase");
  const std::string formatted = nerdss::core::FormatDiagnostic(diagnostic);

  require_true(diagnostic.category == nerdss::error::ErrorCategory::input,
               "unknown reaction interface state should use input category");
  require_true(diagnostic.exit_code == nerdss::error::ExitCode::input,
               "unknown reaction interface state should use input exit code");
  require_contains(
      diagnostic.message,
      "invalid reaction state 'P' for interface 'site' on molecule 'Kinase': "
      "state is not declared on the molecule template interface",
      "unknown reaction interface state message includes state context");
  require_contains(
      formatted, "ERROR [input]",
      "unknown reaction interface state rendering includes category");
  require_contains(
      formatted, "exit_code=input(2)",
      "unknown reaction interface state rendering includes exit code");
}

} // namespace

int main() {
  test_diagnostic_format_with_trace();
  test_diagnostic_format_without_trace();
  test_parser_invalid_keyword_diagnostic();
  test_parser_section_order_diagnostic();
  test_parser_invalid_boolean_diagnostic();
  test_parser_invalid_numeric_array_token_diagnostic();
  test_parser_invalid_boundary_value_diagnostic();
  test_parser_invalid_molecule_count_diagnostic();
  test_parser_invalid_molecule_bond_count_diagnostic();
  test_error_mpi_rank_diagnostic();
  test_error_mpi_molecule_diagnostic();
  test_error_mpi_subcell_assignment_diagnostic();
  test_error_mpi_membrane_placement_diagnostic();
  test_setup_invalid_state_character_diagnostic();
  test_setup_implicit_lipid_ordering_diagnostic();
  test_setup_implicit_molecule_interface_count_diagnostic();
  test_setup_sphere_compartment_conflict_diagnostic();
  test_setup_compartment_water_box_clearance_diagnostic();
  test_parser_too_many_reaction_reactants_diagnostic();
  test_parser_missing_reaction_arrow_diagnostic();
  test_parser_ambiguous_reaction_type_diagnostic();
  test_parser_invalid_reaction_keyword_diagnostic();
  test_parser_incomplete_reaction_diagnostic();
  test_parser_invalid_reaction_molecule_syntax_diagnostic();
  test_parser_invalid_restart_reaction_type_diagnostic();
  test_parser_malformed_restart_diagnostic();
  test_parser_unknown_observable_type_diagnostic();
  test_parser_unknown_state_interface_diagnostic();
  test_parser_unknown_reaction_molecule_template_diagnostic();
  test_parser_unknown_reaction_interface_diagnostic();
  test_parser_unknown_reaction_interface_state_diagnostic();
  return 0;
}
