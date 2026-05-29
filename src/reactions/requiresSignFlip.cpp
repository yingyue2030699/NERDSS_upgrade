#include "core/math_engine.hpp"
#include "reactions/association/association.hpp"

bool requiresSignFlip(Vector axis, Vector v1, Vector v2)
{
    return nerdss::core::MathEngine::RequiresSignFlip(axis, v1, v2);
}
