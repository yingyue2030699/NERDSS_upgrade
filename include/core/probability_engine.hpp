/*! \file probability_engine.hpp
 * \brief Pure probability kernels exposed behind a core service facade.
 */

#pragma once

#include "math/Faddeeva.hpp"

#include <cmath>
#include <complex>
#include <gsl/gsl_sf_bessel.h>

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

  static double RebindingProbabilityRatio3D(double current_radius,
                                            double initial_radius, double time,
                                            double diffusion_total,
                                            double binding_radius,
                                            double alpha, double previous_survival,
                                            double tolerance) {
    const double pi { 3.141592653589793238462643383279502884 };
    const double four_dt { 4.0 * diffusion_total * time };
    const double sqrt_four_dt { std::sqrt(four_dt) };

    const double free_prefactor { 1.0 / std::sqrt(4.0 * pi * time) };
    const double radial_prefactor {
        1.0 / (4.0 * pi * initial_radius * std::sqrt(diffusion_total)) };

    const double sqrt_time { std::sqrt(time) };
    const double alpha_squared { alpha * alpha };

    const double sigma_distance {
        current_radius + initial_radius - 2.0 * binding_radius };
    const double free_distance { current_radius - initial_radius };
    double term1 {
        free_prefactor
        * (std::exp(-free_distance * free_distance / four_dt)
           + std::exp(-sigma_distance * sigma_distance / four_dt)) };

    const double separation { sigma_distance / sqrt_four_dt };
    const double scaled_alpha { sqrt_time * alpha };
    const double exponent {
        2.0 * separation * scaled_alpha + alpha_squared * time };
    const double erfc_argument { separation + scaled_alpha };
    const double exp_exponent { std::exp(exponent) };

    double term2 {};
    if (std::isinf(exp_exponent)) {
      std::complex<double> z;
      z.real(0.0);
      z.imag(erfc_argument);
      double relative_error { 0.0 };
      const std::complex<double> value { Faddeeva::w(z, relative_error) };
      term2 = std::exp(-separation * separation) * std::real(value);
    } else {
      term2 = exp_exponent * std::erfc(erfc_argument);
    }

    double pirr { term1 - alpha * term2 };
    pirr *= radial_prefactor / current_radius;

    const double combined_prefactor { free_prefactor * radial_prefactor };
    const double normalization_prefactor { 4.0 * pi * combined_prefactor };
    const double near_distance { binding_radius - initial_radius };
    const double far_distance { binding_radius + initial_radius };
    const double sqrt_pi { std::sqrt(pi) };

    term1 = -0.5 * four_dt * std::exp(-near_distance * near_distance / four_dt)
            - 0.5 * sqrt_four_dt * sqrt_pi * initial_radius
                  * std::erf(-near_distance / sqrt_four_dt);
    term2 = 0.5 * four_dt * std::exp(-far_distance * far_distance / four_dt)
            + 0.5 * sqrt_four_dt * sqrt_pi * initial_radius
                  * std::erf(far_distance / sqrt_four_dt);
    const double pnorm { 1.0 - normalization_prefactor * (term1 + term2) };

    const double outward_distance { current_radius + initial_radius };
    term1 = std::exp(-free_distance * free_distance / four_dt)
            - std::exp(-outward_distance * outward_distance / four_dt);
    const double pfree { combined_prefactor / current_radius * term1 / pnorm };

    if (std::abs(pirr - pfree * previous_survival) < tolerance) {
      return 1.0;
    }
    return pirr / (pfree * previous_survival);
  }

  static double RebindingProbabilityRatio1D(double current_radius,
                                            double initial_radius, double time,
                                            double diffusion_total,
                                            double binding_radius, double ka,
                                            double previous_survival) {
    const double pi { 3.141592653589793238462643383279502884 };
    const double four_dt { 4.0 * diffusion_total * time };
    const double sqrt_four_dt { std::sqrt(four_dt) };
    const double sqrt_pi_four_dt { std::sqrt(4.0 * pi * diffusion_total * time) };
    const double minus_distance { current_radius - initial_radius };
    const double plus_distance { current_radius + initial_radius };
    const double scaled_time { ka * std::sqrt(time / diffusion_total) };

    if (binding_radius == 0.0) {
      const double exp_minus {
          std::exp(-minus_distance * minus_distance / four_dt) };
      const double exp_plus { std::exp(-plus_distance * plus_distance / four_dt) };
      const double pirr {
          exp_minus / sqrt_pi_four_dt
          - ka / diffusion_total * exp_plus
                * Faddeeva::erfcx(plus_distance / sqrt_four_dt + scaled_time) };
      const double pfree { exp_minus / sqrt_pi_four_dt };
      return pirr / previous_survival / pfree;
    }

    const double sigma_distance {
        current_radius + initial_radius - 2.0 * binding_radius };
    const double sigma_minus { initial_radius - binding_radius };
    const double sigma_plus { initial_radius + binding_radius };

    const double exp_minus {
        std::exp(-minus_distance * minus_distance / four_dt) };
    const double exp_plus { std::exp(-plus_distance * plus_distance / four_dt) };
    const double free_integral {
        std::sqrt(pi * diffusion_total * time)
        * (std::erf(sigma_minus / sqrt_four_dt)
           + std::erf(sigma_plus / sqrt_four_dt)) };
    const double pfree { (exp_minus - exp_plus) / free_integral };

    const double exp_sigma {
        std::exp(-sigma_distance * sigma_distance / four_dt) };
    const double erfc_argument { sigma_distance / sqrt_four_dt + scaled_time };
    const double pirr {
        (exp_minus + exp_sigma) / sqrt_pi_four_dt
        - ka / diffusion_total * exp_sigma * Faddeeva::erfcx(erfc_argument) };

    return pirr / previous_survival / pfree;
  }

  static double SurvivalProbabilityIntegrand2D(double x, double binding_radius,
                                               double diffusion_total,
                                               double association_rate,
                                               double initial_radius,
                                               double time) {
    const double pi { 3.141592653589793238462643383279502884 };
    const double infinite_rate { 1.0 / 0.0 };

    if (association_rate < infinite_rate) {
      const double h { 2.0 * pi * binding_radius * diffusion_total };
      const double scaled_radius { x * binding_radius };
      const double alpha {
          h * x * j1(scaled_radius) + association_rate * j0(scaled_radius) };
      const double beta {
          h * x * y1(scaled_radius) + association_rate * y0(scaled_radius) };
      const double denominator { alpha * alpha + beta * beta };
      const double bessel_product {
          j0(scaled_radius) * y1(scaled_radius)
          - j1(scaled_radius) * y0(scaled_radius) };
      const double transfer {
          (j0(x * initial_radius) * beta - y0(x * initial_radius) * alpha)
          / denominator };

      return transfer * bessel_product
             * (1.0 - std::exp(-diffusion_total * time * x * x))
             * binding_radius * association_rate;
    }

    const double alpha { j0(x * binding_radius) };
    const double beta { y0(x * binding_radius) };
    const double denominator { alpha * alpha + beta * beta };
    const double bessel_product {
        j0(x * binding_radius) * y0(x * initial_radius)
        - j0(x * initial_radius) * y0(x * binding_radius) };

    return (2.0 / pi) * bessel_product
           * std::exp(-diffusion_total * time * x * x) / x / denominator;
  }

  static double IrreversibleProbabilityIntegrand2D(
      double x, double binding_radius, double diffusion_total,
      double association_rate, double initial_radius, double current_radius,
      double time) {
    const double pi { 3.141592653589793238462643383279502884 };
    const double infinite_rate { 1.0 / 0.0 };

    const double scaled_radius { x * binding_radius };
    double alpha {};
    double beta {};
    if (association_rate < infinite_rate) {
      const double h { 2.0 * pi * binding_radius * diffusion_total };
      alpha = h * x * j1(scaled_radius) + association_rate * j0(scaled_radius);
      beta = h * x * y1(scaled_radius) + association_rate * y0(scaled_radius);
    } else {
      alpha = j0(scaled_radius);
      beta = y0(scaled_radius);
    }

    const double denominator { std::sqrt(alpha * alpha + beta * beta) };
    const double current_projection {
        (j0(x * current_radius) * beta - y0(x * current_radius) * alpha)
        / denominator };
    const double initial_projection {
        (j0(x * initial_radius) * beta - y0(x * initial_radius) * alpha)
        / denominator };

    return x * std::exp(-diffusion_total * time * x * x) * current_projection
           * initial_projection / (2.0 * pi);
  }

  static double FreeDiffusionNormIntegrand2D(double x, double initial_radius,
                                             double diffusion_total,
                                             double time) {
    const double scaled_radius {
        x * initial_radius / (2.0 * diffusion_total * time) };
    const double prefactor { x / (2.0 * diffusion_total * time) };
    const double exponent {
        scaled_radius
        - (initial_radius * initial_radius + x * x)
              / (4.0 * time * diffusion_total) };

    return prefactor * std::exp(exponent)
           * gsl_sf_bessel_I0_scaled(scaled_radius);
  }

  static double FreeDiffusionProbability2D(double current_radius,
                                           double initial_radius,
                                           double diffusion_total,
                                           double time) {
    const double pi { 3.141592653589793238462643383279502884 };
    const double scaled_radius {
        current_radius * initial_radius / (2.0 * diffusion_total * time) };
    const double prefactor { 1.0 / (4.0 * pi * time * diffusion_total) };
    const double exponent {
        scaled_radius
        - (initial_radius * initial_radius + current_radius * current_radius)
              / (4.0 * time * diffusion_total) };

    return prefactor * std::exp(exponent)
           * gsl_sf_bessel_I0_scaled(scaled_radius);
  }
};

} // namespace core
} // namespace nerdss
