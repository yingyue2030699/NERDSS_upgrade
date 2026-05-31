#include "reactions/topology/topology_operations.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

std::vector<int> Complex::emptyComList {};

namespace {

void require_true(bool condition, const std::string& label)
{
    if (!condition) {
        std::cerr << label << '\n';
        std::exit(1);
    }
}

void require_equal(long long actual, long long expected, const std::string& label)
{
    if (actual != expected) {
        std::cerr << label << ": expected " << expected << ", got " << actual
                  << '\n';
        std::exit(1);
    }
}

void require_vector_equal(
    const std::vector<int>& actual, const std::vector<int>& expected,
    const std::string& label)
{
    if (actual != expected) {
        std::cerr << label << ": expected";
        for (int value : expected) {
            std::cerr << ' ' << value;
        }
        std::cerr << ", got";
        for (int value : actual) {
            std::cerr << ' ' << value;
        }
        std::cerr << '\n';
        std::exit(1);
    }
}

void reset_empty_complexes()
{
    Complex::emptyComList.clear();
}

void test_reserve_appends_when_no_empty_slot_exists()
{
    reset_empty_complexes();
    std::vector<Complex> complexes(2);

    unsigned reserved_index =
        nerdss::reactions::topology::ReserveDissociationComplexSlot(complexes);

    require_equal(reserved_index, 2, "reserve should return appended index");
    require_equal(
        static_cast<unsigned>(complexes.size()), 3,
        "reserve should append one complex");
    require_true(
        Complex::emptyComList.empty(),
        "reserve should not touch an empty empty-slot list");
}

void test_reserve_reuses_latest_valid_empty_slot()
{
    reset_empty_complexes();
    std::vector<Complex> complexes(3);
    complexes[1].isEmpty = true;
    Complex::emptyComList.push_back(1);

    unsigned reserved_index =
        nerdss::reactions::topology::ReserveDissociationComplexSlot(complexes);

    require_equal(reserved_index, 1, "reserve should reuse latest empty slot");
    require_equal(
        static_cast<unsigned>(complexes.size()), 3,
        "reserve should not append when reusing an empty slot");
    require_true(
        Complex::emptyComList.empty(),
        "reserve should consume the reused empty-slot entry");
}

void test_reserve_appends_when_latest_empty_slot_entry_is_stale()
{
    reset_empty_complexes();
    std::vector<Complex> complexes(2);
    complexes[1].isEmpty = false;
    Complex::emptyComList.push_back(1);

    unsigned reserved_index =
        nerdss::reactions::topology::ReserveDissociationComplexSlot(complexes);

    require_equal(reserved_index, 2, "reserve should append for stale entry");
    require_equal(
        static_cast<unsigned>(complexes.size()), 3,
        "reserve should append exactly one slot for stale entry");
    require_equal(
        static_cast<unsigned>(Complex::emptyComList.size()), 1,
        "reserve should preserve the stale empty-list entry");
    require_equal(
        static_cast<unsigned>(Complex::emptyComList.back()), 1,
        "reserve should leave the stale index on the list");
}

void test_release_removes_appended_reservation()
{
    reset_empty_complexes();
    std::vector<Complex> complexes(2);

    unsigned reserved_index =
        nerdss::reactions::topology::ReserveDissociationComplexSlot(complexes);
    nerdss::reactions::topology::ReleaseReservedComplexSlot(
        reserved_index, complexes);

    require_equal(
        static_cast<unsigned>(complexes.size()), 2,
        "release should remove appended reservation");
    require_true(
        Complex::emptyComList.empty(),
        "release of appended reservation should not add an empty-list entry");
}

void test_release_returns_reused_reservation_to_empty_list()
{
    reset_empty_complexes();
    std::vector<Complex> complexes(3);
    complexes[0].isEmpty = true;
    Complex::emptyComList.push_back(0);

    unsigned reserved_index =
        nerdss::reactions::topology::ReserveDissociationComplexSlot(complexes);
    nerdss::reactions::topology::ReleaseReservedComplexSlot(
        reserved_index, complexes);

    require_equal(
        static_cast<unsigned>(complexes.size()), 3,
        "release should not resize after reused reservation");
    require_equal(
        static_cast<unsigned>(Complex::emptyComList.size()), 1,
        "release should return reused slot to empty-list");
    require_equal(
        static_cast<unsigned>(Complex::emptyComList.back()), reserved_index,
        "release should return the reused slot index");
}

void test_restore_dissociation_reactants_to_parent_complex()
{
    std::vector<Molecule> molecules(4);
    molecules[0].myComIndex = 2;
    molecules[1].myComIndex = 3;
    molecules[2].myComIndex = 4;
    molecules[3].myComIndex = 5;

    nerdss::reactions::topology::RestoreDissociationReactantsToParentComplex(
        1, 3, 7, molecules);

    require_equal(
        molecules[0].myComIndex, 2,
        "restore should not touch non-reactant molecule before first reactant");
    require_equal(
        molecules[1].myComIndex, 7,
        "restore should put first reactant back on parent complex");
    require_equal(
        molecules[2].myComIndex, 4,
        "restore should not touch non-reactant molecule between reactants");
    require_equal(
        molecules[3].myComIndex, 7,
        "restore should put second reactant back on parent complex");
}

void test_apply_dissociation_parent_complex_reassignment()
{
    std::vector<Molecule> molecules(5);
    std::vector<Complex> complexes(3);
    complexes[1].memberList = { 9 };
    complexes[2].memberList = { 8 };
    complexes[2].index = 99;

    std::vector<int> parent_members { 4, 0, 2 };
    std::vector<int> new_members { 3, 1 };

    nerdss::reactions::topology::ApplyDissociationParentComplexReassignment(
        1, 2, parent_members, new_members, molecules, complexes);

    require_vector_equal(
        complexes[1].memberList, { 0, 2, 4 },
        "apply should sort reassigned parent members");
    require_vector_equal(
        complexes[2].memberList, { 1, 3 },
        "apply should sort reassigned new-complex members");
    require_equal(
        complexes[2].index, 2,
        "apply should update the new complex index to its list slot");
    require_equal(
        molecules[0].myComIndex, 1,
        "apply should point parent member 0 to parent complex");
    require_equal(
        molecules[2].myComIndex, 1,
        "apply should point parent member 2 to parent complex");
    require_equal(
        molecules[4].myComIndex, 1,
        "apply should point parent member 4 to parent complex");
    require_equal(
        molecules[1].myComIndex, 2,
        "apply should point new member 1 to new complex");
    require_equal(
        molecules[3].myComIndex, 2,
        "apply should point new member 3 to new complex");
}

} // namespace

int main()
{
    test_reserve_appends_when_no_empty_slot_exists();
    test_reserve_reuses_latest_valid_empty_slot();
    test_reserve_appends_when_latest_empty_slot_entry_is_stale();
    test_release_removes_appended_reservation();
    test_release_returns_reused_reservation_to_empty_list();
    test_restore_dissociation_reactants_to_parent_complex();
    test_apply_dissociation_parent_complex_reassignment();

    std::cout << "topology_operations tests passed\n";
    return 0;
}
