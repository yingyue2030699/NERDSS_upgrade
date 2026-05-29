#include "core/math_engine.hpp"
#include "reactions/association/association.hpp"

bool areParallel(const double& angle)
{
    return nerdss::core::MathEngine::IsParallelAngle(angle);
}
