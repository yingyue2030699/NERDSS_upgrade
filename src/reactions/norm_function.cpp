#include "core/probability_engine.hpp"
#include "reactions/bimolecular/2D_reaction_table_functions.hpp"

double norm_function(double x, void* p)
{
    IntegrandParams& params = *reinterpret_cast<IntegrandParams*>(p);
    return nerdss::core::ProbabilityEngine::FreeDiffusionNormIntegrand2D(
        x, params.r0, params.D, params.t);
}
