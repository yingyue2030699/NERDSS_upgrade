#include "core/math_engine.hpp"
#include "reactions/association/association.hpp"

bool conservedRigid(const Complex& targCom, const std::vector<Molecule>& moleculeList)
{
    for (auto& memMol : targCom.memberList) {
        Vector v1_0 { moleculeList[memMol].interfaceList[0].coord - moleculeList[memMol].comCoord };
        Vector v1_1 { moleculeList[memMol].tmpICoords[0] - moleculeList[memMol].tmpComCoord };
        for (unsigned i { 1 }; i < moleculeList[memMol].tmpICoords.size(); ++i) {
            Vector v2_0 { moleculeList[memMol].interfaceList[i].coord - moleculeList[memMol].comCoord };
            Vector v2_1 { moleculeList[memMol].tmpICoords[i] - moleculeList[memMol].tmpComCoord };
            if (!nerdss::core::MathEngine::HaveEqualRoundedAngles(v1_0, v2_0, v1_1, v2_1)) {
                // std::cerr << "IFACE-COM vector " << i << " in protein " << memMol << " of complex " << targCom.index
                //           << " did not conserve its angle to icoord[0]-COM. Exiting..." << std::endl;
                return false;
            }
        }
    }
    return true;
}
