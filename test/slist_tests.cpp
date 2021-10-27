#include <cstddef>
#include <cstdint>
#include <type_traits>

#include <catch2/catch.hpp>
#include <csg/core/slist.h>

#include "list_modifier_tests.h"
#include "list_operation_tests.h"

using namespace csg;

// Element types that embed their entry in different ways.
using D = DirectEntryList<slist_entry>;
using A = AccessorEntryList<slist_entry>;
using S = StatefulExtractorList<slist_entry>;

// List head/proxy types that combine an element type with an entry
// extractor.
using sl_head_t = CSG_SLIST_HEAD_OFFSET_T(D, next);
using sl_head_inline_t = CSG_SLIST_HEAD_OFFSET_T(D, next, std::size_t);
using sl_head_invoke_t = slist_head_cinvoke_t<&A::next>;
using sl_head_stateful_t = slist_head<S, S::extractor_type>;

using sl_fwd_head_t = slist_fwd_head<D>;
using sl_proxy_t = CSG_SLIST_PROXY_OFFSET_T(D, next);
using sl_proxy_inline_t = CSG_SLIST_PROXY_OFFSET_T(D, next, std::size_t);
using sl_proxy_stateful_t = slist_proxy<slist_fwd_head<S>, S::extractor_type>;
using sl_test_proxy_t = list_test_proxy<sl_proxy_t>;
using sl_test_proxy_inline_t = list_test_proxy<sl_proxy_inline_t>;
using sl_test_proxy_stateful_t = list_test_proxy<sl_proxy_stateful_t>;

// Compile-time tests of list traits classes.
static_assert(slist<sl_head_t>);
static_assert(slist<sl_proxy_t>);
static_assert(slist<sl_test_proxy_t>);

static_assert(!slist<sl_fwd_head_t>);
static_assert(!slist<int>);

static_assert(!tailq<sl_head_t>);
static_assert(!tailq<sl_proxy_t>);

// Check that we can extend the entry object in various ways and the
// slist_entry_extractor<T> concept will still hold.
template <typename T>
struct extended_entry_inherit : slist_entry<T> {
  int extra;
};

template <typename T>
struct extended_entry_member {
  slist_entry<T> entry;
  int extra;
};

using InheritD = DirectEntryList<extended_entry_inherit>;
using ExtendD = DirectEntryList<extended_entry_member>;

using sl_head_entry_inherit_t = slist_head_cinvoke_t<&InheritD::next>;
using sl_head_entry_extend_t = CSG_SLIST_HEAD_OFFSET_T(ExtendD, next.entry);

// List heads are fundamental building blocks of BSD-style intrusive data
// structure design. They must be standard layout types so that they can be
// used within "simple" types without changing the properties of those types,
// namely that offsetof is guaranteed to work. This ensures a type can contain
// both list heads and list linkage and still work with offsetof-based
// extractors.
static_assert(std::is_standard_layout_v<sl_head_t>);
static_assert(std::is_standard_layout_v<sl_head_inline_t>);
static_assert(std::is_standard_layout_v<sl_head_invoke_t>);
static_assert(std::is_standard_layout_v<sl_head_stateful_t>);

TEMPLATE_TEST_CASE("slist.small_size", "[slist][small_size][template]",
  sl_head_t, sl_head_invoke_t, sl_head_entry_inherit_t, sl_head_entry_extend_t) {
  // Ensure [[no_unique_address]] and compressed_invocable_ref are doing
  // what we expect, so that slist heads and iterators are pointer-sized.
  REQUIRE( sizeof(TestType) == sizeof(std::uintptr_t) );
  REQUIRE( sizeof(typename TestType::iterator) == sizeof(std::uintptr_t) );
  REQUIRE( sizeof(typename TestType::const_iterator) == sizeof(std::uintptr_t) );
}

TEMPLATE_TEST_CASE("slist.basic", "[slist][basic][template]", sl_head_t,
    sl_head_inline_t, sl_head_invoke_t, sl_test_proxy_t,
    sl_head_entry_inherit_t, sl_head_entry_extend_t, sl_head_stateful_t) {
  basic_tests<TestType>();
}

TEMPLATE_TEST_CASE("slist.clear", "[slist][clear][template]", sl_head_inline_t,
    sl_head_invoke_t, sl_test_proxy_t, sl_head_entry_inherit_t,
    sl_head_entry_extend_t, sl_head_stateful_t) {
  clear_tests<TestType>();
}

TEST_CASE("slist.move", "[slist][move]") {
  SECTION("stateless") { move_tests<sl_head_t, sl_test_proxy_t>(); }
  SECTION("stateful") { move_tests<sl_head_stateful_t, sl_test_proxy_stateful_t>(); }
  SECTION("inline_computed") { move_tests<sl_head_inline_t, sl_head_t>(); }
  SECTION("computed_inline") { move_tests<sl_head_t, sl_head_inline_t>(); }
  SECTION("inline_computed2") { move_tests<sl_test_proxy_inline_t, sl_head_t>(); }
  SECTION("computed_inline2") { move_tests<sl_head_t, sl_test_proxy_inline_t>(); }
}

TEMPLATE_TEST_CASE("slist.extra_ctor", "[slist][extra_ctor][template]",
    sl_head_inline_t, sl_head_invoke_t, sl_test_proxy_t,
    sl_head_entry_inherit_t, sl_head_entry_extend_t, sl_head_stateful_t) {
  extra_ctor_tests<TestType>();
}

TEMPLATE_TEST_CASE("slist.bulk_insert", "[slist][bulk_insert][template]",
    sl_head_inline_t, sl_head_invoke_t, sl_test_proxy_t,
    sl_head_entry_inherit_t, sl_head_entry_extend_t, sl_head_stateful_t) {
  bulk_insert_tests<TestType>();
}

TEMPLATE_TEST_CASE("slist.bulk_erase", "[slist][bulk_erase][template]",
    sl_head_inline_t, sl_head_invoke_t, sl_test_proxy_t,
    sl_head_entry_inherit_t, sl_head_entry_extend_t, sl_head_stateful_t) {
  bulk_erase_tests<TestType>();
}

TEMPLATE_TEST_CASE("slist.for_each_safe", "[slist][for_each_safe][template]",
    sl_head_t, sl_head_inline_t, sl_head_invoke_t, sl_test_proxy_t,
    sl_head_entry_inherit_t, sl_head_entry_extend_t, sl_head_stateful_t) {
  for_each_safe_tests<TestType>();
}

TEMPLATE_TEST_CASE("slist.push_pop", "[slist][push_pop][template]",
    sl_head_t, sl_head_inline_t, sl_head_invoke_t, sl_test_proxy_t,
    sl_head_entry_inherit_t, sl_head_entry_extend_t, sl_head_stateful_t) {
  push_pop_tests<TestType>();
}

// FIXME: more test cases?
TEST_CASE("slist.swap", "[slist]") {
  SECTION("stateless") { swap_tests<sl_head_t, sl_test_proxy_t>(); }
  SECTION("head_mixed_size_1") { swap_tests<sl_head_t, sl_head_inline_t>(); }
  SECTION("head_mixed_size_2") { swap_tests<sl_head_inline_t, sl_head_t>(); }
  SECTION("proxy_mixed_size_1") { swap_tests<sl_test_proxy_t, sl_test_proxy_inline_t>(); }
  SECTION("proxy_mixed_size_2") { swap_tests<sl_test_proxy_inline_t, sl_test_proxy_t>(); }
  SECTION("stateful") { swap_tests<sl_head_stateful_t, sl_test_proxy_stateful_t>(); }
}

TEMPLATE_TEST_CASE("slist.find_predecessor",
    "[slist][find_predecessor][template]", sl_head_t, sl_head_inline_t,
    sl_head_invoke_t, sl_test_proxy_t, sl_head_entry_inherit_t,
    sl_head_entry_extend_t, sl_head_stateful_t) {
  find_predecessor_tests<TestType>();
}

// FIXME: more test cases?
TEST_CASE("slist.proxy", "[slist]") {
  proxy_tests<sl_fwd_head_t, sl_proxy_t>();
}

TEMPLATE_TEST_CASE("slist.merge", "[slist][merge][template]",
    sl_head_t, sl_head_inline_t, sl_head_invoke_t, sl_test_proxy_t,
    sl_head_entry_inherit_t, sl_head_entry_extend_t, sl_head_stateful_t) {
  merge_tests<TestType>();
}

TEMPLATE_TEST_CASE("slist.splice", "[slist][splice][template]",
    sl_head_t, sl_head_inline_t, sl_head_invoke_t, sl_test_proxy_t,
    sl_head_entry_inherit_t, sl_head_entry_extend_t, sl_head_stateful_t) {
  splice_tests<TestType>();
}

TEST_CASE("slist.splice.other_derived", "[slist][splice][other_derived]") {
  SECTION("stateless") {
    splice_tests_other_derived<sl_head_t, sl_test_proxy_t>();
  }

  SECTION("stateful") {
    splice_tests_other_derived<sl_head_stateful_t, sl_test_proxy_stateful_t>();
  }

  SECTION("inline_computed") {
    splice_tests_other_derived<sl_head_inline_t, sl_head_t>();
  }

  SECTION("computed_inline") {
    splice_tests_other_derived<sl_head_t, sl_head_inline_t>();
  }

  SECTION("inline_computed2") {
    splice_tests_other_derived<sl_test_proxy_inline_t, sl_head_t>();
  }

  SECTION("computed_inline2") {
    splice_tests_other_derived<sl_head_t, sl_test_proxy_inline_t>();
  }
}

TEMPLATE_TEST_CASE("slist.remove", "[slist][remove][template]",
    sl_head_t, sl_head_inline_t, sl_head_invoke_t, sl_test_proxy_t,
    sl_head_entry_inherit_t, sl_head_entry_extend_t, sl_head_stateful_t) {
  remove_tests<TestType>();
}

TEMPLATE_TEST_CASE("slist.reverse", "[slist][reverse][template]",
    sl_head_t, sl_head_inline_t, sl_head_invoke_t, sl_test_proxy_t,
    sl_head_entry_inherit_t, sl_head_entry_extend_t, sl_head_stateful_t) {
  reverse_tests<TestType>();
}

TEMPLATE_TEST_CASE("slist.unique", "[slist][unique][template]",
    sl_head_t, sl_head_inline_t, sl_head_invoke_t, sl_test_proxy_t,
    sl_head_entry_inherit_t, sl_head_entry_extend_t, sl_head_stateful_t) {
  unique_tests<TestType>();
}

TEMPLATE_TEST_CASE("slist.sort", "[slist][sort][template]",
    sl_head_t, sl_head_inline_t, sl_head_invoke_t, sl_test_proxy_t,
    sl_head_entry_inherit_t, sl_head_entry_extend_t, sl_head_stateful_t) {
  sort_tests<TestType>();
}
