/*! \file probability_engine.hpp
 * \brief Pure probability kernels exposed behind a core service facade.
 */

#pragma once

#include "core/probability/association_probability_service.hpp"
#include "core/probability/reaction_table_2d_service.hpp"

#include <cmath>
#include <cstddef>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_integration.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_sf_bessel.h>

namespace nerdss {
namespace core {

class ProbabilityEngine {
public:
  template <typename Real>
  static Real PoissonEventProbability(Real lambda) {
    return Real { 1 } - std::exp(-lambda);
  }

  static double PoissonEventProbability(double rate, double time,
                                        double unit_scale) {
    return PoissonEventProbability(rate * time * unit_scale);
  }

  struct ImplicitLipidIntegralParameters2D {
    double binding_radius {};
    double diffusion_total {};
    double association_rate {};
    double reaction_radius {};
    double time {};
  };

  struct ImplicitLipidBindingProbability2DResult {
    double probability {};
    double reaction_radius {};
  };

  static double AssociationProbability3D(double r0, double time,
                                         double diffusion_total,
                                         double binding_radius, double alpha,
                                         double coefficient) {
    return AssociationProbabilityService::AssociationProbability3D(
        r0, time, diffusion_total, binding_radius, alpha, coefficient);
  }

  static double AssociationProbability1D(double r0, double time,
                                         double diffusion_total,
                                         double binding_radius, double ka) {
    return AssociationProbabilityService::AssociationProbability1D(
        r0, time, diffusion_total, binding_radius, ka);
  }

  static double RebindingProbabilityRatio3D(double current_radius,
                                            double initial_radius, double time,
                                            double diffusion_total,
                                            double binding_radius,
                                            double alpha, double previous_survival,
                                            double tolerance) {
    return AssociationProbabilityService::RebindingProbabilityRatio3D(
        current_radius, initial_radius, time, diffusion_total, binding_radius,
        alpha, previous_survival, tolerance);
  }

  static double RebindingProbabilityRatio1D(double current_radius,
                                            double initial_radius, double time,
                                            double diffusion_total,
                                            double binding_radius, double ka,
                                            double previous_survival) {
    return AssociationProbabilityService::RebindingProbabilityRatio1D(
        current_radius, initial_radius, time, diffusion_total, binding_radius,
        ka, previous_survival);
  }

  static double SurvivalProbabilityIntegrand2D(double x, double binding_radius,
                                               double diffusion_total,
                                               double association_rate,
                                               double initial_radius,
                                               double time) {
    return ReactionTable2DService::SurvivalProbabilityIntegrand(
        x, binding_radius, diffusion_total, association_rate, initial_radius,
        time);
  }

  static double IrreversibleProbabilityIntegrand2D(
      double x, double binding_radius, double diffusion_total,
      double association_rate, double initial_radius, double current_radius,
      double time) {
    return ReactionTable2DService::IrreversibleProbabilityIntegrand(
        x, binding_radius, diffusion_total, association_rate, initial_radius,
        current_radius, time);
  }

  static double FreeDiffusionNormIntegrand2D(double x, double initial_radius,
                                             double diffusion_total,
                                             double time) {
    return ReactionTable2DService::FreeDiffusionNormIntegrand(
        x, initial_radius, diffusion_total, time);
  }

  static double FreeDiffusionProbability2D(double current_radius,
                                           double initial_radius,
                                           double diffusion_total,
                                           double time) {
    return ReactionTable2DService::FreeDiffusionProbability(
        current_radius, initial_radius, diffusion_total, time);
  }

  static double IntegrateSemiInfinite2D(
      gsl_function function, void* integrand_parameters,
      gsl_integration_workspace* workspace,
      double (*integrand)(double, void*)) {
    return ReactionTable2DService::IntegrateSemiInfinite(
        function, integrand_parameters, workspace, integrand);
  }

  static double TableStepSize2D(double diffusion_total, double time) {
    return ReactionTable2DService::TableStepSize(diffusion_total, time);
  }

  static double RotationalDiffusionDisplacement(double rotational_diffusion_z,
                                                double time,
                                                double moment_arm_squared,
                                                int dimensions) {
    const double dimension_count { static_cast<double>(dimensions) };
    const double angular_factor { 2.0 * (dimension_count - 1.0) };
    const double cosine_term {
        std::cos(std::sqrt(angular_factor * rotational_diffusion_z * time)) };
    return 2.0 * moment_arm_squared * (1.0 - cosine_term);
  }

  static double RotationalDiffusionContribution(double rotational_diffusion_z,
                                                double time,
                                                double moment_arm_squared,
                                                int dimensions) {
    return RotationalDiffusionDisplacement(
               rotational_diffusion_z, time, moment_arm_squared, dimensions)
           / (2.0 * static_cast<double>(dimensions) * time);
  }

  static double InterfaceRadius1D(double x) {
    return std::sqrt(x * x);
  }

  static double InterfaceRadius2D(double x, double y) {
    return std::sqrt(x * x + y * y);
  }

  static double InterfaceRadius3D(double x, double y, double z) {
    return std::sqrt(x * x + y * y + z * z);
  }

  static double MeanTranslationalDiffusion3D(
      double first_x, double first_y, double first_z, double second_x,
      double second_y, double second_z) {
    return (first_x + first_y + first_z + second_x + second_y + second_z)
           / 3.0;
  }

  static double TranslationalDiffusionDisplacement(double diffusion_total,
                                                   double time,
                                                   double dimensions) {
    return std::sqrt(2.0 * dimensions * diffusion_total * time);
  }

  static double ScaledDisplacementLimitSquared(double average_displacement,
                                               double scale_factor) {
    const double limit { scale_factor * average_displacement };
    return limit * limit;
  }

  static double BimolecularAssociationRate1D(double association_rate,
                                             double area_3d_to_1d,
                                             bool is_symmetric) {
    double intrinsic_rate { association_rate / area_3d_to_1d };
    if (!is_symmetric) {
      intrinsic_rate /= 2.0;
    }
    return intrinsic_rate;
  }

  static double BimolecularAssociationRate2D(double association_rate,
                                             double length_3d_to_2d,
                                             bool is_symmetric) {
    double intrinsic_rate { association_rate / length_3d_to_2d };
    if (is_symmetric) {
      intrinsic_rate *= 2.0;
    }
    return intrinsic_rate;
  }

  static double BimolecularAssociationRate3D(double association_rate,
                                             bool is_symmetric,
                                             bool has_surface_reactant,
                                             bool has_fiber_reactant) {
    double intrinsic_rate { association_rate };
    if (has_surface_reactant && !has_fiber_reactant) {
      intrinsic_rate *= 2.0;
    }
    if (is_symmetric) {
      intrinsic_rate *= 2.0;
    }
    return intrinsic_rate;
  }

  struct AssociationParameters3D {
    double diffusion_limited_rate {};
    double intrinsic_rate {};
    double alpha {};
    double probability_coefficient {};

    AssociationParameters3D() = default;

    AssociationParameters3D(double diffusion_limited_rate_value,
                            double intrinsic_rate_value, double alpha_value,
                            double probability_coefficient_value)
        : diffusion_limited_rate { diffusion_limited_rate_value }
        , intrinsic_rate { intrinsic_rate_value }
        , alpha { alpha_value }
        , probability_coefficient { probability_coefficient_value } {}
  };

  static double DiffusionLimitedAssociationRate3D(double diffusion_total,
                                                  double binding_radius) {
    const double pi { 3.141592653589793238462643383279502884 };
    return 4.0 * pi * diffusion_total * binding_radius;
  }

  static AssociationParameters3D AssociationParametersFor3D(
      double diffusion_total, double binding_radius, double association_rate,
      bool is_symmetric, bool has_surface_reactant, bool has_fiber_reactant) {
    const double diffusion_limited_rate {
        DiffusionLimitedAssociationRate3D(diffusion_total, binding_radius) };
    const double intrinsic_rate { BimolecularAssociationRate3D(
        association_rate, is_symmetric, has_surface_reactant,
        has_fiber_reactant) };
    const double rate_ratio { intrinsic_rate / diffusion_limited_rate };
    return AssociationParameters3D {
        diffusion_limited_rate,
        intrinsic_rate,
        (1.0 + rate_ratio) * std::sqrt(diffusion_total) / binding_radius,
        intrinsic_rate / (intrinsic_rate + diffusion_limited_rate) };
  }

  static double SurfaceAssociationRate3DTo2D(double association_rate) {
    return 2.0 * association_rate;
  }

  struct CompartmentTransmissionParameters {
    double reaction_radius_2d {};
    double binding_radius {};
    double diffusion_total {};
    double association_rate {};
    double time {};
    double compartment_radius {};
    double site_density {};
  };

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

  static double ReactionSearchRadius1D(double diffusion_total, double time,
                                       double binding_radius) {
    return 4.0 * std::sqrt(2.0 * diffusion_total * time) + binding_radius;
  }

  static double ReactionSearchRadius2D(double diffusion_total, double time,
                                       double binding_radius) {
    return 3.5 * std::sqrt(4.0 * diffusion_total * time) + binding_radius;
  }

  static double ReactionSearchRadius3D(double diffusion_total, double time,
                                       double binding_radius) {
    return 3.0 * std::sqrt(6.0 * diffusion_total * time) + binding_radius;
  }

  static double SetupRMaxLimit3D(double diffusion_total, double time,
                                 double binding_radius,
                                 double first_reactant_radius,
                                 double second_reactant_radius) {
    return ReactionSearchRadius3D(diffusion_total, time, binding_radius)
           + first_reactant_radius + second_reactant_radius;
  }

  struct AssociationContactGeometry {
    double separation {};
    double radius {};
    double radius_ratio {};
    bool was_overlapping {};
  };

  static AssociationContactGeometry NormalizeAssociationContact(
      double separation, double radius, double binding_radius) {
    AssociationContactGeometry geometry {};
    geometry.separation = separation;
    geometry.radius = radius;
    geometry.radius_ratio = binding_radius / radius;
    geometry.was_overlapping = separation < 0.0;
    if (geometry.was_overlapping) {
      geometry.separation = 0.0;
      geometry.radius = binding_radius;
      geometry.radius_ratio = 1.0;
    }
    return geometry;
  }

  static double QuantizeDiffusionFor2DTable(double diffusion_total) {
    double scaled_diffusion {};
    if (diffusion_total < 0.0001) {
      scaled_diffusion = diffusion_total * 100000.0;
    } else if (diffusion_total < 0.001) {
      scaled_diffusion = diffusion_total * 10000.0;
    } else if (diffusion_total < 0.01) {
      scaled_diffusion = diffusion_total * 1000.0;
    } else if (diffusion_total < 0.1) {
      scaled_diffusion = diffusion_total * 100.0;
    } else {
      scaled_diffusion = diffusion_total * 100.0;
    }

    const int rounded_diffusion {
        static_cast<int>(std::round(scaled_diffusion)) };
    double quantized_diffusion {};
    if (diffusion_total < 0.0001) {
      quantized_diffusion = rounded_diffusion * 0.00001;
    } else if (diffusion_total < 0.001) {
      quantized_diffusion = rounded_diffusion * 0.0001;
    } else if (diffusion_total < 0.01) {
      quantized_diffusion = rounded_diffusion * 0.001;
    } else if (diffusion_total < 0.1) {
      quantized_diffusion = rounded_diffusion * 0.01;
    } else {
      quantized_diffusion = rounded_diffusion * 0.01;
    }

    if (quantized_diffusion < 1.0e-50) {
      return 0.0;
    }
    return quantized_diffusion;
  }

  static std::size_t TableSize2D(double binding_radius, double diffusion_total,
                                 double time, double max_radius) {
    return ReactionTable2DService::TableSize(
        binding_radius, diffusion_total, time, max_radius);
  }

  static double InterpolateMatrixRow2D(const gsl_matrix* matrix,
                                       double radius_step_size,
                                       double radius,
                                       double binding_radius) {
    return ReactionTable2DService::InterpolateMatrixRow(
        matrix, radius_step_size, radius, binding_radius);
  }

  static double PreviousSurvivalProbability2D(const gsl_matrix* survival_matrix,
                                              double diffusion_total,
                                              double time, double radius,
                                              double binding_radius) {
    return ReactionTable2DService::PreviousSurvivalProbability(
        survival_matrix, diffusion_total, time, radius, binding_radius);
  }

  static double PreviousNormProbability2D(const gsl_matrix* norm_matrix,
                                          double radius_step_size,
                                          double radius,
                                          double binding_radius) {
    return ReactionTable2DService::PreviousNormProbability(
        norm_matrix, radius_step_size, radius, binding_radius);
  }

  static double IrreversibleProbabilityTable2D(const gsl_matrix* pir_matrix,
                                               const gsl_matrix* survival_matrix,
                                               double radius_step_size,
                                               double current_radius,
                                               double initial_radius,
                                               double binding_radius) {
    return ReactionTable2DService::IrreversibleProbability(
        pir_matrix, survival_matrix, radius_step_size, current_radius,
        initial_radius, binding_radius);
  }

  static double RebindingProbabilityRatioTable2D(
      const gsl_matrix* pir_matrix, const gsl_matrix* survival_matrix,
      const gsl_matrix* norm_matrix, double current_radius,
      double diffusion_total, double time, double initial_radius,
      double previous_survival, double tolerance, double binding_radius) {
    return ReactionTable2DService::RebindingProbabilityRatio(
        pir_matrix, survival_matrix, norm_matrix, current_radius,
        diffusion_total, time, initial_radius, previous_survival, tolerance,
        binding_radius);
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

  static double LoopClosureAssociationProbability(double time,
                                                  double association_rate,
                                                  double cooperativity_factor) {
    const double standard_state_per_nm3 { 0.602 };
    const double poisson {
        time * association_rate * standard_state_per_nm3
        * cooperativity_factor };
    return PoissonEventProbability(poisson);
  }

  static double LoopDissociationCorrectionRatio(double lambda) {
    return PoissonEventProbability(lambda) / lambda;
  }

  static double LoopDissociationCorrectionRatio(double time,
                                                double association_rate,
                                                double cooperativity_factor) {
    const double standard_state_per_nm3 { 0.602 };
    const double poisson {
        time * association_rate * standard_state_per_nm3
        * cooperativity_factor };
    return LoopDissociationCorrectionRatio(poisson);
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

  static double ImplicitLipidIntegralKernel2DCallback(double u,
                                                      void* parameter) {
    ImplicitLipidIntegralParameters2D* params =
        static_cast<ImplicitLipidIntegralParameters2D*>(parameter);
    return ImplicitLipidIntegralKernel2D(
        u, params->binding_radius, params->diffusion_total,
        params->association_rate, params->reaction_radius, params->time);
  }

  static double IntegrateImplicitLipidKernel2D(
      double binding_radius, double diffusion_total, double association_rate,
      double reaction_radius, double time) {
    ImplicitLipidIntegralParameters2D params {};
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
    function.function = &ImplicitLipidIntegralKernel2DCallback;
    function.params = &params;

    gsl_set_error_handler_off();
    int status = gsl_integration_qagiu(
        &function, 0.0, epsabs, epsrel, 1000000, workspace, &result, &error);
    if (status != GSL_SUCCESS) {
      double lower_bound { 0.0 };
      double upper_bound { 1.0e4 };
      while (std::abs(ImplicitLipidIntegralKernel2DCallback(
                 upper_bound, function.params)) > 1.0e-5) {
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

  static double ImplicitLipidBlockDistance2D(
      double time, double diffusion_total, double binding_radius,
      double association_rate, double dissociation_rate, int solution_count,
      int lipid_count, double membrane_area) {
    const double dissociation_rate_per_microsecond {
        dissociation_rate / 1.0e6 };
    const double max_radius {
        binding_radius + 3.0 * std::sqrt(4.0 * diffusion_total * time) };
    const double target_probability {
        ImplicitLipidDissociationProbability2D(
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
          * IntegrateImplicitLipidKernel2D(
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

  static ImplicitLipidBindingProbability2DResult
  ImplicitLipidBindingProbability2D(
      double time, double diffusion_total, double binding_radius,
      double association_rate, double dissociation_rate, int solution_count,
      int lipid_count, double membrane_area, double reaction_radius) {
    ImplicitLipidBindingProbability2DResult result {};
    result.reaction_radius = reaction_radius;
    if (association_rate < 1.0e-15) {
      result.probability = 0.0;
      return result;
    }

    result.reaction_radius = ImplicitLipidBlockDistance2D(
        time, diffusion_total, binding_radius, association_rate,
        dissociation_rate, solution_count, lipid_count, membrane_area);
    result.probability =
        4.0 * association_rate
        * IntegrateImplicitLipidKernel2D(
            binding_radius, diffusion_total, association_rate,
            result.reaction_radius, time);

    return result;
  }
};

} // namespace core
} // namespace nerdss
