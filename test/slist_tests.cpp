#include <algorithm>
#include <cstring>
#include <type_traits>

#include <catch/catch.hpp>
#include <bds/slist.h>

#include "list_modifier_tests.h"
#include "list_operation_tests.h"

using namespace bds;

using S = BaseS<slist_entry>;
using T = BaseT<slist_entry>;
using U = BaseU<slist_entry>;

using SEntryOffset = slist_entry_offset<offsetof(S, next)>;

using sl_head_t = slist_head<S, SEntryOffset, no_size>;
using sl_head_inline_t = slist_head<S, SEntryOffset, std::size_t>;
using sl_head_invoke_t = slist_head<T, constexpr_invocable<&T::next>, no_size>;
using sl_head_stateful_t = slist_head<U, U::accessor_type, no_size>;

using sl_fwd_head_t = slist_fwd_head<no_size>;
using sl_container_t = slist_container<S, SEntryOffset, no_size>;
using sl_container_stateful_t = slist_container<U, U::accessor_type, no_size>;
using sl_test_container_t = list_test_container<sl_container_t>;
using sl_test_container_stateful_t = list_test_container<sl_container_stateful_t>;

// Compile-time tests of list traits classes.
static_assert(SList<sl_head_t>);
static_assert(SList<sl_container_t>);
static_assert(SList<sl_test_container_t>);

static_assert(!SList<sl_fwd_head_t>);
static_assert(!SList<int>);

static_assert(!TailQ<sl_head_t>);
static_assert(!TailQ<sl_container_t>);

TEST_CASE("slist.basic.offset", "[slist][basic][offset]") {
  basic_tests<sl_head_t>();
}

TEST_CASE("slist.basic.offset_inline_size", "[slist][basic][offset][inline]") {
  basic_tests<sl_head_inline_t>();
}

TEST_CASE("slist.basic.member_invoke", "[slist][basic][member_invoke]") {
  basic_tests<sl_head_invoke_t>();
}

TEST_CASE("slist.basic.forward", "[slist][basic][forward]") {
  basic_tests<sl_test_container_t>();
}

TEST_CASE("slist.basic.stateful", "[slist][basic][stateful]") {
  basic_tests<sl_head_stateful_t>();
}

TEST_CASE("slist.dtor", "[slist]") { dtor_test<sl_head_t>(); }

TEST_CASE("slist.clear", "[slist]") {
  SECTION("offset_no_size") { clear_tests<sl_head_t>(); }
  SECTION("offset_inline_size") { clear_tests<sl_head_inline_t>(); }
}

TEST_CASE("slist.move", "[slist]") {
  SECTION("stateless") { move_tests<sl_head_t, sl_test_container_t>(); }
  SECTION("stateful") { move_tests<sl_head_stateful_t, sl_test_container_stateful_t>(); }
}

TEST_CASE("slist.extra_ctor", "[slist]") {
  extra_ctor_tests<sl_head_t>();
  extra_ctor_tests<sl_test_container_t>();
}

TEST_CASE("slist.bulk_insert", "[slist]") {
  bulk_insert_tests<sl_head_t>();
  bulk_insert_tests<sl_test_container_t>();
}

TEST_CASE("slist.bulk_erase", "[slist]") {
  sl_head_t head;
  S s[] = { S{.i = 0}, S{.i = 1}, S{.i = 2} };

  head.insert_after(head.before_begin(), { &s[0], &s[1], &s[2] });

  // Remove (s1, end), then check that the list is still valid and == [s1].
  auto i = head.erase_after(head.begin(), head.end());
  REQUIRE( std::size(head) == 1 );
  REQUIRE( i == head.end() );
  REQUIRE( std::addressof(*head.begin()) == &s[0] );

  // Remove (before_begin, end) and check that the list is empty.
  i = head.erase_after(head.before_begin(), head.end());
  REQUIRE( i == head.end() );
  REQUIRE( head.empty() );

  // Remove the empty range (begin, end) -- this is a no-op, so it must leave
  // the list in a valid state.
  i = head.erase_after(head.begin(), head.end());
  REQUIRE( i == head.end() );
  REQUIRE( head.empty() );
  i = head.erase_after(head.before_begin(), head.end());
  REQUIRE( i == head.end() );
  REQUIRE( head.empty() );

  // Check that list is still usable after empty range erase.
  head.insert_after(head.before_begin(), &s[0]);
  REQUIRE( std::size(head) == 1 );
}

TEST_CASE("slist.for_each_safe", "[slist]") {
  for_each_safe_tests<sl_head_t>();
}

TEST_CASE("slist.push_pop", "[slist]") {
  sl_head_t head;

  S s[] = { S{.i = 0}, S{.i = 1} };

  head.push_front(&s[1]);
  REQUIRE( std::addressof(*head.begin()) == &s[1] );
  REQUIRE( std::size(head) == 1 );
  REQUIRE( !head.empty() );
  REQUIRE( ++head.begin() == head.end() );

  head.push_front(&s[0]);
  REQUIRE( std::size(head) == 2 );
  REQUIRE( std::addressof(*head.begin()) == &s[0] );

  head.pop_front();
  REQUIRE( std::size(head) == 1 );
  REQUIRE( std::addressof(*head.begin()) == &s[1] );

  head.pop_front();
  REQUIRE( head.empty() );
}

TEST_CASE("slist.swap", "[slist]") {
  SECTION("stateless") { swap_tests<sl_head_t, sl_test_container_t>(); }
  SECTION("stateful") { swap_tests<sl_head_stateful_t, sl_test_container_stateful_t>(); }
}

TEST_CASE("slist.find_predecessor", "[slist]") {
  find_predecessor_tests<sl_head_t>();
}

TEST_CASE("slist.merge", "[slist][oper]") { merge_tests<sl_head_t>(); }

TEST_CASE("slist.splice", "[slist][oper]") {
  sl_head_t head1;
  sl_head_t head2;

  S s[] = { S{.i = 0}, S{.i = 1}, S{.i = 2}, S{.i = 3}, S{.i = 4}, S{.i = 5} };

  SECTION("splice_in_middle") {
    head1.insert_after(head1.before_begin(), { &s[0], &s[1], &s[5] });
    head2.insert_after(head2.before_begin(), { &s[2], &s[3], &s[4] });

    head1.splice_after(++head1.begin(), head2);
    REQUIRE( head2.empty() );
    int idx = 0;
    for (const S &item : head1)
      REQUIRE( item.i == s[idx++].i );
  }

  SECTION("splice_at_end") {
    head1.insert_after(head1.before_begin(), { &s[0], &s[1] });
    head2.insert_after(head2.before_begin(), { &s[2], &s[3] });

    head1.splice_after(++head1.begin(), head2);
    REQUIRE( head2.empty() );
    int idx = 0;
    for (const S &item : head1)
      REQUIRE( item.i == s[idx++].i );
  }

  SECTION("splice_empty") {
    head1.insert_after(head1.before_begin(), { &s[0], &s[1] });

    head1.splice_after(++head1.begin(), head2);
    REQUIRE( head2.empty() );
    int idx = 0;
    for (const S &item : head1)
      REQUIRE( item.i == s[idx++].i );
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

    i = head2.cbegin();
    REQUIRE( std::size(head2) == 2 );
    REQUIRE( std::addressof(*i++) == &s[1] );
    REQUIRE( std::addressof(*i++) == &s[4] );
    REQUIRE( i == head2.cend() );
  }

  SECTION("other_derived_type") {
    sl_fwd_head_t fwdHead2;
    sl_container_t head2{fwdHead2};

    head1.insert_after(head1.before_begin(), { &s[0], &s[5] });
    head2.insert_after(head2.before_begin(), { &s[1], &s[2], &s[3], &s[4] });

    head1.splice_after(head1.begin(), head2, head2.begin(),
                       std::next(head2.begin(), 3));

    REQUIRE( std::size(head1) == 4 );
    auto i1 = head1.cbegin();
    REQUIRE( std::addressof(*i1++)== &s[0] );
    REQUIRE( std::addressof(*i1++)== &s[2] );
    REQUIRE( std::addressof(*i1++)== &s[3] );
    REQUIRE( std::addressof(*i1++)== &s[5] );
    REQUIRE( i1 == head1.cend() );

    REQUIRE( std::size(head2) == 2 );
    auto i2 = head2.cbegin();
    REQUIRE( std::addressof(*i2++) == &s[1] );
    REQUIRE( std::addressof(*i2++) == &s[4] );
    REQUIRE( i2 == head2.cend() );
  }
}

TEST_CASE("slist.remove", "[slist][oper]") { remove_tests<sl_head_t>(); }

TEST_CASE("slist.reverse", "[slist][oper]") { reverse_tests<sl_head_t>(); }

TEST_CASE("slist.unique", "[slist][oper]") { unique_tests<sl_head_t>(); }

TEST_CASE("slist.sort", "[slist][oper]") { sort_tests<sl_head_t>(); }
