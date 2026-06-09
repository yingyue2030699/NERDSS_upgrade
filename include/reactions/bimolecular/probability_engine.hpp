#pragma once

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

double two_d_table_step_size(const TwoDReactionTableSpec& spec);

std::size_t two_d_table_size(const TwoDReactionTableSpec& spec);

}  // namespace probability
}  // namespace nerdss
