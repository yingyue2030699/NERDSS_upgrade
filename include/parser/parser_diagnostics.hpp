/*! \file parser_diagnostics.hpp
 * \brief Structured diagnostics for parser boundary failures.
 */

#pragma once

#include "core/diagnostics.hpp"

#include <cstddef>
#include <sstream>
#include <string>

namespace nerdss {
namespace parser {

inline core::Diagnostic MakeFileOpenDiagnostic(const std::string& path,
                                               const char* role) {
  std::ostringstream message;
  message << "cannot open " << role << " file '" << path << "'";
  return core::MakeDiagnostic(error::ErrorCategory::file_io, message.str(), "");
}

inline core::Diagnostic MakeInvalidKeywordDiagnostic(const std::string& keyword,
                                                     const char* context,
                                                     const std::string& path) {
  std::ostringstream message;
  message << "invalid " << context << " keyword '" << keyword << "'";
  if (!path.empty()) {
    message << " in '" << path << "'";
  }
  return core::MakeDiagnostic(error::ErrorCategory::input, message.str(), "");
}

inline core::Diagnostic MakeSectionOrderDiagnostic(const std::string& path,
                                                   const char* encountered,
                                                   const char* required) {
  std::ostringstream message;
  message << "invalid input section order";
  if (!path.empty()) {
    message << " in '" << path << "'";
  }
  message << ": '" << encountered << "' requires '" << required << "' first";
  return core::MakeDiagnostic(error::ErrorCategory::input, message.str(), "");
}

inline core::Diagnostic MakeInvalidBooleanDiagnostic(const std::string& value) {
  std::ostringstream message;
  message << "cannot read boolean value '" << value
          << "': expected one of 0, 1, false, or true";
  return core::MakeDiagnostic(error::ErrorCategory::input, message.str(), "");
}

inline core::Diagnostic MakeInvalidNumericArrayTokenDiagnostic(
    const std::string& token, const std::string& input) {
  std::ostringstream message;
  message << "cannot read numeric array token '" << token << "'";
  if (!input.empty()) {
    message << " in '" << input << "'";
  }
  message << ": expected a number, pi, m_pi, or nan";
  return core::MakeDiagnostic(error::ErrorCategory::input, message.str(), "");
}

inline core::Diagnostic MakeInvalidMoleculeCountDiagnostic(
    const std::string& molecule_name, const std::string& expression,
    const std::string& reason) {
  std::ostringstream message;
  message << "invalid molecule copy-number";
  if (!molecule_name.empty()) {
    message << " for molecule '" << molecule_name << "'";
  }
  if (!expression.empty()) {
    message << " in '" << expression << "'";
  }
  if (!reason.empty()) {
    message << ": " << reason;
  }
  message << "; expected 'molName:100' or state counts like "
             "'molName:100(interface~state)'";
  return core::MakeDiagnostic(error::ErrorCategory::input, message.str(), "");
}

inline core::Diagnostic MakeInvalidMoleculeBondCountDiagnostic(
    const std::string& path, const std::string& value,
    const std::string& reason) {
  std::ostringstream message;
  message << "invalid molecule bond count";
  if (!path.empty()) {
    message << " in '" << path << "'";
  }
  if (!value.empty()) {
    message << " '" << value << "'";
  }
  if (!reason.empty()) {
    message << ": " << reason;
  }
  message << "; expected a non-negative integer after 'bonds ='";
  return core::MakeDiagnostic(error::ErrorCategory::input, message.str(), "");
}

inline core::Diagnostic
MakeTooManyReactionReactantsDiagnostic(const std::string& reaction,
                                       std::size_t reactant_count) {
  std::ostringstream message;
  message << "invalid reaction '" << reaction << "': has " << reactant_count
          << " reacting molecules; expected at most 2";
  return core::MakeDiagnostic(error::ErrorCategory::input, message.str(), "");
}

inline core::Diagnostic
MakeMissingReactionArrowDiagnostic(const std::string& reaction) {
  std::ostringstream message;
  message << "invalid reaction '" << reaction
          << "': missing reaction arrow; expected '->' or '<->'";
  return core::MakeDiagnostic(error::ErrorCategory::input, message.str(), "");
}

inline core::Diagnostic
MakeAmbiguousReactionTypeDiagnostic(const std::string& reaction) {
  std::ostringstream message;
  message << "invalid reaction '" << reaction
          << "': cannot determine reaction type when both sides are null";
  return core::MakeDiagnostic(error::ErrorCategory::input, message.str(), "");
}

inline core::Diagnostic MakeInvalidReactionKeywordDiagnostic(
    const std::string& keyword, const std::string& reaction) {
  std::ostringstream message;
  message << "invalid reaction parameter keyword '" << keyword << "'";
  if (!reaction.empty()) {
    message << " for reaction '" << reaction << "'";
  }
  return core::MakeDiagnostic(error::ErrorCategory::input, message.str(), "");
}

inline core::Diagnostic MakeIncompleteReactionDiagnostic(
    const std::string& reaction, const std::string& reason) {
  std::ostringstream message;
  message << "invalid reaction '" << reaction << "'";
  if (!reason.empty()) {
    message << ": " << reason;
  }
  return core::MakeDiagnostic(error::ErrorCategory::input, message.str(), "");
}

inline core::Diagnostic MakeInvalidReactionMoleculeSyntaxDiagnostic(
    const std::string& molecule_expression, const std::string& reason) {
  std::ostringstream message;
  message << "invalid reaction molecule";
  if (!molecule_expression.empty()) {
    message << " '" << molecule_expression << "'";
  }
  if (!reason.empty()) {
    message << ": " << reason;
  }
  message << "; expected BNGL-like molecule syntax such as "
             "'Mol(iface~state!*)' or 'Mol(iface!1)'";
  return core::MakeDiagnostic(error::ErrorCategory::input, message.str(), "");
}

inline core::Diagnostic
MakeUnknownObservableTypeDiagnostic(const std::string& observable_type) {
  std::ostringstream message;
  message << "invalid observable type '" << observable_type
          << "': expected molecule or complex";
  return core::MakeDiagnostic(error::ErrorCategory::input, message.str(), "");
}

inline core::Diagnostic
MakeUnknownStateInterfaceDiagnostic(const std::string& interface_name,
                                    const std::string& molecule_name) {
  std::ostringstream message;
  message << "invalid state declaration for molecule '" << molecule_name
          << "': interface '" << interface_name
          << "' is not declared on the molecule template";
  return core::MakeDiagnostic(error::ErrorCategory::input, message.str(), "");
}

inline core::Diagnostic MakeUnknownReactionMoleculeTemplateDiagnostic(
    const std::string& molecule_name) {
  std::ostringstream message;
  message << "invalid reaction molecule '" << molecule_name
          << "': molecule template is not declared";
  return core::MakeDiagnostic(error::ErrorCategory::input, message.str(), "");
}

inline core::Diagnostic MakeUnknownReactionInterfaceDiagnostic(
    const std::string& interface_name, const std::string& molecule_name) {
  std::ostringstream message;
  message << "invalid reaction interface '" << interface_name
          << "' for molecule '" << molecule_name
          << "': interface is not declared on the molecule template";
  return core::MakeDiagnostic(error::ErrorCategory::input, message.str(), "");
}

inline core::Diagnostic MakeUnknownReactionInterfaceStateDiagnostic(
    const std::string& state, const std::string& interface_name,
    const std::string& molecule_name) {
  std::ostringstream message;
  message << "invalid reaction state '" << state << "' for interface '"
          << interface_name << "' on molecule '" << molecule_name
          << "': state is not declared on the molecule template interface";
  return core::MakeDiagnostic(error::ErrorCategory::input, message.str(), "");
}

inline void ExitWithFileOpenDiagnostic(const std::string& path,
                                       const char* role) {
  core::ExitWithDiagnostic(MakeFileOpenDiagnostic(path, role));
}

inline void ExitWithInvalidKeywordDiagnostic(const std::string& keyword,
                                             const char* context,
                                             const std::string& path) {
  core::ExitWithDiagnostic(
      MakeInvalidKeywordDiagnostic(keyword, context, path));
}

inline void ExitWithSectionOrderDiagnostic(const std::string& path,
                                           const char* encountered,
                                           const char* required) {
  core::ExitWithDiagnostic(
      MakeSectionOrderDiagnostic(path, encountered, required));
}

inline void ExitWithInvalidBooleanDiagnostic(const std::string& value) {
  core::ExitWithDiagnostic(MakeInvalidBooleanDiagnostic(value));
}

inline void ExitWithInvalidNumericArrayTokenDiagnostic(
    const std::string& token, const std::string& input) {
  core::ExitWithDiagnostic(
      MakeInvalidNumericArrayTokenDiagnostic(token, input));
}

inline void ExitWithInvalidMoleculeCountDiagnostic(
    const std::string& molecule_name, const std::string& expression,
    const std::string& reason) {
  core::ExitWithDiagnostic(
      MakeInvalidMoleculeCountDiagnostic(molecule_name, expression, reason));
}

inline void ExitWithInvalidMoleculeBondCountDiagnostic(
    const std::string& path, const std::string& value,
    const std::string& reason) {
  core::ExitWithDiagnostic(
      MakeInvalidMoleculeBondCountDiagnostic(path, value, reason));
}

inline void ExitWithTooManyReactionReactantsDiagnostic(
    const std::string& reaction, std::size_t reactant_count) {
  core::ExitWithDiagnostic(
      MakeTooManyReactionReactantsDiagnostic(reaction, reactant_count));
}

inline void ExitWithMissingReactionArrowDiagnostic(
    const std::string& reaction) {
  core::ExitWithDiagnostic(MakeMissingReactionArrowDiagnostic(reaction));
}

inline void ExitWithAmbiguousReactionTypeDiagnostic(
    const std::string& reaction) {
  core::ExitWithDiagnostic(MakeAmbiguousReactionTypeDiagnostic(reaction));
}

inline void ExitWithInvalidReactionKeywordDiagnostic(
    const std::string& keyword, const std::string& reaction) {
  core::ExitWithDiagnostic(
      MakeInvalidReactionKeywordDiagnostic(keyword, reaction));
}

inline void ExitWithIncompleteReactionDiagnostic(const std::string& reaction,
                                                 const std::string& reason) {
  core::ExitWithDiagnostic(
      MakeIncompleteReactionDiagnostic(reaction, reason));
}

inline void ExitWithInvalidReactionMoleculeSyntaxDiagnostic(
    const std::string& molecule_expression, const std::string& reason) {
  core::ExitWithDiagnostic(MakeInvalidReactionMoleculeSyntaxDiagnostic(
      molecule_expression, reason));
}

inline void ExitWithUnknownObservableTypeDiagnostic(
    const std::string& observable_type) {
  core::ExitWithDiagnostic(
      MakeUnknownObservableTypeDiagnostic(observable_type));
}

inline void ExitWithUnknownStateInterfaceDiagnostic(
    const std::string& interface_name, const std::string& molecule_name) {
  core::ExitWithDiagnostic(
      MakeUnknownStateInterfaceDiagnostic(interface_name, molecule_name));
}

inline void ExitWithUnknownReactionMoleculeTemplateDiagnostic(
    const std::string& molecule_name) {
  core::ExitWithDiagnostic(
      MakeUnknownReactionMoleculeTemplateDiagnostic(molecule_name));
}

inline void ExitWithUnknownReactionInterfaceDiagnostic(
    const std::string& interface_name, const std::string& molecule_name) {
  core::ExitWithDiagnostic(
      MakeUnknownReactionInterfaceDiagnostic(interface_name, molecule_name));
}

inline void ExitWithUnknownReactionInterfaceStateDiagnostic(
    const std::string& state, const std::string& interface_name,
    const std::string& molecule_name) {
  core::ExitWithDiagnostic(MakeUnknownReactionInterfaceStateDiagnostic(
      state, interface_name, molecule_name));
}

} // namespace parser
} // namespace nerdss
