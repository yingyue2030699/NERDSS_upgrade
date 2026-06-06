/*! \file reaction_table_2d_service.hpp
 * \brief Pure 2D reaction-table probability kernels.
 */

#pragma once

#include <cmath>
#include <cstddef>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_integration.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_sf_bessel.h>

namespace nerdss {
namespace core {

class ReactionTable2DService {
public:
  struct TableParameters {
    double binding_radius {};
    double diffusion_total {};
    double association_rate {};
    double max_radius {};
    double time {};

    TableParameters() = default;

    TableParameters(double binding_radius_value, double diffusion_total_value,
                    double association_rate_value, double max_radius_value,
                    double time_value)
        : binding_radius { binding_radius_value }
        , diffusion_total { diffusion_total_value }
        , association_rate { association_rate_value }
        , max_radius { max_radius_value }
        , time { time_value } {}
  };

  struct IntegrandParameters {
    double binding_radius {};
    double diffusion_total {};
    double association_rate {};
    double initial_radius {};
    double current_radius {};
    double time {};
  };

  static double SurvivalProbabilityIntegrand(double x, double binding_radius,
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

  static double IrreversibleProbabilityIntegrand(
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

    return x * std::exp(-diffusion_total * time * x * x)
           * current_projection * initial_projection / (2.0 * pi);
  }

  static double FreeDiffusionNormIntegrand(double x, double initial_radius,
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

  static double FreeDiffusionProbability(double current_radius,
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

  static double IntegrateSemiInfinite(gsl_function function,
                                      void* integrand_parameters,
                                      gsl_integration_workspace* workspace,
                                      double (*integrand)(double, void*)) {
    double result {};
    double error {};
    const double x_low { 0.0 };
    const double eps_abs { 1.0e-7 };
    const double eps_rel { 1.0e-7 };
    const double publication_criterion { 1.0e-6 };

    int status { gsl_integration_qagiu(
        &function, x_low, eps_abs, eps_rel, 1000000, workspace, &result,
        &error) };

    if (status != GSL_SUCCESS) {
      status = gsl_integration_qagiu(
          &function, x_low, publication_criterion, publication_criterion,
          1000000, workspace, &result, &error);
    }

    if (status != GSL_SUCCESS) {
      double upper_bound { 10000.0 };
      while (std::abs((*integrand)(upper_bound, integrand_parameters))
             > 1.0e-10) {
        upper_bound *= 1.2;
      }

      while (status != GSL_SUCCESS) {
        const int key { 2 };
        status = gsl_integration_qag(
            &function, x_low, upper_bound, eps_abs, publication_criterion,
            1000000, key, workspace, &result, &error);
        upper_bound *= 0.9;
      }
    }

    return result;
  }

  static double TableStepSize(double diffusion_total, double time) {
    return std::sqrt(diffusion_total * time) / 50.0;
  }

  static std::size_t TableSize(double binding_radius, double diffusion_total,
                               double time, double max_radius) {
    std::size_t count { 0 };
    const double step_size { TableStepSize(diffusion_total, time) };
    double radius { binding_radius };
    while (radius <= max_radius + step_size) {
      ++count;
      radius += step_size;
    }
    return count;
  }

  static double InterpolateMatrixRow(const gsl_matrix* matrix,
                                     double radius_step_size, double radius,
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

  static double PreviousSurvivalProbability(const gsl_matrix* survival_matrix,
                                            double diffusion_total,
                                            double time, double radius,
                                            double binding_radius) {
    return InterpolateMatrixRow(
        survival_matrix, TableStepSize(diffusion_total, time), radius,
        binding_radius);
  }

  static double PreviousNormProbability(const gsl_matrix* norm_matrix,
                                        double radius_step_size,
                                        double radius,
                                        double binding_radius) {
    return InterpolateMatrixRow(
        norm_matrix, radius_step_size, radius, binding_radius);
  }

  static double IrreversibleProbability(const gsl_matrix* pir_matrix,
                                        const gsl_matrix* survival_matrix,
                                        double radius_step_size,
                                        double current_radius,
                                        double initial_radius,
                                        double binding_radius) {
    int initial_index {
        static_cast<int>(std::floor((initial_radius - binding_radius)
                                    / radius_step_size)) };
    int current_index {
        static_cast<int>(std::floor((current_radius - binding_radius)
                                    / radius_step_size)) };
    if (initial_index < 0) {
      initial_index = 0;
    }
    if (current_index < 0) {
      current_index = 0;
    }

    if (initial_index == current_index) {
      return gsl_matrix_get(pir_matrix, initial_index, current_index);
    }

    const double initial_a { gsl_matrix_get(survival_matrix, 0,
                                            initial_index) };
    const double current_a { gsl_matrix_get(survival_matrix, 0,
                                            current_index) };
    const double value_a { gsl_matrix_get(pir_matrix, initial_index,
                                          current_index) };
    const double distance_a {
        std::sqrt(std::pow(initial_a - initial_radius, 2.0)
                  + std::pow(current_a - current_radius, 2.0)) };

    const double initial_b { gsl_matrix_get(survival_matrix, 0,
                                            initial_index) };
    const double current_b { gsl_matrix_get(survival_matrix, 0,
                                            current_index + 1) };
    const double value_b { gsl_matrix_get(pir_matrix, initial_index,
                                          current_index + 1) };
    const double distance_b {
        std::sqrt(std::pow(initial_b - initial_radius, 2.0)
                  + std::pow(current_b - current_radius, 2.0)) };

    const double initial_c { gsl_matrix_get(survival_matrix, 0,
                                            initial_index + 1) };
    const double current_c { gsl_matrix_get(survival_matrix, 0,
                                            current_index + 1) };
    const double value_c { gsl_matrix_get(pir_matrix, initial_index + 1,
                                          current_index + 1) };
    const double distance_c {
        std::sqrt(std::pow(initial_c - initial_radius, 2.0)
                  + std::pow(current_c - current_radius, 2.0)) };

    const double initial_d { gsl_matrix_get(survival_matrix, 0,
                                            initial_index + 1) };
    const double current_d { gsl_matrix_get(survival_matrix, 0,
                                            current_index) };
    const double value_d { gsl_matrix_get(pir_matrix, initial_index + 1,
                                          current_index) };
    const double distance_d {
        std::sqrt(std::pow(initial_d - initial_radius, 2.0)
                  + std::pow(current_d - current_radius, 2.0)) };

    return ((value_a / distance_a) + (value_b / distance_b)
            + (value_c / distance_c) + (value_d / distance_d))
           / ((1.0 / distance_a) + (1.0 / distance_b)
              + (1.0 / distance_c) + (1.0 / distance_d));
  }

  static double RebindingProbabilityRatio(
      const gsl_matrix* pir_matrix, const gsl_matrix* survival_matrix,
      const gsl_matrix* norm_matrix, double current_radius,
      double diffusion_total, double time, double initial_radius,
      double previous_survival, double tolerance, double binding_radius) {
    const double radius_step_size { TableStepSize(diffusion_total, time) };
    const double free_probability { FreeDiffusionProbability(
        current_radius, initial_radius, diffusion_total, time) };
    const double norm_probability { PreviousNormProbability(
        norm_matrix, radius_step_size, initial_radius, binding_radius) };
    const double irreversible_probability { IrreversibleProbability(
        pir_matrix, survival_matrix, radius_step_size, current_radius,
        initial_radius, binding_radius) };
    const double normalized_free_probability {
        free_probability / norm_probability };

    if (std::abs(irreversible_probability
                 - normalized_free_probability * previous_survival)
        < tolerance) {
      return 1.0;
    }
    return irreversible_probability
           / (normalized_free_probability * previous_survival);
  }

  static void FillMatrices(gsl_matrix* survival_matrix, gsl_matrix* norm_matrix,
                           gsl_matrix* pir_matrix,
                           const TableParameters& parameters) {
    const double radius_step_size {
        TableStepSize(parameters.diffusion_total, parameters.time) };
    FillNormMatrix(norm_matrix, parameters, radius_step_size);
    FillSurvivalMatrix(survival_matrix, parameters, radius_step_size);
    FillIrreversibleMatrix(pir_matrix, parameters, radius_step_size);
  }

  static void FillSurvivalMatrix(gsl_matrix* survival_matrix,
                                 const TableParameters& parameters,
                                 double radius_step_size) {
    std::size_t index { 0 };
    gsl_function function {};
    function.function = &SurvivalCallback;
    IntegrandParameters integrand_parameters {};
    integrand_parameters.binding_radius = parameters.binding_radius;
    integrand_parameters.diffusion_total = parameters.diffusion_total;
    integrand_parameters.association_rate = parameters.association_rate;
    integrand_parameters.time = parameters.time;

    gsl_set_error_handler_off();
    gsl_integration_workspace* workspace =
        gsl_integration_workspace_alloc(1000000);

    double radius { integrand_parameters.binding_radius };
    while (radius <= parameters.max_radius + radius_step_size) {
      integrand_parameters.initial_radius = radius;
      gsl_matrix_set(survival_matrix, 0, index, radius);
      function.params = reinterpret_cast<void*>(&integrand_parameters);

      const double result { IntegrateSemiInfinite(
          function, &integrand_parameters, workspace, &SurvivalCallback) };
      if (parameters.association_rate < 1.0 / 0.0) {
        gsl_matrix_set(survival_matrix, 1, index, result);
      } else {
        gsl_matrix_set(survival_matrix, 1, index, 1.0 - result);
      }
      ++index;
      radius += radius_step_size;
    }

    gsl_integration_workspace_free(workspace);
    gsl_set_error_handler(nullptr);
  }

  static void FillNormMatrix(gsl_matrix* norm_matrix,
                             const TableParameters& parameters,
                             double radius_step_size) {
    const double x_low { parameters.binding_radius };
    const double eps_abs { 1.0e-6 };
    const double eps_rel { 1.0e-6 };
    std::size_t index { 0 };

    gsl_function function {};
    function.function = &NormCallback;
    IntegrandParameters integrand_parameters {};
    integrand_parameters.binding_radius = parameters.binding_radius;
    integrand_parameters.diffusion_total = parameters.diffusion_total;
    integrand_parameters.association_rate = parameters.association_rate;
    integrand_parameters.time = parameters.time;

    double radius { integrand_parameters.binding_radius };
    while (radius <= parameters.max_radius + radius_step_size) {
      gsl_integration_workspace* workspace =
          gsl_integration_workspace_alloc(10000000);
      integrand_parameters.initial_radius = radius;
      gsl_matrix_set(norm_matrix, 0, index, radius);
      function.params = reinterpret_cast<void*>(&integrand_parameters);

      double result {};
      double error {};
      gsl_integration_qagiu(&function, x_low, eps_abs, eps_rel, 10000000,
                            workspace, &result, &error);
      gsl_matrix_set(norm_matrix, 1, index, result);
      gsl_integration_workspace_free(workspace);
      ++index;
      radius += radius_step_size;
    }
  }

  static void FillIrreversibleMatrix(gsl_matrix* pir_matrix,
                                     const TableParameters& parameters,
                                     double radius_step_size) {
    int initial_index { 0 };
    int current_index { 0 };
    gsl_function function {};
    function.function = &IrreversibleCallback;
    IntegrandParameters integrand_parameters {};
    integrand_parameters.binding_radius = parameters.binding_radius;
    integrand_parameters.diffusion_total = parameters.diffusion_total;
    integrand_parameters.association_rate = parameters.association_rate;
    integrand_parameters.time = parameters.time;

    gsl_set_error_handler_off();
    gsl_integration_workspace* workspace =
        gsl_integration_workspace_alloc(1000000);

    double current_radius { integrand_parameters.binding_radius };
    while (current_radius <= parameters.max_radius + radius_step_size) {
      integrand_parameters.current_radius = current_radius;
      double initial_radius { integrand_parameters.binding_radius };
      while (initial_radius <= parameters.max_radius + radius_step_size) {
        integrand_parameters.initial_radius = initial_radius;
        function.params = reinterpret_cast<void*>(&integrand_parameters);
        const double result { IntegrateSemiInfinite(
            function, &integrand_parameters, workspace,
            &IrreversibleCallback) };
        gsl_matrix_set(pir_matrix, initial_index, current_index, result);
        ++initial_index;
        initial_radius += radius_step_size;
      }
      initial_index = 0;
      ++current_index;
      current_radius += radius_step_size;
    }

    gsl_integration_workspace_free(workspace);
    gsl_set_error_handler(nullptr);
  }

private:
  static double SurvivalCallback(double x, void* parameters) {
    const IntegrandParameters* values =
        reinterpret_cast<IntegrandParameters*>(parameters);
    return SurvivalProbabilityIntegrand(
        x, values->binding_radius, values->diffusion_total,
        values->association_rate, values->initial_radius, values->time);
  }

  static double NormCallback(double x, void* parameters) {
    const IntegrandParameters* values =
        reinterpret_cast<IntegrandParameters*>(parameters);
    return FreeDiffusionNormIntegrand(
        x, values->initial_radius, values->diffusion_total, values->time);
  }

  static double IrreversibleCallback(double x, void* parameters) {
    const IntegrandParameters* values =
        reinterpret_cast<IntegrandParameters*>(parameters);
    return IrreversibleProbabilityIntegrand(
        x, values->binding_radius, values->diffusion_total,
        values->association_rate, values->initial_radius,
        values->current_radius, values->time);
  }
};

} // namespace core
} // namespace nerdss
