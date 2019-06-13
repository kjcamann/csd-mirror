//==-- csg/utility.h - miscellaneous utility classes/functions --*- C++ -*-==//
//
//                Cyril Software Data Structures (CSD) Library
//
// This file is distributed under the 2-clause BSD Open Source License. See
// LICENSE.TXT for details.
//==-----------------------------------------------------------------------==//

/**
 * @file
 * @brief Contains generally useful "utility" classes and functions that are
 *     used in the implementation of other CSG classes.
 *
 * Because of their general usefulness, these utilities are made available
 * "publically" in the nested "util" namespace, rather than living in a
 * "detail" namespace. If these facilities are standardized in the future
 * however, they will be deprecated and removed.
 */

#ifndef CSG_CORE_UTILITY_H
#define CSG_CORE_UTILITY_H

#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <new>

namespace csg::util {

constexpr std::ptrdiff_t type_not_found = -1;

template <typename T, typename U, typename... Us>
constexpr std::ptrdiff_t type_index = std::same_as<T, U>
    ? 0
    : (type_index<T, Us...> == type_not_found)
        ? type_not_found
        : 1 + type_index<T, Us...>;

template <typename T, typename U>
constexpr std::ptrdiff_t type_index<T, U> = std::same_as<T, U>
    ? 0
    : type_not_found;

template <typename U, typename... Ts>
concept tagged_ptr_layout_compatible =
    (type_index<U, Ts...> != type_not_found) && (alignof(U) >= sizeof...(Ts));

template <typename U, typename... Ts>
concept tagged_ptr_convertible_to = (... || std::convertible_to<Ts *, U *>);

template <typename... Ts>
class tagged_ptr_union {
  static_assert(sizeof...(Ts) > 0);

public:
  constexpr tagged_ptr_union() noexcept : m_address{} {}

  constexpr tagged_ptr_union(std::nullptr_t) noexcept : m_address{} {}

  constexpr tagged_ptr_union(const tagged_ptr_union &) = default;

  constexpr tagged_ptr_union(tagged_ptr_union &&) = default;

  template <tagged_ptr_layout_compatible<Ts...> U>
  constexpr tagged_ptr_union(U *u) noexcept :
      m_address{std::bit_cast<std::uintptr_t>(u) | type_index<U, Ts...>} {}

  constexpr ~tagged_ptr_union() = default;

  template <tagged_ptr_layout_compatible<Ts...> U>
  constexpr bool has_type() const noexcept {
    return (m_address & Mask) == type_index<U, Ts...>;
  }

  constexpr std::uintptr_t address() const noexcept {
    return m_address & ~static_cast<std::uintptr_t>(Mask);
  }

  constexpr std::uintptr_t raw() const noexcept { return m_address; }

  constexpr std::ptrdiff_t index() const noexcept {
    return m_address ? (m_address & Mask) : type_not_found;
  }

  constexpr explicit operator bool() const noexcept { return m_address; }

  template <tagged_ptr_convertible_to<Ts...> U>
  constexpr explicit operator U *() const noexcept {
    return std::bit_cast<U *>(address());
  }

  template <tagged_ptr_convertible_to<Ts...> U>
  constexpr U *safe_cast() const noexcept {
    return (index() == type_index<U, Ts...>)
        ? std::bit_cast<U *>(address())
        : nullptr;
  }

  constexpr tagged_ptr_union &operator=(const tagged_ptr_union &) = default;
  constexpr tagged_ptr_union &operator=(tagged_ptr_union &&) = default;

  template <tagged_ptr_layout_compatible<Ts...> U>
  constexpr tagged_ptr_union &operator=(U *u) noexcept {
    return *new(this) tagged_ptr_union{u};
  }

  constexpr bool operator==(const tagged_ptr_union &rhs) const = default;

  template <tagged_ptr_convertible_to<Ts...> U>
  constexpr std::strong_ordering operator<=>(const U *u) const noexcept {
    return static_cast<U *>(*this) <=> u;
  }

  template <tagged_ptr_convertible_to<Ts...> U>
  constexpr bool operator==(const U *u) const noexcept {
    return static_cast<U *>(*this) == u;
  }

private:
  std::uintptr_t m_address;

  constexpr static std::size_t Mask = (1uz << (sizeof...(Ts) - 1)) - 1;
};

// Given the following declarations:
//
//   template <typename A>
//   struct Base {};
//
//   struct Derived : Base<int> {};
//
// we can check that `Derived` is derived from a specialization of Base if
// template argument deduction succeeds on a test function:
//
//   template <typename A>
//   void test(const Base<A> &);
//
//   Derived d;
//   test(d); // OK
//
// This concept uses a similar approach to check if a type is derived from a
// template class with an arbitrary number of type parameters, by checking if
// a "test" expression using a lambda function would be well-formed.
template <typename Instance, template <typename...> class Template>
concept derived_from_template = requires(Instance instance) {
  { []<typename... Args>(const Template<Args...> &){}(instance) };
};

// See the CSD reference documentation in
// "Utility Libraries > Utility Concepts" for a detailed explanation of how
// this concept is used to enable constructor overloads.
template <typename Arg, typename T>
concept can_direct_initialize = std::constructible_from<T, Arg>;

// The above comment applies here also.
template <typename T>
concept default_arg_initializable = std::default_initializable<T> &&
    std::move_constructible<T>;

/// A helper trait used to check that the composition of a projection and
/// another invocable is nothrow invocable; used in range overloads that take
/// projections.
template <typename T, std::invocable<T> Proj,
          std::invocable<std::invoke_result_t<Proj, T>> Fn>
constexpr bool is_nothrow_proj_invocable =
    std::is_nothrow_invocable_v<Proj, T> &&
    std::is_nothrow_invocable_v<Fn, std::invoke_result_t<Proj, T>>;

/// A helper trait used to check that a relation is 
template <typename T, std::invocable<T> Proj,
          std::relation<std::invoke_result_t<Proj, T>,
                        std::invoke_result_t<Proj, T>> Rel>
constexpr bool is_nothrow_proj_relation =
    std::is_nothrow_invocable_v<Proj, T> &&
    std::is_nothrow_invocable_v<Rel, std::invoke_result_t<Proj, T>,
                                std::invoke_result_t<Proj, T>>;

/// A helper function that dereferences two iterators, applies a projection
/// to each, and applies a strict-weak-ordering comparison function to the
/// resulting values.
template <std::indirectly_readable I,
          std::invocable<std::iter_reference_t<I>> Proj,
          std::strict_weak_order<
            std::invoke_result_t<Proj, std::iter_reference_t<I>>,
            std::invoke_result_t<Proj, std::iter_reference_t<I>>> Compare>
constexpr auto
projection_is_ordered_before(const Proj &proj, const Compare &comp,
                             const I &lhs, const I &rhs) {
  return std::invoke(comp, std::invoke(proj, *lhs), std::invoke(proj, *rhs));
}

/// A helper function that dereferences two iterators, applies a projection
/// to each, and applies an equivalence relation invocable to the resulting
/// values.
template <std::indirectly_readable I,
          std::invocable<std::iter_reference_t<I>> Proj,
          std::equivalence_relation<
            std::invoke_result_t<Proj, std::iter_reference_t<I>>,
            std::invoke_result_t<Proj, std::iter_reference_t<I>>> EqRelation>
constexpr auto
projections_are_equivalent(const Proj &proj, const EqRelation &eq,
                           const I &lhs, const I &rhs) {
  return std::invoke(eq, std::invoke(proj, *lhs), std::invoke(proj, *rhs));
}

} // End of namespace csg::util

// FIXME: gcc and clang do not treat noexcept-specifiers the same way. In clang,
// they are instantiated later, such that we won't get certain
// use-before-definition errors that appear in gcc. This is probably a gcc bug,
// but if so, it has been present for many years and thus may not be fixed for
// many more, so we'll hack around it by disabling the noexcept-specifiers
// in certain cases using this macro.
#if defined(__GNUC__) && !defined(__clang__)
#if !defined(CSG_NOEXCEPT)
#define CSG_NOEXCEPT(...)
#endif
#else
#define CSG_NOEXCEPT(...) noexcept(__VA_ARGS__)
#endif

// FIXME: CSD was originally targetting GCC9 which implements P0634 (although
// not correctly). As years have gone by and clang/MSVC still don't support it
// officially, we've added this hack. It is in place even for GCC11 because
// there are corner cases they get wrong even now.
#define CSG_TYPENAME typename

#endif
