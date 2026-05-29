#include "core/probability_engine.hpp"
#include "reactions/bimolecular/2D_reaction_table_functions.hpp"

double get_prevSurv(const gsl_matrix* survMatrix, double Dtot, double deltaT, double r0, double bindRadius)
{
    return nerdss::core::ProbabilityEngine::PreviousSurvivalProbability2D(
        survMatrix, Dtot, deltaT, r0, bindRadius);
}
