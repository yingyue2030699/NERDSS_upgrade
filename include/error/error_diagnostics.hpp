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

inline core::Diagnostic MakeMpiSubcellAssignmentDiagnostic(
    int rank, int nprocs, int sim_itr, int molecule_id, int molecule_index,
    int molecule_type_index, const std::string& molecule_type_name,
    double coord_x, double coord_y, double coord_z, int x_index, int y_index,
    int z_index, int current_bin, int subcell_x, int subcell_y,
    int subcell_z, int subcell_total, int x_offset) {
  std::ostringstream stream;
  stream << "invalid MPI subcell assignment"
         << " (rank=" << rank << ", nprocs=" << nprocs
         << ", simItr=" << sim_itr << ", mol.id=" << molecule_id
         << ", mol.index=" << molecule_index
         << ", mol.type.index=" << molecule_type_index;
  if (!molecule_type_name.empty()) {
    stream << ", mol.type.name=" << molecule_type_name;
  }
  stream << ", mol.comCoord=[" << coord_x << ", " << coord_y << ", "
         << coord_z << "]"
         << ", xItr=" << x_index << ", yItr=" << y_index
         << ", zItr=" << z_index << ", currBin=" << current_bin
         << ", numSubCells=[" << subcell_x << ", " << subcell_y << ", "
         << subcell_z << "]"
         << ", numSubCells.tot=" << subcell_total
         << ", mpi.xOffset=" << x_offset << ")";
  return core::MakeDiagnostic(ErrorCategory::mpi, stream.str(), "");
}

inline core::Diagnostic MakeMpiMembranePlacementDiagnostic(
    int rank, int nprocs, int sim_itr, int molecule_id, int molecule_index,
    int molecule_type_index, const std::string& molecule_type_name,
    double coord_x, double coord_y, double coord_z, double membrane_min_z,
    double rs3d_input, const std::string& dump_path) {
  std::ostringstream stream;
  stream << "MPI molecule is off membrane"
         << " (rank=" << rank << ", nprocs=" << nprocs
         << ", simItr=" << sim_itr << ", mol.id=" << molecule_id
         << ", mol.index=" << molecule_index
         << ", mol.type.index=" << molecule_type_index;
  if (!molecule_type_name.empty()) {
    stream << ", mol.type.name=" << molecule_type_name;
  }
  stream << ", mol.comCoord=[" << coord_x << ", " << coord_y << ", "
         << coord_z << "]"
         << ", membrane.min.z=" << membrane_min_z
         << ", RS3Dinput=" << rs3d_input;
  if (!dump_path.empty()) {
    stream << ", coordinate_dump='" << dump_path << "'";
  }
  stream << ")";
  return core::MakeDiagnostic(ErrorCategory::mpi, stream.str(), "");
}

} // namespace error
} // namespace nerdss
