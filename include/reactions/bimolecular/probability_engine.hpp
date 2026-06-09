#pragma once

#include <gsl/gsl_matrix.h>

#include <cstddef>

namespace nerdss {
namespace probability {

struct TwoDReactionTableSpec {
  double bind_radius;
  double diffusion_total;
  double r_max;
  double reaction_rate;
  double time_step;
};

struct TwoDReactionTableMatrices {
  gsl_matrix* survival_matrix;
  gsl_matrix* norm_matrix;
  gsl_matrix* pir_matrix;
};

double two_d_table_step_size(const TwoDReactionTableSpec& spec);

std::size_t two_d_table_size(const TwoDReactionTableSpec& spec);

TwoDReactionTableMatrices allocate_two_d_table_matrices(std::size_t table_size);

}  // namespace probability
}  // namespace nerdss
