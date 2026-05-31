#pragma once

#include "classes/class_Molecule_Complex.hpp"

#include <algorithm>
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

inline void RestoreDissociationReactantsToParentComplex(
    int pro1_index, int pro2_index, int parent_complex_index,
    std::vector<Molecule>& molecule_list)
{
    molecule_list[pro1_index].myComIndex = parent_complex_index;
    molecule_list[pro2_index].myComIndex = parent_complex_index;
}

inline void ApplyDissociationParentComplexReassignment(
    int parent_complex_index, int new_complex_index,
    std::vector<int>& parent_members, std::vector<int>& new_members,
    std::vector<Molecule>& molecule_list, std::vector<Complex>& complex_list)
{
    complex_list[parent_complex_index].memberList.swap(parent_members);
    complex_list[new_complex_index].memberList.swap(new_members);
    complex_list[new_complex_index].index = new_complex_index;

    std::sort(
        complex_list[parent_complex_index].memberList.begin(),
        complex_list[parent_complex_index].memberList.end());
    std::sort(
        complex_list[new_complex_index].memberList.begin(),
        complex_list[new_complex_index].memberList.end());

    for (auto& mol_index : complex_list[parent_complex_index].memberList) {
        molecule_list[mol_index].myComIndex = parent_complex_index;
    }
    for (auto& mol_index : complex_list[new_complex_index].memberList) {
        molecule_list[mol_index].myComIndex = new_complex_index;
    }
}

inline void ApplyComplexSlotCompactionMove(
    int target_complex_index, int source_complex_index,
    std::vector<Molecule>& molecule_list, std::vector<Complex>& complex_list)
{
    complex_list[target_complex_index] = complex_list[source_complex_index];
    complex_list[target_complex_index].index = target_complex_index;

    for (auto& mol_index : complex_list[target_complex_index].memberList) {
        molecule_list[mol_index].myComIndex = target_complex_index;
    }
}

} // namespace topology
} // namespace reactions
} // namespace nerdss
