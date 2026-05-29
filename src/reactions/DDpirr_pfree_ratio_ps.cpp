#include "core/probability_engine.hpp"
#include "reactions/bimolecular/2D_reaction_table_functions.hpp"

double DDpirr_pfree_ratio_ps(gsl_matrix* pirMatrix, gsl_matrix* survMatrix, gsl_matrix* normMatrix, double r, double Dtot, double deltaT, double r0, double ps_prev, double rTol, double bindRadius)
{
    return nerdss::core::ProbabilityEngine::RebindingProbabilityRatioTable2D(
        pirMatrix, survMatrix, normMatrix, r, Dtot, deltaT, r0, ps_prev, rTol,
        bindRadius);
}
