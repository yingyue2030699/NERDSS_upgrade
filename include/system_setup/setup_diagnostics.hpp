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

inline void ExitWithInvalidStateCharacterDiagnostic(
    char character, const std::string& molecule_name,
    const std::string& state_expression) {
  core::ExitWithDiagnostic(MakeInvalidStateCharacterDiagnostic(
      character, molecule_name, state_expression));
}

} // namespace setup
} // namespace nerdss
