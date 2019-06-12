//==-- csd/tailq.h - tail queue intrusive list implementation ---*- C++ -*-==//
//
//                Cyril Software Data Structures (CSD) Library
//
// This file is distributed under the 2-clause BSD Open Source License. See
// LICENSE.TXT for details.
//==-----------------------------------------------------------------------==//

/**
 * @file
 * @brief Contains an STL-compatible implementation of intrusive tail queues,
 *     inspired by BSD's queue(3) TAILQ_ macros.
 */

#ifndef CSD_TAILQ_H
#define CSD_TAILQ_H

#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <type_traits>

#include <csd/assert.h>
#include <csd/listfwd.h>

namespace csd {

template <typename T>
struct tailq_entry {
  entry_ref_union<tailq_entry, T> next;
  entry_ref_union<tailq_entry, T> prev;
};

template <typename T, TailQEntryAccessor<T> EntryAccess>
struct tailq_entry_ref_traits {
  using entry_ref_type = invocable_tagged_ref<tailq_entry<T>, T>;
  constexpr static auto EntryRefMember =
      &entry_ref_union<tailq_entry<T>, T>::invocableTagged;
};

template <typename T, std::size_t Offset>
struct tailq_entry_ref_traits<T, offset_extractor<tailq_entry<T>, Offset>> {
  using entry_ref_type = offset_entry_ref<tailq_entry<T>>;
  constexpr static auto EntryRefMember =
      &entry_ref_union<tailq_entry<T>, T>::offset;
};

template <typename T, TailQEntryAccessor<T> EntryAccess,
          CompressedSize SizeMember, typename Derived>
class tailq_base;

template <typename T, CompressedSize SizeMember>
class tailq_fwd_head {
public:
  using value_type = T;
  using size_type = std::conditional_t<std::is_same_v<SizeMember, no_size>,
                                       std::size_t, SizeMember>;

  tailq_fwd_head() noexcept : m_sz{} {
    m_endEntry.next.offset = m_endEntry.prev.offset = &m_endEntry;
  }

  tailq_fwd_head(const tailq_fwd_head &) = delete;

  tailq_fwd_head(tailq_fwd_head &&) = delete;

  ~tailq_fwd_head() = default;

  tailq_fwd_head &operator=(const tailq_fwd_head &) = delete;

  tailq_fwd_head &operator=(tailq_fwd_head &&) = delete;

private:
  template <typename T2, TailQEntryAccessor<T2>, CompressedSize, typename>
  friend class tailq_base;

  tailq_entry<T> m_endEntry;
  [[no_unique_address]] SizeMember m_sz;
};

template <typename T, TailQEntryAccessor<T> EntryAccess,
          CompressedSize SizeMember, typename Derived>
class tailq_base {
public:
  using value_type = T;
  using reference = T &;
  using const_reference = const T &;
  using pointer = T *;
  using const_pointer = const T *;
  using size_type = tailq_fwd_head<T, SizeMember>::size_type;
  using difference_type = std::make_signed_t<size_type>;
  using entry_type = tailq_entry<T>;
  using entry_access_type = EntryAccess;
  using size_member_type = SizeMember;

  template <CompressedSize S, typename D>
  using other_list_t = tailq_base<T, EntryAccess, S, D>;

  tailq_base() requires std::is_default_constructible_v<EntryAccess> = default;

  tailq_base(const tailq_base &) = delete;

  tailq_base(tailq_base &&other)
      requires std::is_move_constructible_v<EntryAccess> = default;

  template <CompressedSize S, typename D>
  tailq_base(other_list_t<S, D> &&other)
      noexcept(std::is_nothrow_move_constructible_v<EntryAccess>)
      : m_entryAccessor{std::move(other.m_entryAccessor)} {}

  template <typename... Ts>
      requires std::is_constructible_v<EntryAccess, Ts...>
  explicit tailq_base(Ts &&...vs)
      noexcept(std::is_nothrow_constructible_v<EntryAccess, Ts...>)
      : m_entryAccessor{std::forward<Ts>(vs)...} {}

  ~tailq_base() = default;

  tailq_base &operator=(const tailq_base &) = delete;

  template <CompressedSize S, typename D>
  tailq_base &operator=(other_list_t<S, D> &&other)
      noexcept(std::is_nothrow_move_assignable_v<EntryAccess>)
      requires std::is_move_assignable_v<EntryAccess> {
    m_entryAccessor = std::move(other.m_entryAccessor);
    return *this;
  }

  entry_access_type &get_entry_accessor() noexcept { return m_entryAccessor; }

  const entry_access_type &get_entry_accessor() const noexcept {
    return m_entryAccessor;
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

  using iterator_t = std::type_identity_t<iterator>;
  using const_iterator_t = std::type_identity_t<const_iterator>;

  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  iterator begin() noexcept {
    return {getTailQData().m_endEntry.next.*EntryRefMember, m_entryAccessor};
  }

  const_iterator begin() const noexcept {
    return {getTailQData().m_endEntry.next.*EntryRefMember, m_entryAccessor};
  }

  const_iterator cbegin() const noexcept { return begin(); }

  iterator end() noexcept {
    return {entry_ref_type{&getTailQData().m_endEntry}, m_entryAccessor};
  }

  const_iterator end() const noexcept {
    return const_cast<tailq_base *>(this)->end();
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

  iterator iter(T *t) noexcept { return {t, m_entryAccessor}; }
  const_iterator iter(const T *t) noexcept { return {t, m_entryAccessor}; }
  const_iterator citer(const T *t) noexcept { return {t, m_entryAccessor}; }

  [[nodiscard]] bool empty() const noexcept {
    const entry_type &endEntry = getTailQData().m_endEntry;
    return getEntry(endEntry.next) == &endEntry;
  }

  size_type size() const noexcept;

  constexpr size_type max_size() {
    return std::numeric_limits<size_type>::max();
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

  template <CompressedSize S2, typename D2>
  void swap(other_list_t<S2, D2> &other)
      noexcept(std::is_nothrow_swappable_v<EntryAccess>) {
    std::swap(m_entryAccessor, other.m_entryAccessor);
    swap_lists(other);
  }

  template <CompressedSize S2, typename D2>
  void merge(other_list_t<S2, D2> &other) noexcept {
    return merge(other, std::less<T>{});
  }

  template <CompressedSize S2, typename D2>
  void merge(other_list_t<S2, D2> &&other) noexcept {
    return merge(std::move(other), std::less<T>{});
  }

  template <CompressedSize S2, typename D2, typename Compare>
  void merge(other_list_t<S2, D2> &other, Compare comp) noexcept;

  template <CompressedSize S2, typename D2, typename Compare>
  void merge(other_list_t<S2, D2> &&other, Compare comp) noexcept {
    merge(other, comp);
  }

  template <CompressedSize S2, typename D2>
  void splice(const_iterator pos, other_list_t<S2, D2> &other) noexcept;

  template <CompressedSize S2, typename D2>
  void splice(const_iterator_t pos, other_list_t<S2, D2> &&other) noexcept {
    return splice(pos, other);
  }

  template <CompressedSize S2, typename D2>
  void splice(const_iterator_t pos, other_list_t<S2, D2> &other,
              other_list_t<S2, D2>::const_iterator it) noexcept {
    return splice(pos, other, it, other.cend());
  }

  template <CompressedSize S2, typename D2>
  void splice(const_iterator_t pos, other_list_t<S2, D2> &&other,
              other_list_t<S2, D2>::const_iterator it) noexcept {
    return splice(pos, other, it);
  }

  template <CompressedSize S2, typename D2>
  void splice(const_iterator pos, other_list_t<S2, D2> &other,
              other_list_t<S2, D2>::const_iterator first,
              other_list_t<S2, D2>::const_iterator last) noexcept;

  template <CompressedSize S2, typename D2>
  void splice(const_iterator_t pos, other_list_t<S2, D2> &&other,
              other_list_t<S2, D2>::const_iterator first,
              other_list_t<S2, D2>::const_iterator last) noexcept {
    return splice(pos, other, first, last);
  }

  // FIXME [C++2a] we badly need noexcept(auto) everywhere, if it ever
  // becomes standardized
  size_type remove(const T &value) noexcept {
    return remove_if(std::equal_to<T>{});
  }

  template <typename UnaryPredicate>
  size_type remove_if(UnaryPredicate) noexcept;

  void reverse() noexcept;

  void unique() noexcept { unique(std::equal_to<T>{}); }

  template <typename BinaryPredicate>
  void unique(BinaryPredicate) noexcept;

  void sort() noexcept { sort(std::less<T>{}); }

  template <typename Compare>
  void sort(Compare comp) noexcept {
    merge_sort(cbegin(), cend(), comp, std::size(*this));
  }

protected:
  template <CompressedSize S2, typename D2>
  void swap_lists(other_list_t<S2, D2> &other) noexcept;

private:
  template <typename T2, TailQEntryAccessor<T2>, CompressedSize, typename>
  friend class tailq_base;

  constexpr static bool HasInlineSize = !std::is_same_v<SizeMember, no_size>;

  constexpr static auto EntryRefMember =
      tailq_entry_ref_traits<T, EntryAccess>::EntryRefMember;

  using entry_ref_type = tailq_entry_ref_traits<T, EntryAccess>::entry_ref_type;
  using entry_ref_codec = detail::entry_ref_codec<entry_ref_type, EntryAccess>;

  tailq_fwd_head<T, SizeMember> &getTailQData() noexcept {
    return static_cast<Derived *>(this)->getTailQData();
  }

  const tailq_fwd_head<T, SizeMember> &getTailQData() const noexcept {
    return static_cast<const Derived *>(this)->getTailQData();
  }

  entry_type *getEntry(entry_ref_type ref) const noexcept {
    return entry_ref_codec::get_entry(m_entryAccessor, ref);
  }

  entry_type *getEntry(entry_ref_union<entry_type, T> ref) const noexcept {
    return getEntry(ref.*EntryRefMember);
  }

  static entry_type *getEntry(iterator_t i) noexcept {
    return entry_ref_codec::get_entry(i.m_rEntryAccessor, i.m_current);
  }

  static entry_type *getEntry(const_iterator_t i) noexcept {
    return entry_ref_codec::get_entry(i.m_rEntryAccessor, i.m_current);
  }

  template <typename QueueIt>
  static void insert_range(const_iterator pos, QueueIt first,
                           QueueIt last) noexcept;

  static void remove_range(const_iterator first, const_iterator last) noexcept;

  template <typename Compare, typename SizeType>
      requires std::is_integral_v<SizeType>
  const_iterator merge_sort(const_iterator f1, const_iterator e2, Compare comp,
                            SizeType n) noexcept;

  [[no_unique_address]] mutable EntryAccess m_entryAccessor;
};

template <typename T, TailQEntryAccessor<T> EntryAccess,
          CompressedSize SizeMember, typename Derived>
class tailq_base<T, EntryAccess, SizeMember, Derived>::iterator {
public:
  using value_type = tailq_base::value_type;
  using reference = tailq_base::reference;
  using pointer = tailq_base::pointer;
  using difference_type = tailq_base::difference_type;
  using iterator_category = std::bidirectional_iterator_tag;
  using invocable_ref = compressed_invocable_ref<EntryAccess, T &>;

  iterator() noexcept : m_current{}, m_rEntryAccessor{} {}
  iterator(const iterator &) = default;
  iterator(iterator &&) = default;

  iterator(T *t) noexcept requires Stateless<EntryAccess>
      : m_current{entry_ref_codec::create_entry_ref(t)},
        m_rEntryAccessor{} {}

  iterator(T *t, EntryAccess &fn) noexcept
      : m_current{entry_ref_codec::create_entry_ref(t)},
        m_rEntryAccessor{fn} {}

  ~iterator() = default;

  iterator &operator=(const iterator &) = default;

  iterator &operator=(iterator &&) = default;

  reference operator*() const noexcept { return *operator->(); }

  pointer operator->() const noexcept {
    return entry_ref_codec::get_value(this->m_current);
  }

  iterator &operator++() noexcept {
    m_current = getCurrentEntry()->next.*EntryRefMember;
    return *this;
  }

  iterator operator++(int) noexcept {
    iterator i{*this};
    this->operator++();
    return i;
  }

  iterator operator--() noexcept {
    m_current = getCurrentEntry()->prev.*EntryRefMember;
    return *this;
  }

  iterator operator--(int) noexcept {
    iterator i{*this};
    this->operator--();
    return i;
  }

  bool operator==(const iterator &rhs) const noexcept {
    return m_current == rhs.m_current;
  }

  bool operator==(const const_iterator &rhs) const noexcept {
    return m_current == rhs.m_current;
  }

  bool operator!=(const iterator &rhs) const noexcept {
    return !operator==(rhs);
  }

  bool operator!=(const const_iterator &rhs) const noexcept {
    return !operator==(rhs);
  }

private:
  template <typename T2, TailQEntryAccessor<T2>, CompressedSize, typename>
  friend class tailq_base;

  friend tailq_base::const_iterator;

  using container = tailq_base;

  const entry_type *getCurrentEntry() const noexcept {
    return tailq_base::getEntry(*this);
  }

  iterator(typename tailq_base::entry_ref_type ref, EntryAccess &fn) noexcept
      : m_current{ref}, m_rEntryAccessor{fn} {}

  tailq_base::entry_ref_type m_current;
  [[no_unique_address]] invocable_ref m_rEntryAccessor;
};

template <typename T, TailQEntryAccessor<T> EntryAccess,
          CompressedSize SizeMember, typename Derived>
class tailq_base<T, EntryAccess, SizeMember, Derived>::const_iterator {
public:
  using value_type = tailq_base::value_type;
  using reference = tailq_base::const_reference;
  using pointer = tailq_base::const_pointer;
  using difference_type = tailq_base::difference_type;
  using iterator_category = std::bidirectional_iterator_tag;
  using invocable_ref = compressed_invocable_ref<EntryAccess, T &>;

  const_iterator() noexcept : m_current{}, m_rEntryAccessor{} {}
  const_iterator(const const_iterator &) = default;
  const_iterator(const_iterator &&) = default;
  const_iterator(const iterator &i) noexcept
      : m_current{i.m_current}, m_rEntryAccessor{i.m_rEntryAccessor} {}

  const_iterator(const T *t) noexcept requires Stateless<EntryAccess>
      : m_current{entry_ref_codec::create_entry_ref(t)},
        m_rEntryAccessor{} {}

  const_iterator(const T *t, EntryAccess &fn) noexcept
      : m_current{entry_ref_codec::create_entry_ref(t)},
        m_rEntryAccessor{fn} {}

  ~const_iterator() = default;

  const_iterator &operator=(const const_iterator &) = default;

  const_iterator &operator=(const_iterator &&) = default;

  reference operator*() const noexcept { return *operator->(); }

  pointer operator->() const noexcept {
    return entry_ref_codec::get_value(this->m_current);
  }

  const_iterator &operator++() noexcept {
    m_current = getCurrentEntry()->next.*EntryRefMember;
    return *this;
  }

  const_iterator operator++(int) noexcept {
    const_iterator i{*this};
    this->operator++();
    return i;
  }

  const_iterator operator--() noexcept {
    m_current = getCurrentEntry()->prev.*EntryRefMember;
    return *this;
  }

  const_iterator operator--(int) noexcept {
    const_iterator i{*this};
    this->operator--();
    return i;
  }

  bool operator==(const iterator &rhs) const noexcept {
    return m_current == rhs.m_current;
  }

  bool operator==(const const_iterator &rhs) const noexcept {
    return m_current == rhs.m_current;
  }

  bool operator!=(const iterator &rhs) const noexcept {
    return !operator==(rhs);
  }

  bool operator!=(const const_iterator &rhs) const noexcept {
    return !operator==(rhs);
  }

private:
  template <typename T2, TailQEntryAccessor<T2>, CompressedSize, typename>
  friend class tailq_base;

  friend tailq_base::iterator;

  using container = tailq_base;

  const entry_type *getCurrentEntry() const noexcept {
    return tailq_base::getEntry(*this);
  }

  const_iterator(typename tailq_base::entry_ref_type ref, EntryAccess &fn) noexcept
      : m_current{ref}, m_rEntryAccessor{fn} {}

  tailq_base::entry_ref_type m_current;
  [[no_unique_address]] invocable_ref m_rEntryAccessor;
};

template <typename T, CompressedSize SizeMember,
          TailQEntryAccessor<T> EntryAccess>
class tailq_proxy<tailq_fwd_head<T, SizeMember>, EntryAccess>
    : public tailq_base<T, entry_access_helper_t<T, EntryAccess>, SizeMember,
                        tailq_proxy<tailq_fwd_head<T, SizeMember>, EntryAccess>> {
  using entry_access_type = entry_access_helper_t<T, EntryAccess>;
  using base_type = tailq_base<T, entry_access_type, SizeMember, tailq_proxy>;

public:
  using fwd_head_type = tailq_fwd_head<T, SizeMember>;

  // FIXME [C++20] P0634 not implemented correctly in clang? This error occurs
  // in many other places.
  template <CompressedSize S, typename D>
  using other_list_t = typename tailq_base<
      T, entry_access_type, SizeMember, tailq_proxy>::template other_list_t<S, D>;

  tailq_proxy() = delete;

  tailq_proxy(const tailq_proxy &) = delete;

  tailq_proxy(tailq_proxy &&) = delete;

  tailq_proxy(fwd_head_type &h)
      noexcept(std::is_nothrow_default_constructible_v<entry_access_type>)
      requires std::is_default_constructible_v<entry_access_type>
      : base_type{}, m_head{h} {}

  template <typename... Ts>
      requires std::is_constructible_v<entry_access_type, Ts...>
  explicit tailq_proxy(fwd_head_type &h, Ts &&...vs)
      noexcept(std::is_nothrow_constructible_v<entry_access_type, Ts...>)
      : base_type{std::forward<Ts>(vs)...}, m_head{h} {}

  template <CompressedSize S, typename D>
  tailq_proxy(fwd_head_type &h, other_list_t<S, D> &&other)
      noexcept(std::is_move_constructible_v<entry_access_type>)
      : base_type{std::move(other)}, m_head{h} {
    this->swap_lists(other);
  }

  template <typename InputIt, typename... Ts>
  tailq_proxy(fwd_head_type &h, InputIt first, InputIt last, Ts &&...vs)
      noexcept(std::is_nothrow_constructible_v<entry_access_type, Ts...>)
      : base_type{std::forward<Ts>(vs)...}, m_head{h} {
    base_type::assign(first, last);
  }

  template <typename... Ts>
  tailq_proxy(fwd_head_type &h, std::initializer_list<T *> ilist, Ts &&...vs)
      noexcept(std::is_nothrow_constructible_v<entry_access_type, Ts...>)
      : base_type{std::forward<Ts>(vs)...}, m_head{h} {
    base_type::assign(ilist);
  }

  ~tailq_proxy() = default;

  tailq_proxy &operator=(const tailq_proxy &) = delete;

  tailq_proxy &operator=(tailq_proxy &&rhs)
      noexcept(std::is_nothrow_move_assignable_v<entry_access_type>) {
    base_type::operator=(std::move(rhs));
    base_type::clear();
    base_type::swap_lists(rhs);
    return *this;
  }

  // FIXME: what to do with this?
  template <CompressedSize S, typename D>
  tailq_proxy &operator=(other_list_t<S, D> &&rhs)
      noexcept(std::is_nothrow_move_assignable_v<entry_access_type>) {
    base_type::operator=(std::move(rhs));
    base_type::clear();
    base_type::swap_lists(rhs);
    return *this;
  }

  tailq_proxy &operator=(std::initializer_list<T *> ilist) noexcept {
    base_type::assign(ilist);
    return *this;
  }

private:
  template <typename T2, TailQEntryAccessor<T2>, CompressedSize, typename>
  friend class tailq_base;

  fwd_head_type &getTailQData() noexcept { return m_head; }

  const fwd_head_type &getTailQData() const noexcept { return m_head; }

  fwd_head_type &m_head;
};

template <typename T, TailQEntryAccessor<T> EntryAccess,
          CompressedSize SizeMember>
class tailq_head
    : private tailq_fwd_head<T, SizeMember>,
      public tailq_base<T, entry_access_helper_t<T, EntryAccess>, SizeMember,
                        tailq_head<T, EntryAccess, SizeMember>> {
  using fwd_head_type = tailq_fwd_head<T, SizeMember>;
  using entry_access_type = entry_access_helper_t<T, EntryAccess>;
  using base_type = tailq_base<T, entry_access_type, SizeMember, tailq_head>;

public:
  using typename fwd_head_type::value_type;
  using typename fwd_head_type::size_type;

  template <CompressedSize S, typename D>
  using other_list_t = typename tailq_base<
      T, entry_access_type, SizeMember, tailq_head>::template other_list_t<S, D>;

  tailq_head() = default;

  tailq_head(const tailq_head &) = delete;

  tailq_head(tailq_head &&other)
      noexcept(std::is_nothrow_move_constructible_v<base_type>)
      : base_type{std::move(other)} {
    this->swap_lists(other);
  }

  template <CompressedSize S, typename D>
  tailq_head(other_list_t<S, D> &&other)
      noexcept(std::is_nothrow_move_constructible_v<base_type>)
      : base_type{std::move(other)} {
    this->swap_lists(other);
  }

  template <typename... Ts>
      requires std::is_constructible_v<entry_access_type, Ts...>
  explicit tailq_head(Ts &&...vs)
      noexcept(std::is_nothrow_constructible_v<entry_access_type, Ts...>)
      : base_type{std::forward<Ts>(vs)...} {}

  template <typename InputIt, typename... Ts>
  tailq_head(InputIt first, InputIt last, Ts &&...vs)
      noexcept(std::is_nothrow_constructible_v<entry_access_type, Ts...>)
      : base_type{std::forward<Ts>(vs)...} {
    base_type::assign(first, last);
  }

  template <typename... Ts>
  tailq_head(std::initializer_list<T *> ilist, Ts &&...vs)
      noexcept(std::is_nothrow_constructible_v<entry_access_type, Ts...>)
      : base_type{std::forward<Ts>(vs)...} {
    base_type::assign(ilist);
  }

  ~tailq_head() = default;

  tailq_head &operator=(const tailq_head &) = delete;

  tailq_head &operator=(tailq_head &&rhs)
      noexcept(std::is_nothrow_move_assignable_v<base_type>) {
    base_type::operator=(std::move(rhs));
    base_type::clear();
    base_type::swap_lists(rhs);
    return *this;
  }

  template <CompressedSize S, typename D>
  tailq_head &operator=(other_list_t<S, D> &&rhs) noexcept {
    base_type::operator=(std::move(rhs));
    base_type::clear();
    base_type::swap_lists(rhs);
    return *this;
  }

  tailq_head &operator=(std::initializer_list<T *> ilist) noexcept {
    base_type::assign(ilist);
    return *this;
  }

private:
  template <typename T2, TailQEntryAccessor<T2>, CompressedSize, typename>
  friend class tailq_base;

  fwd_head_type &getTailQData() noexcept { return *this; }

  const fwd_head_type &getTailQData() const noexcept { return *this; }
};

template <typename T, TailQEntryAccessor<T> E, CompressedSize S, typename D>
tailq_base<T, E, S, D>::size_type
tailq_base<T, E, S, D>::size() const noexcept {
  if constexpr (HasInlineSize)
    return getTailQData().m_sz;
  else
    return static_cast<size_type>(std::distance(begin(), end()));
}

template <typename T, TailQEntryAccessor<T> E, CompressedSize S, typename D>
void tailq_base<T, E, S, D>::clear() noexcept {
  auto &endEntry = getTailQData().m_endEntry;
  endEntry.next.*EntryRefMember = &endEntry;
  endEntry.prev.*EntryRefMember = &endEntry;

  if constexpr (HasInlineSize)
    getTailQData().m_sz = 0;
}

template <typename T, TailQEntryAccessor<T> E, CompressedSize S, typename D>
tailq_base<T, E, S, D>::iterator
tailq_base<T, E, S, D>::insert(const_iterator pos, T *value) noexcept {
  entry_type *const posEntry = getEntry(pos);
  entry_type *const prevEntry = getEntry(posEntry->prev);

  const entry_ref_type valueRef =
      entry_ref_codec::create_entry_ref(value);
  entry_type *const insertEntry = getEntry(valueRef);

  insertEntry->prev = posEntry->prev;
  insertEntry->next = pos.m_current;
  prevEntry->next = posEntry->prev = valueRef;

  if constexpr (HasInlineSize)
    ++getTailQData().m_sz;

  return {valueRef, m_entryAccessor};
}

template <typename T, TailQEntryAccessor<T> E, CompressedSize S, typename D>
template <typename InputIt>
tailq_base<T, E, S, D>::iterator
tailq_base<T, E, S, D>::insert(const_iterator pos, InputIt first,
                               InputIt last) noexcept {
  if (first == last)
    return {pos.m_current, m_entryAccessor};

  const iterator firstInsert = insert(pos, *first++);
  pos = firstInsert;

  while (first != last)
    pos = insert(++pos, *first++);

  return firstInsert;
}

template <typename T, TailQEntryAccessor<T> E, CompressedSize S, typename D>
tailq_base<T, E, S, D>::iterator
tailq_base<T, E, S, D>::erase(const_iterator pos) noexcept {
  entry_type *const erasedEntry = getEntry(pos);
  entry_type *const nextEntry = getEntry(erasedEntry->next);
  entry_type *const prevEntry = getEntry(erasedEntry->prev);

  CSD_ASSERT(erasedEntry != &getTailQData().m_endEntry,
             "end() iterator passed to erase");

  prevEntry->next = erasedEntry->next;
  nextEntry->prev = erasedEntry->prev;

  if constexpr (HasInlineSize)
    --getTailQData().m_sz;

  return {erasedEntry->next.*EntryRefMember, m_entryAccessor};
}

template <typename T, TailQEntryAccessor<T> E, CompressedSize S, typename D>
tailq_base<T, E, S, D>::iterator
tailq_base<T, E, S, D>::erase(const_iterator first, const_iterator last) noexcept {
  if (first == last)
    return {last.m_current, m_entryAccessor};

  remove_range(first, std::prev(last));

  if constexpr (HasInlineSize)
    getTailQData().m_sz -= static_cast<size_type>(std::distance(first, last));

  return {last.m_current, m_entryAccessor};
}

template <typename T, TailQEntryAccessor<T> E, CompressedSize S1, typename D1>
template <CompressedSize S2, typename D2, typename Compare>
void tailq_base<T, E, S1, D1>::merge(other_list_t<S2, D2> &other,
                                     Compare comp) noexcept {
  if (this == &other)
    return;

  auto f1 = cbegin();
  auto e1 = cend();
  auto f2 = other.cbegin();
  auto e2 = other.cend();

  if constexpr (HasInlineSize)
    getTailQData().m_sz += std::size(other);

  if constexpr (other_list_t<S2, D2>::HasInlineSize)
    other.getTailQData().m_sz = 0;

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

template <typename T, TailQEntryAccessor<T> E, CompressedSize S1, typename D1>
template <CompressedSize S2, typename D2>
void tailq_base<T, E, S1, D1>::splice(const_iterator pos,
                                      other_list_t<S2, D2> &other) noexcept {
  if (other.empty())
    return;

  auto first = std::cbegin(other);
  auto last = --std::cend(other);

  if constexpr (HasInlineSize)
    getTailQData().m_sz += std::size(other);

  if constexpr (other_list_t<S2, D2>::HasInlineSize)
    getTailQData().m_sz = 0;

  other.remove_range(first, last);
  insert_range(pos, first, last);
}

template <typename T, TailQEntryAccessor<T> E, CompressedSize S1, typename D1>
template <CompressedSize S2, typename D2>
void tailq_base<T, E, S1, D1>::splice(
    const_iterator pos, other_list_t<S2, D2> &other,
    typename other_list_t<S2, D2>::const_iterator first,
    typename other_list_t<S2, D2>::const_iterator last) noexcept {
  if (first == last)
    return;

  if constexpr (HasInlineSize || other_list_t<S2, D2>::HasInlineSize) {
    const auto n = std::distance(first, last);

    if constexpr (HasInlineSize)
      getTailQData().m_sz += n;

    if constexpr (other_list_t<S2, D2>::HasInlineSize)
      other.getTailQData().m_sz -= n;
  }

  --last;
  other.remove_range(first, last);
  insert_range(pos, first, last);
}

template <typename T, TailQEntryAccessor<T> E, CompressedSize S, typename D>
template <typename UnaryPredicate>
tailq_base<T, E, S, D>::size_type
tailq_base<T, E, S, D>::remove_if(UnaryPredicate pred) noexcept {
  size_type nRemoved = 0;
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
    // at a time. Otherwise we're patching list linkage for elements that
    // will eventually be removed anyway.
    const_iterator scanEnd = std::next(i);
    ++nRemoved;

    while (scanEnd != e && pred(*scanEnd)) {
      ++scanEnd;
      ++nRemoved;
    }

    i = erase(i, scanEnd);
    if (i != e)
      ++i; // i != e, so i == scanEnd; we know !pred(*i) already; advance i
  }

  return nRemoved;
}

template <typename T, TailQEntryAccessor<T> E, CompressedSize S, typename D>
void tailq_base<T, E, S, D>::reverse() noexcept {
  entry_type *const endEntry = &getTailQData().m_endEntry;
  entry_type *curEntry = endEntry;

  do {
    std::swap(curEntry->next, curEntry->prev);
    curEntry = getEntry(curEntry->next);
  } while (curEntry != endEntry);
}

template <typename T, TailQEntryAccessor<T> E, CompressedSize S, typename D>
template <typename BinaryPredicate>
void tailq_base<T, E, S, D>::unique(BinaryPredicate pred) noexcept {
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

template <typename T, TailQEntryAccessor<T> E, CompressedSize S, typename D>
template <typename Compare, typename SizeType>
    requires std::is_integral_v<SizeType>
tailq_base<T, E, S, D>::const_iterator
tailq_base<T, E, S, D>::merge_sort(const_iterator f1, const_iterator e2,
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

template <typename T, TailQEntryAccessor<T> E, CompressedSize S1, typename D1>
template <CompressedSize S2, typename D2>
void tailq_base<T, E, S1, D1>::swap_lists(other_list_t<S2, D2> &other) noexcept {
  entry_type *const lhsEndEntry = &getTailQData().m_endEntry;
  entry_type *const lhsFirstEntry = getEntry(lhsEndEntry->next);
  entry_type *const lhsLastEntry = getEntry(lhsEndEntry->prev);

  entry_type *const rhsEndEntry = &other.getTailQData().m_endEntry;
  entry_type *const rhsFirstEntry = other.getEntry(rhsEndEntry->next);
  entry_type *const rhsLastEntry = other.getEntry(rhsEndEntry->prev);

  // Fix the linkage at the beginning and end of each list into
  // the end entries.
  lhsFirstEntry->prev = lhsLastEntry->next = entry_ref_type{rhsEndEntry};
  rhsFirstEntry->prev = rhsLastEntry->next = entry_ref_type{lhsEndEntry};

  // Swap the end entries and size containers.
  std::swap(*lhsEndEntry, *rhsEndEntry);
  std::swap(getTailQData().m_sz, other.getTailQData().m_sz);
}

template <typename T, TailQEntryAccessor<T> E, CompressedSize S, typename D>
template <typename QueueIt>
void tailq_base<T, E, S, D>::insert_range(const_iterator pos, QueueIt first,
                                          QueueIt last) noexcept {
  // Inserts the closed range [first, last] before pos.
  entry_type *const posEntry = getEntry(pos);
  entry_type *const firstEntry = QueueIt::container::getEntry(first);
  entry_type *const lastEntry = QueueIt::container::getEntry(last);

  entry_type *const beforePosEntry =
      entry_ref_codec::get_entry(pos.m_rEntryAccessor,
                                 posEntry->prev.*EntryRefMember);

  firstEntry->prev = posEntry->prev;
  beforePosEntry->next = first.m_current;
  lastEntry->next = pos.m_current;
  posEntry->prev = last.m_current;
}

template <typename T, TailQEntryAccessor<T> E, CompressedSize S, typename D>
void tailq_base<T, E, S, D>::remove_range(const_iterator first,
                                          const_iterator last) noexcept {
  // Removes the closed range [first, last].
  entry_type *const firstEntry = getEntry(first);
  entry_type *const lastEntry = getEntry(last);

  entry_type *const beforeFirstEntry =
      entry_ref_codec::get_entry(first.m_rEntryAccessor,
                                 firstEntry->prev.*EntryRefMember);

  entry_type *const afterLastEntry =
      entry_ref_codec::get_entry(last.m_rEntryAccessor,
                                 lastEntry->next.*EntryRefMember);

  beforeFirstEntry->next = lastEntry->next;
  afterLastEntry->prev = firstEntry->prev;
}

} // End of namespace csd

#endif
