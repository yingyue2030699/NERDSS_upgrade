#include "core/probability_engine.hpp"

/**
 * @brief Association probability from Smoluchovski's reaction diffusion model
 * 
 * @param r0 original distance
 * @param tCurr current time
 * @param Dtot total diffusion constant
 * @param bindRadius sigma
 * @param alpha 
 * @param cof 
 * @return double 
 */
double passocF_1D(double r0, double tCurr, double Dtot, double bindRadius, double ka)
{
  return nerdss::core::ProbabilityEngine::AssociationProbability1D(
      r0, tCurr, Dtot, bindRadius, ka);
}
