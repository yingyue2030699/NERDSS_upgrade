/*! \file trajectory_engine.hpp
 * \brief Facade for trajectory boundary dispatch.
 */

#pragma once

#include "trajectory_functions/trajectory_functions.hpp"

namespace nerdss {
namespace core {

enum class BoundaryGeometry {
  kBox,
  kSphere,
};

class TrajectoryEngine {
public:
  static BoundaryGeometry BoundaryGeometryFor(const Membrane& membrane) {
    return membrane.isSphere ? BoundaryGeometry::kSphere : BoundaryGeometry::kBox;
  }

  static bool UsesSphericalBoundary(const Membrane& membrane) {
    return BoundaryGeometryFor(membrane) == BoundaryGeometry::kSphere;
  }

  static void SweepSeparationComplexRotation(
      int sim_itr, int pro1_index, Parameters& params,
      std::vector<Molecule>& molecule_list,
      std::vector<Complex>& complex_list,
      const std::vector<ForwardRxn>& forward_rxns,
      const std::vector<MolTemplate>& mol_template_list,
      const Membrane& membrane) {
    if (UsesSphericalBoundary(membrane)) {
      sweep_separation_complex_rot_sphere(
          sim_itr, pro1_index, params, molecule_list, complex_list,
          forward_rxns, mol_template_list, membrane);
      return;
    }
    sweep_separation_complex_rot_box(
        sim_itr, pro1_index, params, molecule_list, complex_list,
        forward_rxns, mol_template_list, membrane);
  }

  static void SweepSeparationComplexRotationMembrane(
      int sim_itr, int pro1_index, Parameters& params,
      std::vector<Molecule>& molecule_list,
      std::vector<Complex>& complex_list,
      const std::vector<ForwardRxn>& forward_rxns,
      const std::vector<MolTemplate>& mol_template_list,
      const Membrane& membrane) {
    if (UsesSphericalBoundary(membrane)) {
      sweep_separation_complex_rot_memtest_sphere(
          sim_itr, pro1_index, params, molecule_list, complex_list,
          forward_rxns, mol_template_list, membrane);
      return;
    }
    sweep_separation_complex_rot_memtest_box(
        sim_itr, pro1_index, params, molecule_list, complex_list,
        forward_rxns, mol_template_list, membrane);
  }

  static void SweepSeparationComplexRotationMembraneCluster(
      int sim_itr, int pro1_index, Parameters& params,
      std::vector<Molecule>& molecule_list,
      std::vector<Complex>& complex_list,
      const std::vector<ForwardRxn>& forward_rxns,
      const std::vector<MolTemplate>& mol_template_list,
      const Membrane& membrane) {
    if (UsesSphericalBoundary(membrane)) {
      sweep_separation_complex_rot_memtest_cluster_sphere(
          sim_itr, pro1_index, params, molecule_list, complex_list,
          forward_rxns, mol_template_list, membrane);
      return;
    }
    sweep_separation_complex_rot_memtest_cluster_box(
        sim_itr, pro1_index, params, molecule_list, complex_list,
        forward_rxns, mol_template_list, membrane);
  }
};

} // namespace core
} // namespace nerdss
