#pragma once

#include "classes/class_Molecule_Complex.hpp"

#include <vector>

namespace nerdss {
namespace reactions {
namespace topology {

inline unsigned ReserveDissociationComplexSlot(std::vector<Complex>& complex_list)
{
    unsigned new_complex_index = complex_list.size();
    if (!Complex::emptyComList.empty()
        && complex_list[Complex::emptyComList.back()].isEmpty) {
        new_complex_index = Complex::emptyComList.back();
        Complex::emptyComList.pop_back();
    } else {
        complex_list.emplace_back();
    }
    return new_complex_index;
}

inline void ReleaseReservedComplexSlot(
    unsigned complex_index, std::vector<Complex>& complex_list)
{
    if (complex_index + 1 == complex_list.size()) {
        complex_list.pop_back();
    } else {
        Complex::emptyComList.push_back(complex_index);
    }
}

} // namespace topology
} // namespace reactions
} // namespace nerdss
