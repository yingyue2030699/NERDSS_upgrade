#include "core/probability_engine.hpp"
#include "reactions/bimolecular/2D_reaction_table_functions.hpp"

double get_prevNorm(gsl_matrix *normMatrix, double RStepSize, double r0, double bindRadius)
{
    return nerdss::core::ProbabilityEngine::PreviousNormProbability2D(
        normMatrix, RStepSize, r0, bindRadius);
}
