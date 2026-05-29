#include "core/probability_engine.hpp"

double passocF(double r0, double tCurr, double Dtot, double bindRadius, double alpha, double cof)
{
    return nerdss::core::ProbabilityEngine::AssociationProbability3D(
        r0, tCurr, Dtot, bindRadius, alpha, cof);
}
