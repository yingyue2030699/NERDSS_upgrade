/*! \file error_diagnostics.hpp
 * \brief Structured diagnostics for legacy error helpers.
 */

#pragma once

#include "core/diagnostics.hpp"

#include <cstddef>
#include <sstream>
#include <string>

namespace nerdss {
namespace error {

inline core::Diagnostic MakeMpiRankDiagnostic(int rank,
                                              const std::string& message) {
  std::ostringstream stream;
  stream << message << " (rank=" << rank << ")";
  return core::MakeDiagnostic(ErrorCategory::mpi, stream.str(), "");
}

inline core::Diagnostic MakeMpiMoleculeDiagnostic(
    int rank, const std::string& message, int molecule_id, int molecule_index,
    std::size_t molecule_count, int molecule_complex_index, int complex_id) {
  std::ostringstream stream;
  stream << message << " (rank=" << rank << ", mol.id=" << molecule_id
         << ", mol.index=" << molecule_index
         << ", moleculeList.size()=" << molecule_count
         << ", mol.myComIndex=" << molecule_complex_index
         << ", mol.complex.id=" << complex_id << ")";
  return core::MakeDiagnostic(ErrorCategory::mpi, stream.str(), "");
}

} // namespace error
} // namespace nerdss
