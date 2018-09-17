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

#include <bds/utility.h>

namespace bds {

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
  constexpr std::remove_cvref_t<R> &operator()(T &t) const noexcept {
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
concept bool CompressedSize = std::is_integral_v<T> || std::is_same_v<T, no_size>;

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
  constexpr decltype(auto) operator()(Args &&...args) const
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

  constexpr decltype(auto) operator()(Args &&...args) const
      noexcept(noexcept(std::invoke(std::declval<Invocable>(),
                                    std::declval<Args>()...))) {
    return std::invoke(I, std::forward<Args>(args)...);
  }

  constexpr Invocable &get_invocable() noexcept { return I; }

  constexpr const Invocable &get_invocable() const noexcept { return I; }
};

// FIXME: like invocable_tagged_ref, but cannot directly reference an entry.
// Not currently used; if we don't end up using this for the intrusive
// trees, just remove it.
template <typename EntryType, typename T>
class invocable_item_ref {
public:
  invocable_item_ref() noexcept : m_address{} {}
  invocable_item_ref(const invocable_item_ref &) = default;
  invocable_item_ref(invocable_item_ref &&) = default;
  invocable_item_ref(T *t) noexcept
      : m_address{reinterpret_cast<std::uintptr_t>(t)} {}
  ~invocable_item_ref() = default;

  template <typename EntryAccess>
      requires InvocableEntryAccessor<EntryType, EntryAccess, T>
  EntryType *get_entry(EntryAccess &e) const noexcept {
    return std::addressof(std::invoke(e, *get_value()));
  }

  T *get_value() const noexcept { return static_cast<T *>(m_address); }

  explicit operator bool() const noexcept { return m_address; }

  invocable_item_ref &operator=(const invocable_item_ref &) = default;
  invocable_item_ref &operator=(invocable_item_ref &&) = default;

  bool operator==(const invocable_item_ref &rhs) const noexcept {
    return m_address == rhs.m_address;
  }

  bool operator!=(const invocable_item_ref &rhs) const noexcept {
    return !operator==(rhs);
  }

private:
  std::uintptr_t m_address;
};

template <typename EntryType, typename T>
class invocable_tagged_ref : private util::tagged_ptr_union<EntryType, T> {
  using base_type = util::tagged_ptr_union<EntryType, T>;

public:
  using base_type::base_type;
  using base_type::operator=;
  using base_type::operator bool;

  template <typename EntryAccess>
      requires InvocableEntryAccessor<EntryType, EntryAccess, T>
  EntryType *get_entry(EntryAccess &e) const noexcept {
    return is_value()
        ? std::addressof(std::invoke(e, *get_value()))
        : static_cast<EntryType *>(*this);
  }

  T *get_value() const noexcept { return static_cast<T *>(*this); }

  bool is_entry() const noexcept { return this->template has_type<EntryType>(); }

  bool is_value() const noexcept { return this->template has_type<T>(); }

  bool operator==(const invocable_tagged_ref &rhs) const noexcept {
    return base_type::operator==(rhs);
  }

  bool operator!=(const invocable_tagged_ref &rhs) const noexcept {
    return base_type::operator!=(rhs);
  }
};

template <typename EntryType>
class offset_entry_ref {
public:
  offset_entry_ref() noexcept : m_address{} {}
  offset_entry_ref(const offset_entry_ref &) = default;
  offset_entry_ref(offset_entry_ref &&) = default;
  offset_entry_ref(EntryType *entry) noexcept
      : m_address{reinterpret_cast<std::uintptr_t>(entry)} {}
  ~offset_entry_ref() = default;

  EntryType *get_entry() const noexcept {
    return reinterpret_cast<EntryType *>(m_address);
  }

  void set_entry(EntryType *entry) noexcept {
    m_address = reinterpret_cast<std::uintptr_t>(entry);
  }

  explicit operator bool() const noexcept { return m_address; }

  offset_entry_ref &operator=(const offset_entry_ref &) = default;
  offset_entry_ref &operator=(offset_entry_ref &&) = default;

  bool operator==(const offset_entry_ref &rhs) const noexcept {
    return m_address == rhs.m_address;
  }

  bool operator!=(const offset_entry_ref &rhs) const noexcept {
    return !operator==(rhs);
  }

private:
  std::uintptr_t m_address;
};

template <typename EntryType, typename T>
union entry_ref_union {
  offset_entry_ref<EntryType> offset;
  invocable_item_ref<EntryType, T> invocableItem;
  invocable_tagged_ref<EntryType, T> invocableTagged;

  entry_ref_union() noexcept {}
  entry_ref_union(const entry_ref_union &) = default;
  entry_ref_union(entry_ref_union &&) = default;
  ~entry_ref_union() = default;

  entry_ref_union &operator=(const entry_ref_union &) = default;
  entry_ref_union &operator=(entry_ref_union &&) = default;

  auto &operator=(const offset_entry_ref<EntryType> &rhs) noexcept {
    return *new(&offset) offset_entry_ref<EntryType>{rhs};
  }

  auto &operator=(offset_entry_ref<EntryType> &&rhs) noexcept {
    return *new(&offset) offset_entry_ref<EntryType>{std::move(rhs)};
  }

  auto &operator=(const invocable_item_ref<EntryType, T> &rhs) noexcept {
    return *new(&invocableItem) invocable_item_ref<EntryType, T>{rhs};
  }

  auto &operator=(invocable_item_ref<EntryType, T> &&rhs) noexcept {
    return *new(&invocableItem) invocable_item_ref<EntryType, T>{std::move(rhs)};
  }

  auto &operator=(const invocable_tagged_ref<EntryType, T> &rhs) noexcept {
    return *new(&invocableTagged) invocable_tagged_ref<EntryType, T>{rhs};
  }

  auto &operator=(invocable_tagged_ref<EntryType, T> &&rhs) noexcept {
    return *new(&invocableTagged)
        invocable_tagged_ref<EntryType, T>{std::move(rhs)};
  }
};

// FIXME: putting this in detail for now to avoid a name collision error
namespace detail {

template <typename LinkType, typename EntryAccess>
class entry_ref_codec;

template <template <typename...> class AbstractEntryType, typename T,
          typename... Args, std::size_t Offset>
struct entry_ref_codec<offset_entry_ref<AbstractEntryType<T, Args...>>,
                       offset_extractor<AbstractEntryType<T, Args...>, Offset>> {
  using entry_type = AbstractEntryType<T, Args...>;
  using entry_ref_type = offset_entry_ref<entry_type>;

  static entry_ref_type create_entry_ref(T *t) noexcept {
    auto *const address = reinterpret_cast<std::byte *>(t);
    return reinterpret_cast<entry_type *>(address + Offset);
  }

  // FIXME: const correctness?
  static entry_ref_type create_entry_ref(const T *t) noexcept {
    auto *const address = reinterpret_cast<std::byte *>(const_cast<T *>(t));
    return reinterpret_cast<entry_type *>(address + Offset);
  }

  template <typename Invocable, typename... InvArgs>
  static entry_type *
  get_entry(compressed_invocable_ref<Invocable, InvArgs...> &invocableRef,
            entry_ref_type entryRef) noexcept {
    return get_entry(invocableRef.get_invocable(), entryRef);
  }

  static entry_type *get_entry(offset_extractor<entry_type, Offset>,
                               entry_ref_type ref) noexcept {
    return ref.get_entry();
  }

  static T *get_value(entry_ref_type ref) noexcept {
    auto *const address = reinterpret_cast<std::byte *>(ref.get_entry());
    return reinterpret_cast<T *>(address - Offset);
  }
};

template <typename EntryType, typename T, typename EntryAccess>
      requires InvocableEntryAccessor<EntryType, EntryAccess, T>
struct entry_ref_codec<invocable_item_ref<EntryType, T>, EntryAccess> {
  using entry_ref_type = invocable_item_ref<EntryType, T>;

  static entry_ref_type create_entry_ref(T *t) noexcept {
    return entry_ref_type{t};
  }

  // FIXME: const correctness?
  static entry_ref_type create_entry_ref(const T *t) noexcept {
    return entry_ref_type{const_cast<T *>(t)};
  }

  template <typename Invocable, typename... Args>
  static EntryType *
  get_entry(compressed_invocable_ref<Invocable, Args...> &invocableRef,
            entry_ref_type entryRef) noexcept {
    return get_entry(invocableRef.get_invocable(), entryRef);
  }

  static EntryType *get_entry(EntryAccess &e, entry_ref_type ref) noexcept {
    return ref.get_entry(e);
  }

  static T *get_value(entry_ref_type ref) noexcept { return ref.get_value(); }
};

template <typename EntryType, typename T, typename EntryAccess>
      requires InvocableEntryAccessor<EntryType, EntryAccess, T>
struct entry_ref_codec<invocable_tagged_ref<EntryType, T>, EntryAccess> {
  using entry_ref_type = invocable_tagged_ref<EntryType, T>;

  static entry_ref_type create_entry_ref(T *t) noexcept {
    return entry_ref_type{t};
  }

  // FIXME: const correctness?
  static entry_ref_type create_entry_ref(const T *t) noexcept {
    return entry_ref_type{const_cast<T *>(t)};
  }

  template <typename Invocable, typename... Args>
  static EntryType *
  get_entry(compressed_invocable_ref<Invocable, Args...> &invocableRef,
            entry_ref_type entryRef) noexcept {
    return get_entry(invocableRef.get_invocable(), entryRef);
  }

  static EntryType *get_entry(EntryAccess &e, entry_ref_type ref) noexcept {
    return ref.get_entry(e);
  }

  static T *get_value(entry_ref_type ref) noexcept { return ref.get_value(); }
};

} // End of namespace detail

template <typename T, typename EntryAccess>
struct entry_access_traits { using entry_access_type = EntryAccess; };

template <typename T, typename EntryAccess>
using entry_access_helper_t = entry_access_traits<T, EntryAccess>::entry_access_type;

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

} // End of namespace bds

#endif
