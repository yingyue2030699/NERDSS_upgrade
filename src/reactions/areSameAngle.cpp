#include "core/math_engine.hpp"
#include "reactions/association/association.hpp"

bool areSameAngle(double ang1, double ang2)
{
    return nerdss::core::MathEngine::AreAnglesNearlyEqual(ang1, ang2);
}
