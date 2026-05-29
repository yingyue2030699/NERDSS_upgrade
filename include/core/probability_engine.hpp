/*! \file probability_engine.hpp
 * \brief Pure probability kernels exposed behind a core service facade.
 */

#pragma once

#include "math/Faddeeva.hpp"

#include <cmath>
#include <complex>
#include <gsl/gsl_matrix.h>
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

  static double TableStepSize2D(double diffusion_total, double time) {
    return std::sqrt(diffusion_total * time) / 50.0;
  }

  static double InterpolateMatrixRow2D(const gsl_matrix* matrix,
                                       double radius_step_size,
                                       double radius,
                                       double binding_radius) {
    int index {
        static_cast<int>(std::floor((radius - binding_radius)
                                    / radius_step_size)) };
    if (index < 0) {
      index = 0;
    }

    const double value1 { gsl_matrix_get(matrix, 1, index) };
    const double value2 { gsl_matrix_get(matrix, 1, index + 1) };
    const double radius1 { gsl_matrix_get(matrix, 0, index) };
    const double radius2 { gsl_matrix_get(matrix, 0, index + 1) };

    return (value1 * (radius2 - radius) + value2 * (radius - radius1))
           / radius_step_size;
  }

  static double PreviousSurvivalProbability2D(const gsl_matrix* survival_matrix,
                                              double diffusion_total,
                                              double time, double radius,
                                              double binding_radius) {
    return InterpolateMatrixRow2D(
        survival_matrix, TableStepSize2D(diffusion_total, time), radius,
        binding_radius);
  }

  static double PreviousNormProbability2D(const gsl_matrix* norm_matrix,
                                          double radius_step_size,
                                          double radius,
                                          double binding_radius) {
    return InterpolateMatrixRow2D(
        norm_matrix, radius_step_size, radius, binding_radius);
  }

  static double ImplicitLipidDissociationProbability2D(
      double time, double diffusion_total, double binding_radius,
      double association_rate, double dissociation_rate, int solution_count,
      int lipid_count, double membrane_area) {
    const double pi { 3.141592653589793238462643383279502884 };
    const double dissociation_rate_per_microsecond { dissociation_rate / 1.0e6 };
    if (dissociation_rate_per_microsecond < 1.0e-15) {
      return 0.0;
    }

    const double equilibrium_constant {
        dissociation_rate_per_microsecond / association_rate };
    double max_count { static_cast<double>(lipid_count) };
    if (solution_count > lipid_count) {
      max_count = static_cast<double>(solution_count);
    }

    const double outer_radius {
        2.0 * std::sqrt(membrane_area / pi / max_count
                        + binding_radius * binding_radius) };
    const double radius_ratio { binding_radius / outer_radius };
    const double radius_denominator { 1.0 - radius_ratio * radius_ratio };
    const double diffusion_limited_correction {
        1.0 / (8.0 * pi * diffusion_total)
        * (4.0 * std::log(outer_radius / binding_radius)
               / (radius_denominator * radius_denominator)
           - 2.0 / radius_denominator - 1.0) };
    const double effective_association_rate {
        1.0 / (1.0 / association_rate + diffusion_limited_correction) };
    const double effective_dissociation_rate {
        effective_association_rate * equilibrium_constant };

    return 1.0 - std::exp(-effective_dissociation_rate * time);
  }

  static double ImplicitLipidDissociationProbability3D(
      double time, double diffusion_total, double binding_radius,
      double association_rate, double dissociation_rate) {
    const double pi { 3.141592653589793238462643383279502884 };
    const double dissociation_rate_per_microsecond { dissociation_rate / 1.0e6 };
    if (dissociation_rate_per_microsecond < 1.0e-15) {
      return 0.0;
    }

    const double equilibrium_constant {
        2.0 * dissociation_rate_per_microsecond / association_rate };
    const double effective_association_rate {
        0.5
        / (1.0 / association_rate
           + 1.0 / (4.0 * pi * diffusion_total * binding_radius)) };
    const double effective_dissociation_rate {
        effective_association_rate * equilibrium_constant };

    return 1.0 - std::exp(-effective_dissociation_rate * time);
  }

  static double ImplicitLipidBindingProbability3D(double separation,
                                                  double time,
                                                  double diffusion_total,
                                                  double binding_radius,
                                                  double association_rate) {
    const double pi { 3.141592653589793238462643383279502884 };
    if (association_rate < 1.0e-15) {
      return 0.0;
    }

    const double diffusion_limited_rate {
        4.0 * pi * binding_radius * diffusion_total };
    const double alpha {
        std::sqrt(diffusion_total) / binding_radius
        * (1.0 + association_rate / diffusion_limited_rate) };

    if (separation > binding_radius) {
      const double denominator {
          association_rate + diffusion_limited_rate };
      const double coefficient {
          2.0 * pi * binding_radius * binding_radius * association_rate
          * diffusion_limited_rate / denominator / denominator };
      const double a {
          (separation - binding_radius)
          / std::sqrt(4.0 * diffusion_total * time) };
      const double b { alpha * std::sqrt(time) };
      const double exponent { 2.0 * a * b + b * b };

      if (std::isinf(std::exp(exponent))) {
        return coefficient
               * (std::exp(-a * a) / std::sqrt(pi) / (a + b)
                  - (2.0 * a * b + 1.0) * std::erfc(a)
                  + 2.0 * alpha * std::sqrt(time / pi) * std::exp(-a * a));
      }
      return coefficient
             * (std::exp(exponent) * std::erfc(a + b)
                - (2.0 * a * b + 1.0) * std::erfc(a)
                + 2.0 * alpha * std::sqrt(time / pi) * std::exp(-a * a));
    }

    const double coefficient {
        2.0 * pi * binding_radius * association_rate
        * std::sqrt(diffusion_total) / alpha
        / (association_rate + diffusion_limited_rate) };
    const double b { alpha * std::sqrt(time) };
    if (std::isinf(std::exp(b * b))) {
      return coefficient
             * (1.0 / std::sqrt(pi) / b - 1.0
                + 2.0 * alpha * std::sqrt(time / pi));
    }
    return coefficient
           * (std::exp(b * b) * std::erfc(b) - 1.0
              + 2.0 * alpha * std::sqrt(time / pi));
  }

  static double CompartmentEntryProbability(double distance_to_surface,
                                            double time,
                                            double diffusion_total,
                                            double binding_radius,
                                            double association_rate,
                                            double compartment_radius,
                                            double site_density) {
    const double pi { 3.141592653589793238462643383279502884 };
    if (association_rate < 1.0e-15) {
      return 0.0;
    }

    double clamped_distance { distance_to_surface };
    if (clamped_distance < binding_radius) {
      clamped_distance = binding_radius;
    }

    const double diffusion_limited_rate {
        4.0 * pi * binding_radius * diffusion_total };
    const double radius { clamped_distance + compartment_radius };
    const double alpha {
        std::sqrt(diffusion_total * time)
        * (association_rate + diffusion_limited_rate)
        / (binding_radius * diffusion_limited_rate) };
    const double coefficient {
        compartment_radius / radius * 2.0 * pi * site_density * binding_radius
        * binding_radius * association_rate * diffusion_limited_rate
        / (association_rate + diffusion_limited_rate)
        / (association_rate + diffusion_limited_rate) };
    const double sqrt_four_dt { std::sqrt(4.0 * diffusion_total * time) };
    const double x1 {
        (radius + compartment_radius - binding_radius) / sqrt_four_dt };
    const double x2 {
        (radius - compartment_radius - binding_radius) / sqrt_four_dt };

    double func1 {};
    double func2 {};
    if (std::isinf(std::exp(alpha * alpha + 2.0 * alpha * x2))) {
      func1 = -(2.0 * alpha / std::sqrt(pi)
                + 1.0 / (alpha + x1) / std::sqrt(pi))
                  * std::exp(-x1 * x1)
              + (2.0 * alpha * x1 + 1.0) * std::erfc(x1);
      func2 = -(2.0 * alpha / std::sqrt(pi)
                + 1.0 / (alpha + x2) / std::sqrt(pi))
                  * std::exp(-x2 * x2)
              + (2.0 * alpha * x2 + 1.0) * std::erfc(x2);
    } else if (std::isinf(std::exp(alpha * alpha + 2.0 * alpha * x1))) {
      func1 = -(2.0 * alpha / std::sqrt(pi)
                + 1.0 / (alpha + x1) / std::sqrt(pi))
                  * std::exp(-x1 * x1)
              + (2.0 * alpha * x1 + 1.0) * std::erfc(x1);
      func2 = -std::exp(alpha * alpha + 2.0 * alpha * x2)
                  * std::erfc(alpha + x2)
              + (2.0 * alpha * x2 + 1.0) * std::erfc(x2)
              - 2.0 * alpha / std::sqrt(pi) * std::exp(-x2 * x2);
    } else {
      func1 = -std::exp(alpha * alpha + 2.0 * alpha * x1)
                  * std::erfc(alpha + x1)
              + (2.0 * alpha * x1 + 1.0) * std::erfc(x1)
              - 2.0 * alpha / std::sqrt(pi) * std::exp(-x1 * x1);
      func2 = -std::exp(alpha * alpha + 2.0 * alpha * x2)
                  * std::erfc(alpha + x2)
              + (2.0 * alpha * x2 + 1.0) * std::erfc(x2)
              - 2.0 * alpha / std::sqrt(pi) * std::exp(-x2 * x2);
    }

    return coefficient * (func1 - func2);
  }

  static double CompartmentExitProbability(double distance_to_surface,
                                           double time,
                                           double diffusion_total,
                                           double binding_radius,
                                           double association_rate,
                                           double compartment_radius,
                                           double site_density) {
    const double pi { 3.141592653589793238462643383279502884 };
    if (association_rate < 1.0e-15) {
      return 0.0;
    }

    double clamped_distance { distance_to_surface };
    if (clamped_distance < binding_radius) {
      clamped_distance = binding_radius;
    }

    const double diffusion_limited_rate {
        4.0 * pi * binding_radius * diffusion_total };
    const double radius { -clamped_distance + compartment_radius };
    const double alpha {
        std::sqrt(diffusion_total * time)
        * (association_rate + diffusion_limited_rate)
        / (binding_radius * diffusion_limited_rate) };
    const double coefficient {
        compartment_radius / radius * 2.0 * pi * site_density * binding_radius
        * binding_radius * association_rate * diffusion_limited_rate
        / (association_rate + diffusion_limited_rate)
        / (association_rate + diffusion_limited_rate) };
    const double sqrt_four_dt { std::sqrt(4.0 * diffusion_total * time) };
    const double x1 {
        (compartment_radius - binding_radius + radius) / sqrt_four_dt };
    const double x2 {
        (compartment_radius - binding_radius - radius) / sqrt_four_dt };

    double func1 {};
    double func2 {};
    if (std::isinf(std::exp(alpha * alpha + 2.0 * alpha * x2))) {
      func1 = -(2.0 * alpha / std::sqrt(pi)
                + 1.0 / (alpha + x1) / std::sqrt(pi))
                  * std::exp(-x1 * x1)
              + (2.0 * alpha * x1 + 1.0) * std::erfc(x1);
      func2 = -(2.0 * alpha / std::sqrt(pi)
                + 1.0 / (alpha + x2) / std::sqrt(pi))
                  * std::exp(-x2 * x2)
              + (2.0 * alpha * x2 + 1.0) * std::erfc(x2);
    } else if (std::isinf(std::exp(alpha * alpha + 2.0 * alpha * x1))) {
      func1 = -(2.0 * alpha / std::sqrt(pi)
                + 1.0 / (alpha + x1) / std::sqrt(pi))
                  * std::exp(-x1 * x1)
              + (2.0 * alpha * x1 + 1.0) * std::erfc(x1);
      func2 = -std::exp(alpha * alpha + 2.0 * alpha * x2)
                  * std::erfc(alpha + x2)
              + (2.0 * alpha * x2 + 1.0) * std::erfc(x2)
              - 2.0 * alpha / std::sqrt(pi) * std::exp(-x2 * x2);
    } else {
      func1 = -std::exp(alpha * alpha + 2.0 * alpha * x1)
                  * std::erfc(alpha + x1)
              + (2.0 * alpha * x1 + 1.0) * std::erfc(x1)
              - 2.0 * alpha / std::sqrt(pi) * std::exp(-x1 * x1);
      func2 = -std::exp(alpha * alpha + 2.0 * alpha * x2)
                  * std::erfc(alpha + x2)
              + (2.0 * alpha * x2 + 1.0) * std::erfc(x2)
              - 2.0 * alpha / std::sqrt(pi) * std::exp(-x2 * x2);
    }

    return coefficient * (func1 - func2);
  }

  static double ImplicitLipidIntegralKernel2D(double u, double binding_radius,
                                              double diffusion_total,
                                              double association_rate,
                                              double reaction_radius,
                                              double time) {
    const double pi { 3.141592653589793238462643383279502884 };
    const double r_max {
        5.0
        * (binding_radius
           + 3.0 * std::sqrt(4.0 * diffusion_total * time)) };
    const double h { 2.0 * pi * binding_radius * diffusion_total };
    const double alpha {
        h * u * gsl_sf_bessel_Y1(binding_radius * u)
        + association_rate * gsl_sf_bessel_Y0(binding_radius * u) };
    const double eta {
        h * u * gsl_sf_bessel_J1(binding_radius * u)
        + association_rate * gsl_sf_bessel_J0(binding_radius * u) };
    const double a {
        u * r_max * gsl_sf_bessel_J1(r_max * u)
        - u * reaction_radius * gsl_sf_bessel_J1(reaction_radius * u) };
    const double b {
        u * r_max * gsl_sf_bessel_Y1(r_max * u)
        - u * reaction_radius * gsl_sf_bessel_Y1(reaction_radius * u) };

    return 1.0 / std::pow(u, 3.0)
           * (std::exp(-diffusion_total * u * u * time) - 1.0)
           / (alpha * alpha + eta * eta) * (alpha * a - eta * b);
  }
};

} // namespace core
} // namespace nerdss
