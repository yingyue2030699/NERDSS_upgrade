#include "reactions/association/association.hpp"

#include "core/math_engine.hpp"

void determine_rotation_angles(double targAngle, double currAngle, double& rotAngPos, double& rotAngNeg,
    const Complex& reactCom1, const Complex& reactCom2)
{
    double totDrx { reactCom1.Dr.x + reactCom2.Dr.x };
    
    double tol=1e-11;
    
    // if the molecules are both on the membrane, use relative Dx components.
    // if (reactCom1.D.z<tol && reactCom2.D.z<tol) {
    if (reactCom1.OnSurface && reactCom2.OnSurface) {
      const auto partition = nerdss::core::MathEngine::PartitionRotationAngle(
          targAngle, currAngle, reactCom1.D.x, reactCom2.D.x);
      rotAngPos = partition.positive_angle;
      rotAngNeg = partition.negative_angle;
    } else {
      if (totDrx < tol) {
        // no rotation, use translation in z (correct this below if Dz==0 for
        // both molecules.
        double D1 { (reactCom1.D.x + reactCom1.D.y + reactCom1.D.z)/3 };
        double D2 { (reactCom2.D.x + reactCom2.D.y + reactCom2.D.z)/3 };
        // double totDz { reactCom1.D.z + reactCom2.D.z };
        const auto partition = nerdss::core::MathEngine::PartitionRotationAngle(
            targAngle, currAngle, D1, D2);
        rotAngPos = partition.positive_angle;
        rotAngNeg = partition.negative_angle;

      } else {
        const auto partition = nerdss::core::MathEngine::PartitionRotationAngle(
            targAngle, currAngle, reactCom1.Dr.x, reactCom2.Dr.x);
        rotAngPos = partition.positive_angle;
        rotAngNeg = partition.negative_angle;
      }
    }
}
