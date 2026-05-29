/*! \file probability_engine.hpp
 * \brief Pure probability kernels exposed behind a core service facade.
 */

#pragma once

#include "math/Faddeeva.hpp"

#include <cmath>
#include <complex>

namespace nerdss {
namespace core {

class ProbabilityEngine {
public:
  static double AssociationProbability3D(double r0, double time,
                                         double diffusion_total,
                                         double binding_radius, double alpha,
                                         double coefficient) {
    const double four_dt { 4.0 * diffusion_total * time };
    const double sqrt_four_dt { std::sqrt(four_dt) };

    const double prefactor { coefficient * binding_radius / r0 };

    const double sqrt_time { std::sqrt(time) };
    const double alpha_squared { alpha * alpha };
    const double separation { (r0 - binding_radius) / sqrt_four_dt };

    const double exponent {
        2.0 * separation * sqrt_time * alpha + alpha_squared * time };
    const double erfc_argument { separation + alpha * sqrt_time };
    const double exp_exponent = std::exp(exponent);

    const double term1 { std::erfc(separation) };
    double term2 {};
    if (std::isinf(exp_exponent)) {
      std::complex<double> z;
      z.real(0.0);
      z.imag(erfc_argument);

      double relative_error { 0.0 };
      std::complex<double> value { Faddeeva::w(z, relative_error) };
      const double exp_separation { std::exp(-separation * separation) };
      term2 = exp_separation * std::real(value);
    } else {
      term2 = exp_exponent * std::erfc(erfc_argument);
    }

    return (term1 - term2) * prefactor;
  }

  static double AssociationProbability1D(double r0, double time,
                                         double diffusion_total,
                                         double binding_radius, double ka) {
    if (diffusion_total == 0) {
      if (r0 - binding_radius > 1.0e-6) {
        return 0.0;
      }
      return 1.0;
    }

    const double sqrt_four_dt { std::sqrt(4.0 * diffusion_total * time) };
    const double scaled_time { ka * std::sqrt(time / diffusion_total) };
    const double separation { (r0 - binding_radius) / sqrt_four_dt };

    const double erfc_value { std::erfc(separation) };
    const double erfcx_value { Faddeeva::erfcx(separation + scaled_time) };
    const double exp_separation { std::exp(-separation * separation) };

    return erfc_value - exp_separation * erfcx_value;
  }
};

} // namespace core
} // namespace nerdss
