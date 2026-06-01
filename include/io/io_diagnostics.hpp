/*! \file io_diagnostics.hpp
 * \brief Structured diagnostics for optional I/O artifact failures.
 */

#pragma once

#include "core/diagnostics.hpp"

#include <ostream>
#include <sstream>
#include <string>

namespace nerdss {
namespace io {

inline core::Diagnostic MakeArtifactWriteOpenDiagnostic(
    const std::string& path, const char* artifact_type) {
  std::ostringstream message;
  message << "Error: Unable to open " << artifact_type
          << " file for writing: " << path;
  return core::MakeDiagnostic(error::ErrorCategory::file_io, message.str(), "");
}

inline void WriteArtifactWriteOpenDiagnostic(std::ostream& stream,
                                             const std::string& path,
                                             const char* artifact_type) {
  core::WriteDiagnostic(
      stream, MakeArtifactWriteOpenDiagnostic(path, artifact_type));
}

} // namespace io
} // namespace nerdss
