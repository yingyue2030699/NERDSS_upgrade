#include "core/probability_engine.hpp"
#include "reactions/bimolecular/2D_reaction_table_functions.hpp"
#include "tracing.hpp"

double calc_pirr(gsl_matrix* pirMatrix, gsl_matrix* survMatrix, double RStepSize, double r, double r0, double a)
{
    // TRACE();
    return nerdss::core::ProbabilityEngine::IrreversibleProbabilityTable2D(
        pirMatrix, survMatrix, RStepSize, r, r0, a);
}
