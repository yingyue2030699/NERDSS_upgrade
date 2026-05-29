/*! \file math_engine.hpp
 * \brief Cohesive facade for CPU scalar math primitives used by core NERDSS.
 */

#pragma once

#include "classes/class_Coord.hpp"
#include "classes/class_Vector.hpp"
#include "math/matrix.hpp"

#include <array>
#include <cmath>

namespace nerdss {
namespace core {

using Scalar = double;
using Coordinate3 = ::Coord;
using Vector3 = ::Vector;
using Matrix3 = std::array<Scalar, 9>;

enum class MathBackend {
  kCpuScalar,
};

class MathEngine {
public:
  static MathBackend backend() { return MathBackend::kCpuScalar; }

  static Scalar Dot(const Vector3& lhs, const Vector3& rhs) {
    return lhs.dot(rhs);
  }

  static Vector3 Cross(const Vector3& lhs, const Vector3& rhs) {
    return lhs.cross(rhs);
  }

  static Scalar Magnitude(Coordinate3 coordinate) {
    return coordinate.get_magnitude();
  }

  static Matrix3 CreateEulerRotationMatrix(Scalar x, Scalar y, Scalar z) {
    return ::create_euler_rotation_matrix(x, y, z);
  }

  static Matrix3 CreateEulerRotationMatrix(const Coordinate3& angles) {
    return ::create_euler_rotation_matrix(angles);
  }

  static Scalar Radius(Coordinate3 coordinate) {
    return std::sqrt(coordinate.x * coordinate.x + coordinate.y * coordinate.y
                     + coordinate.z * coordinate.z);
  }

  static Coordinate3 SphericalFromCartesian(Coordinate3 coordinate) {
    const Scalar pi { 3.141592653589793238462643383279502884 };
    Coordinate3 angles;
    const Scalar radius { Radius(coordinate) };
    if (coordinate.z == radius) {
      angles.x = 0.0;
      angles.y = 0.0;
      angles.z = radius;
    } else if (coordinate.z == -radius) {
      angles.x = -pi;
      angles.y = 0.0;
      angles.z = radius;
    } else {
      const Scalar theta { std::acos(coordinate.z / radius) };
      Scalar phi { std::acos(coordinate.x / (radius * std::sin(theta))) };
      if (std::isnan(phi)) {
        phi = 0.0;
      }
      if (coordinate.y < 0) {
        phi = 2.0 * pi - phi;
      }
      angles.x = theta;
      angles.y = phi;
      angles.z = radius;
    }
    return angles;
  }

  static Coordinate3 CartesianFromSpherical(Coordinate3 spherical) {
    Coordinate3 xyz;
    xyz.x = spherical.z * std::sin(spherical.x) * std::cos(spherical.y);
    xyz.y = spherical.z * std::sin(spherical.x) * std::sin(spherical.y);
    xyz.z = spherical.z * std::cos(spherical.x);
    return xyz;
  }

  static Scalar ThetaPlus(Scalar theta1, Scalar theta2) {
    const Scalar pi { 3.141592653589793238462643383279502884 };
    Scalar sum { theta1 + theta2 };
    if (sum > pi) {
      sum = 2.0 * pi - sum;
    } else if (sum < 0.0) {
      sum = -sum;
    }
    return sum;
  }

  static Scalar PhiPlus(Scalar phi1, Scalar phi2) {
    const Scalar pi { 3.141592653589793238462643383279502884 };
    Scalar sum { phi1 + phi2 };
    if (sum > 2.0 * pi) {
      sum -= 2.0 * pi;
    } else if (sum < 0.0) {
      sum = 2.0 * pi + sum;
    }
    return sum;
  }

  static Coordinate3 SphericalAnglePlus(Coordinate3 angle1,
                                        Coordinate3 angle2) {
    const Scalar pi { 3.141592653589793238462643383279502884 };
    Coordinate3 sum;
    Scalar phi { PhiPlus(angle1.y, angle2.y) };
    Scalar theta { angle1.x + angle2.x };
    if (theta > pi) {
      theta = 2.0 * pi - theta;
      phi = PhiPlus(phi, pi);
    } else if (theta < 0.0) {
      theta = -theta;
      phi = PhiPlus(phi, -pi);
    } else if (theta == 0.0 || theta == pi) {
      phi = 0.0;
    }
    sum.x = theta;
    sum.y = phi;
    sum.z = angle1.z;
    return sum;
  }

  static Scalar BindingRadiusOnSphere(Scalar binding_radius,
                                      Coordinate3 interface_coordinate) {
    const Scalar radius { Radius(interface_coordinate) };
    return radius * 2.0 * std::asin((0.5 * binding_radius) / radius);
  }
};

} // namespace core
} // namespace nerdss
