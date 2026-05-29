#include "core/math_engine.hpp"
#include "reactions/association/association.hpp"
#include "tracing.hpp"

double calculate_phi(Coord reactIface1, int ifaceIndex2, Molecule reactMol1, Molecule reactMol2, const Vector& normal,
    Vector axis, const ForwardRxn& currRxn, const std::vector<MolTemplate>& molTemplateList)
{
    // TRACE();
    // coordinate transform along com-iface vector
    transform(reactIface1, reactMol1, reactMol2, axis);

    // orthographic projection onto xy-plane
    Vector vec1 { reactIface1 - reactMol2.tmpICoords[ifaceIndex2] }; //iface1-iface2= sigma
    Vector vec2 { determine_normal(normal, molTemplateList[reactMol1.molTypeIndex], reactMol1) };

    return nerdss::core::MathEngine::SignedProjectedAngleOnXY(vec1, vec2, 1E-12);
}
