#include "core/probability_engine.hpp"
#include "system_setup/system_setup.hpp"
#include "tracing.hpp"

void set_rMaxLimit(Parameters &params,
                   const std::vector<MolTemplate> &molTemplateList,
                   const std::vector<ForwardRxn> &forwardRxns,
                   int numDoubleBeforeAdd, int numMolTemplateBeforeAdd) {
  /*For each reaction, need distance from the interface to the COM for both
   * partners, plus the bindrad+sqrt(6*Dtot*deltat)
   */
  double rMaxTot{0};
  params.rMaxLimit = 0.0;
  for (auto &oneRxn : forwardRxns) {
    if (oneRxn.rxnType == ReactionType::bimolecular) {
      const RxnIface &rxnIface1 = oneRxn.reactantListNew.at(0);
      const RxnIface &rxnIface2 = oneRxn.reactantListNew.at(1);

      // MolTemplates of the interfaces involved in the reaction
      const MolTemplate &pro1Temp = molTemplateList.at(rxnIface1.molTypeIndex);
      const MolTemplate &pro2Temp = molTemplateList.at(rxnIface2.molTypeIndex);

      Interface iface1;
      Interface iface2;
      if (rxnIface1.molTypeIndex < numMolTemplateBeforeAdd) {
        iface1 = molTemplateList.at(rxnIface1.molTypeIndex)
                     .interfaceList.at(MolTemplate::absToRelIface.at(
                         rxnIface1.absIfaceIndex));
      } else {
        iface1 = molTemplateList.at(rxnIface1.molTypeIndex)
                     .interfaceList.at(MolTemplate::absToRelIface.at(
                         rxnIface1.absIfaceIndex - numDoubleBeforeAdd));
      }
      if (rxnIface2.molTypeIndex < numMolTemplateBeforeAdd) {
        iface2 = molTemplateList.at(rxnIface2.molTypeIndex)
                     .interfaceList.at(MolTemplate::absToRelIface.at(
                         rxnIface2.absIfaceIndex));
      } else {
        iface2 = molTemplateList.at(rxnIface2.molTypeIndex)
                     .interfaceList.at(MolTemplate::absToRelIface.at(
                         rxnIface2.absIfaceIndex - numDoubleBeforeAdd));
      }

      double Dtot{nerdss::core::ProbabilityEngine::MeanTranslationalDiffusion3D(
          pro1Temp.D.x, pro1Temp.D.y, pro1Temp.D.z, pro2Temp.D.x,
          pro2Temp.D.y, pro2Temp.D.z)};
      // TODO: add rotational diffusion contribution

      /*Now calculate distance from the interface to the protein COM.*/
      double pro1R1{0};
      double pro2R1{0};
      // pro1
      if (pro1Temp.isPromoter) {
        pro1R1 = nerdss::core::ProbabilityEngine::InterfaceRadius1D(
            iface1.iCoord.x);
      }
      // if (std::abs(pro1Temp.D.z) < 1E-10) {
      if (pro1Temp.isImplicitLipid || pro1Temp.isLipid) {
        double R2 = (iface1.iCoord.x * iface1.iCoord.x) +
                    (iface1.iCoord.y * iface1.iCoord.y);
        pro1R1 = nerdss::core::ProbabilityEngine::InterfaceRadius2D(
            iface1.iCoord.x, iface1.iCoord.y);
        Dtot += nerdss::core::ProbabilityEngine::RotationalDiffusionContribution(
            pro1Temp.Dr.z, params.timeStep, R2, 2);
      } else {
        double R2 = (iface1.iCoord.x * iface1.iCoord.x) +
                    (iface1.iCoord.y * iface1.iCoord.y) +
                    (iface1.iCoord.z * iface1.iCoord.z);
        pro1R1 = nerdss::core::ProbabilityEngine::InterfaceRadius3D(
            iface1.iCoord.x, iface1.iCoord.y, iface1.iCoord.z);
        Dtot += nerdss::core::ProbabilityEngine::RotationalDiffusionContribution(
            pro1Temp.Dr.z, params.timeStep, R2, 3);
      }

      // pro2
      if (pro2Temp.isPromoter) {
        pro2R1 = nerdss::core::ProbabilityEngine::InterfaceRadius1D(
            iface2.iCoord.x);
      }
      // if (std::abs(pro2Temp.D.z) < 1E-10) {
      else if (pro2Temp.isImplicitLipid || pro2Temp.isLipid) {
        double R2 = (iface2.iCoord.x * iface2.iCoord.x) +
                    (iface2.iCoord.y * iface2.iCoord.y);
        pro2R1 = nerdss::core::ProbabilityEngine::InterfaceRadius2D(
            iface2.iCoord.x, iface2.iCoord.y);
        Dtot += nerdss::core::ProbabilityEngine::RotationalDiffusionContribution(
            pro2Temp.Dr.z, params.timeStep, R2, 2);
      } else {
        double R2 = (iface2.iCoord.x * iface2.iCoord.x) +
                    (iface2.iCoord.y * iface2.iCoord.y) +
                    (iface2.iCoord.z * iface2.iCoord.z);
        pro2R1 = nerdss::core::ProbabilityEngine::InterfaceRadius3D(
            iface2.iCoord.x, iface2.iCoord.y, iface2.iCoord.z);
        Dtot += nerdss::core::ProbabilityEngine::RotationalDiffusionContribution(
            pro2Temp.Dr.z, params.timeStep, R2, 3);
      }

      rMaxTot = nerdss::core::ProbabilityEngine::SetupRMaxLimit3D(
          Dtot, params.timeStep, oneRxn.bindRadius, pro1R1, pro2R1);
      if (rMaxTot > params.rMaxLimit) {
        params.rMaxLimit = rMaxTot;
        params.rMaxRadius = pro1R1 + pro2R1;
      }
    }
  }
  if (forwardRxns.size() == 0) {
    params.rMaxLimit = 40.0;
  }
  std::cout << "Rmaxlimit: " << params.rMaxLimit << std::endl;
}
