#include "core/probability_engine.hpp"

double pirr_pfree_ratio_psF_1D(
    double rCurr, double r0, double tCurr, double Dtot, double bindrad, double ka, double ps_prev)
{
    return nerdss::core::ProbabilityEngine::RebindingProbabilityRatio1D(
        rCurr, r0, tCurr, Dtot, bindrad, ka, ps_prev);
}
