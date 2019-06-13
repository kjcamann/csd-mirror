#ifndef CSD_LIST_OPERATION_TESTS_H
#define CSD_LIST_OPERATION_TESTS_H

#include <catch2/catch.hpp>
#include "list_test_util.h"

// Number of iterations to run for random input tests (merge, sort, etc.)
constexpr std::size_t nIter = 1UL << 10;

template <csg::linked_list ListType>
void merge_tests() {
  using E = CSG_TYPENAME ListType::value_type;

  SECTION("simple_case") {
    ListType head1;
    ListType head2;

    E e[] = { {0}, {1}, {2}, {3} };

    insert_front(head1, { &e[0], &e[2] });
    insert_front(head2, { &e[1], &e[3] });

    const auto totalSize = std::size(head1) + std::size(head2);

    head1.merge(head2, {}, get_value<E>);

    REQUIRE( head2.empty() );
    REQUIRE( std::size(head1) == totalSize );

    auto [isSorted, isSizeOk] = is_sorted_check(head1, totalSize, {}, get_value<E>);
    REQUIRE( isSorted );
    REQUIRE( isSizeOk );

    if constexpr (csg::stailq<ListType>)
      REQUIRE( std::addressof(*head1.before_end()) == &e[3] );

    // Check that merge with an empty list is a no-op
    head1.merge(head2, {}, get_value<E>);
    REQUIRE( head2.empty() );

    std::tie(isSorted, isSizeOk) = is_sorted_check(head1, totalSize, {}, get_value<E>);
    REQUIRE( isSorted );
    REQUIRE( isSizeOk );

    // Check that merge with self is a no-op
    head1.merge(head1, {}, get_value<E>);

    std::tie(isSorted, isSizeOk) = is_sorted_check(head1, totalSize, {}, get_value<E>);
    REQUIRE( isSorted );
    REQUIRE( isSizeOk );
  }

  SECTION("simple_case.default_compare") {
    ListType head1;
    ListType head2;

    E e[] = { {0}, {1}, {2}, {3} };

    insert_front(head1, { &e[0], &e[2] });
    insert_front(head2, { &e[1], &e[3] });

    const auto totalSize = std::size(head1) + std::size(head2);

    head1.merge(head2, {}, get_value<E>);

    REQUIRE( head2.empty() );
    REQUIRE( std::size(head1) == totalSize );

    auto [isSorted, isSizeOk] = is_sorted_check(head1, totalSize);
    REQUIRE( isSorted );
    REQUIRE( isSizeOk );
  }

  SECTION("empty") {
    // Check that merge of two empty lists is a no-op; the reason for this
    // test is to ensure that an stailq's before_end() iterator remains equal
    // to that stailq's own before_begin().
    ListType head1;
    ListType head2;

    head1.merge(head2);

    REQUIRE( head1.empty() );
    REQUIRE( head2.empty() );
    if constexpr (csg::stailq<ListType>) {
      REQUIRE( head1.before_begin() == head1.before_end() );
      REQUIRE( head2.before_begin() == head2.before_end() );
    }
  }

  SECTION("random") {
    std::random_device trueRandom;
    std::default_random_engine engine{trueRandom()};
    std::uniform_int_distribution size_dist{0, 100};
    bool anyTestFailed = false;

    for (std::size_t i = 0; i < nIter; ++i) {
      ListType lhs, rhs;

      const std::size_t lhsSize = size_dist(engine);
      const std::size_t rhsSize = size_dist(engine);

      const auto lhsSeed = populateSortedList(lhs, lhsSize, 0, 2 * lhsSize);
      const auto rhsSeed = populateSortedList(rhs, rhsSize, 0, 2 * rhsSize);

      lhs.merge(rhs);

      const auto totalSize = lhsSize + rhsSize;

      const bool rhsIsEmpty = rhs.empty();
      const bool lhsSizeFnOk = std::size(lhs) == totalSize;
      auto [lhsSorted, lhsSizeOk] = is_sorted_check(lhs, totalSize);
      bool beforeEndOk = true;

      if constexpr (csg::stailq<ListType>) {
	// Ensure that before_end() is properly maintained by the merge for
	// singly-linked tail queues.
	if (std::ranges::empty(lhs))
          beforeEndOk = lhs.before_begin() == lhs.before_end();
	else {
          auto last = std::ranges::max_element(lhs);

          if (last != std::cend(lhs)) {
            while (std::ranges::next(last) != std::ranges::cend(lhs) &&
                   *last == *std::ranges::next(last))
              ++last;
          }

          beforeEndOk = last == lhs.cbefore_end();
        }
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

template <csg::linked_list ListType>
void splice_tests() {
  using E = CSG_TYPENAME ListType::value_type;

  ListType head1;
  ListType head2;

  E e[] = { {0}, {1}, {2}, {3}, {4}, {5} };

  SECTION("splice_in_middle") {
    insert_front(head1, { &e[0], &e[1], &e[5] });
    insert_front(head2, { &e[2], &e[3], &e[4] });

    if constexpr (csg::tailq<ListType>)
      head1.splice(--head1.end(), head2);
    else
      head1.splice_after(++head1.begin(), head2);

    REQUIRE( head2.empty() );
    for (int idx = 0; const E &item : head1)
      REQUIRE( get_value(item) == get_value(e[idx++]) );
    if constexpr (csg::stailq<ListType> || csg::tailq<ListType>)
      REQUIRE( std::addressof(head1.back()) == &e[5] );
  }

  SECTION("splice_at_end") {
    insert_front(head1, { &e[0], &e[1] });
    insert_front(head2, { &e[2], &e[3] });

    if constexpr (csg::tailq<ListType>)
      head1.splice(head1.end(), head2);
    else
      head1.splice_after(++head1.begin(), head2);

    REQUIRE( head2.empty() );
    for (int idx = 0; const E &item : head1)
      REQUIRE( get_value(item) == get_value(e[idx++]) );
    if constexpr (csg::stailq<ListType> || csg::tailq<ListType>)
      REQUIRE( std::addressof(head1.back()) == &e[3] );
  }

  SECTION("splice_empty") {
    insert_front(head1, { &e[0], &e[1] });

    if constexpr (csg::tailq<ListType>)
      head1.splice(head1.end(), head2);
    else
      head1.splice_after(++head1.begin(), head2);

    REQUIRE( head2.empty() );
    for (int idx = 0; const E &item : head1)
      REQUIRE( get_value(item) == get_value(e[idx++]) );
    if constexpr (csg::stailq<ListType> || csg::tailq<ListType>)
      REQUIRE( std::addressof(head1.back()) == &e[1] );
  }

  SECTION("splice_partial") {
    insert_front(head1, { &e[0] });
    insert_front(head2, { &e[1], &e[2], &e[3], &e[4] });

    if constexpr (csg::tailq<ListType>) {
      head1.splice(head1.end(), head2, head2.begin(),
                   std::prev(head2.end(), 2));
    }
    else {
      head1.splice_after(head1.begin(), head2, head2.before_begin(),
                         std::next(head2.begin(), 2));
    }

    auto i = head1.cbegin();
    REQUIRE( std::size(head1) == 3 );
    REQUIRE( std::addressof(*i++)== &e[0] );
    REQUIRE( std::addressof(*i++)== &e[1] );
    REQUIRE( std::addressof(*i++)== &e[2] );
    REQUIRE( i == head1.cend() );
    if constexpr (csg::stailq<ListType> || csg::tailq<ListType>)
      REQUIRE( std::addressof(head1.back()) == &e[2] );

    i = head2.cbegin();
    REQUIRE( std::size(head2) == 2 );
    REQUIRE( std::addressof(*i++) == &e[3] );
    REQUIRE( std::addressof(*i++) == &e[4] );
    REQUIRE( i == head2.cend() );
    if constexpr (csg::stailq<ListType> || csg::tailq<ListType>)
    REQUIRE( std::addressof(head2.back()) == &e[4] );
  }
}

template <csg::linked_list ListType1, csg::linked_list ListType2>
void splice_tests_other_derived() {
  static_assert(std::same_as<typename ListType1::value_type,
                             typename ListType2::value_type>);
  using E = CSG_TYPENAME ListType1::value_type;

  ListType1 head1;
  ListType2 head2;

  E e[] = { {0}, {1}, {2}, {3}, {4} };

  insert_front(head1, { &e[0] });
  insert_front(head2, { &e[1], &e[2], &e[3], &e[4] });

  if constexpr (csg::tailq<ListType1>) {
    head1.splice(head1.end(), head2, head2.begin(),
                 std::prev(head2.end(), 2));
  }
  else {
    head1.splice_after(head1.begin(), head2, head2.before_begin(),
                       std::next(head2.begin(), 2));
  }

  auto i1 = head1.cbegin();
  REQUIRE( std::size(head1) == 3 );
  REQUIRE( std::addressof(*i1++)== &e[0] );
  REQUIRE( std::addressof(*i1++)== &e[1] );
  REQUIRE( std::addressof(*i1++)== &e[2] );
  REQUIRE( i1 == head1.cend() );
  if constexpr (csg::stailq<ListType1> || csg::tailq<ListType1>)
    REQUIRE( std::addressof(head1.back()) == &e[2] );

  auto i2 = head2.cbegin();
  REQUIRE( std::size(head2) == 2 );
  REQUIRE( std::addressof(*i2++) == &e[3] );
  REQUIRE( std::addressof(*i2++) == &e[4] );
  REQUIRE( i2 == head2.cend() );
  if constexpr (csg::stailq<ListType2> || csg::tailq<ListType2>)
    REQUIRE( std::addressof(head2.back()) == &e[4] );
}

template <csg::linked_list ListType>
void remove_tests() {
  using E = CSG_TYPENAME ListType::value_type;
  ListType head;

  E e[] = { {0}, {1}, {2}, {4}, {3} };
  insert_front(head, { &e[0], &e[1], &e[2], &e[3], &e[4] });

  SECTION("remove_if.without_projection") {
    const auto nRemoved = head.remove_if([] (const E &item) {
      return (get_value(item) & 0x1) == 0;
    });

    REQUIRE( nRemoved == 3 );
    REQUIRE( std::size(head) == 2 );
    auto i = head.begin();
    REQUIRE( std::addressof(*i++) == &e[1] );
    REQUIRE( std::addressof(*i++) == &e[4] );
    REQUIRE( i == head.end() );
    if constexpr (csg::stailq<ListType>)
      REQUIRE( std::addressof(*head.before_end()) == &e[4] );
  }

  SECTION("remove_if.with_projection") {
    const auto nRemoved = head.remove_if([] (const std::int64_t i) {
      return (i & 0x1) == 0;
    }, get_value<E>);

    REQUIRE( nRemoved == 3 );
    REQUIRE( std::size(head) == 2 );
    auto i = head.begin();
    REQUIRE( std::addressof(*i++) == &e[1] );
    REQUIRE( std::addressof(*i++) == &e[4] );
    REQUIRE( i == head.end() );
    if constexpr (csg::stailq<ListType>)
      REQUIRE( std::addressof(*head.before_end()) == &e[4] );
  }

  SECTION("remove.uniform_container_erasure") {
    const auto nRemoved = erase_if(head, [] (const E &item) {
      return (get_value(item) & 0x1) == 1;
    });

    REQUIRE( nRemoved == 2 );
    REQUIRE( std::size(head) == 3 );
  }

  SECTION("remove.projection") {
    auto nRemoved = head.remove(1, {}, get_value<E>);
    REQUIRE( nRemoved == 1 );

    nRemoved = head.remove(3, {}, get_value<E>);
    REQUIRE( nRemoved == 1 );

    REQUIRE( std::size(head) == 3 );
  }

  SECTION("remove.by_value") {
    const auto nRemoved = head.remove(E{0});
    REQUIRE( nRemoved == 1 );
    REQUIRE( std::size(head) == 4 );
  }
}

template <csg::linked_list ListType>
void reverse_tests() {
  using E = CSG_TYPENAME ListType::value_type;
  ListType head;

  E e[] = { {0}, {1}, {2} };
  insert_front(head, { &e[0], &e[1], &e[2] });
  head.reverse();

  auto i = head.begin();
  REQUIRE( std::addressof(*i++) == &e[2] );
  REQUIRE( std::addressof(*i++) == &e[1] );
  REQUIRE( std::addressof(*i++) == &e[0] );
  REQUIRE( i == head.end() );
  if constexpr (csg::stailq<ListType>)
    REQUIRE( std::addressof(*head.before_end()) == &e[0] );

  // Check reverse of empty list is a no-op
  head.clear();
  head.reverse();
  REQUIRE( head.empty() );
  if constexpr (csg::stailq<ListType>)
    REQUIRE( head.before_end() == head.end() );
}

template <csg::linked_list ListType>
void unique_tests() {
  using E = CSG_TYPENAME ListType::value_type;
  ListType head;

  E e[] = { {0}, {0}, {1}, {1}, {2} };
  insert_front(head, { &e[0], &e[1], &e[2], &e[3], &e[4] });

  SECTION("unique.projection") {
    head.unique(std::ranges::equal_to{}, get_value<E>);

    REQUIRE( std::size(head) == 3 );
    auto i = head.begin();
    REQUIRE( std::addressof(*i++) == &e[0] );
    REQUIRE( std::addressof(*i++) == &e[2] );
    REQUIRE( std::addressof(*i++) == &e[4] );
    REQUIRE( i == head.end() );

    if constexpr (csg::stailq<ListType>)
      REQUIRE( std::addressof(*head.before_end()) == &e[4] );
  }

  SECTION("unique.by_value") {
    // Test default comparison equivalence relation (std::ranges::equal_to)
    // and the default projection (std::identity).
    head.unique();

    REQUIRE( std::size(head) == 3 );
    auto i = head.begin();
    REQUIRE( std::addressof(*i++) == &e[0] );
    REQUIRE( std::addressof(*i++) == &e[2] );
    REQUIRE( std::addressof(*i++) == &e[4] );
    REQUIRE( i == head.end() );

    if constexpr (csg::stailq<ListType>)
      REQUIRE( std::addressof(*head.before_end()) == &e[4] );
  }
}

template <csg::linked_list ListType>
void sort_tests() {
  using E = CSG_TYPENAME ListType::value_type;
  ListType head;

  E e[] = { {0}, {1}, {2}, {3}, {0} };
  constexpr auto proj = get_value<E>;

  SECTION("reversed") {
    insert_front(head, { &e[3], &e[2], &e[1], &e[0] });
    head.sort({}, proj);

    REQUIRE( std::size(head) == 4 );
    auto i = head.begin();
    REQUIRE( std::addressof(*i++) == &e[0] );
    REQUIRE( std::addressof(*i++) == &e[1] );
    REQUIRE( std::addressof(*i++) == &e[2] );
    REQUIRE( std::addressof(*i++) == &e[3] );
    REQUIRE( i == head.end() );

    if constexpr (csg::stailq<ListType>)
      REQUIRE( std::addressof(*head.before_end()) == &e[3] );
  }

  SECTION("sequence1") {
    insert_front(head, { &e[2], &e[0], &e[3] });
    head.sort({}, proj);

    REQUIRE( std::size(head) == 3 );
    auto i = head.begin();
    REQUIRE( std::addressof(*i++) == &e[0] );
    REQUIRE( std::addressof(*i++) == &e[2] );
    REQUIRE( std::addressof(*i++) == &e[3] );
    REQUIRE( i == head.end() );

    if constexpr (csg::stailq<ListType>)
      REQUIRE( std::addressof(*head.before_end()) == &e[3] );
  }

  SECTION("sequence2") {
    insert_front(head, { &e[0], &e[3], &e[4] });
    head.sort({}, proj);

    REQUIRE( std::size(head) == 3 );
    auto i = head.begin();
    REQUIRE( std::addressof(*i++) == &e[4] );
    REQUIRE( std::addressof(*i++) == &e[0] );
    REQUIRE( std::addressof(*i++) == &e[3] );
    REQUIRE( i == head.end() );

    if constexpr (csg::stailq<ListType>)
      REQUIRE( std::addressof(*head.before_end()) == &e[3] );
  }

  SECTION("random") {
    std::random_device trueRandom;
    std::default_random_engine engine{trueRandom()};
    std::uniform_int_distribution size_dist{0, 100};
    bool anyTestFailed = false;

    for (std::size_t i = 0; i < nIter; ++i) {
      ListType head;

      const std::size_t size = size_dist(engine);
      const auto seed = populateRandomList(head, size, 0, 2 * size);
      head.sort({}, proj);

      const bool sizeFnOk = std::size(head) == size;
      auto [sorted, sizeOk] = is_sorted_check(head, size, {}, proj);
      bool beforeEndOk = true;

      if constexpr (csg::stailq<ListType>) {
        // See comments in merge_tests.
        auto last = std::ranges::max_element(head, {}, proj);

        if (last != std::cend(head)) {
          while (std::ranges::next(last) != std::ranges::cend(head) &&
                 proj(*last) == proj(*std::ranges::next(last)))
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
