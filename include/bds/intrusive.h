//==-- bds/intrusive.h - Intrusive data structure utilities -----*- C++ -*-==//
//
//                     BSD Data Structures (BDS) Library
//
// This file is distributed under the 2-clause BSD Open Source License. See
// LICENSE.TXT for details.
//==-----------------------------------------------------------------------==//

/**
 * @file
 * @brief Contains concepts and helper types that are common to intrusive
 *     data structures.
 */

#ifndef BDS_INTRUSIVE_H
#define BDS_INTRUSIVE_H

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

namespace bds {

// FIXME [C++20]: work-around for non-dependent iterator types being
// incomplete suggested by Ricard Smith (will be standardized in C++20 in
// type_traits), see
// https://lists.llvm.org/pipermail/cfe-dev/2018-June/058273.html
// change type_identity_t to std::type_identity_t when it comes in
template <typename T>
using type_identity_t = T;

// FIXME [C++20]: we don't have this in libstdc++ yet. When it comes in,
// change all `remove_cvref` to `std::remove_cvref`
template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

/**
 * @brief Wrap a constexpr invocable in a stateless function object, so it can
 * be passed as a "functor" template type argument.
 */
template <auto I>
struct constexpr_invocable {
  template <typename... Ts>
      requires std::is_invocable_v<decltype(I), Ts...>
  constexpr decltype(auto) operator()(Ts &&... vs) const
      noexcept(noexcept(std::invoke(I, std::declval<Ts>()...))) {
    return std::invoke(I, std::forward<Ts>(vs)...);
  }
};

template <typename R, std::size_t Offset>
struct offset_extractor {
  constexpr static std::size_t offset = Offset;

  template <typename T>
  constexpr remove_cvref_t<R> &operator()(T &t) const noexcept {
    return *reinterpret_cast<R *>(
        reinterpret_cast<std::byte *>(std::addressof(t)) + Offset);
  }
};

template <typename EntryType, typename EntryAccess, typename T>
concept bool InvocableEntryAccessor = requires(EntryAccess e, T &t) {
    {std::invoke(e, t)} -> EntryType &
};

template <typename C>
concept bool Stateless = std::is_empty_v<C> &&
    std::is_trivially_constructible_v<C>;

struct no_size {};

template <typename T>
concept bool SizeMember = std::is_integral_v<T> || std::is_same_v<T, no_size>;

/// @brief Stores a reseatable reference to a `std::Invocable` object.
template <typename Invocable, typename... Args>
    requires std::is_invocable_v<Invocable, Args...>
class compressed_invocable_ref {
public:
  compressed_invocable_ref() = delete;
  constexpr compressed_invocable_ref(const compressed_invocable_ref &) = default;
  constexpr compressed_invocable_ref(compressed_invocable_ref &&) = default;
  ~compressed_invocable_ref() = default;

  constexpr compressed_invocable_ref(Invocable &i) noexcept
      : m_i{std::addressof(i)} {}

  constexpr compressed_invocable_ref &
  operator=(const compressed_invocable_ref &) = default;

  constexpr compressed_invocable_ref &
  operator=(compressed_invocable_ref &&) = default;

  // FIXME [C++20] contracts: assert that m_i not null
  constexpr decltype(auto) operator()(Args &&... args) const
      noexcept(noexcept(std::invoke(std::declval<Invocable>(),
                                    std::declval<Args>()...))) {
    return (*m_i)(std::forward<Args>(args)...);
  }

  constexpr Invocable &get_invocable() noexcept { return *m_i; }

  constexpr const Invocable &get_invocable() const noexcept { return *m_i; }

private:
  Invocable *m_i;
};

template <typename Invocable, typename... Args>
    requires std::is_invocable_v<Invocable, Args...> &&
             Stateless<Invocable>
class compressed_invocable_ref<Invocable, Args...> {
public:
  static inline Invocable I;

  constexpr compressed_invocable_ref() = default;
  constexpr compressed_invocable_ref(const compressed_invocable_ref &) =
      default;
  constexpr compressed_invocable_ref(compressed_invocable_ref &&) = default;
  ~compressed_invocable_ref() = default;

  constexpr compressed_invocable_ref(Invocable &) noexcept {}

  constexpr compressed_invocable_ref &
  operator=(const compressed_invocable_ref &) = default;

  constexpr compressed_invocable_ref &
  operator=(compressed_invocable_ref &&) = default;

  constexpr decltype(auto) operator()(Args &&... args) const
      noexcept(noexcept(std::invoke(std::declval<Invocable>(),
                                    std::declval<Args>()...))) {
    return std::invoke(I, std::forward<Args>(args)...);
  }

  constexpr Invocable &get_invocable() noexcept { return I; }

  constexpr const Invocable &get_invocable() const noexcept { return I; }
};

} // End of namespace bds

#endif
