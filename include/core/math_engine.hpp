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

struct RotationAnglePartition {
  Scalar positive_angle {};
  Scalar negative_angle {};

  RotationAnglePartition() = default;
  RotationAnglePartition(Scalar positive, Scalar negative)
      : positive_angle(positive), negative_angle(negative) {}
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

  static Scalar SquaredCoordinateDistance(Coordinate3 first,
                                          Coordinate3 second) {
    const Scalar dx { first.x - second.x };
    const Scalar dy { first.y - second.y };
    const Scalar dz { first.z - second.z };
    return dx * dx + dy * dy + dz * dz;
  }

  static Scalar CoordinateDistance(Coordinate3 first, Coordinate3 second) {
    return std::sqrt(SquaredCoordinateDistance(first, second));
  }

  static Scalar PlanarDistanceXY(Coordinate3 first, Coordinate3 second) {
    const Scalar dx { first.x - second.x };
    const Scalar dy { first.y - second.y };
    return std::sqrt(dx * dx + dy * dy);
  }

  static Scalar DistanceToSphereSurface(Coordinate3 coordinate,
                                        Scalar sphere_radius) {
    return std::abs(sphere_radius - Magnitude(coordinate));
  }

  static Scalar DistanceToPlaneZ(Coordinate3 coordinate, Scalar plane_z) {
    return std::abs(coordinate.z - plane_z);
  }

  static bool HaveEqualRoundedMagnitudes(Vector3 vector1, Vector3 vector2) {
    vector1.calc_magnitude();
    vector2.calc_magnitude();
    return roundv(vector1.magnitude) == roundv(vector2.magnitude);
  }

  static bool HaveEqualRoundedAngles(Vector3 reference1, Vector3 vector1,
                                     Vector3 reference2, Vector3 vector2) {
    reference1.calc_magnitude();
    vector1.calc_magnitude();
    reference2.calc_magnitude();
    vector2.calc_magnitude();
    return roundv(reference1.dot_theta(vector1))
           == roundv(reference2.dot_theta(vector2));
  }

  static Matrix3 CreateEulerRotationMatrix(Scalar x, Scalar y, Scalar z) {
    return ::create_euler_rotation_matrix(x, y, z);
  }

  static Matrix3 CreateEulerRotationMatrix(const Coordinate3& angles) {
    return ::create_euler_rotation_matrix(angles);
  }

  static Vector3 RotateVector(const Vector3& vector, const Matrix3& matrix) {
    return { matrix[0] * vector.x + matrix[1] * vector.y + matrix[2] * vector.z,
             matrix[3] * vector.x + matrix[4] * vector.y + matrix[5] * vector.z,
             matrix[6] * vector.x + matrix[7] * vector.y + matrix[8] * vector.z };
  }

  static long double Factorial(unsigned n) {
    return n == 0 ? 1.0L : n * Factorial(n - 1);
  }

  static Scalar LogGammaNumericalRecipes(Scalar n) {
    Scalar x { 0.0 };
    Scalar y { 0.0 };
    Scalar tmp { 0.0 };
    Scalar ser { 0.0 };
    static Scalar cof[6] = { 76.18009172947146, -86.50532032941677,
                             24.01409824083091, -1.231739572450155,
                             0.1208650973866179e-2,
                             -0.5395239384953e-5 };
    y = x = n;
    tmp = x + 5.5;
    tmp -= (x + 0.5) * std::log(tmp);
    ser = 1.000000000190015;
    for (int j { 0 }; j <= 5; j++) {
      ser += cof[j] / ++y;
    }
    return -tmp + std::log(2.5066282746310005 * ser / x);
  }

  static Scalar GammaFactorial(unsigned n) {
    static int ntop = 4;
    static float a[33] = { 1.0, 1.0, 2.0, 6.0, 24.0 };

    if (n > 32) {
      return std::exp(LogGammaNumericalRecipes(n + 1.0));
    }

    while (static_cast<unsigned>(ntop) < n) {
      const int j { ntop++ };
      a[ntop] = a[j] * ntop;
    }

    return a[n];
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
        CoordinateDistance({ x1, y1, z1 }, interface2) };

    const Scalar x2 {
        0.5 / quadratic_a * (-quadratic_b - sqrt_discriminant) };
    const Scalar y2 { (a1 * x2 - a11) / a3 };
    const Scalar z2 { -(a2 * x2 - a22) / a3 };
    const Scalar distance2 {
        CoordinateDistance({ x2, y2, z2 }, interface2) };

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

  static Matrix3 InnerCoordinateFrame(Coordinate3 center,
                                      Coordinate3 new_center) {
    Matrix3 frame {};
    Vector3 i;
    Vector3 j;
    Vector3 k;
    Vector3 v;

    if (CoordinateDistance(center, new_center) < 1.0e-8) {
      i = Vector3 { center.x, center.y, center.z };
      i.normalize();
      v = Vector3 { 0.0, 0.0, 1.0 };
      if (std::abs(std::abs(center.z) - Magnitude(center)) < 1.0e-8) {
        v = Vector3 { -1.0, 0.0, 0.0 };
      }
      j = v.cross(i);
      k = i.cross(j);
    } else {
      i = Vector3 { center.x, center.y, center.z };
      i.normalize();
      v = Vector3 { new_center.x, new_center.y, new_center.z };
      v.normalize();
      k = i.cross(v);
      j = k.cross(i);
    }

    i.normalize();
    j.normalize();
    k.normalize();

    frame[0] = i.x;
    frame[1] = i.y;
    frame[2] = i.z;
    frame[3] = j.x;
    frame[4] = j.y;
    frame[5] = j.z;
    frame[6] = k.x;
    frame[7] = k.y;
    frame[8] = k.z;
    return frame;
  }

  static Matrix3 UpdatedInnerCoordinateFrame(Coordinate3 center,
                                             Coordinate3 new_center) {
    Matrix3 frame {};
    Vector3 i;
    Vector3 j;
    Vector3 k;
    Vector3 v;

    if (CoordinateDistance(center, new_center) < 1.0e-8) {
      i = Vector3 { new_center.x, new_center.y, new_center.z };
      i.normalize();
      v = Vector3 { 0.0, 0.0, 1.0 };
      if (std::abs(std::abs(new_center.z) - Magnitude(new_center))
          < 1.0e-8) {
        v = Vector3 { -1.0, 0.0, 0.0 };
      }
      j = v.cross(i);
      k = i.cross(j);
    } else {
      Coordinate3 displacement { new_center - center };
      const Scalar length { Magnitude(displacement) };
      const Scalar radius { Magnitude(center) };
      const Scalar new_length {
          length + length * radius * radius / (radius * radius - length * length) };
      Coordinate3 reference_displacement { (new_length / length) * displacement };
      Coordinate3 reference { center + reference_displacement };
      reference = (radius / Magnitude(reference)) * reference;
      center = new_center;
      new_center = reference;

      i = Vector3 { center.x, center.y, center.z };
      v = Vector3 { new_center.x, new_center.y, new_center.z };
      k = i.cross(v);
      j = k.cross(i);
    }

    i.normalize();
    j.normalize();
    k.normalize();

    frame[0] = i.x;
    frame[1] = i.y;
    frame[2] = i.z;
    frame[3] = j.x;
    frame[4] = j.y;
    frame[5] = j.z;
    frame[6] = k.x;
    frame[7] = k.y;
    frame[8] = k.z;
    return frame;
  }

  static std::array<Scalar, 3> InnerCoordinateCoefficients(
      Coordinate3 target, Coordinate3 center, Matrix3 frame) {
    std::array<Scalar, 3> coefficients {};

    Vector3 target_vector { target - center };
    target_vector.calc_magnitude();
    if (target_vector.magnitude < 1.0e-8) {
      return coefficients;
    }

    Vector3 i { frame[0], frame[1], frame[2] };
    Vector3 j { frame[3], frame[4], frame[5] };
    Vector3 k { frame[6], frame[7], frame[8] };

    Scalar alpha {};
    Scalar beta {};
    Scalar gamma {};
    Vector3 normalized_target { target_vector };
    normalized_target.normalize();
    if (std::abs(normalized_target.dot(i)) < 1.0e-8) {
      alpha = 0.0;
      if (std::abs(normalized_target.dot(j)) < 1.0e-8) {
        beta = 0.0;
        gamma = target_vector.magnitude;
        if (normalized_target.dot(k) < 0.0) {
          gamma = -gamma;
        }
      } else if (std::abs(normalized_target.dot(k)) < 1.0e-8) {
        gamma = 0.0;
        beta = target_vector.magnitude;
        if (normalized_target.dot(j) < 0.0) {
          beta = -beta;
        }
      } else {
        beta = (target_vector.x * k.y - target_vector.y * k.x)
               / (j.x * k.y - j.y * k.x);
        gamma = (target_vector.x * j.y - target_vector.y * j.x)
                / (k.x * j.y - k.y * j.x);
      }
    } else if (std::abs(normalized_target.dot(j)) < 1.0e-8) {
      beta = 0.0;
      if (std::abs(normalized_target.dot(i)) < 1.0e-8) {
        alpha = 0.0;
        gamma = target_vector.magnitude;
        if (normalized_target.dot(k) < 0.0) {
          gamma = -gamma;
        }
      } else if (std::abs(normalized_target.dot(k)) < 1.0e-8) {
        gamma = 0.0;
        alpha = target_vector.magnitude;
        if (normalized_target.dot(i) < 0.0) {
          alpha = -alpha;
        }
      } else {
        alpha = (target_vector.x * k.y - target_vector.y * k.x)
                / (i.x * k.y - i.y * k.x);
        gamma = (target_vector.x * i.y - target_vector.y * i.x)
                / (k.x * i.y - k.y * i.x);
      }
    } else if (std::abs(normalized_target.dot(k)) < 1.0e-8) {
      gamma = 0.0;
      if (std::abs(normalized_target.dot(i)) < 1.0e-8) {
        alpha = 0.0;
        beta = target_vector.magnitude;
        if (normalized_target.dot(j) < 0.0) {
          beta = -beta;
        }
      } else if (std::abs(normalized_target.dot(j)) < 1.0e-8) {
        beta = 0.0;
        alpha = target_vector.magnitude;
        if (normalized_target.dot(i) < 0.0) {
          alpha = -alpha;
        }
      } else {
        alpha = (target_vector.x * j.y - target_vector.y * j.x)
                / (i.x * j.y - i.y * j.x);
        beta = (target_vector.x * i.y - target_vector.y * i.x)
               / (j.x * i.y - j.y * i.x);
      }
    } else {
      const Scalar n1 { k.y * target_vector.x - k.x * target_vector.y };
      const Scalar n2 { i.x * k.y - i.y * k.x };
      const Scalar n3 { j.x * k.y - j.y * k.x };
      const Scalar n4 { k.z * target_vector.x - k.x * target_vector.z };
      const Scalar n5 { i.x * k.z - i.z * k.x };
      const Scalar n6 { j.x * k.z - j.z * k.x };
      alpha = (n1 * n6 - n4 * n3) / (n2 * n6 - n5 * n3);
      beta = (n1 * n6 - n2 * n6 * alpha) / (n3 * n6);
      gamma = (target_vector.x - alpha * i.x - beta * j.x) / k.x;
    }

    coefficients[0] = alpha;
    coefficients[1] = beta;
    coefficients[2] = gamma;
    return coefficients;
  }

  static Coordinate3 TranslateOnSphere(Coordinate3 target, Coordinate3 center,
                                       Coordinate3 new_center, Matrix3 frame,
                                       Matrix3 new_frame) {
    Coordinate3 displacement { new_center - center };
    if (Magnitude(displacement) < 1.0e-8) {
      return target;
    }

    const std::array<Scalar, 3> coefficients {
        InnerCoordinateCoefficients(target, center, frame) };
    const Scalar alpha { coefficients[0] };
    const Scalar beta { coefficients[1] };
    const Scalar gamma { coefficients[2] };
    Vector3 i { new_frame[0], new_frame[1], new_frame[2] };
    Vector3 j { new_frame[3], new_frame[4], new_frame[5] };
    Vector3 k { new_frame[6], new_frame[7], new_frame[8] };
    Coordinate3 translated { alpha * i + beta * j + gamma * k };
    return translated + new_center;
  }

  static Coordinate3 RotateOnSphere(Coordinate3 target, Coordinate3 center,
                                    Matrix3 frame, Scalar angle) {
    Vector3 i { frame[0], frame[1], frame[2] };
    Vector3 j { frame[3], frame[4], frame[5] };
    Vector3 k { frame[6], frame[7], frame[8] };
    Vector3 target_vector { target - center };

    Vector3 target_i { i * target_vector.dot(i) };
    Vector3 target_jk { target_vector - target_i };
    target_i.calc_magnitude();
    target_jk.calc_magnitude();

    if (target_jk.magnitude < 1.0e-8
        || std::abs(target_i.magnitude - 1.0) < 1.0e-8) {
      return target;
    }

    const Scalar pi { 3.141592653589793238462643383279502884 };
    Scalar phi { std::acos(target_jk.dot(j) / target_jk.magnitude) };
    if (target_jk.dot(k) < 0.0) {
      phi = 2.0 * pi - phi;
    }
    phi += angle;

    target_jk = j * (target_jk.magnitude * std::cos(phi))
                + k * (target_jk.magnitude * std::sin(phi));
    target_vector = target_i + target_jk;
    return { target_vector.x + center.x, target_vector.y + center.y,
             target_vector.z + center.z };
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

  static RotationAnglePartition PartitionRotationAngle(
      Scalar target_angle, Scalar current_angle, Scalar positive_diffusion,
      Scalar negative_diffusion) {
    const Scalar total_diffusion { positive_diffusion + negative_diffusion };
    const Scalar delta { target_angle - current_angle };
    return RotationAnglePartition(
        delta * (positive_diffusion / total_diffusion),
        -delta * (negative_diffusion / total_diffusion));
  }
};

} // namespace core
} // namespace nerdss
