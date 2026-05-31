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

void require_equal(unsigned actual, unsigned expected, const std::string& label)
{
    if (actual != expected) {
        std::cerr << label << ": expected " << expected << ", got " << actual
                  << '\n';
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

} // namespace

int main()
{
    test_reserve_appends_when_no_empty_slot_exists();
    test_reserve_reuses_latest_valid_empty_slot();
    test_reserve_appends_when_latest_empty_slot_entry_is_stale();
    test_release_removes_appended_reservation();
    test_release_returns_reused_reservation_to_empty_list();

    std::cout << "topology_operations tests passed\n";
    return 0;
}
