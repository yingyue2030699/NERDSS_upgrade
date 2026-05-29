/*! \file math_engine.hpp
 * \brief Cohesive facade for CPU scalar math primitives used by core NERDSS.
 */

#pragma once

#include "classes/class_Coord.hpp"
#include "classes/class_Vector.hpp"
#include "math/matrix.hpp"

#include <array>

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
};

} // namespace core
} // namespace nerdss
