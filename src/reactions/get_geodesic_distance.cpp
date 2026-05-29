#include "core/math_engine.hpp"
#include "classes/class_Coord.hpp"
#include "reactions/association/association.hpp"

double get_geodesic_distance(Coord intFace1, Coord intFace2)
{
    return nerdss::core::MathEngine::GeodesicDistance(intFace1, intFace2);
}
