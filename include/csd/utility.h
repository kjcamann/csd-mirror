//==-- csd/utility.h - miscellaneous utility classes/functions --*- C++ -*-==//
//
//                Cyril Software Data Structures (CSD) Library
//
// This file is distributed under the 2-clause BSD Open Source License. See
// LICENSE.TXT for details.
//==-----------------------------------------------------------------------==//

/**
 * @file
 * @brief Contains generally useful "utility" classes and functions that are
 *     used in the implementation of other CSD classes.
 *
 * Because of their general usefulness, these utilities are made available
 * "publically" in the nested "util" namespace, rather than living in a
 * "detail" namespace. If these facilities are standardized in the future
 * however, they will be deprecated and removed.
 */

#ifndef CSD_UTILITY_H
#define CSD_UTILITY_H

#include <cstddef>
#include <cstdint>
#include <new>
#include <type_traits>

namespace csd::util {

// FIXME [C++20] change std::ptrdiff_t to std::ssize_t
constexpr std::ptrdiff_t type_not_found = -1;

template <typename T, typename U, typename... Us>
struct type_index {
  constexpr static std::ptrdiff_t index = std::is_same_v<T, U>
      ? 0
      : (type_index<T, Us...>::index == type_not_found)
          ? type_not_found
          : 1 + type_index<T, Us...>::index;
};

template <typename T, typename U>
struct type_index<T, U> {
  constexpr static std::ptrdiff_t index = std::is_same_v<T, U>
      ? 0
      : type_not_found;
};

template <typename T, typename... Us>
constexpr std::ptrdiff_t type_index_v = type_index<T, Us...>::index;

template <typename... Ts>
class tagged_ptr_union {
  static_assert(sizeof...(Ts) > 0);

public:
  tagged_ptr_union() noexcept : m_address{} {}

  tagged_ptr_union(std::nullptr_t) noexcept : m_address{} {}

  tagged_ptr_union(const tagged_ptr_union &) = default;

  tagged_ptr_union(tagged_ptr_union &&) = default;

  template <typename U>
      requires (type_index_v<U, Ts...> != type_not_found) &&
               (alignof(U) >= sizeof...(Ts))
  tagged_ptr_union(U *u) noexcept :
      m_address{reinterpret_cast<std::uintptr_t>(u) | type_index_v<U, Ts...>} {}

  ~tagged_ptr_union() = default;

  template <typename U>
      requires (type_index_v<U, Ts...> != type_not_found)
  bool has_type() const noexcept {
    return (m_address & Mask) == type_index_v<U, Ts...>;
  }

  std::uintptr_t address() const noexcept {
    return m_address & ~std::uintptr_t(Mask);
  }

  std::uintptr_t raw() const noexcept { return m_address; }

  std::ptrdiff_t index() const noexcept {
    return m_address ? (m_address & Mask) : type_not_found;
  }

  explicit operator bool() const noexcept { return m_address; }

  template <typename U>
      requires (... || std::is_constructible_v<U *, Ts *>) &&
               (alignof(U) >= sizeof...(Ts))
  explicit operator U *() const noexcept {
    return reinterpret_cast<U *>(address());
  }

  template <typename U>
      requires (... || std::is_constructible_v<U *, Ts *>) &&
               (alignof(U) >= sizeof...(Ts))
  U *safe_cast() const noexcept {
    return (index() == type_index_v<U, Ts...>)
        ? reinterpret_cast<U *>(address())
        : nullptr;
  }

  tagged_ptr_union &operator=(const tagged_ptr_union &) = default;
  tagged_ptr_union &operator=(tagged_ptr_union &&) = default;

  template <typename U>
      requires (type_index_v<U, Ts...> != type_not_found) &&
               (alignof(U) >= sizeof...(Ts))
  tagged_ptr_union &operator=(U *u) noexcept {
    return *new(this) tagged_ptr_union{u};
  }

  bool operator==(const tagged_ptr_union &rhs) const noexcept {
    return m_address == rhs.m_address;
  }

  bool operator!=(const tagged_ptr_union &rhs) const noexcept {
    return !operator==(rhs);
  }

private:
  std::uintptr_t m_address;

  constexpr static std::size_t Mask =
      std::size_t((1ull << (sizeof...(Ts) - 1)) - 1);

  // FIXME [C++20]: zu literal for size_t
  // constexpr static std::size_t Mask = (1zu << sizeof...(Ts) - 1) - 1;
};

template <typename Instance, template <typename...> class Template>
constexpr bool is_instantiated_from = false;

template <typename... Args, template <typename...> class Template>
constexpr bool is_instantiated_from<Template<Args...>, Template> = true;

template <typename Instance, template <typename...> class Template>
concept InstantiatedFrom = is_instantiated_from<Instance, Template>;

// FIXME: we might be able to express this as a lambda inside the concept
// definition rather than introducing this dummy function, but even simple
// (non-variadic) lambda expressions seem broken when they appear in
// requirements expressions in gcc's concepts TS implementation.
template <template <typename...> class Template, typename... Args>
void derived_binds_to_base(Template<Args...> &);

template <typename Instance, template <typename...> class Template>
concept DerivedFromTemplate = requires(Instance instance) {
  derived_binds_to_base<Template>(instance);
};

} // End of namespace csd::util

#endif
