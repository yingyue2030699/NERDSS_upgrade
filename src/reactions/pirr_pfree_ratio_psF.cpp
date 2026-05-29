#include "core/probability_engine.hpp"

double pirr_pfree_ratio_psF(
    double rCurr, double r0, double tCurr, double Dtot, double bindrad, double alpha, double ps_prev, double rtol)
{
    return nerdss::core::ProbabilityEngine::RebindingProbabilityRatio3D(
        rCurr, r0, tCurr, Dtot, bindrad, alpha, ps_prev, rtol);
}
