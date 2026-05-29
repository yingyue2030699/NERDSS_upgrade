#include "core/probability_engine.hpp"
#include "reactions/bimolecular/2D_reaction_table_functions.hpp"

double pir_function(double x, void* p)
{
    IntegrandParams& params = *reinterpret_cast<IntegrandParams*>(p);
    return nerdss::core::ProbabilityEngine::IrreversibleProbabilityIntegrand2D(
        x, params.a, params.D, params.k, params.r0, params.r, params.t);
}
