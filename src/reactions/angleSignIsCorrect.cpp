#include "core/math_engine.hpp"
#include "reactions/association/association.hpp"

bool angleSignIsCorrect(const Vector& vec1, const Vector& vec2)
{
    return nerdss::core::MathEngine::IsAngleSignCorrect(vec1, vec2);
}
