#include "core/math_engine.hpp"
#include "reactions/association/association.hpp"

Vector create_arbitrary_vector(Vector& vec)
{
    vec.normalize();
    return nerdss::core::MathEngine::CreateArbitraryOrthogonalVector(vec);
}
