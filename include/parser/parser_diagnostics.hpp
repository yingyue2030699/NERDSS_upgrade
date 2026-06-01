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

} // namespace parser
} // namespace nerdss
