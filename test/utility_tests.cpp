#include <cstdint>

#include <catch2/catch.hpp>
#include <csg/core/utility.h>

using namespace csg;
using namespace csg::util;

struct S {
  std::int64_t i;
};

struct T {
  double d;
};

static_assert(sizeof(S) >= 8 && alignof(S) >= 8 &&
              sizeof(T) >= 8 && alignof(T));

TEST_CASE("tagged_ptr_union.empty", "[utility][tagged_ptr_union][empty]") {
  tagged_ptr_union<S, T> u;
  int i;
  int *pi = &i;

  REQUIRE( u.address() == 0 );
  REQUIRE( u.raw() == 0 );
  REQUIRE( u.index() == type_not_found );
  REQUIRE( !u );

  REQUIRE( !u.has_type<S>() );
  REQUIRE( !u.has_type<T>() );
  REQUIRE( !u.has_type<int>() );

  REQUIRE( static_cast<S *>(u) == nullptr );
  REQUIRE( static_cast<T *>(u) == nullptr );
  REQUIRE( static_cast<int *>(u) == nullptr );

  REQUIRE( u.safe_cast<S>() == nullptr );
  REQUIRE( u.safe_cast<T>() == nullptr );
  REQUIRE( u.safe_cast<int>() == nullptr );

  REQUIRE( u == u );
  REQUIRE( u == nullptr );
  REQUIRE( u != pi );

  tagged_ptr_union<S, T> v{nullptr};
  REQUIRE( u == v );
}

TEST_CASE("tagged_ptr_union.basic", "[utility][tagged_ptr_union][basic]") {
  S s;
  tagged_ptr_union<S, T> u{&s};
  int i;
  int *pi = &i;

  REQUIRE( u.address() == std::bit_cast<std::uintptr_t>(&s) );
  REQUIRE( u.raw() == std::bit_cast<std::uintptr_t>(&s) );
  REQUIRE( u.index() == 0 );
  REQUIRE( u );

  REQUIRE( u.has_type<S>() );
  REQUIRE( !u.has_type<T>() );
  REQUIRE( !u.has_type<int>() );

  REQUIRE( static_cast<S *>(u) == &s );

  // A possibly-surprising case: casts which are well-defined but incorrect
  // in terms of dynamic type are *not* checked (use safe_cast instead).
  REQUIRE( static_cast<T *>(u) == reinterpret_cast<T *>(&s) );

  // This works because "int" is not a member type of the tagged_ptr_union,
  // thus a special overload is selected which always returns nullptr.
  REQUIRE( static_cast<int *>(u) == nullptr );

  REQUIRE( u.safe_cast<S>() == &s );
  REQUIRE( u.safe_cast<T>() == nullptr );
  REQUIRE( u.safe_cast<int>() == nullptr );

  REQUIRE( u == u );
  REQUIRE( u == &s );
  REQUIRE( u != pi );

  T t;
  tagged_ptr_union<S, T> v{&t};

  REQUIRE( v.address() == std::bit_cast<std::uintptr_t>(&t) );
  REQUIRE( v.raw() == (std::bit_cast<std::uintptr_t>(&t) | 0x1uz) );
  REQUIRE( v.index() == 1 );
  REQUIRE( v );

  REQUIRE( static_cast<S *>(v) == reinterpret_cast<S *>(&t) );
  REQUIRE( static_cast<T *>(v) == &t );
  REQUIRE( static_cast<int *>(v) == nullptr );

  REQUIRE( v.safe_cast<S>() == nullptr );
  REQUIRE( v.safe_cast<T>() == &t );
  REQUIRE( v.safe_cast<int>() == nullptr );

  REQUIRE( v == v );
  REQUIRE( v == &t );
  REQUIRE( v != u );
  REQUIRE( v != pi );

  // Changing the type works.
  v = &s;
  REQUIRE( v.address() == std::bit_cast<std::uintptr_t>(&s) );
  REQUIRE( v.raw() == std::bit_cast<std::uintptr_t>(&s) );
  REQUIRE( v.index() == 0 );
  REQUIRE( v );

  REQUIRE( static_cast<S *>(v) == &s );
  REQUIRE( static_cast<T *>(v) == reinterpret_cast<T *>(&s) );
  REQUIRE( static_cast<int *>(v) == nullptr );

  REQUIRE( v.safe_cast<S>() == &s );
  REQUIRE( v.safe_cast<T>() == nullptr );
  REQUIRE( v.safe_cast<int>() == nullptr );

  REQUIRE( v == v );
  REQUIRE( v == &s );
  REQUIRE( v == u );
  REQUIRE( v != pi );

  // Changing the type via. another tagged_ptr_union also works.
  v = u;
  REQUIRE( v == u );
}

// The tagged pointer ignores cv-qualification of the pointee type it is
// constructed from, allowing it to "capture" a `T *` from a `const T *` input,
// effectively casting away const. To respect cv-qualification might make sense,
// i.e., if we expected all top-level cv-qualifiers to match. For example, one
// design choice might define `tagged_ptr_union<const S, const T>`. We do not
// do this, and use `tagged_ptr_union` as an efficient storage utility that is
// just sugar on top of `std::bit_cast<std::uintptr_t>`, which does not respect
// const by its nature.
TEST_CASE("tagged_ptr_union.const_basic", "[utility][tagged_ptr_union][basic]") {
  S s;
  const S *const ps = &s;
  tagged_ptr_union<S, T> u{ps};
  int i;
  int *pi = &i;

  REQUIRE( u.address() == std::bit_cast<std::uintptr_t>(ps) );
  REQUIRE( u.raw() == std::bit_cast<std::uintptr_t>(ps) );
  REQUIRE( u.index() == 0 );
  REQUIRE( u );

  REQUIRE( u.has_type<S>() );
  REQUIRE( u.has_type<const S>() );
  REQUIRE( u.has_type<volatile S>() );
  REQUIRE( !u.has_type<T>() );
  REQUIRE( !u.has_type<int>() );

  REQUIRE( static_cast<S *>(u) == ps );
  REQUIRE( static_cast<const S *>(u) == ps );
  REQUIRE( static_cast<volatile S *>(u) == ps);
  REQUIRE( static_cast<const S *const>(u) == ps );

  REQUIRE( static_cast<T *>(u) == reinterpret_cast<const T *>(ps) );
  REQUIRE( static_cast<int *>(u) == nullptr );

  REQUIRE( u.safe_cast<S>() == ps );
  REQUIRE( u.safe_cast<const S>() == ps );
  REQUIRE( u.safe_cast<volatile S>() == ps );
  REQUIRE( u.safe_cast<T>() == nullptr );
  REQUIRE( u.safe_cast<int>() == nullptr );

  REQUIRE( u == u );
  REQUIRE( u == &s );
  REQUIRE( u == ps );
  REQUIRE( u != pi );
}
