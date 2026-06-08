/*! \file implicit_lipid_probability_service.hpp
 * \brief Pure implicit-lipid and compartment probability kernels.
 */

#pragma once

#include <cmath>

#include <gsl/gsl_errno.h>
#include <gsl/gsl_integration.h>
#include <gsl/gsl_sf_bessel.h>

namespace nerdss {
namespace core {

class ImplicitLipidProbabilityService {
public:
  struct IntegralParameters2D {
    double binding_radius {};
    double diffusion_total {};
    double association_rate {};
    double reaction_radius {};
    double time {};
  };

  struct BindingProbability2DResult {
    double probability {};
    double reaction_radius {};
  };

  struct CompartmentTransmissionParameters {
    double reaction_radius_2d {};
    double binding_radius {};
    double diffusion_total {};
    double association_rate {};
    double time {};
    double compartment_radius {};
    double site_density {};
  };

  static double SurfaceAssociationRate3DTo2D(double association_rate) {
    return 2.0 * association_rate;
  }

  static CompartmentTransmissionParameters CompartmentTransmissionSetup(
      double association_rate, double binding_radius, double diffusion_total,
      double time, double compartment_radius, double site_density) {
    CompartmentTransmissionParameters parameters {};
    parameters.reaction_radius_2d = 0.0;
    parameters.binding_radius = binding_radius;
    parameters.diffusion_total = diffusion_total;
    parameters.association_rate =
        SurfaceAssociationRate3DTo2D(association_rate);
    parameters.time = time;
    parameters.compartment_radius = compartment_radius;
    parameters.site_density = site_density;
    return parameters;
  }

  static double DissociationProbability2D(
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

  static double DissociationProbability3D(double time, double diffusion_total,
                                          double binding_radius,
                                          double association_rate,
                                          double dissociation_rate) {
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

  static double BindingProbability3D(double separation, double time,
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
      const double denominator { association_rate + diffusion_limited_rate };
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
    return CompartmentTransmissionProbability(
        distance_to_surface, time, diffusion_total, binding_radius,
        association_rate, compartment_radius, site_density, true);
  }

  static double CompartmentExitProbability(double distance_to_surface,
                                           double time,
                                           double diffusion_total,
                                           double binding_radius,
                                           double association_rate,
                                           double compartment_radius,
                                           double site_density) {
    return CompartmentTransmissionProbability(
        distance_to_surface, time, diffusion_total, binding_radius,
        association_rate, compartment_radius, site_density, false);
  }

  static double IntegralKernel2D(double u, double binding_radius,
                                 double diffusion_total,
                                 double association_rate,
                                 double reaction_radius, double time) {
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

  static double IntegralKernel2DCallback(double u, void* parameter) {
    IntegralParameters2D* params =
        static_cast<IntegralParameters2D*>(parameter);
    return IntegralKernel2D(
        u, params->binding_radius, params->diffusion_total,
        params->association_rate, params->reaction_radius, params->time);
  }

  static double IntegrateKernel2D(double binding_radius,
                                  double diffusion_total,
                                  double association_rate,
                                  double reaction_radius, double time) {
    IntegralParameters2D params {};
    params.binding_radius = binding_radius;
    params.diffusion_total = diffusion_total;
    params.association_rate = association_rate;
    params.reaction_radius = reaction_radius;
    params.time = time;

    gsl_integration_workspace* workspace =
        gsl_integration_workspace_alloc(1000000);
    double result {};
    double error {};
    const double epsabs { 1.0e-5 };
    const double epsrel { epsabs };
    gsl_function function {};
    function.function = &IntegralKernel2DCallback;
    function.params = &params;

    gsl_set_error_handler_off();
    int status = gsl_integration_qagiu(
        &function, 0.0, epsabs, epsrel, 1000000, workspace, &result, &error);
    if (status != GSL_SUCCESS) {
      double lower_bound { 0.0 };
      double upper_bound { 1.0e4 };
      while (std::abs(IntegralKernel2DCallback(upper_bound, function.params))
             > 1.0e-5) {
        upper_bound *= 1.5;
      }
      while (status != GSL_SUCCESS) {
        status = gsl_integration_qags(
            &function, lower_bound, upper_bound, epsabs, epsabs, 1000000,
            workspace, &result, &error);
        upper_bound *= 0.9;
      }
    }
    gsl_integration_workspace_free(workspace);
    gsl_set_error_handler(nullptr);

    return result;
  }

  static double BlockDistance2D(
      double time, double diffusion_total, double binding_radius,
      double association_rate, double dissociation_rate, int solution_count,
      int lipid_count, double membrane_area) {
    const double dissociation_rate_per_microsecond {
        dissociation_rate / 1.0e6 };
    const double max_radius {
        binding_radius + 3.0 * std::sqrt(4.0 * diffusion_total * time) };
    const double target_probability {
        DissociationProbability2D(
            time, diffusion_total, binding_radius, association_rate,
            dissociation_rate, solution_count, lipid_count, membrane_area) };
    const double criterion { 1.0e-5 };
    double radius_min { binding_radius };
    double radius_max { max_radius };
    double radius_mean { binding_radius };

    while (std::abs(radius_max - radius_min) > criterion) {
      radius_mean = 0.5 * (radius_max + radius_min);
      const double integrated_probability {
          4.0 * dissociation_rate_per_microsecond
          * IntegrateKernel2D(
              binding_radius, diffusion_total, association_rate, radius_mean,
              time) };
      if (integrated_probability > target_probability) {
        radius_min = radius_mean;
      } else {
        radius_max = radius_mean;
      }
    }

    return radius_mean;
  }

  static BindingProbability2DResult BindingProbability2D(
      double time, double diffusion_total, double binding_radius,
      double association_rate, double dissociation_rate, int solution_count,
      int lipid_count, double membrane_area, double reaction_radius) {
    BindingProbability2DResult result {};
    result.reaction_radius = reaction_radius;
    if (association_rate < 1.0e-15) {
      result.probability = 0.0;
      return result;
    }

    result.reaction_radius = BlockDistance2D(
        time, diffusion_total, binding_radius, association_rate,
        dissociation_rate, solution_count, lipid_count, membrane_area);
    result.probability =
        4.0 * association_rate
        * IntegrateKernel2D(
            binding_radius, diffusion_total, association_rate,
            result.reaction_radius, time);

    return result;
  }

private:
  static double CompartmentTransmissionProbability(
      double distance_to_surface, double time, double diffusion_total,
      double binding_radius, double association_rate, double compartment_radius,
      double site_density, bool entering) {
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
    const double radius {
        entering ? clamped_distance + compartment_radius
                 : -clamped_distance + compartment_radius };
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
        entering ? (radius + compartment_radius - binding_radius)
                       / sqrt_four_dt
                 : (compartment_radius - binding_radius + radius)
                       / sqrt_four_dt };
    const double x2 {
        entering ? (radius - compartment_radius - binding_radius)
                       / sqrt_four_dt
                 : (compartment_radius - binding_radius - radius)
                       / sqrt_four_dt };

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
};

} // namespace core
} // namespace nerdss
