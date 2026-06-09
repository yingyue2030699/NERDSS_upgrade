#include "reactions/bimolecular/probability_engine.hpp"

#include <cmath>

namespace nerdss {
namespace probability {
namespace {

void free_matrix_vector(std::vector<gsl_matrix*>& matrices) {
  for (gsl_matrix* matrix : matrices) {
    if (matrix != nullptr) {
      gsl_matrix_free(matrix);
    }
  }
  matrices.clear();
}

}  // namespace

double two_d_table_step_size(const TwoDReactionTableSpec& spec) {
  return std::sqrt(spec.diffusion_total * spec.time_step) / 50;
}

std::size_t two_d_table_size(const TwoDReactionTableSpec& spec) {
  int count{0};
  const double step_size{two_d_table_step_size(spec)};

  double radius_index{spec.bind_radius};
  while (radius_index <= spec.r_max + step_size) {
    count += 1;
    radius_index += step_size;
  }

  return static_cast<std::size_t>(count);
}

TwoDReactionTableMatrices allocate_two_d_table_matrices(std::size_t table_size) {
  return TwoDReactionTableMatrices{
      gsl_matrix_alloc(2, table_size),
      gsl_matrix_alloc(2, table_size),
      gsl_matrix_alloc(table_size, table_size),
  };
}

TwoDReactionTableMatrices allocate_and_store_two_d_table_matrices(
    std::vector<gsl_matrix*>& survival_matrices, std::vector<gsl_matrix*>& norm_matrices,
    std::vector<gsl_matrix*>& pir_matrices, std::size_t table_index, std::size_t table_size) {
  survival_matrices.resize(table_index + 1);
  norm_matrices.resize(table_index + 1);
  pir_matrices.resize(table_index + 1);

  const auto matrices = allocate_two_d_table_matrices(table_size);
  survival_matrices[table_index] = matrices.survival_matrix;
  norm_matrices[table_index] = matrices.norm_matrix;
  pir_matrices[table_index] = matrices.pir_matrix;
  return matrices;
}

void release_two_d_table_matrices(std::vector<gsl_matrix*>& survival_matrices,
                                  std::vector<gsl_matrix*>& norm_matrices,
                                  std::vector<gsl_matrix*>& pir_matrices) {
  free_matrix_vector(survival_matrices);
  free_matrix_vector(norm_matrices);
  free_matrix_vector(pir_matrices);
}

}  // namespace probability
}  // namespace nerdss
