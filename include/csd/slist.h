//==-- csd/slist.h - singly-linked tail queue implementation ----*- C++ -*-==//
//
//                Cyril Software Data Structures (CSD) Library
//
// This file is distributed under the 2-clause BSD Open Source License. See
// LICENSE.TXT for details.
//==-----------------------------------------------------------------------==//

/**
 * @file
 * @brief Contains an STL-compatible implementation of singly-linked intrusive
 *     lists, inspired by BSD's queue(3) SLIST_ macros.
 */

#ifndef CSD_SLIST_H
#define CSD_SLIST_H

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <utility>

#include <csd/assert.h>
#include <csd/listfwd.h>

namespace csd {

template <typename T>
struct slist_entry {
  slist_entry() noexcept { next.offset = nullptr; }

  entry_ref_union<slist_entry, T> next;
};

template <typename T, SListEntryAccessor<T> EntryAccess>
struct slist_entry_ref_traits {
  using entry_ref_type = invocable_tagged_ref<slist_entry<T>, T>;
  constexpr static auto EntryRefMember =
      &entry_ref_union<slist_entry<T>, T>::invocableTagged;
};

template <typename T, std::size_t Offset>
struct slist_entry_ref_traits<T, offset_extractor<slist_entry<T>, Offset>> {
  using entry_ref_type = offset_entry_ref<slist_entry<T>>;
  constexpr static auto EntryRefMember =
      &entry_ref_union<slist_entry<T>, T>::offset;
};

template <typename T, SListEntryAccessor<T> EntryAccess,
          CompressedSize SizeMember, typename Derived>
class slist_base;

template <typename T, CompressedSize SizeMember>
class slist_fwd_head {
public:
  using value_type = T;
  using size_type = std::conditional_t<std::is_same_v<SizeMember, no_size>,
                                       std::size_t, SizeMember>;

  slist_fwd_head() noexcept : m_headEntry{}, m_sz{} {}

  slist_fwd_head(const slist_fwd_head &) = delete;

  slist_fwd_head(slist_fwd_head &&other) noexcept : slist_fwd_head{} {
    std::swap(m_headEntry, other.m_headEntry);
    std::swap(m_sz, other.m_sz);
  }

  ~slist_fwd_head() = default;

  slist_fwd_head &operator=(const slist_fwd_head &) = delete;

  slist_fwd_head &operator=(slist_fwd_head &&other) noexcept {
    return *new(this) slist_fwd_head{std::move(other)};
  }

private:
  template <typename, CompressedSize>
  friend class slist_fwd_head;

  template <typename T2, SListEntryAccessor<T2>, CompressedSize, typename>
  friend class slist_base;

  // FIXME: the extra spacing around `typename T2::value_value` is due to a
  // a bug in the gcc9 concepts TS support (not parsing >> in templates
  // correctly) and can be removed later. This problem exists in a few other
  // places, search for "> >" to fix everywhere.
  template <typename T2, SListEntryAccessor< typename T2::value_type> >
  friend class slist_proxy;

  template <typename T2, SListEntryAccessor<T2>, CompressedSize>
  friend class slist_head;

  template <CompressedSize S2>
  slist_fwd_head(slist_fwd_head<T, S2> &&other, std::size_t size) noexcept {
    move_from(std::move(other), size);
  }

  template <CompressedSize S2>
  void move_from(slist_fwd_head<T, S2> &&other, std::size_t size) noexcept {
    std::swap(*new(&m_headEntry) slist_entry<T>{}, other.m_headEntry);

    if constexpr (std::is_integral_v<S2>) {
      if constexpr (std::is_integral_v<SizeMember>)
	m_sz = static_cast<size_type>(other.m_sz);
      other.m_sz = 0;
    }
    else if constexpr (std::is_integral_v<SizeMember>) {
      // Our size member is integral but the other size member is empty; in
      // this case, the caller must pass the pre-computed list size to us in
      // the `size` parameter.
      m_sz = static_cast<size_type>(size);
    }
  }

  slist_entry<T> m_headEntry;
  [[no_unique_address]] SizeMember m_sz;
};

template <typename T, SListEntryAccessor<T> EntryAccess,
          CompressedSize SizeMember, typename Derived>
class slist_base {
public:
  using value_type = T;
  using reference = T &;
  using const_reference = const T &;
  using pointer = T *;
  using const_pointer = const T *;
  using size_type = slist_fwd_head<SizeMember>::size_type;
  using difference_type = std::make_signed_t<size_type>;
  using entry_type = slist_entry<T>;
  using entry_access_type = EntryAccess;
  using size_member_type = SizeMember;

  template <CompressedSize S, typename D>
  using other_list_t = slist_base<T, EntryAccess, S, D>;

  slist_base() requires std::is_default_constructible_v<EntryAccess> = default;

  slist_base(const slist_base &) = delete;

  slist_base(slist_base &&other)
      requires std::is_move_constructible_v<EntryAccess> = default;

  template <CompressedSize S, typename D>
  slist_base(other_list_t<S, D> &&other)
      noexcept(std::is_nothrow_move_constructible_v<EntryAccess>)
      : m_entryAccessor{std::move(other.m_entryAccessor)} {}

  template <typename... Ts>
      requires std::is_constructible_v<EntryAccess, Ts...>
  slist_base(Ts &&...vs)
      noexcept(std::is_nothrow_constructible_v<EntryAccess, Ts...>)
      : m_entryAccessor{std::forward<Ts>(vs)...} {}

  ~slist_base() = default;

  slist_base &operator=(const slist_base &) = delete;

  template <CompressedSize S, typename D>
  slist_base &operator=(other_list_t<S, D> &&other)
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
    insert_after(cbefore_begin(), first, last);
  }

  void assign(std::initializer_list<T *> ilist) noexcept {
    assign(std::begin(ilist), std::end(ilist));
  }

  reference front() noexcept { return *begin(); }

  const_reference front() const noexcept { return *begin(); }

  class iterator;
  class const_iterator;

  using iterator_t = std::type_identity_t<iterator>;
  using const_iterator_t = std::type_identity_t<const_iterator>;

  iterator before_begin() noexcept {
    return {entry_ref_type{&getSListData().m_headEntry}, m_entryAccessor};
  }

  const_iterator before_begin() const noexcept {
    return const_cast<slist_base *>(this)->before_begin();
  }

  const_iterator cbefore_begin() const noexcept { return before_begin(); }

  iterator begin() noexcept { return ++before_begin(); }
  const_iterator begin() const noexcept { return ++before_begin(); }
  const_iterator cbegin() const noexcept { return begin(); }

  iterator end() noexcept { return {nullptr, m_entryAccessor}; }

  const_iterator end() const noexcept { return {nullptr, m_entryAccessor}; }

  const_iterator cend() const noexcept { return end(); }

  iterator iter(T *t) noexcept { return {t, m_entryAccessor}; }
  const_iterator iter(const T *t) noexcept { return {t, m_entryAccessor}; }
  const_iterator citer(const T *t) noexcept { return {t, m_entryAccessor}; }

  [[nodiscard]] bool empty() const noexcept {
    return !(getSListData().m_headEntry.next.*EntryRefMember);
  }

  size_type size() const noexcept;

  constexpr size_type max_size() {
    return std::numeric_limits<size_type>::max();
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

  template <CompressedSize S2, typename D2>
  void swap(other_list_t<S2, D2> &other) noexcept {
    std::swap(getSListData(), other.getSListData());
    std::swap(m_entryAccessor, other.m_entryAccessor);
  }

  [[nodiscard]] iterator find_predecessor(const_iterator_t pos) const noexcept {
    return find_predecessor(before_begin(), end(), pos);
  }

  [[nodiscard]] iterator find_predecessor(const_iterator first,
                                          const_iterator last,
                                          const_iterator pos) const noexcept;

  // FIXME: noexcept based on UnaryPredicate
  template <typename UnaryPredicate>
  [[nodiscard]] std::pair<iterator, bool>
      find_predecessor_if(UnaryPredicate p) const noexcept {
    return find_predecessor_if(before_begin(), end(), p);
  }

  template <typename UnaryPredicate>
  [[nodisard]] std::pair<iterator, bool>
      find_predecessor_if(const_iterator first, const_iterator last,
                          UnaryPredicate) const noexcept;

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
  void splice_after(const_iterator pos, other_list_t<S2, D2> &other) noexcept;

  template <CompressedSize S2, typename D2>
  void splice_after(const_iterator_t pos, other_list_t<S2, D2> &&other) noexcept {
    return splice_after(pos, other);
  }

  template <CompressedSize S2, typename D2>
  void splice_after(const_iterator_t pos, other_list_t<S2, D2> &other,
                    other_list_t<S2, D2>::const_iterator it) noexcept {
    return splice_after(pos, other, it, other.cend());
  }

  template <CompressedSize S2, typename D2>
  void splice_after(const_iterator_t pos, other_list_t<S2, D2> &&other,
                    other_list_t<S2, D2>::const_iterator it) noexcept {
    return splice_after(pos, other, it);
  }

  template <CompressedSize S2, typename D2>
  void splice_after(const_iterator pos, other_list_t<S2, D2> &other,
                    other_list_t<S2, D2>::const_iterator first,
                    other_list_t<S2, D2>::const_iterator last) noexcept;

  template <CompressedSize S2, typename D2>
  void splice_after(const_iterator_t pos, other_list_t<S2, D2> &&other,
                    other_list_t<S2, D2>::const_iterator first,
                    other_list_t<S2, D2>::const_iterator last) noexcept {
    return splice_after(pos, other, first, last);
  }

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
  void sort(Compare) noexcept;

private:
  template <typename T2, SListEntryAccessor<T2>, CompressedSize, typename>
  friend class slist_base;

  template <typename T2, SListEntryAccessor< typename T2::value_type> >
  friend class slist_proxy;

  template <typename T2, SListEntryAccessor<T2>, CompressedSize>
  friend class slist_head;

  template <typename L, typename C, typename S>
      requires std::is_integral_v<S>
  friend L::const_iterator
      detail::forward_list_merge_sort(typename L::const_iterator,
                                      typename L::const_iterator,
                                      C, S) noexcept;

  constexpr static bool HasInlineSize = !std::is_same_v<SizeMember, no_size>;

  constexpr static auto EntryRefMember =
      slist_entry_ref_traits<T, EntryAccess>::EntryRefMember;

  using entry_ref_type = slist_entry_ref_traits<T, EntryAccess>::entry_ref_type;
  using entry_ref_codec = detail::entry_ref_codec<entry_ref_type, EntryAccess>;

  slist_fwd_head<T, SizeMember> &getSListData() noexcept {
    return static_cast<Derived *>(this)->getSListData();
  }

  const slist_fwd_head<T, SizeMember> &getSListData() const noexcept {
    return static_cast<const Derived *>(this)->getSListData();
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

  static entry_ref_type getEncoding(const_iterator_t i) noexcept {
    return i.m_current;
  }

  template <typename QueueIt>
  static iterator insert_range_after(const_iterator pos, QueueIt first,
                                     QueueIt last) noexcept;

  [[no_unique_address]] mutable EntryAccess m_entryAccessor;
};

template <typename T, SListEntryAccessor<T> EntryAccess,
          CompressedSize SizeMember, typename Derived>
class slist_base<T, EntryAccess, SizeMember, Derived>::iterator {
public:
  using value_type = slist_base::value_type;
  using reference = slist_base::reference;
  using pointer = slist_base::pointer;
  using difference_type = slist_base::difference_type;
  using iterator_category = std::forward_iterator_tag;
  using invocable_ref = compressed_invocable_ref<EntryAccess, T &>;

  iterator() noexcept : m_current{}, m_rEntryAccessor{} {}
  iterator(const iterator &) = default;
  iterator(iterator &&) = default;

  iterator(std::nullptr_t) noexcept requires Stateless<EntryAccess>
      : m_current{nullptr}, m_rEntryAccessor{} {}

  iterator(std::nullptr_t, EntryAccess &fn) noexcept
      : m_current{nullptr}, m_rEntryAccessor{fn} {}

  iterator(T *t) noexcept requires Stateless<EntryAccess>
      : m_current{entry_ref_codec::create_entry_ref(t)}, m_rEntryAccessor{} {}

  iterator(T *t, EntryAccess &fn) noexcept
      : m_current{entry_ref_codec::create_entry_ref(t)}, m_rEntryAccessor{fn} {}

  ~iterator() = default;

  iterator &operator=(const iterator &) = default;
  iterator &operator=(iterator &&) = default;

  reference operator*() const noexcept { return *operator->(); }

  pointer operator->() const noexcept {
    return entry_ref_codec::get_value(this->m_current);
  }

  iterator &operator++() noexcept {
    m_current = slist_base::getEntry(*this)->next.*EntryRefMember;
    return *this;
  }

  iterator operator++(int) noexcept {
    iterator i{*this};
    this->operator++();
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
  template <typename T2, SListEntryAccessor<T2>, CompressedSize, typename>
  friend class slist_base;

  friend slist_base::const_iterator;

  using container = slist_base;

  iterator(typename slist_base::entry_ref_type ref, EntryAccess &fn) noexcept
      : m_current{ref}, m_rEntryAccessor{fn} {}

  slist_base::entry_ref_type m_current;
  [[no_unique_address]] invocable_ref m_rEntryAccessor;
};

template <typename T, SListEntryAccessor<T> EntryAccess,
          CompressedSize SizeMember, typename Derived>
class slist_base<T, EntryAccess, SizeMember, Derived>::const_iterator {
public:
  using value_type = slist_base::value_type;
  using reference = slist_base::const_reference;
  using pointer = slist_base::const_pointer;
  using difference_type = slist_base::difference_type;
  using iterator_category = std::forward_iterator_tag;
  using invocable_ref = compressed_invocable_ref<EntryAccess, T &>;

  const_iterator() noexcept : m_current{}, m_rEntryAccessor{} {}
  const_iterator(const const_iterator &) = default;
  const_iterator(const_iterator &&) = default;
  const_iterator(const iterator &i) noexcept
      : m_current{i.m_current}, m_rEntryAccessor{i.m_rEntryAccessor} {}

  const_iterator(std::nullptr_t) noexcept requires Stateless<EntryAccess>
      : m_current{nullptr}, m_rEntryAccessor{} {}

  const_iterator(std::nullptr_t, EntryAccess &fn) noexcept
      : m_current{nullptr}, m_rEntryAccessor{fn} {}

  const_iterator(const T *t) noexcept requires Stateless<EntryAccess>
      : m_current{entry_ref_codec::create_entry_ref(t)}, m_rEntryAccessor{} {}

  const_iterator(const T *t, EntryAccess &fn) noexcept
      : m_current{entry_ref_codec::create_entry_ref(t)}, m_rEntryAccessor{fn} {}

  ~const_iterator() = default;

  const_iterator &operator=(const const_iterator &) = default;

  const_iterator &operator=(const_iterator &&) = default;

  reference operator*() const noexcept { return *operator->(); }

  pointer operator->() const noexcept {
    return entry_ref_codec::get_value(this->m_current);
  }

  const_iterator &operator++() noexcept {
    m_current = slist_base::getEntry(*this)->next.*EntryRefMember;
    return *this;
  }

  const_iterator operator++(int) noexcept {
    const_iterator i{*this};
    this->operator++();
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
  template <typename T2, SListEntryAccessor<T2>, CompressedSize, typename>
  friend class slist_base;

  friend slist_base::iterator;

  using container = slist_base;

  const_iterator(typename slist_base::entry_ref_type ref, EntryAccess &fn) noexcept
      : m_current{ref}, m_rEntryAccessor{fn} {}

  slist_base::entry_ref_type m_current;
  [[no_unique_address]] invocable_ref m_rEntryAccessor;
};

template <typename T, CompressedSize SizeMember,
          SListEntryAccessor<T> EntryAccess>
class slist_proxy<slist_fwd_head<T, SizeMember>, EntryAccess>
    : public slist_base<T, entry_access_helper_t<T, EntryAccess>, SizeMember,
                        slist_proxy<slist_fwd_head<T, SizeMember>, EntryAccess>> {
  using entry_access_type = entry_access_helper_t<T, EntryAccess>;
  using base_type = slist_base<T, entry_access_type, SizeMember, slist_proxy>;

public:
  using fwd_head_type = slist_fwd_head<T, SizeMember>;

  // FIXME [C++20]: P0634
  template <CompressedSize S, typename D>
  using other_list_t = typename slist_base<
      T, entry_access_type, SizeMember, slist_proxy>::template other_list_t<S, D>;

  slist_proxy() = delete;

  slist_proxy(const slist_proxy &) = delete;

  slist_proxy(slist_proxy &&) = delete;

  slist_proxy(fwd_head_type &h)
      noexcept(std::is_nothrow_default_constructible_v<entry_access_type>)
      requires std::is_default_constructible_v<entry_access_type>
      : base_type{}, m_head{h} {}

  template <typename... Ts>
      requires std::is_constructible_v<entry_access_type, Ts...>
  explicit slist_proxy(fwd_head_type &h, Ts &&...vs)
      noexcept(std::is_nothrow_constructible_v<entry_access_type, Ts...>)
      : base_type{std::forward<Ts>(vs)...}, m_head{h} {}

  slist_proxy(fwd_head_type &h, slist_proxy &&other)
      noexcept(std::is_nothrow_move_constructible_v<entry_access_type>)
      : base_type{std::move(other)}, m_head{h} {
    m_head = std::move(other.getSListData());
  }

  template <CompressedSize S, typename D>
  slist_proxy(fwd_head_type &h, other_list_t<S, D> &&other)
      noexcept(std::is_nothrow_move_constructible_v<entry_access_type>)
      : base_type{std::move(other)}, m_head{h} {
    constexpr bool computeOtherSize = base_type::HasInlineSize &&
        !other_list_t<S, D>::HasInlineSize;
    const std::size_t otherSize = computeOtherSize ? std::size(other) : 0;
    m_head.move_from(std::move(other.getSListData()), otherSize);
  }

  template <typename InputIt, typename... Ts>
  slist_proxy(fwd_head_type &h, InputIt first, InputIt last, Ts &&...vs)
      noexcept(std::is_nothrow_constructible_v<entry_access_type, Ts...>)
      : base_type{std::forward<Ts>(vs)...}, m_head{h} {
    base_type::assign(first, last);
  }

  template <typename... Ts>
  slist_proxy(fwd_head_type &h, std::initializer_list<T *> ilist, Ts &&...vs)
      noexcept(std::is_nothrow_constructible_v<entry_access_type, Ts...>)
      : base_type{std::forward<Ts>(vs)...}, m_head{h} {
    base_type::assign(ilist);
  }

  ~slist_proxy() = default;

  slist_proxy &operator=(const slist_proxy &) = delete;

  slist_proxy &operator=(slist_proxy &&rhs)
      noexcept(std::is_nothrow_move_assignable_v<entry_access_type>) {
    m_head = std::move(rhs.getSListData());
    base_type::operator=(std::move(rhs));
    return *this;
  }

  template <CompressedSize S, typename D>
  slist_proxy &operator=(other_list_t<S, D> &&rhs)
      noexcept(std::is_nothrow_move_assignable_v<entry_access_type>) {
    base_type::operator=(std::move(rhs));
    constexpr bool computeRhsSize = base_type::HasInlineSize &&
        !other_list_t<S, D>::HasInlineSize;
    const std::size_t rhsSize = computeRhsSize ? std::size(rhs) : 0;
    m_head.move_from(std::move(rhs.getSListData()), rhsSize);
    return *this;
  }

  slist_proxy &operator=(std::initializer_list<T *> ilist) noexcept {
    base_type::assign(ilist);
    return *this;
  }

private:
  template <typename T2, SListEntryAccessor<T2>, CompressedSize, typename>
  friend class slist_base;

  fwd_head_type &getSListData() noexcept { return m_head; }

  const fwd_head_type &getSListData() const noexcept { return m_head; }

  fwd_head_type &m_head;
};

template <typename T, SListEntryAccessor<T> EntryAccess,
          CompressedSize SizeMember>
class slist_head
    : private slist_fwd_head<T, SizeMember>,
      public slist_base<T, entry_access_helper_t<T, EntryAccess>, SizeMember,
                        slist_head<T, EntryAccess, SizeMember>> {
  using fwd_head_type = slist_fwd_head<T, SizeMember>;
  using entry_access_type = entry_access_helper_t<T, EntryAccess>;
  using base_type = slist_base<T, entry_access_type, SizeMember, slist_head>;

public:
  using typename fwd_head_type::value_type;
  using typename fwd_head_type::size_type;

  template <CompressedSize S, typename D>
  using other_list_t = typename slist_base<
      T, entry_access_type, SizeMember, slist_head>::template other_list_t<S, D>;

  slist_head() = default;

  slist_head(const slist_head &) = delete;

  slist_head(slist_head &&other)
      noexcept(std::is_nothrow_move_constructible_v<base_type>)
      : fwd_head_type{std::move(other)}, base_type{std::move(other)} {}

  template <CompressedSize S, typename D>
  slist_head(other_list_t<S, D> &&other)
      noexcept(std::is_nothrow_move_constructible_v<base_type>)
      : fwd_head_type{std::move(other.getSListData()),
                      (base_type::HasInlineSize && std::is_same_v<S, no_size>)
                          ? std::size(other)
                          : 0},
        base_type{std::move(other)} {}

  template <typename... Ts>
      requires std::is_constructible_v<entry_access_type, Ts...>
  explicit slist_head(Ts &&...vs)
      noexcept(std::is_nothrow_constructible_v<entry_access_type, Ts...>)
      : fwd_head_type{}, base_type{std::forward<Ts>(vs)...} {}

  template <typename InputIt, typename... Ts>
  slist_head(InputIt first, InputIt last, Ts &&...vs)
      noexcept(std::is_nothrow_constructible_v<entry_access_type, Ts...>)
      : base_type{std::forward<Ts>(vs)...} {
    base_type::assign(first, last);
  }

  template <typename... Ts>
  slist_head(std::initializer_list<T *> ilist, Ts &&...vs)
      noexcept(std::is_nothrow_constructible_v<entry_access_type, Ts...>)
      : base_type{std::forward<Ts>(vs)...} {
    base_type::assign(ilist);
  }

  ~slist_head() = default;

  slist_head &operator=(const slist_head &) = delete;

  slist_head &operator=(slist_head &&rhs)
      noexcept(std::is_nothrow_move_assignable_v<entry_access_type>) {
    fwd_head_type::operator=(std::move(rhs.getSListData()));
    base_type::operator=(std::move(rhs));
    return *this;
  }

  template <CompressedSize S, typename D>
  slist_head &operator=(other_list_t<S, D> &&rhs)
      noexcept(std::is_nothrow_move_assignable_v<entry_access_type>) {
    fwd_head_type::move_from(
        std::move(rhs.getSListData()),
        (base_type::HasInlineSize && std::is_same_v<S, no_size>)
            ? std::size(rhs)
            : 0);
    base_type::operator=(std::move(rhs));
    return *this;
  }

  slist_head &operator=(std::initializer_list<T *> ilist) noexcept {
    base_type::assign(ilist);
    return *this;
  }

private:
  template <typename T2, SListEntryAccessor<T2>, CompressedSize, typename>
  friend class slist_base;

  fwd_head_type &getSListData() noexcept { return *this; }

  const fwd_head_type &getSListData() const noexcept { return *this; }
};

template <typename T, SListEntryAccessor<T> E, CompressedSize S, typename D>
slist_base<T, E, S, D>::size_type
slist_base<T, E, S, D>::size() const noexcept {
  if constexpr (HasInlineSize)
    return getSListData().m_sz;
  else
    return static_cast<size_type>(std::distance(begin(), end()));
}

template <typename T, SListEntryAccessor<T> E, CompressedSize S, typename D>
void slist_base<T, E, S, D>::clear() noexcept {
  auto &data = getSListData();
  data.m_headEntry.next = entry_ref_type{nullptr};

  if constexpr (HasInlineSize)
    data.m_sz = 0;
}

template <typename T, SListEntryAccessor<T> E, CompressedSize S, typename D>
slist_base<T, E, S, D>::iterator
slist_base<T, E, S, D>::insert_after(const_iterator pos, T *value) noexcept {
  CSD_ASSERT(pos != end(), "end() iterator passed to insert_after");

  const entry_ref_type valueRef = entry_ref_codec::create_entry_ref(value);

  entry_type *const posEntry = getEntry(pos);
  entry_type *const insertEntry = getEntry(valueRef);

  insertEntry->next = posEntry->next;
  posEntry->next = valueRef;

  if constexpr (HasInlineSize)
    ++getSListData().m_sz;

  return {valueRef, m_entryAccessor};
}

template <typename T, SListEntryAccessor<T> E, CompressedSize S, typename D>
template <typename InputIt>
slist_base<T, E, S, D>::iterator
slist_base<T, E, S, D>::insert_after(const_iterator pos, InputIt first,
                                     InputIt last) noexcept {
  while (first != last)
    pos = insert_after(pos, *first++);

  return {pos.m_current, m_entryAccessor};
}

template <typename T, SListEntryAccessor<T> E, CompressedSize S, typename D>
slist_base<T, E, S, D>::iterator
slist_base<T, E, S, D>::erase_after(const_iterator pos) noexcept {
  CSD_ASSERT(pos != end(), "end() iterator passed to erase_after");

  entry_type *const posEntry = getEntry(pos);
  const bool isLastEntry = !(posEntry->next.*EntryRefMember);

  if (isLastEntry)
    return end();

  if constexpr (HasInlineSize)
    --getSListData().m_sz;

  entry_type *const erasedEntry = getEntry(posEntry->next);
  const entry_ref_type nextRef = erasedEntry->next.*EntryRefMember;
  posEntry->next = nextRef;

  return {nextRef, m_entryAccessor};
}

template <typename T, SListEntryAccessor<T> E, CompressedSize S, typename D>
slist_base<T, E, S, D>::iterator
slist_base<T, E, S, D>::erase_after(const_iterator first,
                                    const_iterator last) noexcept {
  if (first == end())
    return {first.m_current, m_entryAccessor};

  // Remove the open range (first, last) by linking first directly to last,
  // thereby removing all the internal elements.
  entry_type *const firstEntry = getEntry(first);
  firstEntry->next = last.m_current;

  if constexpr (HasInlineSize)
    getSListData().m_sz -= std::distance(++first, last);

  return {last.m_current, m_entryAccessor};
}

template <typename T, SListEntryAccessor<T> E, CompressedSize S, typename D>
slist_base<T, E, S, D>::iterator
slist_base<T, E, S, D>::find_predecessor(const_iterator scan,
                                         const_iterator last,
                                         const_iterator pos) const noexcept {
  while (scan != last) {
    if (auto prev = scan++; scan == pos)
      return {prev.m_current, m_entryAccessor};
  }

  return {nullptr, m_entryAccessor};
}

template <typename T, SListEntryAccessor<T> E, CompressedSize S, typename D>
template <typename UnaryPredicate>
std::pair<typename slist_base<T, E, S, D>::iterator, bool>
slist_base<T, E, S, D>::find_predecessor_if(const_iterator prev,
                                            const_iterator last,
                                            UnaryPredicate pred) const noexcept {
  if (prev == last)
    return {iterator{nullptr, m_entryAccessor}, false};

  const_iterator scan = std::next(prev);

  while (scan != last) {
    if (pred(*scan))
      return {iterator{prev.m_current, m_entryAccessor}, true};
    prev = scan++;
  }

  return {iterator{prev.m_current, m_entryAccessor}, false};
}

template <typename T, SListEntryAccessor<T> E, CompressedSize S1, typename D1>
template <CompressedSize S2, typename D2, typename Compare>
void slist_base<T, E, S1, D1>::merge(other_list_t<S2, D2> &other,
                                     Compare comp) noexcept {
  if (this == &other)
    return;

  auto p1 = cbefore_begin();
  auto f1 = std::next(p1);
  auto e1 = cend();

  auto f2 = other.cbegin();
  auto e2 = other.cend();

  if constexpr (HasInlineSize)
    getSListData().m_sz += std::size(other);

  while (f1 != e1 && f2 != e2) {
    if (comp(*f1, *f2)) {
      p1 = f1++;
      continue;
    }

    // Scan the range items where *f2 < *f1. When we're done, [f2, mergeEnd]
    // will be the closed range of elements that needs to merged after p1
    // (before f1).
    auto mergeEnd = f2;

    for (auto scan = std::next(mergeEnd); scan != e2 && comp(*scan, *f1); ++scan)
      mergeEnd = scan;

    f2 = insert_range_after(p1, f2, mergeEnd);

    p1 = mergeEnd;
    f1 = std::next(mergeEnd);
  }

  if (f2 != e2) {
    // Merge the remaining range, [f2, e2), at the end of the list.
    getEntry(p1)->next = f2.m_current;
  }

  other.clear();
}

template <typename T, SListEntryAccessor<T> E, CompressedSize S1, typename D1>
template <CompressedSize S2, typename D2>
void slist_base<T, E, S1, D1>::splice_after(const_iterator pos,
                                            other_list_t<S2, D2> &other) noexcept {
  if (other.empty())
    return;

  CSD_ASSERT(pos.m_current && "end() iterator passed as pos");

  getEntry(pos)->next = other.begin().m_current;

  if constexpr (HasInlineSize)
    getSListData().m_sz += std::size(other);

  other.clear();
}

template <typename T, SListEntryAccessor<T> E, CompressedSize S1, typename D1>
template <CompressedSize S2, typename D2>
void slist_base<T, E, S1, D1>::splice_after(
    const_iterator pos, other_list_t<S2, D2> &other,
    typename other_list_t<S2, D2>::const_iterator first,
    typename other_list_t<S2, D2>::const_iterator last) noexcept {
  if (first == last)
    return;

  // When the above is false, first++ must be legal.
  CSD_ASSERT(first.m_current, "first is end() but last was not end()?");

  // Remove the open range (first, last) from `other`, by directly linking
  // first and last. Also post-increment first, so that it will point to the
  // start of the closed range [first + 1, last - 1] that we're inserting.
  other.getEntry(first++)->next = last.m_current;

  if (first == last)
    return;

  // Find the last element in the closed range we're inserting, then use
  // insert_range_after to insert the closed range after pos.
  // FIXME: make a utility function that does this, and also use it in
  // the merge and remove_if implementations?
  auto lastInsert = first, scan = std::next(lastInsert);
  std::common_type_t<difference_type, typename D2::difference_type> sz = 0;

  while (scan != last) {
    lastInsert = scan++;
    ++sz;
  }

  if constexpr (HasInlineSize)
    getSListData().m_sz += sz;

  if constexpr (other_list_t<S2, D2>::HasInlineSize)
    other.getSListData().m_sz -= sz;

  insert_range_after(pos, first, lastInsert);
}

template <typename T, SListEntryAccessor<T> E, CompressedSize S, typename D>
template <typename UnaryPredicate>
slist_base<T, E, S, D>::size_type
slist_base<T, E, S, D>::remove_if(UnaryPredicate pred) noexcept {
  size_type nRemoved = 0;
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
    ++nRemoved;

    while (i != end && pred(*i)) {
      ++i;
      ++nRemoved;
    }

    prev = erase_after(prev, i);
    i = (prev != end) ? std::next(prev) : end;
  }

  return nRemoved;
}

template <typename T, SListEntryAccessor<T> E, CompressedSize S, typename D>
void slist_base<T, E, S, D>::reverse() noexcept {
  const const_iterator end = cend();
  const_iterator i = cbegin();
  const_iterator prev = end;

  while (i != end) {
    const auto current = i;
    entry_type *const entry = getEntry(i++);
    entry->next = prev.m_current;
    prev = current;
  }

  getSListData().m_headEntry.next = prev.m_current;
}

template <typename T, SListEntryAccessor<T> E, CompressedSize S, typename D>
template <typename BinaryPredicate>
void slist_base<T, E, S, D>::unique(BinaryPredicate pred) noexcept {
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

template <typename T, SListEntryAccessor<T> E, CompressedSize S, typename D>
template <typename Compare>
void slist_base<T, E, S, D>::sort(Compare comp) noexcept {
  detail::forward_list_merge_sort<slist_base<T, E, S, D>>(cbefore_begin(), cend(),
                                                          comp, std::size(*this));
}

template <typename T, SListEntryAccessor<T> E, CompressedSize S, typename D>
template <typename QueueIt>
slist_base<T, E, S, D>::iterator
slist_base<T, E, S, D>::insert_range_after(const_iterator pos, QueueIt first,
                                           QueueIt last) noexcept {
  // Inserts the closed range [first, last] after pos and returns
  // std::next(last)
  CSD_ASSERT(pos.m_current && last.m_current,
             "end() iterator passed as pos or last");

  entry_type *const posEntry = getEntry(pos);
  entry_type *const lastEntry = QueueIt::container::getEntry(last);

  const entry_ref_type oldNext = lastEntry->next.*EntryRefMember;

  lastEntry->next = posEntry->next;
  posEntry->next = first.m_current;

  return iterator{oldNext, last.m_rEntryAccessor.get_invocable()};
}

} // End of namespace csd

#endif
