//==-- bds/tailq.h - tail queue intrusive list implementation ---*- C++ -*-==//
//
//                     BSD Data Structures (BDS) Library
//
// This file is distributed under the 2-clause BSD Open Source License. See
// LICENSE.TXT for details.
//==-----------------------------------------------------------------------==//

/**
 * @file
 * @brief Contains an STL-compatible implementation of intrusive tail queues,
 *     inspired by BSD's queue(3) TAILQ_ macros.
 */

#ifndef BDS_TAILQ_H
#define BDS_TAILQ_H

#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <type_traits>

#include "assert.h"
#include "list_common.h"

namespace bds {

struct tailq_entry {
  std::uintptr_t next;
  std::uintptr_t prev;
};

inline tailq_entry &make_sentinel_entry(tailq_entry *e) noexcept {
  e->next = e->prev = reinterpret_cast<std::uintptr_t>(e);
  return *e;
}

template <typename T, typename EntryAccess, typename Derived>
    requires TailQEntryAccessor<EntryAccess, T>
class tailq_base {
public:
  using value_type = T;
  using reference = T &;
  using const_reference = const T &;
  using pointer = T *;
  using const_pointer = const T *;
  using entry_type = tailq_entry;
  using entry_access_type = EntryAccess;
  using derived_type = Derived;

  template <typename D>
  using other_list_t = tailq_base<T, EntryAccess, D>;

  tailq_base() requires std::is_default_constructible_v<EntryAccess> = default;

  tailq_base(const tailq_base &) = delete;

  tailq_base(tailq_base &&other)
      requires std::is_move_constructible_v<EntryAccess> = default;

  template <typename D>
  tailq_base(other_list_t<D> &&other)
      noexcept(std::is_nothrow_move_constructible_v<EntryAccess>)
      : entryAccessor{std::move(other.entryAccessor)} {}

  template <typename... Ts>
      requires std::is_constructible_v<EntryAccess, Ts...>
  explicit tailq_base(Ts &&... vs)
      noexcept(std::is_nothrow_constructible_v<EntryAccess, Ts...>)
      : entryAccessor{std::forward<Ts>(vs)...} {}

  ~tailq_base() noexcept { clear(); }

  tailq_base &operator=(const tailq_base &) = delete;

  template <typename D>
  tailq_base &operator=(other_list_t<D> &&other)
      noexcept(std::is_nothrow_move_assignable_v<EntryAccess>)
      requires std::is_move_assignable_v<EntryAccess> {
    clear();
    swap_lists(other);
    entryAccessor = std::move(other.entryAccessor);
    return *this;
  }

  tailq_base &operator=(std::initializer_list<T *> ilist) noexcept {
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
    insert(cbegin(), first, last);
  }

  void assign(std::initializer_list<T *> ilist) noexcept {
    assign(std::begin(ilist), std::end(ilist));
  }

  reference front() noexcept { return *begin(); }

  const_reference front() const noexcept { *begin(); }

  reference back() noexcept { return *--end(); }

  const_reference back() const noexcept { return *--end(); }

  class iterator;
  class const_iterator;

  using iterator_t = type_identity_t<iterator>;
  using const_iterator_t = type_identity_t<const_iterator>;

  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  iterator begin() noexcept { return {getEndEntry()->next, entryAccessor}; }

  const_iterator begin() const noexcept {
    return {getEndEntry()->next, entryAccessor};
  }

  const_iterator cbegin() const noexcept { return begin(); }

  iterator end() noexcept {
    return {(std::uintptr_t)getEndEntry(), entryAccessor};
  }

  const_iterator end() const noexcept {
    return {(std::uintptr_t)getEndEntry(), entryAccessor};
  }

  const_iterator cend() const noexcept { return end(); }

  reverse_iterator rbegin() noexcept { return reverse_iterator{end()}; }

  const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator{end()};
  }

  const_reverse_iterator crbegin() const noexcept { return rbegin(); }

  reverse_iterator rend() noexcept { return reverse_iterator{begin()}; }

  const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator{begin()};
  }

  const_reverse_iterator crend() const noexcept { return rend(); }

  iterator iter(T *t) noexcept { return {t, entryAccessor}; }
  const_iterator iter(const T *t) noexcept { return {t, entryAccessor}; }
  const_iterator citer(const T *t) noexcept { return {t, entryAccessor}; }

  [[nodiscard]] bool empty() const noexcept {
    const tailq_entry *endEntry = getEndEntry();
    return getEntry(endEntry->next) == endEntry;
  }

  auto size() const noexcept;

  constexpr auto max_size() {
    return std::numeric_limits<typename Derived::size_type>::max();
  }

  void clear() noexcept;

  iterator insert(const_iterator, T *) noexcept;

  template <typename InputIt>
  iterator insert(const_iterator, InputIt first, InputIt last) noexcept;

  iterator insert(const_iterator_t pos, std::initializer_list<T *> i) noexcept {
    return insert(pos, std::begin(i), std::end(i));
  }

  iterator erase(const_iterator) noexcept;

  iterator erase(const_iterator first, const_iterator last) noexcept;

  // FIXME: need to accept &&v? How does this work?
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

  void push_back(T *t) noexcept { insert(cend(), t); }

  void pop_back() noexcept { erase(--end()); }

  void push_front(T *t) noexcept { insert(cbegin(), t); }

  void pop_front() noexcept { erase(begin()); }

  template <typename D2>
  void swap(other_list_t<D2> &other)
      noexcept(std::is_nothrow_swappable_v<EntryAccess>) {
    std::swap(entryAccessor, other.entryAccessor);
    swap_lists(other);
  }

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
  void splice(const_iterator pos, other_list_t<D2> &other) noexcept;

  template <typename D2>
  void splice(const_iterator_t pos, other_list_t<D2> &&other) noexcept {
    return splice(pos, other);
  }

  template <typename D2>
  void splice(const_iterator_t pos, other_list_t<D2> &other,
              typename other_list_t<D2>::const_iterator it) noexcept {
    return splice(pos, other, it, other.cend());
  }

  template <typename D2>
  void splice(const_iterator_t pos, other_list_t<D2> &&other,
              typename other_list_t<D2>::const_iterator it) noexcept {
    return splice(pos, other, it);
  }

  template <typename D2>
  void splice(const_iterator pos, other_list_t<D2> &other,
              typename other_list_t<D2>::const_iterator first,
              typename other_list_t<D2>::const_iterator last) noexcept;

  template <typename D2>
  void splice(const_iterator_t pos, other_list_t<D2> &&other,
              typename other_list_t<D2>::const_iterator first,
              typename other_list_t<D2>::const_iterator last) noexcept {
    return splice(pos, other, first, last);
  }

  // FIXME [C++2a] we badly need noexcept(auto) everywhere, if it ever
  // becomes standardized
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
  friend class tailq_base;

  using tailq_link_encoder = link_encoder<tailq_entry, EntryAccess, T>;

  tailq_entry *getEndEntry() noexcept {
    return static_cast<Derived *>(this)->getEndEntry();
  }

  const tailq_entry *getEndEntry() const noexcept {
    return const_cast<tailq_base *>(this)->getEndEntry();
  }

  auto &getSizeRef() noexcept {
    return static_cast<Derived *>(this)->getSizeRef();
  }

  template <typename U>
  tailq_entry *getEntry(U u) const noexcept {
    compressed_invocable_ref<EntryAccess, T &> fn{entryAccessor};
    return tailq_link_encoder::getEntry(fn, u);
  }

  template <typename It>
      requires std::is_same_v<It, iterator_t> ||
               std::is_same_v<It, const_iterator_t>
  static tailq_entry *getEntry(It i) noexcept {
    return tailq_link_encoder::getEntry(i.rEntryAccessor, i.current);
  }

  template <typename QueueIt>
  static void insert_range(const_iterator pos, QueueIt first,
                           QueueIt last) noexcept;

  static void remove_range(const_iterator first, const_iterator last) noexcept;

  template <typename Compare, typename SizeType>
      requires std::is_integral_v<SizeType>
  const_iterator merge_sort(const_iterator f1, const_iterator e2, Compare comp,
                            SizeType n) noexcept;

  [[no_unique_address]] mutable EntryAccess entryAccessor;
};

template <typename T, typename EntryAccess, typename Derived>
class tailq_base<T, EntryAccess, Derived>::iterator {
public:
  using container = tailq_base<T, EntryAccess, Derived>;
  using value_type = container::value_type;
  using reference = container::reference;
  using pointer = container::pointer;
  using difference_type = typename Derived::difference_type;
  using iterator_category = std::bidirectional_iterator_tag;
  using invocable_ref = compressed_invocable_ref<EntryAccess, T &>;

  iterator() noexcept : current{}, rEntryAccessor{} {}
  iterator(const iterator &) = default;
  iterator(iterator &&) = default;

  iterator(T *t) noexcept requires Stateless<EntryAccess>
      : current{tailq_link_encoder::encode(t)}, rEntryAccessor{} {}

  iterator(T *t, EntryAccess &fn) noexcept
      : current{tailq_link_encoder::encode(t)}, rEntryAccessor{fn} {}

  ~iterator() = default;

  iterator &operator=(const iterator &) = default;

  iterator &operator=(iterator &&) = default;

  reference operator*() const noexcept { return *operator->(); }

  pointer operator->() const noexcept {
    return tailq_link_encoder::getValue(this->current);
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

  iterator operator--() noexcept {
    current = container::getEntry(*this)->prev;
    return *this;
  }

  iterator operator--(int) noexcept {
    iterator i{*this};
    this->operator--();
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
  friend class tailq_base;

  friend container::const_iterator;

  iterator(std::uintptr_t e, EntryAccess &fn) noexcept
      : current{e}, rEntryAccessor{fn} {}

  std::uintptr_t current;
  [[no_unique_address]] invocable_ref rEntryAccessor;
};

template <typename T, typename EntryAccess, typename Derived>
class tailq_base<T, EntryAccess, Derived>::const_iterator {
public:
  using container = tailq_base<T, EntryAccess, Derived>;
  using value_type = container::value_type;
  using reference = container::const_reference;
  using pointer = container::const_pointer;
  using difference_type = typename Derived::difference_type;
  using iterator_category = std::bidirectional_iterator_tag;
  using invocable_ref = compressed_invocable_ref<EntryAccess, T &>;

  const_iterator() noexcept : current{}, rEntryAccessor{} {}
  const_iterator(const const_iterator &) = default;
  const_iterator(const_iterator &&) = default;
  const_iterator(const iterator &i) noexcept
      : current{i.current}, rEntryAccessor{i.rEntryAccessor} {}

  const_iterator(const T *t) noexcept requires Stateless<EntryAccess>
      : current{tailq_link_encoder::encode(t)}, rEntryAccessor{} {}

  const_iterator(const T *t, EntryAccess &fn) noexcept
      : current{tailq_link_encoder::encode(t)}, rEntryAccessor{fn} {}

  ~const_iterator() = default;

  const_iterator &operator=(const const_iterator &) = default;

  const_iterator &operator=(const_iterator &&) = default;

  reference operator*() const noexcept { return *operator->(); }

  pointer operator->() const noexcept {
    return tailq_link_encoder::getValue(this->current);
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

  const_iterator operator--() noexcept {
    current = container::getEntry(*this)->prev;
    return *this;
  }

  const_iterator operator--(int) noexcept {
    const_iterator i{*this};
    this->operator--();
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
  friend class tailq_base;

  friend container::iterator;

  const_iterator(std::uintptr_t e, EntryAccess &fn) noexcept
      : current{e}, rEntryAccessor{fn} {}

  std::uintptr_t current;
  [[no_unique_address]] invocable_ref rEntryAccessor;
};

template <SizeMember>
class tailq_fwd_head;

template <typename T, typename EntryAccess, SizeMember SizeType>
class tailq_container
    : public tailq_base<T, EntryAccess,
                        tailq_container<T, EntryAccess, SizeType>> {
public:
  using base_type =
      tailq_base<T, EntryAccess, tailq_container<T, EntryAccess, SizeType>>;

  using size_type = std::conditional_t<std::is_same_v<SizeType, no_size>,
                                       std::size_t, SizeType>;

  using difference_type = std::make_signed_t<size_type>;

  using fwd_head_type = tailq_fwd_head<SizeType>;

  template <typename D>
  using other_list_t = typename tailq_base<
      T, EntryAccess,
      tailq_container<T, EntryAccess, SizeType>>::template other_list_t<D>;

  tailq_container() = delete;

  tailq_container(const tailq_container &) = delete;

  tailq_container(tailq_container &&) = delete;

  tailq_container(tailq_fwd_head<SizeType> &h)
      noexcept(std::is_nothrow_default_constructible_v<EntryAccess>)
      requires std::is_default_constructible_v<EntryAccess>
      : base_type{}, head{h} {}

  template <typename... Ts>
      requires std::is_constructible_v<EntryAccess, Ts...>
  explicit tailq_container(tailq_fwd_head<SizeType> &h, Ts &&... vs)
      noexcept(std::is_nothrow_constructible_v<EntryAccess, Ts...>)
      : base_type{std::forward<Ts>(vs)...}, head{h} {}

  template <typename D>
  tailq_container(tailq_fwd_head<SizeType> &h, other_list_t<D> && other)
      noexcept(std::is_move_constructible_v<EntryAccess>)
      : base_type{std::move(other)}, head{h} {
    this->swap_lists(other);
  }

  template <typename InputIt, typename... Ts>
  tailq_container(tailq_fwd_head<SizeType> &h, InputIt first, InputIt last,
                  Ts &&... vs)
      noexcept(std::is_nothrow_constructible_v<EntryAccess, Ts...>)
      : base_type{std::forward<Ts>(vs)...}, head{h} {
    base_type::assign(first, last);
  }

  template <typename... Ts>
  tailq_container(tailq_fwd_head<SizeType> &h,
                  std::initializer_list<T *> ilist, Ts &&... vs)
      noexcept(std::is_nothrow_constructible_v<EntryAccess, Ts...>)
      : base_type{std::forward<Ts>(vs)...}, head{h} {
    base_type::assign(ilist);
  }

  ~tailq_container() = default;

  tailq_container &operator=(const tailq_container &) = delete;

  tailq_container &operator=(tailq_container &&rhs)
      noexcept(std::is_nothrow_move_assignable_v<EntryAccess>) {
    base_type::operator=(std::move(rhs));
    return *this;
  }

  template <typename D>
  tailq_container &operator=(other_list_t<D> &&rhs)
      noexcept(std::is_nothrow_move_assignable_v<EntryAccess>) {
    base_type::operator=(std::move(rhs));
    return *this;
  }

  tailq_container &operator=(std::initializer_list<T *> ilist) noexcept {
    base_type::operator=(ilist);
    return *this;
  }

private:
  template <typename, typename, typename>
  friend class tailq_base;

  constexpr static bool HasInlineSize = !std::is_same_v<SizeType, no_size>;

  tailq_entry *getEndEntry() noexcept { return head.getEndEntry(); }

  auto &getSizeRef() noexcept { return head.getSizeRef(); }

  tailq_fwd_head<SizeType> &head;
};

template <SizeMember SizeType>
class tailq_fwd_head {
public:
  using size_type = std::conditional_t<std::is_same_v<SizeType, no_size>,
                                       std::size_t, SizeType>;

  using difference_type = std::make_signed_t<size_type>;

  tailq_fwd_head() noexcept : sz{} {
    make_sentinel_entry(std::addressof(endEntry));
  }

  tailq_fwd_head(const tailq_fwd_head &) = delete;

  tailq_fwd_head(tailq_fwd_head &&) = delete;

  ~tailq_fwd_head() = default;

  tailq_fwd_head &operator=(const tailq_fwd_head &) = delete;

  tailq_fwd_head &operator=(tailq_fwd_head &&) = delete;

protected:
  template <typename, typename, typename>
  friend class tailq_base;

  template <typename, typename, typename>
  friend class tailq_container;

  constexpr static bool HasInlineSize = !std::is_same_v<SizeType, no_size>;

  tailq_entry *getEndEntry() noexcept { return &endEntry; }

  auto &getSizeRef() noexcept { return sz; }

  tailq_entry endEntry;
  [[no_unique_address]] SizeType sz;
};

template <typename T, typename EntryAccess, SizeMember SizeType>
class tailq_head
    : public tailq_base<T, EntryAccess, tailq_head<T, EntryAccess, SizeType>>,
      public tailq_fwd_head<SizeType> {
public:
  using base_type =
      tailq_base<T, EntryAccess, tailq_head<T, EntryAccess, SizeType>>;

  template <typename D>
  using other_list_t = typename tailq_base<
      T, EntryAccess,
      tailq_head<T, EntryAccess, SizeType>>::template other_list_t<D>;

  tailq_head() = default;

  tailq_head(const tailq_head &) = delete;

  tailq_head(tailq_head &&other)
      noexcept(std::is_nothrow_move_constructible_v<base_type>)
      : base_type{std::move(other)} {
    this->swap_lists(other);
  }

  template <typename D>
  tailq_head(other_list_t<D> &&other)
      noexcept(std::is_nothrow_move_constructible_v<base_type>)
      : base_type{std::move(other)} {
    this->swap_lists(other);
  }

  template <typename... Ts>
      requires std::is_constructible_v<EntryAccess, Ts...>
  explicit tailq_head(Ts &&... vs)
      noexcept(std::is_nothrow_constructible_v<EntryAccess, Ts...>)
      : base_type{std::forward<Ts>(vs)...} {}

  template <typename InputIt, typename... Ts>
  tailq_head(InputIt first, InputIt last, Ts &&... vs)
      noexcept(std::is_nothrow_constructible_v<EntryAccess, Ts...>)
      : base_type{std::forward<Ts>(vs)...} {
    base_type::assign(first, last);
  }

  template <typename... Ts>
  tailq_head(std::initializer_list<T *> ilist, Ts &&... vs)
      noexcept(std::is_nothrow_constructible_v<EntryAccess, Ts...>)
      : base_type{std::forward<Ts>(vs)...} {
    base_type::assign(ilist);
  }

  ~tailq_head() = default;

  tailq_head &operator=(const tailq_head &) = delete;

  tailq_head &operator=(tailq_head &&rhs)
      noexcept(std::is_nothrow_move_assignable_v<base_type>) {
    base_type::operator=(std::move(rhs));
    return *this;
  }

  template <typename D>
  tailq_head &operator=(other_list_t<D> &&rhs) noexcept {
    base_type::operator=(std::move(rhs));
    return *this;
  }

  tailq_head &operator=(std::initializer_list<T *> ilist) noexcept {
    base_type::operator=(ilist);
    return *this;
  }

private:
  template <typename, typename, typename>
  friend class tailq_base;

  // Pull tailq_fwd_head's getEndEntry into our scope, so that the CRTP
  // polymorphic call in the base class will find it unambiguously.
  using tailq_fwd_head<SizeType>::getEndEntry;
  using tailq_fwd_head<SizeType>::getSizeRef;
};

template <typename T, typename E, typename D>
auto tailq_base<T, E, D>::size() const noexcept {
  if constexpr (D::HasInlineSize)
    return const_cast<tailq_base *>(this)->getSizeRef();
  else {
    const auto s = std::distance(begin(), end());
    return static_cast<typename D::size_type>(s);
  }
}

template <typename T, typename E, typename D>
void tailq_base<T, E, D>::clear() noexcept {
  make_sentinel_entry(getEndEntry());

  if constexpr (D::HasInlineSize)
    getSizeRef() = 0;
}

template <typename T, typename E, typename D>
typename tailq_base<T, E, D>::iterator
tailq_base<T, E, D>::insert(const_iterator pos, T *value) noexcept {
  tailq_entry *const posEntry = getEntry(pos);
  tailq_entry *const prevEntry = getEntry(posEntry->prev);
  tailq_entry *const insertEntry = getEntry(value);

  insertEntry->prev = posEntry->prev;
  insertEntry->next = pos.current;
  const auto encoded = prevEntry->next = posEntry->prev =
      tailq_link_encoder::encode(value);

  if constexpr (D::HasInlineSize)
    ++getSizeRef();

  return {encoded, entryAccessor};
}

template <typename T, typename E, typename D>
template <typename InputIt>
typename tailq_base<T, E, D>::iterator
tailq_base<T, E, D>::insert(const_iterator pos, InputIt first,
                            InputIt last) noexcept {
  if (first == last)
    return {pos.current, entryAccessor};

  const iterator firstInsert = insert(pos, *first++);
  pos = firstInsert;

  while (first != last)
    pos = insert(++pos, *first++);

  return firstInsert;
}

template <typename T, typename E, typename D>
typename tailq_base<T, E, D>::iterator
tailq_base<T, E, D>::erase(const_iterator pos) noexcept {
  tailq_entry *const erasedEntry = getEntry(pos);
  tailq_entry *const nextEntry = getEntry(erasedEntry->next);
  tailq_entry *const prevEntry = getEntry(erasedEntry->prev);

  BDS_ASSERT(erasedEntry != getEndEntry(), "end() iterator passed to erase");

  const auto encodedNext = prevEntry->next = erasedEntry->next;
  nextEntry->prev = erasedEntry->prev;

  if constexpr (D::HasInlineSize)
    --getSizeRef();

  return {encodedNext, entryAccessor};
}

template <typename T, typename E, typename D>
typename tailq_base<T, E, D>::iterator
tailq_base<T, E, D>::erase(const_iterator first, const_iterator last) noexcept {
  if (first == last)
    return {last.current, entryAccessor};

  remove_range(first, std::prev(last));

  if constexpr (D::HasInlineSize) {
    typename D::size_type sz = 0;
    while (first++ != last)
      ++sz;
    getSizeRef() -= sz;
  }

  return {last.current, entryAccessor};
}

template <typename T, typename E, typename D1>
template <typename D2, typename Compare>
void tailq_base<T, E, D1>::merge(other_list_t<D2> &other,
                                 Compare comp) noexcept {
  if (this == &other)
    return;

  auto f1 = cbegin();
  auto e1 = cend();
  auto f2 = other.cbegin();
  auto e2 = other.cend();

  if constexpr (D1::HasInlineSize)
    getSizeRef() += std::size(other);

  if constexpr (D2::HasInlineSize)
    other.getSizeRef() = 0;

  while (f1 != e1 && f2 != e2) {
    if (comp(*f1, *f2)) {
      ++f1;
      continue;
    }

    // f2 < f1, scan the range [f2, mEnd) containing all items smaller than
    // f1 (the "merge range"), which are out of order. Unlike most generic
    // algorithms, remove_range uses an inclusive range rather than a
    // half-open one, so keep track of the previous element.
    auto mPrev = f2;
    auto mEnd = std::next(mPrev);
    while (mEnd != e2 && comp(*mEnd, *f1))
      mPrev = mEnd++;

    // Remove [f2, mPrev] from its old list, and insert it in front of f1.
    D2::remove_range(f2, mPrev);
    insert_range(f1, f2, mPrev);

    ++f1;
    f2 = mEnd;
  }

  // At this point, `other` contains all elements which are greater
  // than *(--e1) so splice them into our tailq, at the end.
  if (f2 != e2) {
    --e2;
    other.remove_range(f2, e2);
    insert_range(e1, f2, e2);
  }
}

template <typename T, typename E, typename D1>
template <typename D2>
void tailq_base<T, E, D1>::splice(const_iterator pos,
                                  other_list_t<D2> &other) noexcept {
  if (other.empty())
    return;

  auto first = std::cbegin(other);
  auto last = --std::cend(other);

  if constexpr (D1::HasInlineSize)
    getSizeRef() += std::size(other);

  if constexpr (D2::HasInlineSize)
    getSizeRef() = 0;

  other.remove_range(first, last);
  insert_range(pos, first, last);
}

template <typename T, typename E, typename D1>
template <typename D2>
void tailq_base<T, E, D1>::splice(
    const_iterator pos, other_list_t<D2> &other,
    typename other_list_t<D2>::const_iterator first,
    typename other_list_t<D2>::const_iterator last) noexcept {
  if (first == last)
    return;

  if constexpr (D1::HasInlineSize || D2::HasInlineSize) {
    const auto n = std::distance(first, last);

    if constexpr (D1::HasInlineSize)
      getSizeRef() += n;

    if constexpr (D2::HasInlineSize)
      other.getSizeRef() -= n;
  }

  --last;
  other.remove_range(first, last);
  insert_range(pos, first, last);
}

template <typename T, typename E, typename D>
template <typename UnaryPredicate>
void tailq_base<T, E, D>::remove_if(UnaryPredicate pred) noexcept {
  const const_iterator e = cend();
  const_iterator i = cbegin();

  while (i != e) {
    if (!pred(*i)) {
      // Not removing *i, advance and continue.
      ++i;
      continue;
    }

    // Removing *i; it is slightly more efficient to scan for a contiguous
    // range and call range erase, than to call single-element erase one
    // at a time.
    const_iterator scanEnd = std::next(i);
    while (scanEnd != e && pred(*scanEnd))
      ++scanEnd;
    i = erase(i, scanEnd);
    if (i != e)
      ++i; // i != e, so i == scanEnd; we know !pred(*i) already; advance i
  }
}

template <typename T, typename E, typename D>
void tailq_base<T, E, D>::reverse() noexcept {
  tailq_entry *const endEntry = getEndEntry();
  tailq_entry *curEntry = endEntry;

  do {
    std::swap(curEntry->next, curEntry->prev);
    curEntry = getEntry(curEntry->next);
  } while (curEntry != endEntry);
}

template <typename T, typename E, typename D>
template <typename BinaryPredicate>
void tailq_base<T, E, D>::unique(BinaryPredicate pred) noexcept {
  const const_iterator e = cend();
  const_iterator scanStart = cbegin();

  while (scanStart != e) {
    // scanStart is potentially the start of a range of unique items; it always
    // remains in the list, but the others will be removed if they exist.
    const_iterator scanEnd = std::next(scanStart);
    while (scanEnd != e && pred(*scanStart, *scanEnd))
      ++scanEnd;

    // If the range has more than than one item in it, erase all but the first.
    if (++scanStart != scanEnd)
      scanStart = erase(scanStart, scanEnd);
  }
}

template <typename T, typename E, typename D>
template <typename Compare>
void tailq_base<T, E, D>::sort(Compare comp) noexcept {
  merge_sort(cbegin(), cend(), comp, std::size(*this));
}

template <typename T, typename E, typename D>
template <typename Compare, typename SizeType>
    requires std::is_integral_v<SizeType>
typename tailq_base<T, E, D>::const_iterator
tailq_base<T, E, D>::merge_sort(const_iterator f1, const_iterator e2,
                                Compare comp, SizeType n) noexcept {
  // In-place merge sort; we cannot reuse our `merge` member function because
  // that merges two different tailq's, leaving the second queue empty. Here,
  // the merge operation is "in place" so the code has a different structure.
  // It is slightly more efficient: it doesn't need to modify the tailq size.
  //
  // This function is called to sort the range [f1, e2). It splits this input
  // range into the subranges [f1, e1) and [f2, e2), where e1 == f2, and each
  // subrange is recursively sorted. The two sorted subranges are then merged.
  //
  // This function returns an iterator pointing to the first element of the
  // sorted range. In general, the first element in the input range [f1, e2)
  // will not be the same prior to sorting vs. after sorting, so the iterator
  // pointing to the new first element of the sorted range must be returned.
  // How the end iterators are maintained is explained below, in a comment
  // above the recursion step.

  // Base cases for recursion: manually sort small lists.
  switch (n) {
  case 0:
  case 1:
    return f1;

  case 2:
    --e2; // Move e2 to the second element so we can compare.
    if (comp(*f1, *e2))
      return f1;
    else {
      // Two element queue in reversed order; swap order of the elements.
      remove_range(e2, e2);
      insert_range(f1, e2, e2);
      return e2;
    }
  }

  // Explicitly form the ranges [f1, e1) and [f2, e2) from [f1, e2) by selecting
  // a pivot element based on the range length.
  SizeType pivot = n / 2;
  const const_iterator e1 = std::next(f1, pivot);

  // Recursively sort the two subranges. Because f2 and e1 point to the same
  // element and all links are maintained, the merge sort of the "right half"
  // finds both the value for new f2 and the new value of the e1 iterator.
  // Because f2 and e1 are always equal, e1 is not modified during the merge
  // algorithm. The end iterator for the right half, e2, is not part of
  // either range so it is also never modified.
  f1 = merge_sort(f1, e1, comp, pivot);
  const_iterator f2 = merge_sort(e1, e2, comp, n - pivot);

  // The iterator `min` will point to the first element in the merged range
  // (the smallest element in both lists), which we need to return.
  const const_iterator min = comp(*f1, *f2) ? f1 : f2;

  // Merge step between the two sorted sublists. Like the `merge` member
  // function, the sorted right half is "merged into" the sorted left half.
  // The invariant we want to maintain is that *f1 < *f2. As long as this
  // is true, f1 is advanced. Whenever it is not true, we scan the maximum
  // subsequence of [f2, e) such that the elements are less than *f1, then
  // unlink that range from its current location and relink it in front of *f1.
  while (f1 != f2 && f2 != e2) {
    if (comp(*f1, *f2)) {
      ++f1;
      continue;
    }

    // This code is mostly the same as in the `merge` member function.
    auto mPrev = f2;
    auto mEnd = std::next(mPrev);
    while (mEnd != e2 && comp(*mEnd, *f1))
      mPrev = mEnd++;

    remove_range(f2, mPrev);
    insert_range(f1, f2, mPrev);

    ++f1;
    f2 = mEnd;
  }

  return min;
}

template <typename T, typename E, typename D1>
template <typename D2>
void tailq_base<T, E, D1>::swap_lists(other_list_t<D2> &other) noexcept {
  tailq_entry *const lhsEndEntry = getEndEntry();
  tailq_entry *const lhsFirstEntry = getEntry(lhsEndEntry->next);
  tailq_entry *const lhsLastEntry = getEntry(lhsEndEntry->prev);

  tailq_entry *const rhsEndEntry = other.getEndEntry();
  tailq_entry *const rhsFirstEntry = other.getEntry(rhsEndEntry->next);
  tailq_entry *const rhsLastEntry = other.getEntry(rhsEndEntry->prev);

  // Fix the linkage at the beginning and end of each list into
  // the end entries.
  lhsFirstEntry->prev = lhsLastEntry->next =
      reinterpret_cast<uintptr_t>(rhsEndEntry);

  rhsFirstEntry->prev = rhsLastEntry->next =
      reinterpret_cast<uintptr_t>(lhsEndEntry);

  // Swap the end entries and size containers.
  std::swap(*lhsEndEntry, *rhsEndEntry);
  std::swap(getSizeRef(), other.getSizeRef());
}

template <typename T, typename E, typename D>
template <typename QueueIt>
void tailq_base<T, E, D>::insert_range(const_iterator pos, QueueIt first,
                                       QueueIt last) noexcept {
  // Inserts the closed range [first, last] before pos.
  tailq_entry *const posEntry = getEntry(pos);
  tailq_entry *const firstEntry = QueueIt::container::getEntry(first);
  tailq_entry *const lastEntry = QueueIt::container::getEntry(last);

  tailq_entry *const beforePosEntry =
      tailq_link_encoder::getEntry(pos.rEntryAccessor, posEntry->prev);

  firstEntry->prev = posEntry->prev;
  beforePosEntry->next = first.current;
  lastEntry->next = pos.current;
  posEntry->prev = last.current;
}

template <typename T, typename E, typename D>
void tailq_base<T, E, D>::remove_range(const_iterator first,
                                       const_iterator last) noexcept {
  // Removes the closed range [first, last].
  tailq_entry *const firstEntry = getEntry(first);
  tailq_entry *const lastEntry = getEntry(last);

  auto &entryAccessor = first.rEntryAccessor;

  tailq_entry *const beforeFirstEntry =
      tailq_link_encoder::getEntry(entryAccessor, firstEntry->prev);

  tailq_entry *const afterLastEntry =
      tailq_link_encoder::getEntry(entryAccessor, lastEntry->next);

  beforeFirstEntry->next = lastEntry->next;
  afterLastEntry->prev = firstEntry->prev;
}

} // End of namespace bds

#endif
