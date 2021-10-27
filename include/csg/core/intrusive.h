//==-- csg/core/intrusive.h - Intrusive data structure utilities -*- C++ -*-==//
//
//                Cyril Software Data Structures (CSD) Library
//
// This file is distributed under the 2-clause BSD Open Source License. See
// LICENSE.TXT for details.
//==-----------------------------------------------------------------------==//

/**
 * @file
 * @brief Contains concepts and helper types that are common to intrusive
 *     data structures.
 */

#ifndef CSG_CORE_INTRUSIVE_H
#define CSG_CORE_INTRUSIVE_H

#include <bit>
#include <compare>
#include <concepts>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <ranges>
#include <type_traits>
#include <utility>

#include <csg/core/utility.h>

namespace csg {

/**
 * @brief Wrap a constexpr invocable NTTP in a stateless function object, so it
 * can be passed as a "functor" template type argument.
 */
template <auto I>
struct invocable_constant {
  constexpr static auto value = I;

  template <typename... Ts>
      requires std::invocable<decltype(I), Ts...>
  constexpr decltype(auto) operator()(Ts &&... vs) const
      noexcept(noexcept(std::invoke(I, std::forward<Ts>(vs)... ))) {
    return std::invoke(I, std::forward<Ts>(vs)...);
  }
};

template <typename C>
concept stateless = std::is_empty_v<C> &&
    std::is_trivially_constructible_v<C> && std::is_trivially_destructible_v<C>;

template <typename F, typename R, typename T>
concept extractor = std::invocable<F, T&> &&
    std::common_reference_with<std::invoke_result_t<F, T&>,
                               std::add_lvalue_reference_t<R>>;

template <typename R, typename T, std::size_t Offset>
struct offset_extractor {
  constexpr static std::size_t offset = Offset;

  constexpr std::remove_reference_t<R> &operator()(T &t) const noexcept {
    auto *const p = std::bit_cast<std::byte *>(std::addressof(t));
    return *std::bit_cast<R *>(p + Offset);
  }
};

template <typename>
constexpr bool is_offset_extractor = false;

template <typename R, typename T, std::size_t O>
constexpr bool is_offset_extractor<offset_extractor<R, T, O>> = true;

#define CSG_OFFSET_EXTRACTOR(TYPE, MEMBER)                                    \
    csg::offset_extractor<decltype(std::declval<TYPE>().MEMBER), TYPE,        \
                          offsetof(TYPE, MEMBER)>

struct no_size {};

template <typename T>
concept optional_size = std::integral<T> || std::same_as<T, no_size>;

template <std::input_iterator InputIt, std::sentinel_for<InputIt> Sentinel,
          class Proj = std::identity,
          std::indirectly_unary_invocable<std::projected<InputIt, Proj>> Fun>
constexpr void for_each_safe(InputIt first, Sentinel last, Fun f,
                             Proj proj = {})
    noexcept( noexcept(first != last) &&
              noexcept(std::invoke(f, std::invoke(proj, *first++))) )
{
  while (first != last)
    std::invoke(f, std::invoke(proj, *first++));
}

template <std::ranges::input_range Range, class Proj = std::identity,
          std::indirectly_unary_invocable<
              std::projected<std::ranges::iterator_t<Range>, Proj>> Fun>
constexpr void for_each_safe(Range &&r, Fun f, Proj proj = {})
    noexcept( noexcept(for_each_safe(std::ranges::begin(r), std::ranges::end(r),
                                     std::move(f), std::move(proj))) )
{
  for_each_safe(std::ranges::begin(r), std::ranges::end(r), std::move(f),
                std::move(proj));
}

/**
 * @brief Stores a "compressible" pointer to a `std::invocable` object.
 *
 * In the general case, this class wraps a pointer to a `std::invocable`
 * object. In the case where the invocable is also @ref stateless, it provides
 * a partial template specialization that is empty and points to a static
 * inline instance of the invocable.
 *
 * The intention is to allow users to store a pointer to an invocable by
 * declaring a compressed_invocable_ref member. By further adding the
 * [[no_unique_address]] attribute, the invocable will consume no additional
 * storage when it points to a stateless invocable.
 */
template <typename I, typename... Args>
    requires std::invocable<I, Args...>
class compressed_invocable_ref {
public:
  constexpr compressed_invocable_ref() noexcept : m_i{nullptr} {}
  constexpr compressed_invocable_ref(const compressed_invocable_ref &) = default;
  constexpr compressed_invocable_ref(compressed_invocable_ref &&) = default;
  constexpr ~compressed_invocable_ref() = default;

  constexpr compressed_invocable_ref(I &i) noexcept
      : m_i{std::addressof(i)} {}

  constexpr compressed_invocable_ref &
  operator=(const compressed_invocable_ref &) = default;

  constexpr compressed_invocable_ref &
  operator=(compressed_invocable_ref &&) = default;

  // FIXME [C++23] contracts: assert that m_i not null
  constexpr decltype(auto) operator()(Args &&...args) const
      noexcept(noexcept(std::invoke(*m_i, std::forward<Args>(args)...))) {
    return std::invoke(*m_i, std::forward<Args>(args)...);
  }

  constexpr I &get_invocable() noexcept { return *m_i; }

  constexpr const I &get_invocable() const noexcept { return *m_i; }

private:
  I *m_i;
};

template <stateless I, typename... Args>
    requires std::invocable<I, Args...>
class compressed_invocable_ref<I, Args...> {
public:
  static inline I Instance;

  constexpr compressed_invocable_ref() = default;
  constexpr compressed_invocable_ref(const compressed_invocable_ref &) =
      default;
  constexpr compressed_invocable_ref(compressed_invocable_ref &&) = default;
  ~compressed_invocable_ref() = default;

  constexpr compressed_invocable_ref(I &) noexcept {}

  constexpr compressed_invocable_ref &
  operator=(const compressed_invocable_ref &) = default;

  constexpr compressed_invocable_ref &
  operator=(compressed_invocable_ref &&) = default;

  constexpr decltype(auto) operator()(Args &&...args) const
      noexcept(noexcept(std::invoke(Instance, std::forward<Args>(args)...))) {
    return std::invoke(Instance, std::forward<Args>(args)...);
  }

  constexpr I &get_invocable() noexcept { return Instance; }

  constexpr const I &get_invocable() const noexcept { return Instance; }
};

template <typename EntryType, typename T>
class invocable_tagged_ref : private util::tagged_ptr_union<EntryType, T> {
  using base_type = util::tagged_ptr_union<EntryType, T>;

public:
  using base_type::base_type;
  using base_type::operator=;
  using base_type::operator bool;
  using base_type::operator==;
  using base_type::operator<=>;

  template <extractor<EntryType, T> EntryEx>
  constexpr EntryType *get_entry(EntryEx &e) const
      noexcept(std::is_nothrow_invocable_v<EntryEx, T&>) {
    return is_value()
        ? std::addressof(std::invoke(e, get_value()))
        : static_cast<EntryType *>(*this);
  }

  constexpr T &get_value() const noexcept { return *static_cast<T *>(*this); }

  constexpr bool is_entry() const noexcept {
    return this->template has_type<EntryType>();
  }

  constexpr bool is_value() const noexcept {
    return this->template has_type<T>();
  }
};

template <typename EntryType>
class offset_entry_ref {
public:
  constexpr offset_entry_ref() noexcept : m_address{} {}
  constexpr offset_entry_ref(const offset_entry_ref &) = default;
  constexpr offset_entry_ref(offset_entry_ref &&) = default;
  constexpr offset_entry_ref(EntryType *entry) noexcept
      : m_address{std::bit_cast<std::uintptr_t>(entry)} {}
  ~offset_entry_ref() = default;

  constexpr EntryType *get_entry() const noexcept {
    return std::bit_cast<EntryType *>(m_address);
  }

  constexpr void set_entry(EntryType *entry) noexcept {
    m_address = std::bit_cast<std::uintptr_t>(entry);
  }

  constexpr explicit operator bool() const noexcept { return m_address; }

  constexpr offset_entry_ref &operator=(const offset_entry_ref &) = default;
  constexpr offset_entry_ref &operator=(offset_entry_ref &&) = default;

  constexpr std::strong_ordering
  operator<=>(const offset_entry_ref &) const noexcept = default;

private:
  std::uintptr_t m_address;
};

template <typename EntryType, typename T>
union entry_ref_union {
  offset_entry_ref<EntryType> offset;
  invocable_tagged_ref<EntryType, T> invocableTagged;

  constexpr entry_ref_union() noexcept {}
  constexpr entry_ref_union(const entry_ref_union &) = default;
  constexpr entry_ref_union(entry_ref_union &&) = default;
  constexpr ~entry_ref_union() = default;

  constexpr entry_ref_union(std::nullptr_t) noexcept : offset{nullptr} {}

  constexpr entry_ref_union(const offset_entry_ref<EntryType> &r) noexcept
  : offset{r} {}

  constexpr entry_ref_union(const invocable_tagged_ref<EntryType, T> &r) noexcept
  : invocableTagged{r} {}

  constexpr entry_ref_union &operator=(const entry_ref_union &) = default;
  constexpr entry_ref_union &operator=(entry_ref_union &&) = default;

  constexpr auto &operator=(std::nullptr_t) noexcept {
    return *new(this) offset_entry_ref<EntryType>{nullptr};
  }

  // FIXME: should we make an "address" member just to do the comparisons?
  constexpr bool operator==(const entry_ref_union &rhs) const noexcept {
    return offset == rhs.offset;
  }

  constexpr explicit operator bool() const noexcept {
    return static_cast<bool>(offset);
  }
};

namespace detail {

template <typename EntryType, typename T, extractor<EntryType, T> EntryEx>
struct entry_ref_codec {
  constexpr static entry_ref_union<EntryType, T>
  create_direct_entry_ref(EntryType *entry) noexcept {
    return invocable_tagged_ref<EntryType, T>{entry};
  }

  template <typename U>
    requires std::same_as<std::remove_cv_t<U>, T>
  constexpr static entry_ref_union<EntryType, T>
  create_item_entry_ref(U *u) noexcept {
    return invocable_tagged_ref<EntryType, T>{u};
  }

  constexpr static EntryType *
  get_entry(EntryEx &e, entry_ref_union<EntryType, T> u)
      noexcept(noexcept(u.invocableTagged.get_entry(e))) {
    return u.invocableTagged.get_entry(e);
  }

  constexpr static T &get_value(entry_ref_union<EntryType, T> u) noexcept {
    return u.invocableTagged.get_value();
  }
};

template <typename EntryType, typename T, std::size_t Offset>
struct entry_ref_codec<EntryType, T, offset_extractor<EntryType, T, Offset>> {
  using extractor_type = offset_extractor<EntryType, T, Offset>;

  constexpr static entry_ref_union<EntryType, T>
  create_direct_entry_ref(EntryType *entry) noexcept {
    return offset_entry_ref<EntryType>{entry};
  }

  template <typename U>
    requires std::same_as<std::remove_cv_t<U>, T>
  constexpr static entry_ref_union<EntryType, T>
  create_item_entry_ref(U *u) noexcept {
    auto *const p = std::bit_cast<std::byte *>(u);
    return create_direct_entry_ref(std::bit_cast<EntryType *>(p + Offset));
  }

  constexpr static EntryType *
  get_entry(extractor_type, entry_ref_union<EntryType, T> u) noexcept {
    return u.offset.get_entry();
  }

  constexpr static T &get_value(entry_ref_union<EntryType, T> u) noexcept {
    auto *const p = std::bit_cast<std::byte *>(u.offset.get_entry());
    return *std::bit_cast<T *>(p - Offset);
  }
};

} // End of namespace detail

// FIXME: document somewhere that this is insufficient now that we can have
// constexpr objects as non-type parameters
template <typename T>
struct invocable_traits;

template <typename Class, typename Member>
struct invocable_traits<Member Class::*> {
  using argument_type = Class;
  using invoke_result = Member;
};

template <typename Class, typename Member>
struct invocable_traits<Member(Class)> {
  using argument_type = Class;
  using invoke_result = Member;
};

template <auto Invocable>
using cinvoke_traits_t = invocable_traits<decltype(Invocable)>;

} // End of namespace csg

#endif
