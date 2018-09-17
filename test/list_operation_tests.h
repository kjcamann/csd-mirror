#ifndef BDS_LIST_OPERATION_TESTS_H
#define BDS_LIST_OPERATION_TESTS_H

#include <catch2/catch.hpp>
#include "list_test_util.h"

// Number of iterations to run for random input tests (merge, sort, etc.)
constexpr std::size_t nIter = 1UL << 10;

template <bds::LinkedList ListHeadType>
void merge_tests() {
  using E = ListHeadType::value_type;

  constexpr auto comp = [](const E &lhs, const E &rhs) { return lhs.i < rhs.i; };
  constexpr auto equal = [](const E &lhs, const E &rhs) { return lhs.i == rhs.i; };

  SECTION("simple_case") {
    ListHeadType head1;
    ListHeadType head2;

    E e[] = { {0}, {1}, {2}, {3} };

    insert_front(head1, { &e[0], &e[2] });
    insert_front(head2, { &e[1], &e[3] });

    const auto totalSize = std::size(head1) + std::size(head2);

    head1.merge(head2, comp);

    REQUIRE( head2.empty() );
    REQUIRE( std::size(head1) == 4 );

    auto [ isSorted, isSizeOk ] =
        is_sorted_check(std::begin(head1), std::end(head1), comp, totalSize);

    REQUIRE( isSorted );
    REQUIRE( isSizeOk );

    if constexpr (bds::STailQ<ListHeadType>)
      REQUIRE( std::addressof(*head1.before_end()) == &e[3] );

    // Check that merge with an empty list is a no-op
    head1.merge(head2, comp);
    REQUIRE( head2.empty() );

    std::tie(isSorted, isSizeOk) =
        is_sorted_check(std::begin(head1), std::end(head1), comp, totalSize);

    REQUIRE( isSorted );
    REQUIRE( isSizeOk );

    // Check that merge with self is a no-op
    head1.merge(head1, comp);

    std::tie(isSorted, isSizeOk) =
        is_sorted_check(std::begin(head1), std::end(head1), comp, totalSize);

    REQUIRE( isSorted );
    REQUIRE( isSizeOk );
  }

  SECTION("random") {
    std::random_device trueRandom;
    std::default_random_engine engine{trueRandom()};
    std::uniform_int_distribution size_dist{0, 100};
    bool anyTestFailed = false;

    for (std::size_t i = 0; i < nIter; ++i) {
      ListHeadType lhs, rhs;

      const std::size_t lhsSize = size_dist(engine);
      const std::size_t rhsSize = size_dist(engine);

      const auto lhsSeed = populateSortedList(lhs, lhsSize, 0, 2 * lhsSize);
      const auto rhsSeed = populateSortedList(rhs, rhsSize, 0, 2 * rhsSize);

      lhs.merge(rhs, comp);

      const auto totalSize = lhsSize + rhsSize;

      const bool rhsIsEmpty = rhs.empty();
      const bool lhsSizeFnOk = std::size(lhs) == totalSize;
      auto [ lhsSorted, lhsSizeOk ] =
          is_sorted_check(std::begin(lhs), std::end(lhs), comp, totalSize);
      bool beforeEndOk = true;

      if constexpr (bds::STailQ<ListHeadType>) {
	// Ensure that before_end() is properly maintained by the merge for
	// singly-linked tail queues.
        auto last = std::max_element(std::cbegin(lhs), std::cend(lhs), comp);

        if (last != std::cend(lhs)) {
          while (std::next(last) != std::cend(lhs) && equal(*last, *std::next(last)))
            ++last;
        }

        beforeEndOk = last == lhs.cbefore_end();
      }

      const bool testPassed = rhsIsEmpty && lhsSizeFnOk && lhsSorted && lhsSizeOk
          && beforeEndOk;

      if (!testPassed) {
        std::fprintf(stderr, "merge test failed -- re: %c lszfn: %c lsort: %c "
                     "lsz: %c be: %c\n", rhsIsEmpty ? 'Y' : 'N',
                     lhsSizeFnOk ? 'Y' : 'N', lhsSorted ? 'Y' : 'N',
                     lhsSizeOk ? 'Y' : 'N', beforeEndOk ? 'Y' : 'N');
        std::fprintf(stderr, "lhs seed: %zu, lhs size: %zu\n",
                     std::size_t(lhsSeed), lhsSize);
        std::fprintf(stderr, "rhs seed: %zu, rhs size: %zu\n",
                     std::size_t(rhsSeed), rhsSize);
	anyTestFailed = true;
      }

      destroyList(lhs);
      destroyList(rhs);
    }

    REQUIRE( !anyTestFailed );
  }
}

template <bds::LinkedList ListHeadType>
void remove_tests() {
  using E = ListHeadType::value_type;
  ListHeadType head;

  E e[] = { {0}, {1}, {2}, {4}, {3} };
  insert_front(head, { &e[0], &e[1], &e[2], &e[3], &e[4] });
  const auto nRemoved =
      head.remove_if([] (const E &item) { return (item.i & 0x1) == 0; });

  REQUIRE( nRemoved == 3 );
  REQUIRE( std::size(head) == 2 );
  auto i = head.begin();
  REQUIRE( std::addressof(*i++) == &e[1] );
  REQUIRE( std::addressof(*i++) == &e[4] );
  REQUIRE( i == head.end() );
  if constexpr (bds::STailQ<ListHeadType>)
    REQUIRE( std::addressof(*head.before_end()) == &e[4] );

  // Also test uniform container erasure
  erase_if(head, [] (const E &item) { return (item.i) & 0x1 == 1; });
  REQUIRE( std::size(head) == 0 );
}

template <bds::LinkedList ListHeadType>
void reverse_tests() {
  using E = ListHeadType::value_type;
  ListHeadType head;

  E e[] = { {0}, {1}, {2} };
  insert_front(head, { &e[0], &e[1], &e[2] });
  head.reverse();

  auto i = head.begin();
  REQUIRE( std::addressof(*i++) == &e[2] );
  REQUIRE( std::addressof(*i++) == &e[1] );
  REQUIRE( std::addressof(*i++) == &e[0] );
  REQUIRE( i == head.end() );
  if constexpr (bds::STailQ<ListHeadType>)
    REQUIRE( std::addressof(*head.before_end()) == &e[0] );

  // Check reverse of empty list is a no-op
  head.clear();
  head.reverse();
  REQUIRE( head.empty() );
  if constexpr (bds::STailQ<ListHeadType>)
    REQUIRE( head.before_end() == head.end() );
}

template <bds::LinkedList ListHeadType>
void unique_tests() {
  using E = ListHeadType::value_type;
  ListHeadType head;

  E e[] = { {0}, {0}, {1}, {1}, {2} };
  insert_front(head, { &e[0], &e[1], &e[2], &e[3], &e[4] });
  head.unique([] (const E &lhs, const E &rhs) { return lhs.i == rhs.i; });

  REQUIRE( std::size(head) == 3 );
  auto i = head.begin();
  REQUIRE( std::addressof(*i++) == &e[0] );
  REQUIRE( std::addressof(*i++) == &e[2] );
  REQUIRE( std::addressof(*i++) == &e[4] );
  REQUIRE( i == head.end() );

  if constexpr (bds::STailQ<ListHeadType>)
    REQUIRE( std::addressof(*head.before_end()) == &e[4] );
}

template <bds::LinkedList ListHeadType>
void sort_tests() {
  using E = ListHeadType::value_type;
  ListHeadType head;

  E e[] = { {0}, {1}, {2}, {3}, {0} };
  constexpr auto comp = [] (const E &lhs, const E &rhs) { return lhs.i < rhs.i; };
  constexpr auto equal = [](const E &lhs, const E &rhs) { return lhs.i == rhs.i; };

  SECTION("reversed") {
    insert_front(head, { &e[3], &e[2], &e[1], &e[0] });
    head.sort(comp);

    REQUIRE( std::size(head) == 4 );
    auto i = head.begin();
    REQUIRE( std::addressof(*i++) == &e[0] );
    REQUIRE( std::addressof(*i++) == &e[1] );
    REQUIRE( std::addressof(*i++) == &e[2] );
    REQUIRE( std::addressof(*i++) == &e[3] );
    REQUIRE( i == head.end() );

    if constexpr (bds::STailQ<ListHeadType>)
      REQUIRE( std::addressof(*head.before_end()) == &e[3] );
  }

  SECTION("sequence1") {
    insert_front(head, { &e[2], &e[0], &e[3] });
    head.sort(comp);

    REQUIRE( std::size(head) == 3 );
    auto i = head.begin();
    REQUIRE( std::addressof(*i++) == &e[0] );
    REQUIRE( std::addressof(*i++) == &e[2] );
    REQUIRE( std::addressof(*i++) == &e[3] );
    REQUIRE( i == head.end() );

    if constexpr (bds::STailQ<ListHeadType>)
      REQUIRE( std::addressof(*head.before_end()) == &e[3] );
  }

  SECTION("sequence2") {
    insert_front(head, { &e[0], &e[3], &e[4] });
    head.sort(comp);

    REQUIRE( std::size(head) == 3 );
    auto i = head.begin();
    REQUIRE( std::addressof(*i++) == &e[4] );
    REQUIRE( std::addressof(*i++) == &e[0] );
    REQUIRE( std::addressof(*i++) == &e[3] );
    REQUIRE( i == head.end() );

    if constexpr (bds::STailQ<ListHeadType>)
      REQUIRE( std::addressof(*head.before_end()) == &e[3] );
  }

  SECTION("random") {
    std::random_device trueRandom;
    std::default_random_engine engine{trueRandom()};
    std::uniform_int_distribution size_dist{0, 100};
    bool anyTestFailed = false;

    for (std::size_t i = 0; i < nIter; ++i) {
      ListHeadType head;

      const std::size_t size = size_dist(engine);
      const auto seed = populateRandomList(head, size, 0, 2 * size);
      head.sort(comp);

      const bool sizeFnOk = std::size(head) == size;
      auto [ sorted, sizeOk ] =
          is_sorted_check(std::begin(head), std::end(head), comp, size);
      bool beforeEndOk = true;

      if constexpr (bds::STailQ<ListHeadType>) {
        // See comments in merge_tests.
        auto last = std::max_element(std::cbegin(head), std::cend(head), comp);

        if (last != std::cend(head)) {
          while (std::next(last) != std::cend(head) && equal(*last, *std::next(last)))
            ++last;
        }

        beforeEndOk = last == head.cbefore_end();
      }

      const bool testPassed = sizeFnOk && sorted && sizeOk && beforeEndOk;

      if (!testPassed) {
        std::fprintf(stderr, "sort test failed -- szfn: %c sort: %c sz: %c "
                     "be: %c\n", sizeFnOk ? 'Y' : 'N', sorted ? 'Y' : 'N',
                     sizeOk ? 'Y' : 'N', beforeEndOk ? 'Y' : 'N');
        std::fprintf(stderr, "seed: %zu, size: %zu\n", std::size_t(seed), size);
	anyTestFailed = true;
      }

      destroyList(head);
    }

    REQUIRE( !anyTestFailed );
  }
}

#endif
