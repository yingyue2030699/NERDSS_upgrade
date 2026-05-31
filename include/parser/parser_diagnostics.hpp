/*! \file parser_diagnostics.hpp
 * \brief Structured diagnostics for parser boundary failures.
 */

#pragma once

#include "core/diagnostics.hpp"

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

inline void ExitWithFileOpenDiagnostic(const std::string& path,
                                       const char* role) {
  core::ExitWithDiagnostic(MakeFileOpenDiagnostic(path, role));
}

} // namespace parser
} // namespace nerdss
