#include "reactions/association/association.hpp"

#include "core/math_engine.hpp"

void determine_rotation_angles(double targAngle, double currAngle, double& rotAngPos, double& rotAngNeg,
    const Complex& reactCom1, const Complex& reactCom2)
{
    const nerdss::core::AssociationRotationDiffusion first {
        reactCom1.D.x, reactCom1.D.y, reactCom1.D.z, reactCom1.Dr.x,
        reactCom1.OnSurface
    };
    const nerdss::core::AssociationRotationDiffusion second {
        reactCom2.D.x, reactCom2.D.y, reactCom2.D.z, reactCom2.Dr.x,
        reactCom2.OnSurface
    };
    const auto partition =
        nerdss::core::MathEngine::PartitionAssociationRotationAngle(
            targAngle, currAngle, first, second);
    rotAngPos = partition.positive_angle;
    rotAngNeg = partition.negative_angle;
}
