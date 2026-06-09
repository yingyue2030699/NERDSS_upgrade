#include "reactions/bimolecular/2D_reaction_table_functions.hpp"
#include "reactions/bimolecular/probability_engine.hpp"

size_t size_lookup(double bindRadius, double Dtot, const Parameters& params, double Rmax) {
  const nerdss::probability::TwoDReactionTableSpec spec{bindRadius, Dtot, Rmax, 0.0,
                                                        params.timeStep};
  return nerdss::probability::two_d_table_size(spec);
}
