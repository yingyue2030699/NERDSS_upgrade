#include "core/probability_engine.hpp"
#include "reactions/shared_reaction_functions.hpp"

double loop_closure_probability(double timeStep, double associationRate, double loopCoopFactor)
{
    return nerdss::core::ProbabilityEngine::LoopClosureAssociationProbability(
        timeStep, associationRate, loopCoopFactor);
}
