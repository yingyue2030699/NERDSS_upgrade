/*
 * ### Created on 2/05/2020 by Yiben Fu
 * ### Purpose
 * ***
 * all functions that are used in spherical systems
 */

#include "classes/class_Membrane.hpp"
#include "classes/class_Molecule_Complex.hpp"
#include "classes/class_Vector.hpp"
#include "core/math_engine.hpp"
#include "reactions/association/functions_for_spherical_system.hpp"

#include <array>
#include <math.h>

double radius(Coord mol)
{
    return nerdss::core::MathEngine::Radius(mol);
}

Coord find_spherical_coords(Coord mol) // mol: cardesian coords, output spherical coords
{
    return nerdss::core::MathEngine::SphericalFromCartesian(mol);
}

Coord find_cardesian_coords(Coord mol) // mol: spherical coords, output cardesian coords
{
    return nerdss::core::MathEngine::CartesianFromSpherical(mol);
}

double theta_plus(double theta1, double theta2) // sum of two theta
{
    return nerdss::core::MathEngine::ThetaPlus(theta1, theta2);
}

double phi_plus(double phi1, double phi2) // sum of two phi
{
    return nerdss::core::MathEngine::PhiPlus(phi1, phi2);
}

Coord angle_plus(Coord angle1, Coord angle2)
{
    return nerdss::core::MathEngine::SphericalAnglePlus(angle1, angle2);
}

Coord find_position_after_association(double arc1, Coord Iface1, Coord Iface2, double arc_total, double bindRadius)
{
    Coord new_position1 = nerdss::core::MathEngine::AssociationPositionOnSphere(
        arc1, Iface1, Iface2, arc_total, bindRadius);
    if (std::isnan(new_position1.x) || std::isnan(new_position1.y)) {
        std::cout << "WRONG: non position is generated in 'find_position_after_association'...EXIT! " << std::endl;
        exit(1);
    }
    return new_position1;
}

/*dtheta is the polar angle change, not the solid angle
  COM and targ has already been moved along the azimuth by dphi.
*/
// COM COMnew are cardeseian coords
std::array<double, 9> inner_coord_set(Coord com, Coord comnew)
{
    return nerdss::core::MathEngine::InnerCoordinateFrame(com, comnew);
}
std::array<double, 9> inner_coord_set_new(Coord com, Coord comnew)
{
    return nerdss::core::MathEngine::UpdatedInnerCoordinateFrame(com, comnew);
}
// crdset is the previous one, not the new or updated one
std::array<double, 3> calculate_inner_coord_coefficients(Coord TARG, Coord COM, std::array<double, 9> crdset)
{
    return nerdss::core::MathEngine::InnerCoordinateCoefficients(TARG, COM, crdset);
}

// input and output are cardesian coords
Coord translate_on_sphere(Coord targ, Coord COM, Coord COMnew, std::array<double, 9> crdset, std::array<double, 9> crdsetnew)
{
    return nerdss::core::MathEngine::TranslateOnSphere(targ, COM, COMnew, crdset, crdsetnew);
}

// input and output are cardesian coords
Coord rotate_on_sphere(Coord Targ, Coord COM, std::array<double, 9> crdset, double dangle)
{
    Coord targnew = nerdss::core::MathEngine::RotateOnSphere(Targ, COM, crdset, dangle);
    if (std::isnan(targnew.x)) {
        std::cout << "WRONG! NON is generated after the rotation on sphere! EXIT..." << std::endl;
        exit(1);
    }
    return targnew;
}

double calc_bindRadius2D(double bindRadius, Coord iFace)
{
    return nerdss::core::MathEngine::BindingRadiusOnSphere(bindRadius, iFace);
}

void set_memProtein_sphere(Complex reactCom, Molecule& memProtein, std::vector<Molecule> moleculeList, const Membrane membraneObject)
{
    //if (membraneObject.implicitLipid == false){ //for explicit lipid model; lipid is a member of reactCom
    double r = 0.0;
    for (auto mol : reactCom.memberList) {
        if ((moleculeList[mol].isLipid == true || moleculeList[mol].isImplicitLipid == true) && moleculeList[mol].tmpComCoord.get_magnitude() > r) {
            memProtein = moleculeList[mol];
            r = moleculeList[mol].tmpComCoord.get_magnitude();
        }
    }
    memProtein.comCoord = memProtein.tmpComCoord;
    for (int i = 0; i < memProtein.interfaceList.size(); i++) { // here memProtein is an Lipid, has only one interface
        Coord ifaceToCom = memProtein.tmpICoords[i] - memProtein.tmpComCoord;
        double bond = ifaceToCom.get_magnitude();
        double R = memProtein.comCoord.get_magnitude();
        memProtein.interfaceList[i].coord = (R - bond) / R * memProtein.comCoord;
    }

    // for implicit lipid model, on 2D->2D case, reactCom has no implicitlipid member.
    if (memProtein.isLipid == false && memProtein.isImplicitLipid == false && membraneObject.implicitLipid == true) {
        Coord iface;
        Coord com;
        r = 0.0;
        for (auto mol : reactCom.memberList) {
            for (int i = 0; i < moleculeList[mol].interfaceList.size(); i++) {
                if (moleculeList[mol].interfaceList[i].isBound == true) {
                    int index = moleculeList[mol].interfaceList[i].interaction.partnerIndex;
                    if (moleculeList[index].isImplicitLipid == true && moleculeList[mol].tmpICoords[i].get_magnitude() > r) {
                        memProtein = moleculeList[index];
                        com = moleculeList[mol].tmpComCoord;
                        iface = moleculeList[mol].tmpICoords[i];
                        r = moleculeList[mol].tmpICoords[i].get_magnitude();
                    }
                }
            }
        }
        memProtein.comCoord = iface; // here targ is an ImplicitLipid, has only one interface
        Coord ifaceToCom = iface - com;
        double bond = ifaceToCom.get_magnitude();
        double R = memProtein.comCoord.get_magnitude();
        memProtein.interfaceList[0].coord = (R - bond) / R * memProtein.comCoord;
    }
    if (memProtein.isLipid == false && memProtein.isImplicitLipid == false) {
        std::cout << "WRONG: failed to create memProtein, in the step to adjust complex's orientation on sphere. Exit..." << std::endl;
        exit(1);
    }
    //memProtein.comCoord =  (memProtein.comCoord.get_magnitude() + 0.1) / memProtein.comCoord.get_magnitude()  * memProtein.comCoord;
    //memProtein.interfaceList[0].coord = (memProtein.interfaceList[0].coord.get_magnitude() + 0.1)/memProtein.interfaceList[0].coord.get_magnitude() *  memProtein.interfaceList[0].coord;
}
void find_Lipid_sphere(Complex reactCom, Molecule& Lipid, std::vector<Molecule> moleculeList, const Membrane membraneObject)
{
    //if (membraneObject.implicitLipid == false){ //for explicit lipid model; lipid is a member of reactCom
    double r = 0.0;
    for (auto mol : reactCom.memberList) {
        if ((moleculeList[mol].isLipid == true || moleculeList[mol].isImplicitLipid == true) && moleculeList[mol].tmpComCoord.get_magnitude() > r) {
            Lipid = moleculeList[mol];
            r = moleculeList[mol].tmpComCoord.get_magnitude();
        }
    }
    // for implicit lipid model, on 2D->2D case, reactCom has no implicitlipid member.
    if (Lipid.isLipid == false && Lipid.isImplicitLipid == false && membraneObject.implicitLipid == true) {
        r = 0.0;
        Coord iface;
        Coord com;
        for (auto mol : reactCom.memberList) {
            for (int i = 0; i < moleculeList[mol].interfaceList.size(); i++) {
                if (moleculeList[mol].interfaceList[i].isBound == true) {
                    int index = moleculeList[mol].interfaceList[i].interaction.partnerIndex;
                    if (moleculeList[index].isImplicitLipid == true && moleculeList[mol].tmpICoords[i].get_magnitude() > r) {
                        Lipid = moleculeList[index];
                        com = moleculeList[mol].tmpComCoord;
                        iface = moleculeList[mol].tmpICoords[i];
                        r = moleculeList[mol].tmpICoords[i].get_magnitude();
                    }
                }
            }
        }
        Lipid.set_tmp_association_coords();
        Lipid.comCoord = iface; // here targ is an ImplicitLipid, has only one interface
        Lipid.tmpComCoord = iface;
        Lipid.interfaceList[0].coord = com;
        Lipid.tmpICoords[0] = com;
    }

    if (Lipid.isLipid == false && Lipid.isImplicitLipid == false) {
        std::cout << "WRONG: failed to create memProtein, in the step to adjust complex's orientation on sphere. Exit..." << std::endl;
        exit(1);
    }

    //Lipid.comCoord = ( Lipid.comCoord.get_magnitude()+ 0.1)/Lipid.comCoord.get_magnitude() * Lipid.comCoord;
    //Lipid.tmpComCoord = ( Lipid.tmpComCoord.get_magnitude()+ 0.1)/Lipid.tmpComCoord.get_magnitude() * Lipid.tmpComCoord;
    //Lipid.interfaceList[0].coord = ( Lipid.interfaceList[0].coord.get_magnitude()+ 0.1)/Lipid.interfaceList[0].coord.get_magnitude() * Lipid.interfaceList[0].coord;
    //Lipid.tmpICoords[0] = ( Lipid.tmpICoords[0].get_magnitude()+ 0.1)/Lipid.tmpICoords[0].get_magnitude() * Lipid.tmpICoords[0];
}
