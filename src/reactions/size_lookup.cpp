#include "core/probability_engine.hpp"
#include "reactions/bimolecular/2D_reaction_table_functions.hpp"

size_t size_lookup(double bindRadius, double Dtot, const Parameters& params, double Rmax)
{
    return nerdss::core::ProbabilityEngine::TableSize2D(
        bindRadius, Dtot, params.timeStep, Rmax);
}
