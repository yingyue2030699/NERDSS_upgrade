#include "reactions/bimolecular/probability_engine.hpp"

#include <cmath>

namespace nerdss {
namespace probability {

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

}  // namespace probability
}  // namespace nerdss
