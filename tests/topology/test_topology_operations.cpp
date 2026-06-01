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

void test_multiple_appended_reservations_release_by_tail_compaction()
{
    reset_empty_complexes();
    std::vector<Complex> complexes(2);

    unsigned first_reserved_index =
        nerdss::reactions::topology::ReserveDissociationComplexSlot(complexes);
    unsigned second_reserved_index =
        nerdss::reactions::topology::ReserveDissociationComplexSlot(complexes);

    require_equal(first_reserved_index, 2, "first reserve should append at tail");
    require_equal(second_reserved_index, 3, "second reserve should append at tail");
    require_equal(
        static_cast<unsigned>(complexes.size()), 4,
        "two reservations should append two complex slots");

    nerdss::reactions::topology::ReleaseReservedComplexSlot(
        second_reserved_index, complexes);
    require_equal(
        static_cast<unsigned>(complexes.size()), 3,
        "release should compact the most recent appended reservation");

    nerdss::reactions::topology::ReleaseReservedComplexSlot(
        first_reserved_index, complexes);
    require_equal(
        static_cast<unsigned>(complexes.size()), 2,
        "release should compact the remaining appended reservation");
    require_true(
        Complex::emptyComList.empty(),
        "tail compaction should not add entries to the empty-list");
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

void test_apply_dissociation_reassignment_for_larger_split()
{
    std::vector<Molecule> molecules(10);
    for (int index = 0; index < static_cast<int>(molecules.size()); ++index) {
        molecules[index].myComIndex = 9;
    }

    std::vector<Complex> complexes(6);
    complexes[2].memberList = { 99 };
    complexes[5].memberList = { 42 };
    complexes[5].index = 1234;

    std::vector<int> parent_members { 8, 0, 6, 2 };
    std::vector<int> new_members { 9, 7, 5, 3, 1 };

    nerdss::reactions::topology::ApplyDissociationParentComplexReassignment(
        2, 5, parent_members, new_members, molecules, complexes);

    require_vector_equal(
        complexes[2].memberList, { 0, 2, 6, 8 },
        "large apply should sort parent-side members");
    require_vector_equal(
        complexes[5].memberList, { 1, 3, 5, 7, 9 },
        "large apply should sort new-complex members");
    require_equal(
        complexes[5].index, 5,
        "large apply should update the new complex index");

    for (int member : complexes[2].memberList) {
        require_equal(
            molecules[member].myComIndex, 2,
            "large apply should update every parent-side molecule");
    }
    for (int member : complexes[5].memberList) {
        require_equal(
            molecules[member].myComIndex, 5,
            "large apply should update every new-complex molecule");
    }
    require_equal(
        molecules[4].myComIndex, 9,
        "large apply should leave non-member molecules untouched");
}

void test_reused_empty_slot_can_receive_dissociation_reassignment()
{
    reset_empty_complexes();
    std::vector<Molecule> molecules(6);
    std::vector<Complex> complexes(5);

    complexes[4].isEmpty = true;
    complexes[4].memberList = { -1 };
    Complex::emptyComList.push_back(4);

    unsigned new_complex_index =
        nerdss::reactions::topology::ReserveDissociationComplexSlot(complexes);

    std::vector<int> parent_members { 0, 2, 4 };
    std::vector<int> new_members { 1, 3, 5 };

    nerdss::reactions::topology::ApplyDissociationParentComplexReassignment(
        1, new_complex_index, parent_members, new_members, molecules,
        complexes);

    require_equal(
        new_complex_index, 4,
        "dissociation should reuse the latest valid empty complex slot");
    require_equal(
        static_cast<unsigned>(complexes.size()), 5,
        "reused empty slot should not grow the complex list");
    require_true(
        Complex::emptyComList.empty(),
        "reused empty slot should be consumed before reassignment");
    require_vector_equal(
        complexes[4].memberList, { 1, 3, 5 },
        "reused empty slot should receive the new split members");
    for (int member : complexes[4].memberList) {
        require_equal(
            molecules[member].myComIndex, 4,
            "reused empty slot should own its reassigned molecules");
    }
}

void test_complex_slot_compaction_move_updates_members()
{
    std::vector<Molecule> molecules(7);
    for (int index = 0; index < static_cast<int>(molecules.size()); ++index) {
        molecules[index].myComIndex = 99;
    }
    molecules[1].myComIndex = 5;
    molecules[3].myComIndex = 5;
    molecules[6].myComIndex = 5;

    std::vector<Complex> complexes(6);
    complexes[1].isEmpty = true;
    complexes[1].index = 1;
    complexes[5].index = 5;
    complexes[5].id = 77;
    complexes[5].memberList = { 6, 1, 3 };

    nerdss::reactions::topology::ApplyComplexSlotCompactionMove(
        1, 5, molecules, complexes);

    require_equal(
        complexes[1].index, 1,
        "compaction move should rewrite moved complex index");
    require_equal(
        complexes[1].id, 77,
        "compaction move should preserve moved complex metadata");
    require_vector_equal(
        complexes[1].memberList, { 6, 1, 3 },
        "compaction move should preserve moved complex members");
    for (int member : complexes[1].memberList) {
        require_equal(
            molecules[member].myComIndex, 1,
            "compaction move should repoint moved member molecules");
    }
    require_equal(
        molecules[0].myComIndex, 99,
        "compaction move should leave non-members untouched");
}

void test_compact_empty_complex_slots_moves_live_tail_complexes()
{
    reset_empty_complexes();
    std::vector<Molecule> molecules(8);
    for (int index = 0; index < static_cast<int>(molecules.size()); ++index) {
        molecules[index].myComIndex = 99;
    }

    std::vector<Complex> complexes(6);
    complexes[0].index = 0;
    complexes[0].id = 10;
    complexes[0].memberList = { 0 };
    molecules[0].myComIndex = 0;

    complexes[1].index = 1;
    complexes[1].isEmpty = true;

    complexes[2].index = 2;
    complexes[2].id = 20;
    complexes[2].memberList = { 2 };
    molecules[2].myComIndex = 2;

    complexes[3].index = 3;
    complexes[3].isEmpty = true;

    complexes[4].index = 4;
    complexes[4].id = 40;
    complexes[4].memberList = { 4, 6 };
    molecules[4].myComIndex = 4;
    molecules[6].myComIndex = 4;

    complexes[5].index = 5;
    complexes[5].id = 50;
    complexes[5].memberList = { 5, 7 };
    molecules[5].myComIndex = 5;
    molecules[7].myComIndex = 5;

    Complex::emptyComList.push_back(3);
    Complex::emptyComList.push_back(1);

    nerdss::reactions::topology::CompactEmptyComplexSlots(molecules, complexes);

    require_equal(
        static_cast<unsigned>(complexes.size()), 4,
        "complex compaction should remove one slot per empty complex");
    require_true(
        Complex::emptyComList.empty(),
        "complex compaction should clear the empty-complex list");
    require_equal(
        complexes[1].index, 1,
        "complex compaction should rewrite first moved complex index");
    require_equal(
        complexes[1].id, 50,
        "complex compaction should move the last live complex first");
    require_vector_equal(
        complexes[1].memberList, { 5, 7 },
        "complex compaction should preserve first moved member order");
    require_equal(
        complexes[3].index, 3,
        "complex compaction should rewrite second moved complex index");
    require_equal(
        complexes[3].id, 40,
        "complex compaction should move the next live tail complex");
    require_vector_equal(
        complexes[3].memberList, { 4, 6 },
        "complex compaction should preserve second moved member order");
    require_equal(
        molecules[5].myComIndex, 1,
        "complex compaction should repoint first moved member");
    require_equal(
        molecules[7].myComIndex, 1,
        "complex compaction should repoint second first-move member");
    require_equal(
        molecules[4].myComIndex, 3,
        "complex compaction should repoint second moved member");
    require_equal(
        molecules[6].myComIndex, 3,
        "complex compaction should repoint second second-move member");
    require_equal(
        molecules[0].myComIndex, 0,
        "complex compaction should leave earlier live members untouched");
    require_equal(
        molecules[2].myComIndex, 2,
        "complex compaction should leave middle live members untouched");
}

void test_compact_empty_complex_slots_trims_tail_empty_slots()
{
    reset_empty_complexes();
    std::vector<Molecule> molecules(2);
    molecules[0].myComIndex = 0;
    molecules[1].myComIndex = 1;

    std::vector<Complex> complexes(4);
    complexes[0].index = 0;
    complexes[0].memberList = { 0 };
    complexes[1].index = 1;
    complexes[1].memberList = { 1 };
    complexes[2].index = 2;
    complexes[2].isEmpty = true;
    complexes[3].index = 3;
    complexes[3].isEmpty = true;

    Complex::emptyComList.push_back(3);
    Complex::emptyComList.push_back(2);

    nerdss::reactions::topology::CompactEmptyComplexSlots(molecules, complexes);

    require_equal(
        static_cast<unsigned>(complexes.size()), 2,
        "tail-only complex compaction should trim empty tail slots");
    require_true(
        Complex::emptyComList.empty(),
        "tail-only complex compaction should clear empty-complex list");
    require_vector_equal(
        complexes[0].memberList, { 0 },
        "tail-only complex compaction should leave first live complex in place");
    require_vector_equal(
        complexes[1].memberList, { 1 },
        "tail-only complex compaction should leave second live complex in place");
    require_equal(
        molecules[0].myComIndex, 0,
        "tail-only complex compaction should leave first molecule index");
    require_equal(
        molecules[1].myComIndex, 1,
        "tail-only complex compaction should leave second molecule index");
}

} // namespace

int main()
{
    test_reserve_appends_when_no_empty_slot_exists();
    test_reserve_reuses_latest_valid_empty_slot();
    test_reserve_appends_when_latest_empty_slot_entry_is_stale();
    test_release_removes_appended_reservation();
    test_release_returns_reused_reservation_to_empty_list();
    test_multiple_appended_reservations_release_by_tail_compaction();
    test_restore_dissociation_reactants_to_parent_complex();
    test_apply_dissociation_parent_complex_reassignment();
    test_apply_dissociation_reassignment_for_larger_split();
    test_reused_empty_slot_can_receive_dissociation_reassignment();
    test_complex_slot_compaction_move_updates_members();
    test_compact_empty_complex_slots_moves_live_tail_complexes();
    test_compact_empty_complex_slots_trims_tail_empty_slots();

    std::cout << "topology_operations tests passed\n";
    return 0;
}
