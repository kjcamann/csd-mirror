#include <algorithm>
#include <cstring>
#include <cstdio>
#include <random>
#include <type_traits>

#include <catch2/catch.hpp>
#include <csd/tailq.h>

#include "list_modifier_tests.h"
#include "list_operation_tests.h"

using namespace csd;

using S = BaseS<tailq_entry>;
using T = BaseT<tailq_entry>;
using U = BaseU<tailq_entry>;

using tq_head_t = CSD_TAILQ_HEAD_OFFSET_T(S, next);
using tq_head_inline_t = CSD_TAILQ_HEAD_OFFSET_T(S, next, std::size_t);
using tq_head_invoke_t = tailq_head_cinvoke_t<&T::next>;
using tq_head_stateful_t = tailq_head<U, U::accessor_type>;

using tq_fwd_head_t = tailq_fwd_head<S>;
using tq_proxy_t = CSD_TAILQ_PROXY_OFFSET_T(S, next);
using tq_proxy_stateful_t = tailq_proxy<tailq_fwd_head<U>, U::accessor_type>;
using tq_test_proxy_t = list_test_proxy<tq_proxy_t>;
using tq_test_proxy_stateful_t = list_test_proxy<tq_proxy_stateful_t>;

// Compile-time tests of list traits classes.
static_assert(TailQ<tq_head_t>);
static_assert(TailQ<tq_proxy_t>);
static_assert(TailQ<tq_test_proxy_t>);

static_assert(!TailQ<tq_fwd_head_t>);
static_assert(!TailQ<int>);

static_assert(!SList<tq_head_t>);
static_assert(!SList<tq_proxy_t>);

// This doesn't really belong here, but we need to ensure both invocable_link
// and offset_link remain standard layout, so that they can use the common
// initial sequence rule to read from either `link_union` member, regardless
// of which member was written. This is needed to get proper default
// initialization for tailq_entry<T>, where we cannot know which union member
// will be used, so we pick one.
static_assert(
    std::is_standard_layout_v<csd::invocable_tagged_ref<tailq_entry<S>, S>> &&
    std::is_standard_layout_v<csd::offset_entry_ref<tailq_entry<S>>>);

TEST_CASE("tailq.basic.offset", "[tailq][basic][offset]") {
  // "offset" in the test case name refers to the offsetof(...) link accessor
  // template argument.
  basic_tests<tq_head_t>();
}

TEST_CASE("tailq.basic.offset_inline_size", "[tailq][basic][offset]") {
  basic_tests<tq_head_inline_t>();
}

TEST_CASE("tailq.basic.member_invoke", "[tailq][basic][member_invoke]") {
  basic_tests<tq_head_invoke_t>();
}

TEST_CASE("tailq.basic.proxy", "[tailq][basic][proxy]") {
  basic_tests<tq_test_proxy_t>();
}

TEST_CASE("tailq.basic.stateful", "[tailq][basic][stateful]") {
  basic_tests<tq_head_stateful_t>();
}

TEST_CASE("tailq.clear", "[tailq]") {
  SECTION("offset_no_size") { clear_tests<tq_head_t>(); }
  SECTION("offset_inline_size") { clear_tests<tq_head_inline_t>(); }
}

TEST_CASE("tailq.move", "[tailq]") {
  SECTION("stateless") { move_tests<tq_head_t, tq_test_proxy_t>(); }
  SECTION("stateful") { move_tests<tq_head_stateful_t, tq_test_proxy_stateful_t>(); }
}

TEST_CASE("tailq.extra_ctor", "[tailq]") {
  extra_ctor_tests<tq_head_t>();
  extra_ctor_tests<tq_test_proxy_t>();
}

TEST_CASE("tailq.bulk_insert", "[tailq]") {
  bulk_insert_tests<tq_head_t>();
  bulk_insert_tests<tq_test_proxy_t>();
}

TEST_CASE("tailq.bulk_erase", "[tailq]") {
  tq_head_t head;
  S s[] = { S{.i = 0}, S{.i = 1}, S{.i = 2} };

  head.insert(head.end(), { &s[0], &s[1], &s[2] });

  // Remove [s2, end), then check that the list is still valid and == [s1].
  auto i = head.erase(++head.begin(), head.end());
  REQUIRE( std::size(head) == 1 );
  REQUIRE( i == head.end() );
  REQUIRE( std::addressof(*--i) == &s[0] );
  REQUIRE( i == head.begin() );

  // Remove [s1, end) and check that the list is empty.
  i = head.erase(head.begin(), head.end());
  REQUIRE( i == head.end() );
  REQUIRE( head.empty() );

  // Remove the empty range [begin, end) -- this is a no-op, so it must leave
  // the list in a valid state.
  i = head.erase(head.begin(), head.end());
  REQUIRE( i == head.end() );
  REQUIRE( head.empty() );

  // Check that list is still usable after empty range erase.
  head.insert(head.end(), &s[0]);
  REQUIRE( std::size(head) == 1 );
}

TEST_CASE("tailq.for_each_safe", "[tailq]") {
  for_each_safe_tests<tq_head_t>();
}

TEST_CASE("tailq.reverse_iterator", "[tailq]") {
  tq_head_t head;

  S s[] = { S{.i = 0}, S{.i = 1} };

  auto i = head.insert(head.end(), { &s[0], &s[1] });

  REQUIRE( std::addressof(*i++) == &s[0] );
  REQUIRE( std::addressof(*i++) == &s[1] );
  REQUIRE( i == head.end() );

  auto ri = head.rbegin();
  REQUIRE( std::addressof(*ri++) == &s[1] );
  REQUIRE( std::addressof(*ri++) == &s[0] );
  REQUIRE( ri == head.rend() );

  REQUIRE( head.rbegin().base() == head.end() );
  REQUIRE( head.rend().base() == head.begin() );

  REQUIRE( head.rbegin() == head.crbegin() );
  REQUIRE( head.rend() == head.crend() );
}

TEST_CASE("tailq.push_pop", "[tailq]") {
  tq_head_t head;

  S s[] = { S{.i = 0}, S{.i = 1} };

  head.push_front(&s[0]);
  REQUIRE( std::addressof(*head.begin()) == &s[0] );
  REQUIRE( std::size(head) == 1 );
  REQUIRE( !head.empty() );
  REQUIRE( ++head.begin() == head.end() );

  head.push_back(&s[1]);
  REQUIRE( std::size(head) == 2 );
  REQUIRE( std::addressof(*--head.end()) == &s[1] );

  head.pop_front();
  REQUIRE( std::size(head) == 1 );
  REQUIRE( std::addressof(*head.begin()) == &s[1] );

  head.pop_back();
  REQUIRE( head.empty() );
}

TEST_CASE("tailq.swap", "[tailq]") {
  SECTION("stateless") { swap_tests<tq_head_t, tq_test_proxy_t>(); }
  SECTION("stateful") { swap_tests<tq_head_stateful_t, tq_test_proxy_stateful_t>(); }
}

TEST_CASE("tailq.proxy", "[tailq]") {
  proxy_tests<tq_fwd_head_t, tq_proxy_t>();
}

TEST_CASE("tailq.merge", "[tailq][oper]") { merge_tests<tq_head_t>(); }

TEST_CASE("tailq.splice", "[tailq][oper]") {
  tq_head_t head1;
  tq_head_t head2;

  S s[] = { S{.i = 0}, S{.i = 1}, S{.i = 2}, S{.i = 3} };

  SECTION("splice_in_middle") {
    head1.insert(head1.end(), { &s[0], &s[3] });
    head2.insert(head2.end(), { &s[1], &s[2] });

    head1.splice(++head1.begin(), head2);
    REQUIRE( head2.empty() );
    int idx = 0;
    for (const S &item : head1)
      REQUIRE( item.i == s[idx++].i );
  }

  SECTION("splice_at_end") {
    head1.insert(head1.end(), { &s[0], &s[1] });
    head2.insert(head2.end(), { &s[2], &s[3] });

    head1.splice(head1.end(), head2);
    REQUIRE( head2.empty() );
    int idx = 0;
    for (const S &item : head1)
      REQUIRE( item.i == s[idx++].i );
  }

  SECTION("splice_empty") {
    head1.insert(head1.end(), { &s[0], &s[1], &s[2], &s[3] });

    head1.splice(head1.end(), head2);
    REQUIRE( head2.empty() );
    int idx = 0;
    for (const S &item : head1)
      REQUIRE( item.i == s[idx++].i );
  }

  SECTION("splice_partial") {
    head1.insert(head1.end(), { &s[0] });
    head2.insert(head2.end(), { &s[1], &s[2], &s[3] });

    head1.splice(head1.end(), head2, head2.begin(), --head2.end());

    REQUIRE( std::size(head2) == 1 );
    REQUIRE( std::addressof(*head2.begin()) == &s[3] );
    REQUIRE( std::size(head1) == 3 );

    auto i = head1.begin();
    int idx = 0;

    while (i != head1.end())
      REQUIRE( s[idx++].i == (*i++).i );
  }

  SECTION("other_derived_type") {
    tq_fwd_head_t fwdHead2;
    tq_proxy_t head2{fwdHead2};

    head1.insert(head1.end(), { &s[0] });
    head2.insert(head2.end(), { &s[1], &s[2], &s[3] });

    head1.splice(head1.end(), head2, head2.begin(), --head2.end());

    REQUIRE( std::size(head2) == 1 );
    REQUIRE( std::addressof(*head2.begin()) == &s[3] );
    REQUIRE( std::size(head1) == 3 );

    auto i = head1.begin();
    int idx = 0;

    while (i != head1.end())
      REQUIRE( s[idx++].i == (*i++).i );
  }
}

TEST_CASE("tailq.remove", "[tailq][oper]") { remove_tests<tq_head_t>(); }

TEST_CASE("tailq.reverse", "[tailq][oper]") { reverse_tests<tq_head_t>(); }

TEST_CASE("tailq.unique", "[tailq][oper]") { unique_tests<tq_head_t>(); }

TEST_CASE("tailq.sort", "[tailq][oper]") { sort_tests<tq_head_t>(); }
