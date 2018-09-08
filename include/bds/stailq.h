//==-- bds/stailq.h - singly-linked tail queue implementation ---*- C++ -*-==//
//
//                     BSD Data Structures (BDS) Library
//
// This file is distributed under the 2-clause BSD Open Source License. See
// LICENSE.TXT for details.
//==-----------------------------------------------------------------------==//

/**
 * @file
 * @brief Contains an STL-compatible implementation of singly-linked intrusive
 *     tail queues, inspired by BSD's queue(3) STAILQ_ macros.
 */

#ifndef BDS_STAILQ_H
#define BDS_STAILQ_H

#include <functional>
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <utility>

#include "assert.h"
#include "list_common.h"

namespace bds {

struct stailq_entry {
  std::uintptr_t next;
};

template <typename T, typename EntryAccess, typename Derived>
    requires STailQEntryAccessor<EntryAccess, T>
class stailq_base {
public:
  using value_type = T;
  using reference = T &;
  using const_reference = const T &;
  using pointer = T *;
  using const_pointer = const T *;
  using entry_type = stailq_entry;
  using entry_access_type = EntryAccess;
  using derived_type = Derived;

  template <typename D>
  using other_list_t = stailq_base<T, EntryAccess, D>;

  stailq_base() requires std::is_default_constructible_v<EntryAccess> = default;

  stailq_base(const stailq_base &) = delete;

  stailq_base(stailq_base &&other)
      requires std::is_move_constructible_v<EntryAccess> = default;

  template <typename D>
  stailq_base(other_list_t<D> &&other)
      noexcept(std::is_nothrow_move_constructible_v<EntryAccess>)
      : entryAccessor{std::move(other.entryAccessor)} {}

  template <typename... Ts>
      requires std::is_constructible_v<EntryAccess, Ts...>
  stailq_base(Ts &&... vs)
      noexcept(std::is_nothrow_constructible_v<EntryAccess, Ts...>)
      : entryAccessor{std::forward<Ts>(vs)...} {}

  ~stailq_base() noexcept { clear(); }

  stailq_base &operator=(const stailq_base &) = delete;

  template <typename D>
  stailq_base &operator=(other_list_t<D> &&other)
      noexcept(std::is_nothrow_move_assignable_v<EntryAccess>)
      requires std::is_move_assignable_v<EntryAccess> {
    clear();
    swap_lists(other);
    entryAccessor = std::move(other.entryAccessor);
    return *this;
  }

  stailq_base &operator=(std::initializer_list<T *> ilist) noexcept {
    assign(ilist);
    return *this;
  }

  entry_access_type &get_entry_accessor() noexcept { return entryAccessor; }

  const entry_access_type &get_entry_accessor() const noexcept {
    return entryAccessor;
  }

  template <typename InputIt>
  void assign(InputIt first, InputIt last) noexcept {
    clear();
    insert_after(cbefore_begin(), first, last);
  }

  void assign(std::initializer_list<T *> ilist) noexcept {
    assign(std::begin(ilist), std::end(ilist));
  }

  reference front() noexcept { return *begin(); }

  const_reference front() const noexcept { return *begin(); }

  reference back() noexcept { return *before_end(); }

  const_reference back() const noexcept { return *before_end(); }

  class iterator;
  class const_iterator;

  using iterator_t = type_identity_t<iterator>;
  using const_iterator_t = type_identity_t<const_iterator>;

  iterator before_begin() noexcept {
    return {(std::uintptr_t)getHeadEntry(), entryAccessor};
  }

  const_iterator before_begin() const noexcept {
    return {(std::uintptr_t)getHeadEntry(), entryAccessor};
  }

  const_iterator cbefore_begin() const noexcept { return before_begin(); }

  iterator begin() noexcept { return ++before_begin(); }
  const_iterator begin() const noexcept { return ++before_begin(); }
  const_iterator cbegin() const noexcept { return begin(); }

  iterator before_end() noexcept { return {getEncodedTail(), entryAccessor}; }

  const_iterator before_end() const noexcept {
    return {getEncodedTail(), entryAccessor};
  }

  const_iterator cbefore_end() const noexcept { return before_end(); }

  iterator end() noexcept { return {std::uintptr_t(0), entryAccessor}; }

  const_iterator end() const noexcept {
    return {std::uintptr_t(0), entryAccessor};
  }

  const_iterator cend() const noexcept { return end(); }

  iterator iter(T *t) noexcept { return {t, entryAccessor}; }
  const_iterator iter(const T *t) noexcept { return {t, entryAccessor}; }
  const_iterator citer(const T *t) noexcept { return {t, entryAccessor}; }

  [[nodiscard]] bool empty() const noexcept { return !getHeadEntry()->next; }

  auto size() const noexcept;

  constexpr auto max_size() {
    return std::numeric_limits<typename Derived::size_type>::max();
  }

  void clear() noexcept;

  iterator insert_after(const_iterator, T *) noexcept;

  template <typename InputIt>
  iterator insert_after(const_iterator, InputIt first, InputIt last) noexcept;

  iterator insert_after(const_iterator_t pos,
                        std::initializer_list<T *> i) noexcept {
    return insert_after(pos, std::begin(i), std::end(i));
  }

  iterator erase_after(const_iterator) noexcept;

  iterator erase_after(const_iterator first, const_iterator last) noexcept;

  std::pair<T *, iterator> find_erase(const_iterator_t pos) noexcept {
    T *const erased = const_cast<T *>(std::addressof(*pos));
    return {erased, erase_after(find_predecessor(pos))};
  }

  template <typename Visitor>
  void for_each_safe(Visitor v) noexcept {
    for_each_safe(begin(), end(), v);
  }

  template <typename Visitor>
  void for_each_safe(iterator_t first, const const_iterator_t last,
                     Visitor v) noexcept {
    while (first != last)
      v(*first++);
  }

  void push_front(T *t) noexcept { insert_after(cbefore_begin(), t); }

  void pop_front() noexcept { erase_after(cbefore_begin()); }

  void push_back(T *t) noexcept { insert_after(cbefore_end(), t); }

  template <typename D2>
  void swap(other_list_t<D2> &other)
      noexcept(std::is_nothrow_swappable_v<EntryAccess>) {
    std::swap(entryAccessor, other.entryAccessor);
    swap_lists(other);
  }

  [[nodiscard]] iterator find_predecessor(const_iterator pos) const noexcept;

  template <typename D2>
  void merge(other_list_t<D2> &other) noexcept {
    return merge(other, std::less<T>{});
  }

  template <typename D2>
  void merge(other_list_t<D2> &&other) noexcept {
    return merge(std::move(other), std::less<T>{});
  }

  template <typename D2, typename Compare>
  void merge(other_list_t<D2> &other, Compare comp) noexcept;

  template <typename D2, typename Compare>
  void merge(other_list_t<D2> &&other, Compare comp) noexcept {
    merge(other, comp);
  }

  template <typename D2>
  void splice_after(const_iterator pos, other_list_t<D2> &other) noexcept;

  template <typename D2>
  void splice_after(const_iterator_t pos, other_list_t<D2> &&other) noexcept {
    return splice_after(pos, other);
  }

  template <typename D2>
  void splice_after(const_iterator_t pos, other_list_t<D2> &other,
                    typename other_list_t<D2>::const_iterator it) noexcept {
    return splice_after(pos, other, it, other.cend());
  }

  template <typename D2>
  void splice_after(const_iterator_t pos, other_list_t<D2> &&other,
                    typename other_list_t<D2>::const_iterator it) noexcept {
    return splice_after(pos, other, it);
  }

  template <typename D2>
  void splice_after(const_iterator pos, other_list_t<D2> &other,
                    typename other_list_t<D2>::const_iterator first,
                    typename other_list_t<D2>::const_iterator last) noexcept;

  template <typename D2>
  void splice_after(const_iterator_t pos, other_list_t<D2> &&other,
                    typename other_list_t<D2>::const_iterator first,
                    typename other_list_t<D2>::const_iterator last) noexcept {
    return splice_after(pos, other, first, last);
  }

  void remove(const T &value) noexcept { remove_if(std::equal_to<T>{}); }

  template <typename UnaryPredicate>
  void remove_if(UnaryPredicate) noexcept;

  void reverse() noexcept;

  void unique() noexcept { unique(std::equal_to<T>{}); }

  template <typename BinaryPredicate>
  void unique(BinaryPredicate) noexcept;

  void sort() noexcept { sort(std::less<T>{}); }

  template <typename Compare>
  void sort(Compare) noexcept;

protected:
  template <typename D2>
  void swap_lists(other_list_t<D2> &other) noexcept;

private:
  template <typename, typename, typename>
  friend class stailq_base;

#if 0
  template <SListOrQueue L, typename C, typename S>
      requires std::is_integral_v<S>
  friend typename L::const_iterator
      detail::forward_list_merge_sort(typename L::const_iterator,
                                      typename L::const_iterator,
                                      C, S) noexcept;
#endif

  template <typename L, typename C, typename S>
  friend typename L::const_iterator
  detail::forward_list_merge_sort(typename L::const_iterator,
                                  typename L::const_iterator, C, S) noexcept;

  using stailq_link_encoder = link_encoder<stailq_entry, EntryAccess, T>;

  stailq_entry *getHeadEntry() noexcept {
    return static_cast<Derived *>(this)->getHeadEntry();
  }

  const stailq_entry *getHeadEntry() const noexcept {
    return const_cast<stailq_base *>(this)->getHeadEntry();
  }

  std::uintptr_t &getEncodedTail() noexcept {
    return static_cast<Derived *>(this)->getEncodedTail();
  }

  const std::uintptr_t &getEncodedTail() const noexcept {
    return const_cast<stailq_base *>(this)->getEncodedTail();
  }

  auto &getSizeRef() noexcept {
    return static_cast<Derived *>(this)->getSizeRef();
  }

  template <typename U>
  stailq_entry *getEntry(U u) const noexcept {
    compressed_invocable_ref<EntryAccess, T &> fn{entryAccessor};
    return stailq_link_encoder::getEntry(fn, u);
  }

  template <typename It>
      requires std::is_same_v<It, iterator_t> ||
               std::is_same_v<It, const_iterator_t>
  static stailq_entry *getEntry(It i) noexcept {
    return stailq_link_encoder::getEntry(i.rEntryAccessor, i.current);
  }

  static std::uintptr_t getEncoding(const_iterator_t i) noexcept {
    return i.current;
  }

  template <typename QueueIt>
  static iterator insert_range_after(const_iterator pos, QueueIt first,
                                     QueueIt last) noexcept;

  [[no_unique_address]] mutable EntryAccess entryAccessor;
};

template <typename T, typename EntryAccess, typename Derived>
class stailq_base<T, EntryAccess, Derived>::iterator {
public:
  using container = stailq_base<T, EntryAccess, Derived>;
  using value_type = container::value_type;
  using reference = container::reference;
  using pointer = container::pointer;
  using difference_type = typename Derived::difference_type;
  using iterator_category = std::forward_iterator_tag;
  using invocable_ref = compressed_invocable_ref<EntryAccess, T &>;

  iterator() noexcept : current{}, rEntryAccessor{} {}
  iterator(const iterator &) noexcept = default;
  iterator(iterator &&) noexcept = default;

  iterator(T *t) noexcept requires Stateless<EntryAccess>
      : current{stailq_link_encoder::encode(t)}, rEntryAccessor{} {}

  iterator(T *t, EntryAccess &fn) noexcept
      : current{stailq_link_encoder::encode(t)}, rEntryAccessor{fn} {}

  ~iterator() noexcept = default;

  iterator &operator=(const iterator &) noexcept = default;

  iterator &operator=(iterator &&) noexcept = default;

  reference operator*() const noexcept { return *operator->(); }

  pointer operator->() const noexcept {
    return stailq_link_encoder::getValue(this->current);
  }

  iterator &operator++() noexcept {
    current = container::getEntry(*this)->next;
    return *this;
  }

  iterator operator++(int) noexcept {
    iterator i{*this};
    this->operator++();
    return i;
  }

  bool operator==(const iterator &rhs) const noexcept {
    return current == rhs.current;
  }

  bool operator==(const const_iterator &rhs) const noexcept {
    return current == rhs.current;
  }

  bool operator!=(const iterator &rhs) const noexcept {
    return current != rhs.current;
  }

  bool operator!=(const const_iterator &rhs) const noexcept {
    return current != rhs.current;
  }

private:
  template <typename, typename, typename>
  friend class stailq_base;

  friend container::const_iterator;

  iterator(std::uintptr_t e, EntryAccess &fn) noexcept
      : current{e}, rEntryAccessor{fn} {}

  std::uintptr_t current;
  [[no_unique_address]] invocable_ref rEntryAccessor;
};

template <typename T, typename EntryAccess, typename Derived>
class stailq_base<T, EntryAccess, Derived>::const_iterator {
public:
  using container = stailq_base<T, EntryAccess, Derived>;
  using value_type = container::value_type;
  using reference = container::const_reference;
  using pointer = container::const_pointer;
  using difference_type = typename Derived::difference_type;
  using iterator_category = std::forward_iterator_tag;
  using invocable_ref = compressed_invocable_ref<EntryAccess, T &>;

  const_iterator() noexcept : current{}, rEntryAccessor{} {}
  const_iterator(const const_iterator &) noexcept = default;
  const_iterator(const_iterator &&) noexcept = default;
  const_iterator(const iterator &i) noexcept
      : current{i.current}, rEntryAccessor{i.rEntryAccessor} {}

  const_iterator(const T *t) noexcept requires Stateless<EntryAccess>
      : current{stailq_link_encoder::encode(t)}, rEntryAccessor{} {}

  const_iterator(const T *t, EntryAccess &fn) noexcept
      : current{stailq_link_encoder::encode(t)}, rEntryAccessor{fn} {}

  ~const_iterator() noexcept = default;

  const_iterator &operator=(const const_iterator &) noexcept = default;

  const_iterator &operator=(const_iterator &&) noexcept = default;

  reference operator*() const noexcept { return *operator->(); }

  pointer operator->() const noexcept {
    return stailq_link_encoder::getValue(this->current);
  }

  const_iterator &operator++() noexcept {
    current = container::getEntry(*this)->next;
    return *this;
  }

  const_iterator operator++(int) noexcept {
    const_iterator i{*this};
    this->operator++();
    return i;
  }

  bool operator==(const iterator &rhs) const noexcept {
    return current == rhs.current;
  }

  bool operator==(const const_iterator &rhs) const noexcept {
    return current == rhs.current;
  }

  bool operator!=(const iterator &rhs) const noexcept {
    return current != rhs.current;
  }

  bool operator!=(const const_iterator &rhs) const noexcept {
    return current != rhs.current;
  }

private:
  template <typename, typename, typename>
  friend class stailq_base;

  friend container::iterator;

  const_iterator(std::uintptr_t e, EntryAccess &fn) noexcept
      : current{e}, rEntryAccessor{fn} {}

  std::uintptr_t current;
  [[no_unique_address]] invocable_ref rEntryAccessor;
};

template <SizeMember>
class stailq_fwd_head;

template <typename T, typename EntryAccess, SizeMember SizeType>
class stailq_container
    : public stailq_base<T, EntryAccess,
                         stailq_container<T, EntryAccess, SizeType>> {
public:
  using base_type =
      stailq_base<T, EntryAccess, stailq_container<T, EntryAccess, SizeType>>;

  using size_type = std::conditional_t<std::is_same_v<SizeType, no_size>,
                                       std::size_t, SizeType>;

  using difference_type = std::make_signed_t<size_type>;

  using fwd_head_type = stailq_fwd_head<SizeType>;

  template <typename D>
  using other_list_t = typename stailq_base<
      T, EntryAccess,
      stailq_container<T, EntryAccess, SizeType>>::template other_list_t<D>;

  stailq_container() = delete;

  stailq_container(const stailq_container &) = delete;

  stailq_container(stailq_container &&) = delete;

  stailq_container(stailq_fwd_head<SizeType> &h)
      noexcept(std::is_nothrow_default_constructible_v<EntryAccess>)
      requires std::is_default_constructible_v<EntryAccess>
      : base_type{}, head{h} {}

  template <typename... Ts>
      requires std::is_constructible_v<EntryAccess, Ts...>
  explicit stailq_container(stailq_fwd_head<SizeType> &h, Ts &&... vs)
      noexcept(std::is_nothrow_constructible_v<EntryAccess, Ts...>)
      : base_type{std::forward<Ts>(vs)...}, head{h} {}

  stailq_container(stailq_fwd_head<SizeType> &h, stailq_container &&other)
      noexcept(std::is_nothrow_move_constructible_v<EntryAccess>)
      : base_type{std::move(other)}, head{h} {
    this->swap_lists(other);
  }

  template <typename D>
  stailq_container(stailq_fwd_head<SizeType> &h, other_list_t<D> &&other)
      noexcept(std::is_nothrow_move_constructible_v<EntryAccess>)
      : base_type{std::move(other)}, head{h} {
    this->swap_lists(other);
  }

  template <typename InputIt, typename... Ts>
  stailq_container(stailq_fwd_head<SizeType> &h, InputIt first, InputIt last,
                   Ts &&... vs)
      noexcept(std::is_nothrow_constructible_v<EntryAccess, Ts...>)
      : base_type{std::forward<Ts>(vs)...}, head{h} {
    base_type::assign(first, last);
  }

  template <typename... Ts>
  stailq_container(stailq_fwd_head<SizeType> &h,
                   std::initializer_list<T *> ilist, Ts &&... vs)
      noexcept(std::is_nothrow_constructible_v<EntryAccess, Ts...>)
      : base_type{std::forward<Ts>(vs)...}, head{h} {
    base_type::assign(ilist);
  }

  ~stailq_container() = default;

  stailq_container &operator=(const stailq_container &) = delete;

  stailq_container &operator=(stailq_container &&rhs)
      noexcept(std::is_nothrow_move_assignable_v<EntryAccess>) {
    base_type::operator=(std::move(rhs));
    return *this;
  }

  template <typename D>
  stailq_container &operator=(other_list_t<D> &&rhs)
      noexcept(std::is_nothrow_move_assignable_v<EntryAccess>) {
    base_type::operator=(std::move(rhs));
    return *this;
  }

  stailq_container &operator=(std::initializer_list<T *> ilist) noexcept {
    base_type::operator=(ilist);
    return *this;
  }

private:
  template <typename, typename, typename>
  friend class stailq_base;

  constexpr static bool HasInlineSize = !std::is_same_v<SizeType, no_size>;

  stailq_entry *getHeadEntry() noexcept { return head.getHeadEntry(); }

  std::uintptr_t &getEncodedTail() noexcept { return head.getEncodedTail(); }

  auto &getSizeRef() noexcept { return head.getSizeRef(); }

  stailq_fwd_head<SizeType> &head;
};

template <SizeMember SizeType>
class stailq_fwd_head {
public:
  using size_type = std::conditional_t<std::is_same_v<SizeType, no_size>,
                                       std::size_t, SizeType>;

  using difference_type = std::make_signed_t<size_type>;

  stailq_fwd_head() noexcept : headEntry{}, encodedTail{}, sz{} {}

  stailq_fwd_head(const stailq_fwd_head &) = delete;

  stailq_fwd_head(stailq_fwd_head &&) = delete;

  ~stailq_fwd_head() = default;

  stailq_fwd_head &operator=(const stailq_fwd_head &) = delete;

  stailq_fwd_head &operator=(stailq_fwd_head &&) = delete;

protected:
  template <typename, typename, typename>
  friend class stailq_base;

  template <typename, typename, typename>
  friend class stailq_container;

  constexpr static bool HasInlineSize = !std::is_same_v<SizeType, no_size>;

  stailq_entry *getHeadEntry() noexcept { return &headEntry; }

  std::uintptr_t &getEncodedTail() noexcept { return encodedTail; }

  auto &getSizeRef() noexcept { return sz; }

  stailq_entry headEntry;
  std::uintptr_t encodedTail;
  [[no_unique_address]] SizeType sz;
};

template <typename T, typename EntryAccess, SizeMember SizeType>
class stailq_head
    : public stailq_base<T, EntryAccess, stailq_head<T, EntryAccess, SizeType>>,
      public stailq_fwd_head<SizeType> {
public:
  using base_type =
      stailq_base<T, EntryAccess, stailq_head<T, EntryAccess, SizeType>>;

  template <typename D>
  using other_list_t = typename stailq_base<
      T, EntryAccess,
      stailq_head<T, EntryAccess, SizeType>>::template other_list_t<D>;

  stailq_head() = default;

  stailq_head(const stailq_head &) = delete;

  stailq_head(stailq_head &&other)
      noexcept(std::is_nothrow_move_constructible_v<base_type>)
      : base_type{std::move(other)} {
    this->swap_lists(other);
  }

  template <typename D>
  stailq_head(other_list_t<D> &&other)
      noexcept(std::is_nothrow_move_constructible_v<base_type>)
      : base_type{std::move(other)} {
    this->swap_lists(other);
  }

  template <typename... Ts>
      requires std::is_constructible_v<EntryAccess, Ts...>
  explicit stailq_head(Ts &&... vs)
      noexcept(std::is_nothrow_constructible_v<EntryAccess, Ts...>)
      : base_type{std::forward<Ts>(vs)...} {}

  template <typename InputIt, typename... Ts>
  stailq_head(InputIt first, InputIt last, Ts &&... vs)
      noexcept(std::is_nothrow_constructible_v<EntryAccess, Ts...>)
      : base_type{std::forward<Ts>(vs)...} {
    base_type::assign(first, last);
  }

  template <typename... Ts>
  stailq_head(std::initializer_list<T *> ilist, Ts &&... vs)
      noexcept(std::is_nothrow_constructible_v<EntryAccess, Ts...>)
      : base_type{std::forward<Ts>(vs)...} {
    base_type::assign(ilist);
  }

  ~stailq_head() = default;

  stailq_head &operator=(const stailq_head &) = delete;

  stailq_head &operator=(stailq_head &&rhs)
      noexcept(std::is_nothrow_move_assignable_v<EntryAccess>) {
    base_type::operator=(std::move(rhs));
    return *this;
  }

  template <typename D>
  stailq_head &operator=(other_list_t<D> &&rhs)
      noexcept(std::is_nothrow_move_assignable_v<EntryAccess>) {
    base_type::operator=(std::move(rhs));
    return *this;
  }

  stailq_head &operator=(std::initializer_list<T *> ilist) noexcept {
    base_type::operator=(ilist);
    return *this;
  }

private:
  template <typename, typename, typename>
  friend class stailq_base;

  // Pull stailq_fwd_head's get{Head,Tail}Entry into our scope, so that the
  // CRTP polymorphic call in the base class will find it unambiguously.
  using stailq_fwd_head<SizeType>::getHeadEntry;
  using stailq_fwd_head<SizeType>::getEncodedTail;
  using stailq_fwd_head<SizeType>::getSizeRef;
};

template <typename T, typename E, typename D>
auto stailq_base<T, E, D>::size() const noexcept {
  if constexpr (D::HasInlineSize)
    return const_cast<stailq_base *>(this)->getSizeRef();
  else {
    const auto s = std::distance(begin(), end());
    return static_cast<typename D::size_type>(s);
  }
}

template <typename T, typename E, typename D>
void stailq_base<T, E, D>::clear() noexcept {
  getHeadEntry()->next = getEncodedTail() = 0;

  if constexpr (D::HasInlineSize)
    getSizeRef() = 0;
}

template <typename T, typename E, typename D>
typename stailq_base<T, E, D>::iterator
stailq_base<T, E, D>::insert_after(const_iterator pos, T *value) noexcept {
  BDS_ASSERT(pos != end(), "end() iterator passed to insert_after");

  stailq_entry *const posEntry = getEntry(pos);
  stailq_entry *const insertEntry = getEntry(value);

  insertEntry->next = posEntry->next;
  const auto encoded = posEntry->next = stailq_link_encoder::encode(value);

  if (!insertEntry->next)
    getEncodedTail() = encoded;

  if constexpr (D::HasInlineSize)
    ++getSizeRef();

  return {encoded, entryAccessor};
}

template <typename T, typename E, typename D>
template <typename InputIt>
typename stailq_base<T, E, D>::iterator
stailq_base<T, E, D>::insert_after(const_iterator pos, InputIt first,
                                   InputIt last) noexcept {
  while (first != last)
    pos = insert_after(pos, *first++);

  return {pos.current, entryAccessor};
}

template <typename T, typename E, typename D>
typename stailq_base<T, E, D>::iterator
stailq_base<T, E, D>::erase_after(const_iterator pos) noexcept {
  BDS_ASSERT(pos != end(), "end() iterator passed to erase_after");

  stailq_entry *const posEntry = getEntry(pos);
  const bool isLastEntry = !posEntry->next;

  if constexpr (D::HasInlineSize) {
    if (!isLastEntry)
      --getSizeRef();
  }

  if (isLastEntry)
    return end();

  stailq_entry *const erasedEntry = getEntry(posEntry->next);
  const auto encodedNext = posEntry->next = erasedEntry->next;

  if (!encodedNext) {
    // removedEntry was the tail element so now posEntry becomes the tail,
    // unless the list is empty.
    const bool isEmpty = pos.current == (std::uintptr_t)getHeadEntry();
    getEncodedTail() = isEmpty ? 0 : pos.current;
  }

  return {encodedNext, entryAccessor};
}

template <typename T, typename E, typename D>
typename stailq_base<T, E, D>::iterator
stailq_base<T, E, D>::erase_after(const_iterator first,
                                  const_iterator last) noexcept {
  if (first == end())
    return {first.current, entryAccessor};

  // Remove the open range (first, last) by linking first directly to last,
  // thereby removing all the internal elements.
  stailq_entry *const firstEntry = getEntry(first);
  firstEntry->next = last.current;

  if (!last.current) {
    // last is end(), so first is the new tail element, unless first is
    // before_begin() -- in that case the tail element will be end().
    getEncodedTail() = (first == cbefore_begin()) ? 0 : first.current;
  }

  if constexpr (D::HasInlineSize)
    getSizeRef() -= std::distance(++first, last);

  return {last.current, entryAccessor};
}

template <typename T, typename E, typename D>
typename stailq_base<T, E, D>::iterator
stailq_base<T, E, D>::find_predecessor(const_iterator pos) const noexcept {
  const_iterator scan = cbefore_begin();
  const const_iterator end = cend();

  while (scan != end) {
    if (auto prev = scan++; scan == pos)
      return {prev.current, entryAccessor};
  }

  return {end.current, entryAccessor};
}

template <typename T, typename E, typename D1>
template <typename D2, typename Compare>
void stailq_base<T, E, D1>::merge(other_list_t<D2> &other,
                                  Compare comp) noexcept {
  if (this == &other)
    return;

  auto p1 = cbefore_begin();
  auto f1 = std::next(p1);
  auto e1 = cend();

  const_iterator mergeEnd;
  auto f2 = other.cbegin();
  auto e2 = other.cend();

  if constexpr (D1::HasInlineSize)
    getSizeRef() += std::size(other);

  stailq_entry *const otherHead = other.getHeadEntry();

  while (f1 != e1 && f2 != e2) {
    if (comp(*f1, *f2)) {
      p1 = f1++;
      continue;
    }

    // Scan the range items where *f2 < *f1. When we're done, [f2, mergeEnd]
    // will be the closed range of elements that needs to merged after p1
    // (before f1).
    mergeEnd = f2;

    for (auto scan = std::next(mergeEnd); scan != e2 && comp(*scan, *f1); ++scan)
      mergeEnd = scan;

    f2 = insert_range_after(p1, f2, mergeEnd);

    p1 = mergeEnd;
    f1 = std::next(mergeEnd);
  }

  if (f2 != e2) {
    // Merge the remaining range, [f2, e2) at the end of the list; the tail
    // element is located in `other`.
    getEntry(p1)->next = f2.current;
    getEncodedTail() = other.getEncodedTail();
  }

  other.clear();
}

template <typename T, typename E, typename D1>
template <typename D2>
void stailq_base<T, E, D1>::splice_after(const_iterator pos,
                                         other_list_t<D2> &other) noexcept {
  if (other.empty())
    return;

  BDS_ASSERT(pos.current && "end() iterator passed as pos");

  stailq_entry *const posEntry = getEntry(pos);

  if (!posEntry->next) {
    // pos is the tail entry; new tail entry will come from the other list.
    getEncodedTail() = other.getEncodedTail();
  }

  posEntry->next = other.begin().current;

  if constexpr (D1::HasInlineSize)
    getSizeRef() += std::size(other);

  other.clear();
}

template <typename T, typename E, typename D1>
template <typename D2>
void stailq_base<T, E, D1>::splice_after(
    const_iterator pos, other_list_t<D2> &other,
    typename other_list_t<D2>::const_iterator first,
    typename other_list_t<D2>::const_iterator last) noexcept {
  if (first == last)
    return;

  // When the above is false, getEntry(first) and first++ must be legal.
  BDS_ASSERT(first.current, "first is end() but last was not end()?");

  if (!last.current) {
    // last is end(), so we're removing the tail element -- first points to
    // the new tail element of other.
    other.getEncodedTail() = first.current;
  }

  // Remove the open range (first, last) from `other`, by directly linking
  // first and last. Also post-increment first, so that it will point to the
  // start of the closed range [first + 1, last - 1] that we're inserting.
  other.getEntry(first++)->next = last.current;

  if (first == last)
    return;

  // Find the last element in the closed range we're inserting, then use
  // insert_range_after to insert the closed range after pos.
  auto lastInsert = first, scan = std::next(lastInsert);
  std::common_type_t<typename D1::difference_type, typename D2::difference_type> sz = 0;

  while (scan != last) {
    lastInsert = scan++;
    ++sz;
  }

  if constexpr (D1::HasInlineSize)
    getSizeRef() += sz;

  if constexpr (D2::HasInlineSize)
    other.getSizeRef() -= sz;

  insert_range_after(pos, first, lastInsert);

  if (!getEntry(lastInsert.current)->next) {
    // lastInsert is the new tail element.
    getEncodedTail() = lastInsert.current;
  }
}

template <typename T, typename E, typename D>
template <typename UnaryPredicate>
void stailq_base<T, E, D>::remove_if(UnaryPredicate pred) noexcept {
  const_iterator prev = cbefore_begin();
  const_iterator i = std::next(prev);
  const const_iterator end = cend();

  while (i != end) {
    if (!pred(*i)) {
      // Not removing i, advance to the next element and restart.
      prev = i++;
      continue;
    }

    // It is slightly more efficient to bulk remove a range of elements if
    // we have contiguous sequences where the predicate matches. Build the
    // open range (prev, i), where all contained elements are to be removed.
    ++i;
    while (i != end && pred(*i))
      ++i;

    prev = erase_after(prev, i);
    i = (prev != end) ? std::next(prev) : end;
  }
}

template <typename T, typename E, typename D>
void stailq_base<T, E, D>::reverse() noexcept {
  const const_iterator end = cend();
  const_iterator i = cbegin();
  const_iterator prev = end;

  getEncodedTail() = i.current;

  while (i != end) {
    const auto current = i;
    stailq_entry *const entry = getEntry(i++);
    entry->next = prev.current;
    prev = current;
  }

  getHeadEntry()->next = prev.current;
}

template <typename T, typename E, typename D>
template <typename BinaryPredicate>
void stailq_base<T, E, D>::unique(BinaryPredicate pred) noexcept {
  if (empty())
    return;

  const_iterator prev = cbegin();
  const_iterator i = std::next(prev);
  const const_iterator end = cend();

  while (i != end) {
    if (!pred(*prev, *i)) {
      // Adjacent items are unique, keep scanning.
      prev = i++;
      continue;
    }

    // `i` is the start of a list of duplicates. Scan for the end of the
    // duplicate range and erase the open range (prev, scanEnd)
    auto scanEnd = std::next(i);

    while (scanEnd != end && pred(*prev, *scanEnd))
      ++scanEnd;

    prev = erase_after(prev, scanEnd);
    i = (prev != end) ? std::next(prev) : end;
  }
}

template <typename T, typename E, typename D>
template <typename Compare>
void stailq_base<T, E, D>::sort(Compare comp) noexcept {
  const auto pEnd = detail::forward_list_merge_sort<stailq_base<T, E, D>>(
      cbefore_begin(), cend(), comp, std::size(*this));
  getEncodedTail() = pEnd.current;
}

template <typename T, typename E, typename D1>
template <typename D2>
void stailq_base<T, E, D1>::swap_lists(other_list_t<D2> &other) noexcept {
  std::swap(*getHeadEntry(), *other.getHeadEntry());
  std::swap(getEncodedTail(), other.getEncodedTail());
  std::swap(getSizeRef(), other.getSizeRef());
}

template <typename T, typename E, typename D>
template <typename QueueIt>
typename stailq_base<T, E, D>::iterator
stailq_base<T, E, D>::insert_range_after(const_iterator pos, QueueIt first,
                                         QueueIt last) noexcept {
  // Inserts the closed range [first, last] after pos, and returns the
  // successor to last.
  BDS_ASSERT(pos.current && last.current,
             "end() iterator passed as pos or last");

  stailq_entry *const posEntry = getEntry(pos);
  stailq_entry *const lastEntry = QueueIt::container::getEntry(last);

  const auto oldNext = lastEntry->next;

  lastEntry->next = posEntry->next;
  posEntry->next = first.current;

  return iterator{oldNext, last.rEntryAccessor.get_invocable()};
}

} // End of namespace bds

#endif
