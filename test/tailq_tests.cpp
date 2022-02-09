#include <cstddef>
#include <cstdint>
#include <type_traits>

#include <catch2/catch.hpp>
#include <csg/core/tailq.h>

#include "list_modifier_tests.h"
#include "list_operation_tests.h"

using namespace csg;

// See the comments in slist_tests.cpp for an explanation of these types.
using D = DirectEntryList<tailq_entry>;
using A = AccessorEntryList<tailq_entry>;
using S = StatefulExtractorList<tailq_entry>;

using tq_head_t = CSG_TAILQ_HEAD_OFFSET_T(D, next);
using tq_head_inline_t = CSG_TAILQ_HEAD_OFFSET_T(D, next, std::size_t);
using tq_head_invoke_t = tailq_head_cinvoke_t<&A::next>;
using tq_head_stateful_t = tailq_head<S, S::extractor_type>;

using tq_fwd_head_t = tailq_fwd_head<D>;
using tq_proxy_t = CSG_TAILQ_PROXY_OFFSET_T(D, next);
using tq_proxy_inline_t = CSG_TAILQ_PROXY_OFFSET_T(D, next, std::size_t);
using tq_proxy_stateful_t = tailq_proxy<tailq_fwd_head<S>, S::extractor_type>;
using tq_test_proxy_t = list_test_proxy<tq_proxy_t>;
using tq_test_proxy_inline_t = list_test_proxy<tq_proxy_inline_t>;
using tq_test_proxy_stateful_t = list_test_proxy<tq_proxy_stateful_t>;

// Compile-time tests of list traits classes.
static_assert(tailq<tq_head_t>);
static_assert(tailq<tq_proxy_t>);
static_assert(tailq<tq_test_proxy_t>);

static_assert(!tailq<tq_fwd_head_t>);
static_assert(!tailq<int>);

static_assert(!slist<tq_head_t>);
static_assert(!slist<tq_proxy_t>);

template <typename T>
struct extended_entry_inherit : tailq_entry<T> {
  int extra;
};

template <typename T>
struct extended_entry_member {
  tailq_entry<T> entry;
  int extra;
};

using InheritD = DirectEntryList<extended_entry_inherit>;
using ExtendD = DirectEntryList<extended_entry_member>;

using tq_head_entry_inherit_t = tailq_head_cinvoke_t<&InheritD::next>;
using tq_head_entry_extend_t = CSG_TAILQ_HEAD_OFFSET_T(ExtendD, next.entry);

static_assert(std::is_standard_layout_v<tq_head_t>);
static_assert(std::is_standard_layout_v<tq_head_inline_t>);
static_assert(std::is_standard_layout_v<tq_head_invoke_t>);
static_assert(std::is_standard_layout_v<tq_head_stateful_t>);

// This doesn't really belong here, but we need to ensure both invocable_link
// and offset_link remain standard layout, so that they can use the common
// initial sequence rule to read from either `link_union` member, regardless
// of which member was written. This is needed to get proper default
// initialization for tailq_entry<T>, where we cannot know which union member
// will be used, so we pick one.
static_assert(
    std::is_standard_layout_v<invocable_tagged_ref<tailq_entry<S>, S>> &&
    std::is_standard_layout_v<offset_entry_ref<tailq_entry<S>>>);

TEMPLATE_TEST_CASE("tailq.small_size", "[tailq][small_size][template]",
  tq_head_t, tq_head_invoke_t, tq_head_entry_inherit_t, tq_head_entry_extend_t) {
  // Ensure [[no_unique_address]] and compressed_invocable_ref are doing
  // what we expect, so that tailq heads are the size of two pointers, and
  // iterators are pointer-sized.
  REQUIRE( sizeof(TestType) == 2 * sizeof(std::uintptr_t) );
  REQUIRE( sizeof(typename TestType::iterator) == sizeof(std::uintptr_t) );
  REQUIRE( sizeof(typename TestType::const_iterator) == sizeof(std::uintptr_t) );
}

TEMPLATE_TEST_CASE("tailq.basic", "[tailq][basic][template]", tq_head_t,
    tq_head_inline_t, tq_head_invoke_t, tq_test_proxy_t,
    tq_head_entry_inherit_t, tq_head_entry_extend_t, tq_head_stateful_t) {
  basic_tests<TestType>();
}

TEMPLATE_TEST_CASE("tailq.clear", "[tailq][clear][template]",
    tq_head_inline_t, tq_head_invoke_t, tq_test_proxy_t,
    tq_head_entry_inherit_t, tq_head_entry_extend_t, tq_head_stateful_t) {
  clear_tests<TestType>();
}

TEST_CASE("tailq.move", "[tailq][move]") {
  SECTION("stateless") { move_tests<tq_head_t, tq_test_proxy_t>(); }
  SECTION("stateful") { move_tests<tq_head_stateful_t, tq_test_proxy_stateful_t>(); }
  SECTION("inline_computed") { move_tests<tq_head_inline_t, tq_head_t>(); }
  SECTION("computed_inline") { move_tests<tq_head_t, tq_head_inline_t>(); }
  SECTION("inline_computed2") { move_tests<tq_test_proxy_inline_t, tq_head_t>(); }
  SECTION("computed_inline2") { move_tests<tq_head_t, tq_test_proxy_inline_t>(); }
}

TEMPLATE_TEST_CASE("tailq.extra_ctor", "[tailq][extra_ctor][template]",
    tq_head_inline_t, tq_head_invoke_t, tq_test_proxy_t,
    tq_head_entry_inherit_t, tq_head_entry_extend_t, tq_head_stateful_t) {
  extra_ctor_tests<TestType>();
}

TEMPLATE_TEST_CASE("tailq.bulk_insert", "[tailq][bulk_insert][template]",
    tq_head_inline_t, tq_head_invoke_t, tq_test_proxy_t,
    tq_head_entry_inherit_t, tq_head_entry_extend_t, tq_head_stateful_t) {
  bulk_insert_tests<TestType>();
}

TEMPLATE_TEST_CASE("tailq.bulk_erase", "[tailq][bulk_erase][template]",
    tq_head_inline_t, tq_head_invoke_t, tq_test_proxy_t,
    tq_head_entry_inherit_t, tq_head_entry_extend_t, tq_head_stateful_t) {
  bulk_erase_tests<TestType>();
}

TEMPLATE_TEST_CASE("tailq.for_each_safe", "[tailq][for_each_safe][template]",
    tq_head_t, tq_head_inline_t, tq_head_invoke_t, tq_test_proxy_t,
    tq_head_entry_inherit_t, tq_head_entry_extend_t, tq_head_stateful_t) {
  for_each_safe_tests<TestType>();
}

TEMPLATE_TEST_CASE("tailq.push_pop", "[tailq][push_pop][template]",
    tq_head_t, tq_head_inline_t, tq_head_invoke_t, tq_test_proxy_t,
    tq_head_entry_inherit_t, tq_head_entry_extend_t, tq_head_stateful_t) {
  push_pop_tests<TestType>();
}

TEMPLATE_TEST_CASE("tailq.reverse_iterator", "[tailq][reverse][template]",
    tq_head_t, tq_head_inline_t, tq_head_invoke_t, tq_test_proxy_t,
    tq_head_entry_inherit_t, tq_head_entry_extend_t, tq_head_stateful_t) {
  using E = CSG_TYPENAME TestType::value_type;
  TestType head;

  E e[] = { {0}, {1} };

  auto i = head.insert(head.end(), { &e[0], &e[1] });

  REQUIRE( std::addressof(*i++) == &e[0] );
  REQUIRE( std::addressof(*i++) == &e[1] );
  REQUIRE( i == head.end() );

  auto ri = head.rbegin();
  REQUIRE( std::addressof(*ri++) == &e[1] );
  REQUIRE( std::addressof(*ri++) == &e[0] );
  REQUIRE( ri == head.rend() );

  REQUIRE( head.rbegin().base() == head.end() );
  REQUIRE( head.rend().base() == head.begin() );

  REQUIRE( head.rbegin() == head.crbegin() );
  REQUIRE( head.rend() == head.crend() );
}

// FIXME: more test cases?
TEST_CASE("tailq.swap", "[tailq]") {
  SECTION("stateless") { swap_tests<tq_head_t, tq_test_proxy_t>(); }
  SECTION("head_mixed_size_1") { swap_tests<tq_head_t, tq_head_inline_t>(); }
  SECTION("head_mixed_size_2") { swap_tests<tq_head_inline_t, tq_head_t>(); }
  SECTION("proxy_mixed_size_1") { swap_tests<tq_test_proxy_t, tq_test_proxy_inline_t>(); }
  SECTION("proxy_mixed_size_2") { swap_tests<tq_test_proxy_inline_t, tq_test_proxy_t>(); }
  SECTION("stateful") { swap_tests<tq_head_stateful_t, tq_test_proxy_stateful_t>(); }
}

TEST_CASE("tailq.proxy", "[tailq]") {
  proxy_tests<tq_proxy_t>();
  proxy_tests<tq_proxy_inline_t>();
  proxy_tests<tq_proxy_stateful_t>();
}

TEMPLATE_TEST_CASE("tailq.merge", "[tailq][merge][template]",
    tq_head_t, tq_head_inline_t, tq_head_invoke_t, tq_test_proxy_t,
    tq_head_entry_inherit_t, tq_head_entry_extend_t, tq_head_stateful_t) {
  merge_tests<TestType>();
}

TEMPLATE_TEST_CASE("tailq.splice", "[tailq][splice][template]",
    tq_head_t, tq_head_inline_t, tq_head_invoke_t, tq_test_proxy_t,
    tq_head_entry_inherit_t, tq_head_entry_extend_t, tq_head_stateful_t) {
  splice_tests<TestType>();
}

TEST_CASE("tailq.splice.other_derived", "[tailq][splice][other_derived]") {
  SECTION("stateless") {
    splice_tests_other_derived<tq_head_t, tq_test_proxy_t>();
  }

  SECTION("stateful") {
    splice_tests_other_derived<tq_head_stateful_t, tq_test_proxy_stateful_t>();
  }

  SECTION("inline_computed") {
    splice_tests_other_derived<tq_head_inline_t, tq_head_t>();
  }

  SECTION("computed_inline") {
    splice_tests_other_derived<tq_head_t, tq_head_inline_t>();
  }

  SECTION("inline_computed2") {
    splice_tests_other_derived<tq_test_proxy_inline_t, tq_head_t>();
  }

  SECTION("computed_inline2") {
    splice_tests_other_derived<tq_head_t, tq_test_proxy_inline_t>();
  }
}

TEMPLATE_TEST_CASE("tailq.remove", "[tailq][remove][template]",
    tq_head_t, tq_head_inline_t, tq_head_invoke_t, tq_test_proxy_t,
    tq_head_entry_inherit_t, tq_head_entry_extend_t, tq_head_stateful_t) {
  remove_tests<TestType>();
}

TEMPLATE_TEST_CASE("tailq.reverse", "[tailq][reverse][template]",
    tq_head_t, tq_head_inline_t, tq_head_invoke_t, tq_test_proxy_t,
    tq_head_entry_inherit_t, tq_head_entry_extend_t, tq_head_stateful_t) {
  reverse_tests<TestType>();
}

TEMPLATE_TEST_CASE("tailq.unique", "[tailq][unique][template]",
    tq_head_t, tq_head_inline_t, tq_head_invoke_t, tq_test_proxy_t,
    tq_head_entry_inherit_t, tq_head_entry_extend_t, tq_head_stateful_t) {
  unique_tests<TestType>();
}

TEMPLATE_TEST_CASE("tailq.sort", "[tailq][sort][template]",
    tq_head_t, tq_head_inline_t, tq_head_invoke_t, tq_test_proxy_t,
    tq_head_entry_inherit_t, tq_head_entry_extend_t, tq_head_stateful_t) {
  sort_tests<TestType>();
}
