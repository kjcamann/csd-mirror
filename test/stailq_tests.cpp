#include <algorithm>
#include <cstring>
#include <type_traits>

#include <catch/catch.hpp>
#include <bds/stailq.h>

#include "list_modifier_tests.h"
#include "list_operation_tests.h"

using namespace bds;

using S = BaseS<stailq_entry>;
using T = BaseT<stailq_entry>;
using U = BaseU<stailq_entry>;

using SEntryOffset = stailq_entry_offset<offsetof(S, next)>;

using stq_head_t = stailq_head<S, SEntryOffset, no_size>;
using stq_head_inline_t = stailq_head<S, SEntryOffset, std::size_t>;
using stq_head_invoke_t = stailq_head<T, constexpr_invocable<&T::next>, no_size>;
using stq_head_stateful_t = stailq_head<U, U::accessor_type, no_size>;

using stq_fwd_head_t = stailq_fwd_head<no_size>;
using stq_container_t = stailq_container<S, SEntryOffset, no_size>;
using stq_container_stateful_t = stailq_container<U, U::accessor_type, no_size>;
using stq_test_container_t = list_test_container<stq_container_t>;
using stq_test_container_stateful_t = list_test_container<stq_container_stateful_t>;

// Compile-time tests of list traits classes.
static_assert(STailQ<stq_head_t>);
static_assert(STailQ<stq_container_t>);
static_assert(STailQ<stq_test_container_t>);

static_assert(!STailQ<stq_fwd_head_t>);
static_assert(!STailQ<int>);

static_assert(!TailQ<stq_head_t>);
static_assert(!TailQ<stq_container_t>);

TEST_CASE("stailq.basic.offset", "[stailq][basic][offset]") {
  basic_tests<stq_head_t>();
}

TEST_CASE("stailq.basic.offset_inline_size", "[stailq][basic][offset]") {
  basic_tests<stq_head_inline_t>();
}

TEST_CASE("stailq.basic.member_invoke", "[stailq][basic][member_invoke]") {
  basic_tests<stq_head_invoke_t>();
}

TEST_CASE("stailq.basic.forward", "[stailq][basic][forward]") {
  basic_tests<stq_test_container_t>();
}

TEST_CASE("stailq.basic.stateful", "[stailq][basic][stateful]") {
  basic_tests<stq_head_stateful_t>();
}

TEST_CASE("stailq.dtor", "[stailq]") { dtor_test<stq_head_t>(); }

TEST_CASE("stailq.clear", "[stailq]") {
  SECTION("offset_no_size") { clear_tests<stq_head_t>(); }
  SECTION("offset_inline_size") { clear_tests<stq_head_inline_t>(); }
}

TEST_CASE("stailq.move", "[stailq]") {
  SECTION("stateless") { move_tests<stq_head_t, stq_test_container_t>(); }
  SECTION("stateful") { move_tests<stq_head_stateful_t, stq_test_container_stateful_t>(); }
}

TEST_CASE("stailq.extra_ctor", "[stailq]") {
  extra_ctor_tests<stq_head_t>();
  extra_ctor_tests<stq_test_container_t>();
}

TEST_CASE("stailq.bulk_insert", "[stailq]") {
  bulk_insert_tests<stq_head_t>();
  bulk_insert_tests<stq_test_container_t>();

  // An extra test is added for the stailq; like bulk_insert.initializer_list,
  // but insert after the first element to test that before_end() will need to
  // be maintained.
  stq_head_t head;
  S s[] = { S{.i = 0}, S{.i = 1}, S{.i = 2} };

  head.insert_after(head.before_begin(), &s[0] );
  auto i = head.insert_after(head.before_end(), { &s[1], &s[2] });
  REQUIRE( std::size(head) == 3 );
  REQUIRE( std::addressof(*i++) == &s[2] );
  REQUIRE( i == head.end() );
  REQUIRE( std::addressof(head.front()) == &s[0] );
  REQUIRE( std::addressof(head.back()) == &s[2] );
}

TEST_CASE("stailq.bulk_erase", "[stailq]") {
  stq_head_t head;
  S s[] = { S{.i = 0}, S{.i = 1}, S{.i = 2} };

  head.insert_after(head.before_begin(), { &s[0], &s[1], &s[2] });

  // Remove (s1, end), then check that the list is still valid and == [s1].
  auto i = head.erase_after(head.begin(), head.end());
  REQUIRE( std::size(head) == 1 );
  REQUIRE( i == head.end() );
  REQUIRE( std::addressof(*head.begin()) == &s[0] );
  REQUIRE( std::addressof(*head.before_end()) == &s[0] );

  // Remove (before_begin, end) and check that the list is empty.
  i = head.erase_after(head.before_begin(), head.end());
  REQUIRE( i == head.end() );
  REQUIRE( head.before_end() == head.end() );
  REQUIRE( head.empty() );

  // Remove the empty range (begin, end) -- this is a no-op, so it must leave
  // the list in a valid state.
  i = head.erase_after(head.begin(), head.end());
  REQUIRE( i == head.end() );
  REQUIRE( head.before_end() == head.end() );
  REQUIRE( head.empty() );
  i = head.erase_after(head.before_begin(), head.end());
  REQUIRE( i == head.end() );
  REQUIRE( head.before_end() == head.end() );
  REQUIRE( head.empty() );

  // Check that list is still usable after empty range erase.
  head.insert_after(head.before_begin(), &s[0]);
  REQUIRE( std::size(head) == 1 );
  REQUIRE( std::addressof(head.front()) == &s[0] );
  REQUIRE( std::addressof(head.back()) == &s[0] );
}

TEST_CASE("stailq.for_each_safe", "[stailq]") {
  for_each_safe_tests<stq_head_t>();
}

TEST_CASE("stailq.push_pop", "[stailq]") {
  stq_head_t head;

  S s[] = { S{.i = 0}, S{.i = 1} };

  head.push_front(&s[0]);
  REQUIRE( std::addressof(*head.begin()) == &s[0] );
  REQUIRE( std::addressof(*head.before_end()) == &s[0] );
  REQUIRE( std::size(head) == 1 );
  REQUIRE( !head.empty() );
  REQUIRE( ++head.begin() == head.end() );

  head.push_back(&s[1]);
  REQUIRE( std::size(head) == 2 );
  REQUIRE( std::addressof(*head.begin()) == &s[0] );
  REQUIRE( std::addressof(*head.before_end()) == &s[1] );

  head.pop_front();
  REQUIRE( std::size(head) == 1 );
  REQUIRE( std::addressof(*head.begin()) == &s[1] );
  REQUIRE( std::addressof(*head.before_end()) == &s[1] );

  head.pop_front();
  REQUIRE( head.empty() );
  REQUIRE( head.before_end() == head.end() );
}

TEST_CASE("stailq.swap", "[stailq]") {
  SECTION("stateless") { swap_tests<stq_head_t, stq_test_container_t>(); }
  SECTION("stateful") { swap_tests<stq_head_stateful_t, stq_test_container_stateful_t>(); }
}

TEST_CASE("stailq.find_predecessor", "[stailq]") {
  find_predecessor_tests<stq_head_t>();
}

TEST_CASE("stailq.merge", "[stailq][oper]") { merge_tests<stq_head_t>(); }

TEST_CASE("stailq.splice", "[stailq][oper]") {
  stq_head_t head1;
  stq_head_t head2;

  S s[] = { S{.i = 0}, S{.i = 1}, S{.i = 2}, S{.i = 3}, S{.i = 4}, S{.i = 5} };

  SECTION("splice_in_middle") {
    head1.insert_after(head1.before_begin(), { &s[0], &s[1], &s[5] });
    head2.insert_after(head2.before_begin(), { &s[2], &s[3], &s[4] });

    head1.splice_after(++head1.begin(), head2);
    REQUIRE( head2.empty() );
    int idx = 0;
    for (const S &item : head1)
      REQUIRE( item.i == s[idx++].i );
    REQUIRE( std::addressof(head1.back()) == &s[5] );
  }

  SECTION("splice_at_end") {
    head1.insert_after(head1.before_begin(), { &s[0], &s[1] });
    head2.insert_after(head2.before_begin(), { &s[2], &s[3] });

    head1.splice_after(++head1.begin(), head2);
    REQUIRE( head2.empty() );
    int idx = 0;
    for (const S &item : head1)
      REQUIRE( item.i == s[idx++].i );
    REQUIRE( std::addressof(head1.back()) == &s[3] );
  }

  SECTION("splice_empty") {
    head1.insert_after(head1.before_begin(), { &s[0], &s[1] });

    head1.splice_after(++head1.begin(), head2);
    REQUIRE( head2.empty() );
    int idx = 0;
    for (const S &item : head1)
      REQUIRE( item.i == s[idx++].i );
    REQUIRE( std::addressof(head1.back()) == &s[1] );
  }

  SECTION("splice_partial") {
    head1.insert_after(head1.before_begin(), { &s[0], &s[5] });
    head2.insert_after(head2.before_begin(), { &s[1], &s[2], &s[3], &s[4] });

    head1.splice_after(head1.begin(), head2, head2.begin(),
                       std::next(head2.begin(), 3));

    auto i = head1.cbegin();
    REQUIRE( std::size(head1) == 4 );
    REQUIRE( std::addressof(*i++)== &s[0] );
    REQUIRE( std::addressof(*i++)== &s[2] );
    REQUIRE( std::addressof(*i++)== &s[3] );
    REQUIRE( std::addressof(*i++)== &s[5] );
    REQUIRE( i == head1.cend() );
    REQUIRE( std::addressof(head1.back()) == &s[5] );

    i = head2.cbegin();
    REQUIRE( std::size(head2) == 2 );
    REQUIRE( std::addressof(*i++) == &s[1] );
    REQUIRE( std::addressof(*i++) == &s[4] );
    REQUIRE( i == head2.cend() );
    REQUIRE( std::addressof(head2.back()) == &s[4] );
  }

  SECTION("other_derived_type") {
    stq_fwd_head_t fwdHead2;
    stq_container_t head2{fwdHead2};

    head1.insert_after(head1.before_begin(), { &s[0], &s[5] });
    head2.insert_after(head2.before_begin(), { &s[1], &s[2], &s[3], &s[4] });

    head1.splice_after(head1.begin(), head2, head2.begin(),
                       std::next(head2.begin(), 3));

    auto i1 = head1.cbegin();
    REQUIRE( std::size(head1) == 4 );
    REQUIRE( std::addressof(*i1++)== &s[0] );
    REQUIRE( std::addressof(*i1++)== &s[2] );
    REQUIRE( std::addressof(*i1++)== &s[3] );
    REQUIRE( std::addressof(*i1++)== &s[5] );
    REQUIRE( i1 == head1.cend() );
    REQUIRE( std::addressof(head1.back()) == &s[5] );

    auto i2 = head2.cbegin();
    REQUIRE( std::size(head2) == 2 );
    REQUIRE( std::addressof(*i2++) == &s[1] );
    REQUIRE( std::addressof(*i2++) == &s[4] );
    REQUIRE( i2 == head2.cend() );
    REQUIRE( std::addressof(head2.back()) == &s[4] );
  }
}

TEST_CASE("stailq.remove", "[stailq][oper]") { remove_tests<stq_head_t>(); }

TEST_CASE("stailq.reverse", "[stailq][oper]") { reverse_tests<stq_head_t>(); }

TEST_CASE("stailq.unique", "[stailq][oper]") { unique_tests<stq_head_t>(); }

TEST_CASE("stailq.sort", "[stailq][oper]") { sort_tests<stq_head_t>(); }
