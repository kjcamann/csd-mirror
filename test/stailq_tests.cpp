#include <cstddef>
#include <cstdint>
#include <type_traits>

#include <catch2/catch.hpp>
#include <csg/core/stailq.h>

#include "list_modifier_tests.h"
#include "list_operation_tests.h"

using namespace csg;

// See the comments in slist_tests.cpp for an explanation of these types.
using D = DirectEntryList<stailq_entry>;
using A = AccessorEntryList<stailq_entry>;
using S = StatefulExtractorList<stailq_entry>;

using stq_head_t = CSG_STAILQ_HEAD_OFFSET_T(D, next);
using stq_head_inline_t = CSG_STAILQ_HEAD_OFFSET_T(D, next, std::size_t);
using stq_head_invoke_t = stailq_head_cinvoke_t<&A::next>;
using stq_head_stateful_t = stailq_head<S, S::extractor_type>;

using stq_fwd_head_t = stailq_fwd_head<D>;
using stq_proxy_t = CSG_STAILQ_PROXY_OFFSET_T(D, next);
using stq_proxy_inline_t = CSG_STAILQ_PROXY_OFFSET_T(D, next, std::size_t);
using stq_proxy_stateful_t = stailq_proxy<stailq_fwd_head<S>, S::extractor_type>;
using stq_test_proxy_t = list_test_proxy<stq_proxy_t>;
using stq_test_proxy_inline_t = list_test_proxy<stq_proxy_inline_t>;
using stq_test_proxy_stateful_t = list_test_proxy<stq_proxy_stateful_t>;

// Compile-time tests of list traits classes.
static_assert(stailq<stq_head_t>);
static_assert(stailq<stq_proxy_t>);
static_assert(stailq<stq_test_proxy_t>);

static_assert(!stailq<stq_fwd_head_t>);
static_assert(!stailq<int>);

static_assert(!tailq<stq_head_t>);
static_assert(!tailq<stq_proxy_t>);

template <typename T>
struct extended_entry_inherit : stailq_entry<T> {
  int extra;
};

template <typename T>
struct extended_entry_member {
  stailq_entry<T> entry;
  int extra;
};

using InheritD = DirectEntryList<extended_entry_inherit>;
using ExtendD = DirectEntryList<extended_entry_member>;

using stq_head_entry_inherit_t = stailq_head_cinvoke_t<&InheritD::next>;
using stq_head_entry_extend_t = CSG_STAILQ_HEAD_OFFSET_T(ExtendD, next.entry);

static_assert(std::is_standard_layout_v<stq_head_t>);
static_assert(std::is_standard_layout_v<stq_head_inline_t>);
static_assert(std::is_standard_layout_v<stq_head_invoke_t>);
static_assert(std::is_standard_layout_v<stq_head_stateful_t>);

TEMPLATE_TEST_CASE("stailq.small_size", "[stailq][small_size][template]",
  stq_head_t, stq_head_invoke_t, stq_head_entry_inherit_t, stq_head_entry_extend_t) {
  // Ensure [[no_unique_address]] and compressed_invocable_ref are doing
  // what we expect, so that stailq heads are the size of two pointers, and
  // iterators are pointer-sized.
  REQUIRE( sizeof(TestType) == 2 * sizeof(std::uintptr_t) );
  REQUIRE( sizeof(typename TestType::iterator) == sizeof(std::uintptr_t) );
  REQUIRE( sizeof(typename TestType::const_iterator) == sizeof(std::uintptr_t) );
}

TEMPLATE_TEST_CASE("stailq.basic", "[stailq][basic][template]", stq_head_t,
    stq_head_inline_t, stq_head_invoke_t, stq_test_proxy_t,
    stq_head_entry_inherit_t, stq_head_entry_extend_t, stq_head_stateful_t) {
  basic_tests<TestType>();
}

TEMPLATE_TEST_CASE("stailq.clear", "[stailq][clear][template]",
    stq_head_inline_t, stq_head_invoke_t, stq_test_proxy_t,
    stq_head_entry_inherit_t, stq_head_entry_extend_t, stq_head_stateful_t) {
  clear_tests<TestType>();
}

TEST_CASE("stailq.move", "[stailq][move]") {
  SECTION("stateless") { move_tests<stq_head_t, stq_test_proxy_t>(); }
  SECTION("stateful") { move_tests<stq_head_stateful_t, stq_test_proxy_stateful_t>(); }
  SECTION("inline_computed") { move_tests<stq_head_inline_t, stq_head_t>(); }
  SECTION("computed_inline") { move_tests<stq_head_t, stq_head_inline_t>(); }
  SECTION("inline_computed2") { move_tests<stq_test_proxy_inline_t, stq_head_t>(); }
  SECTION("computed_inline2") { move_tests<stq_head_t, stq_test_proxy_inline_t>(); }
}

TEMPLATE_TEST_CASE("stailq.extra_ctor", "[stailq][extra_ctor][template]",
    stq_head_inline_t, stq_head_invoke_t, stq_test_proxy_t,
    stq_head_entry_inherit_t, stq_head_entry_extend_t, stq_head_stateful_t) {
  extra_ctor_tests<TestType>();
}

TEMPLATE_TEST_CASE("stailq.bulk_insert", "[stailq][bulk_insert][template]",
    stq_head_inline_t, stq_head_invoke_t, stq_test_proxy_t,
    stq_head_entry_inherit_t, stq_head_entry_extend_t, stq_head_stateful_t) {
  bulk_insert_tests<TestType>();
}

TEMPLATE_TEST_CASE("stailq.bulk_erase", "[stailq][bulk_erase][template]",
    stq_head_inline_t, stq_head_invoke_t, stq_test_proxy_t,
    stq_head_entry_inherit_t, stq_head_entry_extend_t, stq_head_stateful_t) {
  bulk_erase_tests<TestType>();
}

TEMPLATE_TEST_CASE("stailq.for_each_safe", "[stailq][for_each_safe][template]",
    stq_head_t, stq_head_inline_t, stq_head_invoke_t, stq_test_proxy_t,
    stq_head_entry_inherit_t, stq_head_entry_extend_t, stq_head_stateful_t) {
  for_each_safe_tests<TestType>();
}

TEMPLATE_TEST_CASE("stailq.push_pop", "[stailq][push_pop][template]",
    stq_head_t, stq_head_inline_t, stq_head_invoke_t, stq_test_proxy_t,
    stq_head_entry_inherit_t, stq_head_entry_extend_t, stq_head_stateful_t) {
  push_pop_tests<TestType>();
}

// FIXME: more test cases?
TEST_CASE("stailq.swap", "[stailq]") {
  SECTION("stateless") { swap_tests<stq_head_t, stq_test_proxy_t>(); }
  SECTION("head_mixed_size_1") { swap_tests<stq_head_t, stq_head_inline_t>(); }
  SECTION("head_mixed_size_2") { swap_tests<stq_head_inline_t, stq_head_t>(); }
  SECTION("proxy_mixed_size_1") { swap_tests<stq_test_proxy_t, stq_test_proxy_inline_t>(); }
  SECTION("proxy_mixed_size_2") { swap_tests<stq_test_proxy_inline_t, stq_test_proxy_t>(); }
  SECTION("stateful") { swap_tests<stq_head_stateful_t, stq_test_proxy_stateful_t>(); }
}

TEMPLATE_TEST_CASE("stailq.find_predecessor",
    "[stailq][find_predecessor][template]", stq_head_t, stq_head_inline_t,
    stq_head_invoke_t, stq_test_proxy_t, stq_head_entry_inherit_t,
    stq_head_entry_extend_t, stq_head_stateful_t) {
  find_predecessor_tests<TestType>();
}

TEST_CASE("stailq.proxy", "[stailq]") {
  proxy_tests<stq_proxy_t>();
  proxy_tests<stq_proxy_inline_t>();
  proxy_tests<stq_proxy_stateful_t>();
}

TEMPLATE_TEST_CASE("stailq.merge", "[stailq][merge][template]",
    stq_head_t, stq_head_inline_t, stq_head_invoke_t, stq_test_proxy_t,
    stq_head_entry_inherit_t, stq_head_entry_extend_t, stq_head_stateful_t) {
  merge_tests<TestType>();
}

TEMPLATE_TEST_CASE("stailq.splice", "[stailq][splice][template]",
    stq_head_t, stq_head_inline_t, stq_head_invoke_t, stq_test_proxy_t,
    stq_head_entry_inherit_t, stq_head_entry_extend_t, stq_head_stateful_t) {
  splice_tests<TestType>();
}

TEST_CASE("stailq.splice.other_derived", "[stailq][splice][other_derived]") {
  SECTION("stateless") {
    splice_tests_other_derived<stq_head_t, stq_test_proxy_t>();
  }

  SECTION("stateful") {
    splice_tests_other_derived<stq_head_stateful_t, stq_test_proxy_stateful_t>();
  }

  SECTION("inline_computed") {
    splice_tests_other_derived<stq_head_inline_t, stq_head_t>();
  }

  SECTION("computed_inline") {
    splice_tests_other_derived<stq_head_t, stq_head_inline_t>();
  }

  SECTION("inline_computed2") {
    splice_tests_other_derived<stq_test_proxy_inline_t, stq_head_t>();
  }

  SECTION("computed_inline2") {
    splice_tests_other_derived<stq_head_t, stq_test_proxy_inline_t>();
  }
}

TEMPLATE_TEST_CASE("stailq.remove", "[stailq][remove][template]",
    stq_head_t, stq_head_inline_t, stq_head_invoke_t, stq_test_proxy_t,
    stq_head_entry_inherit_t, stq_head_entry_extend_t, stq_head_stateful_t) {
  remove_tests<TestType>();
}

TEMPLATE_TEST_CASE("stailq.reverse", "[stailq][reverse][template]",
    stq_head_t, stq_head_inline_t, stq_head_invoke_t, stq_test_proxy_t,
    stq_head_entry_inherit_t, stq_head_entry_extend_t, stq_head_stateful_t) {
  reverse_tests<TestType>();
}

TEMPLATE_TEST_CASE("stailq.unique", "[stailq][unique][template]",
    stq_head_t, stq_head_inline_t, stq_head_invoke_t, stq_test_proxy_t,
    stq_head_entry_inherit_t, stq_head_entry_extend_t, stq_head_stateful_t) {
  unique_tests<TestType>();
}

TEMPLATE_TEST_CASE("stailq.sort", "[stailq][sort][template]",
    stq_head_t, stq_head_inline_t, stq_head_invoke_t, stq_test_proxy_t,
    stq_head_entry_inherit_t, stq_head_entry_extend_t, stq_head_stateful_t) {
  sort_tests<TestType>();
}
