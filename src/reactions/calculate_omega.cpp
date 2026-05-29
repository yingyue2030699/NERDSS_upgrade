#include "core/math_engine.hpp"
#include "reactions/association/association.hpp"
#include "tracing.hpp"

double calculate_omega(Coord reactIface1, int reactIface2, Vector& sigma,
    const ForwardRxn& currRxn, Molecule reactMol1, Molecule reactMol2, const std::vector<MolTemplate>& molTemplateList)
{
    // TRACE();
    /*Re-aligns the molecules so that Sigma faces purely along the z-axis. 
     */
    transform(reactIface1, reactMol1, reactMol2, sigma);

    Vector v1 {};
    Vector v2 {};

    if (areSameAngle(currRxn.assocAngles.theta1, M_PI) || areSameAngle(currRxn.assocAngles.theta2, M_PI)) {
        v1 = determine_normal(currRxn.norm1, molTemplateList[reactMol1.molTypeIndex], reactMol1);
        v2 = determine_normal(currRxn.norm2, molTemplateList[reactMol2.molTypeIndex], reactMol2);
    } else {
        v1 = Vector(reactIface1 - reactMol1.tmpComCoord);
        v2 = Vector(reactMol2.tmpICoords[reactIface2] - reactMol2.tmpComCoord);
    }

    return nerdss::core::MathEngine::SignedProjectedAngleOnXY(v1, v2, 1E-11);
}
