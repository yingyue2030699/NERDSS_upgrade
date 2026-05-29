#include "core/probability_engine.hpp"
#include "reactions/bimolecular/2D_reaction_table_functions.hpp"

double DDpirr_pfree_ratio_ps(gsl_matrix* pirMatrix, gsl_matrix* survMatrix, gsl_matrix* normMatrix, double r, double Dtot, double deltaT, double r0, double ps_prev, double rTol, double bindRadius)
{
    double pNormVal {};
    double pirrVal {};
    /*RstepSize is coupled to the DDmatrixcreate RstepSize, they must be the SAME definition,
     *so RstepSize should not be defined any other way!
     */
    const double RstepSize { std::sqrt(Dtot * deltaT) / 50 };

    const double pFree {
        nerdss::core::ProbabilityEngine::FreeDiffusionProbability2D(
            r, r0, Dtot, deltaT) };
    pNormVal = get_prevNorm(normMatrix, RstepSize, r0, bindRadius);
    pirrVal = calc_pirr(pirMatrix, survMatrix, RstepSize, r, r0, bindRadius);

    double pfreeN = pFree / pNormVal; // NORMALIZES TO ONE

    double ratio;
    if (std::abs(pirrVal - pfreeN * ps_prev) < rTol)
        ratio = 1.0;
    else {
        ratio = pirrVal / (pfreeN * ps_prev);
    }
    return ratio;
}
