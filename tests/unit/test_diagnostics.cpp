#include "core/diagnostics.hpp"
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
  test_parser_invalid_molecule_count_diagnostic();
  test_setup_invalid_state_character_diagnostic();
  test_setup_implicit_lipid_ordering_diagnostic();
  test_setup_implicit_molecule_interface_count_diagnostic();
  test_setup_sphere_compartment_conflict_diagnostic();
  test_setup_compartment_water_box_clearance_diagnostic();
  test_parser_too_many_reaction_reactants_diagnostic();
  test_parser_unknown_observable_type_diagnostic();
  test_parser_unknown_state_interface_diagnostic();
  test_parser_unknown_reaction_molecule_template_diagnostic();
  test_parser_unknown_reaction_interface_diagnostic();
  test_parser_unknown_reaction_interface_state_diagnostic();
  return 0;
}
