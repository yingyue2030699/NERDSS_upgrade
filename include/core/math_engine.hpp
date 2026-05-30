/*! \file math_engine.hpp
 * \brief Cohesive facade for CPU scalar math primitives used by core NERDSS.
 */

#pragma once

#include "classes/class_Coord.hpp"
#include "classes/class_Quat.hpp"
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

  static Coordinate3 AssociationPositionOnSphere(Scalar arc,
                                                 Coordinate3 interface1,
                                                 Coordinate3 interface2,
                                                 Scalar total_arc,
                                                 Scalar binding_radius) {
    const Scalar radius { Radius(interface1) };
    Scalar normal_x { 1.0 };
    Scalar normal_y {
        (interface2.x * interface1.z - interface1.x * interface2.z)
        / (interface1.y * interface2.z - interface2.y * interface1.z) };
    Scalar normal_z {
        (interface2.x * interface1.y - interface1.x * interface2.y)
        / (interface1.z * interface2.y - interface2.z * interface1.y) };
    const Scalar normal_magnitude {
        std::sqrt(normal_x * normal_x + normal_y * normal_y
                  + normal_z * normal_z) };
    normal_x /= normal_magnitude;
    normal_y /= normal_magnitude;
    normal_z /= normal_magnitude;

    arc = std::abs(arc);
    const Scalar a1 { normal_z * interface1.x - normal_x * interface1.z };
    const Scalar a11 { radius * radius * normal_z * std::cos(arc / radius) };
    const Scalar a2 { normal_y * interface1.x - normal_x * interface1.y };
    const Scalar a22 { radius * radius * normal_y * std::cos(arc / radius) };
    const Scalar a3 { normal_y * interface1.z - normal_z * interface1.y };
    const Scalar quadratic_a { a1 * a1 + a2 * a2 + a3 * a3 };
    const Scalar quadratic_b { -2.0 * (a1 * a11 + a2 * a22) };
    const Scalar quadratic_c {
        a11 * a11 + a22 * a22 - a3 * a3 * radius * radius };
    Scalar discriminant {
        quadratic_b * quadratic_b - 4.0 * quadratic_a * quadratic_c };
    if (discriminant < 0.0) {
      discriminant = 0.0;
    }

    const Scalar sqrt_discriminant { std::sqrt(discriminant) };
    const Scalar x1 {
        0.5 / quadratic_a * (-quadratic_b + sqrt_discriminant) };
    const Scalar y1 { (a1 * x1 - a11) / a3 };
    const Scalar z1 { -(a2 * x1 - a22) / a3 };
    const Scalar distance1 {
        std::sqrt(std::pow(x1 - interface2.x, 2.0)
                  + std::pow(y1 - interface2.y, 2.0)
                  + std::pow(z1 - interface2.z, 2.0)) };

    const Scalar x2 {
        0.5 / quadratic_a * (-quadratic_b - sqrt_discriminant) };
    const Scalar y2 { (a1 * x2 - a11) / a3 };
    const Scalar z2 { -(a2 * x2 - a22) / a3 };
    const Scalar distance2 {
        std::sqrt(std::pow(x2 - interface2.x, 2.0)
                  + std::pow(y2 - interface2.y, 2.0)
                  + std::pow(z2 - interface2.z, 2.0)) };

    if ((binding_radius < total_arc && distance1 < distance2)
        || (binding_radius >= total_arc && distance1 > distance2)) {
      return { x1, y1, z1 };
    }
    return { x2, y2, z2 };
  }

  static Scalar GeodesicDistance(Coordinate3 first, Coordinate3 second) {
    const Scalar first_radius { Radius(first) };
    const Scalar second_radius { Radius(second) };
    const Scalar dot_product {
        first.x * second.x + first.y * second.y + first.z * second.z };
    const Scalar theta {
        std::acos(dot_product / (first_radius * second_radius)) };
    const Scalar mean_radius { (first_radius + second_radius) / 2.0 };
    return mean_radius * theta;
  }

  static bool AreAnglesNearlyEqual(Scalar angle1, Scalar angle2) {
    return std::abs(angle1 - angle2) < 1.0e-4;
  }

  static bool IsParallelAngle(Scalar angle) {
    const Scalar pi { 3.141592653589793238462643383279502884 };
    return angle == pi || angle == 0.0;
  }

  static bool IsAngleSignCorrect(const Vector3& vector1,
                                 const Vector3& vector2) {
    if (std::abs(vector1.z) > 1.0e-12
        && std::abs(vector2.z) > 1.0e-12) {
      Vector3 projected1 { vector1.x, 0.0, vector1.z };
      Vector3 projected2 { vector2.x, 0.0, vector2.z };
      return projected1.cross(projected2).y < 0.0;
    }

    Vector3 projected1 { vector1.x, vector1.y, 0.0 };
    Vector3 projected2 { vector2.x, vector2.y, 0.0 };
    return projected1.cross(projected2).z < 0.0;
  }

  static Scalar SignedProjectedAngleOnXY(Vector3 vector1, Vector3 vector2,
                                         Scalar tolerance) {
    const Scalar pi { 3.141592653589793238462643383279502884 };
    Vector3 projected1 { vector1.x, vector1.y, 0.0 };
    Vector3 projected2 { vector2.x, vector2.y, 0.0 };
    projected1.calc_magnitude();
    projected2.calc_magnitude();
    const Vector3 sign { projected1.cross(projected2) };
    Scalar angle { projected1.dot_theta(projected2) };
    if (sign.z > 0.0 && std::abs(angle) > tolerance
        && (pi - std::abs(angle)) > tolerance) {
      angle = -angle;
    }
    return angle;
  }

  static Scalar SignedAngularDisplacement(Vector3 original, Vector3 displaced,
                                          Scalar tolerance) {
    displaced.calc_magnitude();
    if (displaced.magnitude < tolerance) {
      return 0.0;
    }
    original.calc_magnitude();

    Scalar angle { original.dot_theta(displaced) };
    const Vector3 sign { original.cross(displaced) };
    const Scalar pi { 3.141592653589793238462643383279502884 };
    if (sign.z > 0.0 && std::abs(angle) > tolerance
        && (pi - std::abs(angle)) > tolerance) {
      angle = -angle;
    }
    return angle;
  }

  static Vector3 CreateArbitraryOrthogonalVector(Vector3 vector) {
    const Scalar pi { 3.141592653589793238462643383279502884 };
    Vector3 x_axis { 1.0, 0.0, 0.0 };
    Vector3 y_axis { 0.0, 1.0, 0.0 };
    vector.normalize();

    return (vector.dot_theta(x_axis) != 0.0 && vector.dot_theta(x_axis) != pi)
        ? Vector3(vector).cross(x_axis)
        : Vector3(vector).cross(y_axis);
  }

  static bool RequiresSignFlip(Vector3 axis, Vector3 vector1,
                               Vector3 vector2) {
    Vector3 z_axis { 0.0, 0.0, 1.0 };
    Vector3 x_axis { 1.0, 0.0, 0.0 };
    z_axis.magnitude = 1.0;
    x_axis.magnitude = 1.0;
    Vector3 rotation_axis { z_axis.cross(axis) };
    rotation_axis.calc_magnitude();
    Scalar theta { z_axis.dot_theta(axis) };
    bool use_x_axis { false };
    if (std::abs(rotation_axis.x) < 1.0e-8
        && std::abs(rotation_axis.y) < 1.0e-8
        && std::abs(rotation_axis.z) < 1.0e-8) {
      rotation_axis = x_axis.cross(axis);
      rotation_axis.calc_magnitude();
      theta = x_axis.dot_theta(axis);
      use_x_axis = true;
    }

    Quat rotation(std::cos(theta / 2.0),
                  std::sin(theta / 2.0) * rotation_axis.x,
                  std::sin(theta / 2.0) * rotation_axis.y,
                  std::sin(theta / 2.0) * rotation_axis.z);
    rotation = rotation.unit();
    rotation.rotate(vector1);
    rotation.rotate(vector2);
    rotation.rotate(axis);

    if ((z_axis.dot_theta(axis) > 0.01 && !use_x_axis)
        || (use_x_axis && x_axis.dot_theta(axis) < 0.01)) {
      rotation = rotation.inverse();
      rotation.rotate(vector1);
      rotation.rotate(vector2);
      rotation = Quat(std::cos(-theta / 2.0),
                      std::sin(-theta / 2.0) * rotation_axis.x,
                      std::sin(-theta / 2.0) * rotation_axis.y,
                      std::sin(-theta / 2.0) * rotation_axis.z);
      rotation.rotate(vector1);
      rotation.rotate(vector2);
    }

    if (!use_x_axis) {
      Vector3 projected1 { vector1.x, vector1.y, 0.0 };
      Vector3 projected2 { vector2.x, vector2.y, 0.0 };
      return projected1.cross(projected2).z > 0.0;
    }
    Vector3 projected1 { 0.0, vector1.y, vector1.z };
    Vector3 projected2 { 0.0, vector2.y, vector2.z };
    return projected1.cross(projected2).x > 0.0;
  }
};

} // namespace core
} // namespace nerdss
