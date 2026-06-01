/*! \file setup_diagnostics.hpp
 * \brief Structured diagnostics for system setup validation failures.
 */

#pragma once

#include "core/diagnostics.hpp"

#include <sstream>
#include <string>

namespace nerdss {
namespace setup {

inline core::Diagnostic
MakeInvalidStateCharacterDiagnostic(char character,
                                    const std::string& molecule_name,
                                    const std::string& state_expression) {
  std::ostringstream message;
  message << "invalid starting copy-number state character '" << character
          << "'";
  if (!state_expression.empty()) {
    message << " in '" << state_expression << "'";
  }
  if (!molecule_name.empty()) {
    message << " for molecule '" << molecule_name << "'";
  }
  message << ": expected alphanumeric interface/state tokens separated by '~'"
             " and ','";
  return core::MakeDiagnostic(error::ErrorCategory::input, message.str(), "");
}

inline core::Diagnostic MakeImplicitLipidOrderingDiagnostic(
    const std::string& molecule_name, int molecule_type_index) {
  std::ostringstream message;
  message << "invalid implicit lipid molecule ordering";
  if (!molecule_name.empty()) {
    message << ": molecule '" << molecule_name << "'";
  }
  message << " has type index " << molecule_type_index
          << ", but implicit lipid must be molecule type index 0";
  return core::MakeDiagnostic(error::ErrorCategory::input, message.str(), "");
}

inline core::Diagnostic MakeImplicitMoleculeInterfaceCountDiagnostic(
    const std::string& molecule_name, const std::string& state_expression) {
  std::ostringstream message;
  message << "invalid implicit molecule starting state";
  if (!state_expression.empty()) {
    message << " '" << state_expression << "'";
  }
  if (!molecule_name.empty()) {
    message << " for molecule '" << molecule_name << "'";
  }
  message << ": implicit molecules can only have one interface";
  return core::MakeDiagnostic(error::ErrorCategory::input, message.str(), "");
}

inline core::Diagnostic MakeSphereCompartmentConflictDiagnostic() {
  return core::MakeDiagnostic(
      error::ErrorCategory::input,
      "invalid boundary setup: compartment cannot be used with a sphere system",
      "");
}

inline core::Diagnostic MakeCompartmentWaterBoxClearanceDiagnostic(
    char axis, double dimension, double compartment_radius,
    double r_max_limit) {
  std::ostringstream message;
  message << "invalid compartment setup: water box " << axis
          << " dimension " << dimension
          << " is too small for compartment radius " << compartment_radius
          << " and rMaxLimit " << r_max_limit
          << "; require half box length minus compartment radius >= "
          << 2.0 * r_max_limit;
  return core::MakeDiagnostic(error::ErrorCategory::input, message.str(), "");
}

inline void ExitWithInvalidStateCharacterDiagnostic(
    char character, const std::string& molecule_name,
    const std::string& state_expression) {
  core::ExitWithDiagnostic(MakeInvalidStateCharacterDiagnostic(
      character, molecule_name, state_expression));
}

inline void ExitWithImplicitLipidOrderingDiagnostic(
    const std::string& molecule_name, int molecule_type_index) {
  core::ExitWithDiagnostic(MakeImplicitLipidOrderingDiagnostic(
      molecule_name, molecule_type_index));
}

inline void ExitWithImplicitMoleculeInterfaceCountDiagnostic(
    const std::string& molecule_name, const std::string& state_expression) {
  core::ExitWithDiagnostic(MakeImplicitMoleculeInterfaceCountDiagnostic(
      molecule_name, state_expression));
}

inline void ExitWithSphereCompartmentConflictDiagnostic() {
  core::ExitWithDiagnostic(MakeSphereCompartmentConflictDiagnostic());
}

} // namespace setup
} // namespace nerdss
