//==-- csg/core/slist.h - singly-linked list implementation -----*- C++ -*-==//
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

#ifndef CSG_CORE_SLIST_H
#define CSG_CORE_SLIST_H

#include <concepts>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

#include <csg/core/assert.h>
#include <csg/core/listfwd.h>
#include <csg/core/utility.h>

namespace csg {

template <typename T>
struct slist_entry {
  entry_ref_union<slist_entry, T> next;
};

template <typename T, slist_entry_extractor<T> EntryEx,
          optional_size SizeMember, typename Derived>
class slist_base;

template <typename T, typename O>
concept compatible_slist = slist<T> &&
    std::same_as<typename T::value_type, typename O::value_type> &&
    std::same_as<typename T::entry_extractor_type, typename O::entry_extractor_type>;

template <typename T, optional_size SizeMember>
class slist_fwd_head {
public:
  using value_type = T;
  using size_type = std::conditional_t<std::same_as<SizeMember, no_size>,
                                       std::size_t, SizeMember>;

  constexpr slist_fwd_head() noexcept : m_headEntry{nullptr}, m_sz{} {}

  slist_fwd_head(const slist_fwd_head &) = delete;

  constexpr slist_fwd_head(slist_fwd_head &&other) noexcept : slist_fwd_head{} {
    swap(other);
  }

  ~slist_fwd_head() = default;

  constexpr void swap(slist_fwd_head &other) noexcept {
    std::ranges::swap(m_headEntry, other.m_headEntry);
    std::ranges::swap(m_sz, other.m_sz);
  }

  slist_fwd_head &operator=(const slist_fwd_head &) = delete;

  constexpr slist_fwd_head &operator=(slist_fwd_head &&other) noexcept {
    return *new(this) slist_fwd_head{std::move(other)};
  }

private:
  template <typename, optional_size>
  friend class slist_fwd_head;

  template <typename T2, slist_entry_extractor<T2>, optional_size, typename>
  friend class slist_base;

  template <util::derived_from_template<slist_fwd_head> FH2,
            slist_entry_extractor<typename FH2::value_type>>
  friend class slist_proxy;

  template <typename T2, slist_entry_extractor<T2>, optional_size>
  friend class slist_head;

  using size_member_type = SizeMember;

  template <optional_size S2>
  constexpr void swap_with(slist_fwd_head<T, S2> &other,
                           CSG_TYPENAME slist_fwd_head<T, S2>::size_type otherSize,
                           size_type ourSize) noexcept {
    std::ranges::swap(m_headEntry, other.m_headEntry);
    if constexpr (std::integral<size_member_type>)
      m_sz = static_cast<size_type>(otherSize);
    if constexpr (std::integral<S2>)
      other.m_sz = static_cast<S2>(ourSize);
  }

  slist_entry<T> m_headEntry;
  [[no_unique_address]] SizeMember m_sz;
};

template <typename T, slist_entry_extractor<T> EntryEx,
          optional_size SizeMember, typename Derived>
class slist_base {
protected:
  constexpr static bool s_has_nothrow_extractor =
      std::is_nothrow_invocable_v<EntryEx, T &>;

public:
  using value_type = T;
  using reference = T &;
  using const_reference = const T &;
  using pointer = T *;
  using const_pointer = const T *;
  using size_type = CSG_TYPENAME slist_fwd_head<SizeMember>::size_type;
  using difference_type = std::make_signed_t<size_type>;
  using entry_type = slist_entry<T>;
  using entry_extractor_type = EntryEx;
  using size_member_type = SizeMember;

  template <optional_size S, typename D>
  using other_list_t = slist_base<T, EntryEx, S, D>;

  constexpr entry_extractor_type &get_entry_extractor() noexcept {
    return static_cast<Derived *>(this)->getEntryExtractor();
  }

  constexpr const entry_extractor_type &get_entry_extractor() const noexcept {
    return const_cast<slist_base *>(this)->get_entry_extractor();
  }

  template <std::input_iterator InputIt, std::sentinel_for<InputIt> Sentinel>
      requires std::constructible_from<pointer, std::iter_reference_t<InputIt>>
  constexpr void assign(InputIt first, Sentinel last)
      CSG_NOEXCEPT(noexcept(insert_after(cbefore_begin(), first, last))) {
    clear();
    insert_after(cbefore_begin(), first, last);
  }

  template <std::ranges::input_range Range>
      requires std::constructible_from<pointer, std::ranges::range_reference_t<Range>>
  constexpr void assign(Range &&r)
      noexcept(noexcept(assign(std::ranges::begin(r), std::ranges::end(r)))) {
    assign(std::ranges::begin(r), std::ranges::end(r));
  }

  constexpr void assign(std::initializer_list<pointer> ilist)
      noexcept(s_has_nothrow_extractor) {
    assign(std::ranges::begin(ilist), std::ranges::end(ilist));
  }

  constexpr reference front() noexcept(s_has_nothrow_extractor) {
    return *begin();
  }

  constexpr const_reference front() const noexcept(s_has_nothrow_extractor) {
    return *begin();
  }

  class iterator;
  class const_iterator;

  using iterator_t = std::type_identity_t<iterator>;
  using const_iterator_t = std::type_identity_t<const_iterator>;

  constexpr iterator before_begin() noexcept;

  constexpr const_iterator before_begin() const noexcept {
    return const_cast<slist_base *>(this)->before_begin();
  }

  constexpr const_iterator cbefore_begin() const noexcept {
    return before_begin();
  }

  constexpr iterator begin() noexcept(s_has_nothrow_extractor) {
    return ++before_begin();
  }

  constexpr const_iterator begin() const noexcept(s_has_nothrow_extractor) {
    return ++before_begin();
  }

  constexpr const_iterator cbegin() const noexcept(s_has_nothrow_extractor) {
    return begin();
  }

  constexpr iterator end() noexcept {
    return {nullptr, get_entry_extractor()};
  }

  constexpr const_iterator end() const noexcept {
    return {nullptr, get_entry_extractor_mutable()};
  }

  constexpr const_iterator cend() const noexcept {
    return end();
  }

  constexpr iterator iter(pointer p) noexcept {
    return {p, get_entry_extractor()};
  }

  constexpr const_iterator iter(const_pointer p) const noexcept {
    return {p, get_entry_extractor_mutable()};
  }

  constexpr const_iterator citer(const_pointer p) const noexcept {
    return iter(p);
  }

  [[nodiscard]] constexpr bool empty() const noexcept {
    return !getHeadData().m_headEntry.next;
  }

  constexpr size_type size() const
      noexcept(std::integral<SizeMember> || s_has_nothrow_extractor);

  constexpr static size_type max_size() noexcept {
    return std::numeric_limits<size_type>::max();
  }

  constexpr void clear() noexcept;

  constexpr iterator insert_after(const_iterator, pointer)
      noexcept(s_has_nothrow_extractor);

  template <std::input_iterator InputIt, std::sentinel_for<InputIt> Sentinel>
      requires std::constructible_from<pointer, std::iter_reference_t<InputIt>>
  constexpr iterator
  insert_after(const_iterator, InputIt first, Sentinel last)
      noexcept(noexcept(*first++) && noexcept(first != last) &&
               s_has_nothrow_extractor);

  template <std::ranges::input_range Range>
      requires std::constructible_from<pointer, std::ranges::range_reference_t<Range>>
  constexpr iterator insert_after(const_iterator_t pos, Range &&r)
      noexcept( noexcept(insert_after(pos, std::ranges::begin(r),
                                      std::ranges::end(r))) ) {
    return insert_after(pos, std::ranges::begin(r), std::ranges::end(r));
  }

  constexpr iterator
  insert_after(const_iterator_t pos, std::initializer_list<pointer> i)
      noexcept(s_has_nothrow_extractor) {
    return insert_after(pos, std::ranges::begin(i), std::ranges::end(i));
  }

  constexpr iterator erase_after(const_iterator)
      noexcept(s_has_nothrow_extractor);

  constexpr iterator erase_after(const_iterator first, const_iterator last)
      noexcept(s_has_nothrow_extractor);

  constexpr std::pair<pointer, iterator> find_erase(const_iterator_t pos)
      noexcept(noexcept(erase_after(find_predecessor(pos)))) {
    T *const erased = const_cast<T *>(std::addressof(*pos));
    return {erased, erase_after(find_predecessor(pos))};
  }

  template <std::invocable<reference> Proj = std::identity,
            std::invocable<std::invoke_result_t<Proj, reference>> Fun>
  constexpr void for_each_safe(Fun f, Proj proj = {})
      noexcept(s_has_nothrow_extractor &&
               util::is_nothrow_proj_invocable<reference, Proj, Fun>) {
    for_each_safe(begin(), end(), std::ref(f), std::ref(proj));
  }

  template <std::invocable<reference> Proj = std::identity,
            std::invocable<std::invoke_result_t<Proj, reference>> Fun>
  constexpr void for_each_safe(iterator_t first, const const_iterator_t last,
                               Fun f, Proj proj = {})
      noexcept(s_has_nothrow_extractor &&
	       util::is_nothrow_proj_invocable<reference, Proj, Fun>) {
    while (first != last)
      std::invoke(f, std::invoke(proj, *first++));
  }

  constexpr void push_front(pointer p) noexcept(s_has_nothrow_extractor) {
    insert_after(cbefore_begin(), p);
  }

  constexpr void pop_front() noexcept(s_has_nothrow_extractor) {
    erase_after(cbefore_begin());
  }

  template <optional_size S2, typename D2>
  constexpr void swap(other_list_t<S2, D2> &other)
      noexcept(std::is_nothrow_swappable_v<entry_extractor_type> &&
               noexcept(size()) && noexcept(other.size()));

  [[nodiscard]] constexpr iterator
  find_predecessor(const_iterator_t pos) const noexcept(s_has_nothrow_extractor) {
    return find_predecessor(before_begin(), end(), pos);
  }

  [[nodiscard]] constexpr iterator
  find_predecessor(const_iterator first, const_iterator last,
                   const_iterator pos) const noexcept(s_has_nothrow_extractor);

  template <std::invocable<const_reference> Proj = std::identity,
            std::predicate<std::invoke_result_t<Proj, const_reference>> Pred>
  [[nodiscard]] constexpr std::pair<iterator, bool>
  find_predecessor_if(Pred pred, Proj proj = {}) const
      noexcept(s_has_nothrow_extractor &&
               util::is_nothrow_proj_invocable<const_reference, Proj, Pred>) {
    return find_predecessor_if(before_begin(), end(), std::ref(pred),
                               std::ref(proj));
  }

  template <std::invocable<const_reference> Proj = std::identity,
            std::predicate<std::invoke_result_t<Proj, const_reference>> Pred>
  [[nodiscard]] constexpr std::pair<iterator, bool>
  find_predecessor_if(const_iterator first, const_iterator last,
                      Pred, Proj = {}) const
      noexcept(s_has_nothrow_extractor &&
               util::is_nothrow_proj_invocable<const_reference, Proj, Pred>);

  template <optional_size S2, typename D2,
            std::invocable<const_reference> Proj = std::identity,
            std::strict_weak_order<
              std::invoke_result_t<Proj, const_reference>,
              std::invoke_result_t<Proj, const_reference>
            > Compare = std::ranges::less>
  constexpr void merge(other_list_t<S2, D2> &other,
                       Compare = {}, Proj = {})
      noexcept(s_has_nothrow_extractor &&
               util::is_nothrow_proj_relation<const_reference, Proj, Compare>);

  template <optional_size S2, typename D2,
            std::invocable<const_reference> Proj = std::identity,
	    std::strict_weak_order<
	      std::invoke_result_t<Proj, const_reference>,
	      std::invoke_result_t<Proj, const_reference>
	    > Compare = std::ranges::less>
  constexpr void merge(other_list_t<S2, D2> &&other,
                       Compare comp = {}, Proj proj = {})
      noexcept(noexcept(merge(other, std::ref(comp), std::ref(proj)))) {
    merge(other, std::ref(comp), std::ref(proj));
  }

  template <optional_size S2, typename D2>
  constexpr void splice_after(const_iterator pos,
                              other_list_t<S2, D2> &other)
      noexcept(s_has_nothrow_extractor);

  template <optional_size S2, typename D2>
  constexpr void splice_after(const_iterator_t pos,
                              other_list_t<S2, D2> &&other)
      noexcept(noexcept(splice_after(pos, other))) {
    return splice_after(pos, other);
  }

  template <optional_size S2, typename D2>
  constexpr void
  splice_after(const_iterator_t pos, other_list_t<S2, D2> &other,
               CSG_TYPENAME other_list_t<S2, D2>::const_iterator it)
      noexcept(noexcept(splice_after(pos, other, it, other.cend()))) {
    return splice_after(pos, other, it, other.cend());
  }

  template <optional_size S2, typename D2>
  constexpr void
  splice_after(const_iterator_t pos, other_list_t<S2, D2> &&other,
               CSG_TYPENAME other_list_t<S2, D2>::const_iterator it)
      noexcept(noexcept(splice_after(pos, other, it))) {
    return splice_after(pos, other, it);
  }

  template <optional_size S2, typename D2>
  constexpr void
  splice_after(const_iterator pos, other_list_t<S2, D2> &other,
               CSG_TYPENAME other_list_t<S2, D2>::const_iterator first,
               CSG_TYPENAME other_list_t<S2, D2>::const_iterator last)
      noexcept(s_has_nothrow_extractor);

  template <optional_size S2, typename D2>
  constexpr void
  splice_after(const_iterator_t pos, other_list_t<S2, D2> &&other,
               CSG_TYPENAME other_list_t<S2, D2>::const_iterator first,
               CSG_TYPENAME other_list_t<S2, D2>::const_iterator last)
      noexcept(noexcept(splice_after(pos, other, first, last))) {
    return splice_after(pos, other, first, last);
  }

  template <typename V, std::invocable<const_reference> Proj = std::identity,
            std::equivalence_relation<
              std::invoke_result_t<Proj, const_reference>,
              const V &
            > EqRelation = std::ranges::equal_to>
  constexpr size_type remove(const V &value, EqRelation eq = {}, Proj proj = {})
      noexcept(s_has_nothrow_extractor &&
               std::is_nothrow_invocable_v<Proj, const_reference> &&
               std::is_nothrow_invocable_v<EqRelation,
                   std::invoke_result_t<Proj, const_reference>, const V &>) {
    return remove_if( [&value, &eq] (const auto &projected) {
        return std::invoke(eq, projected, value); }, std::ref(proj));
  }

  template <std::invocable<const_reference> Proj = std::identity,
            std::predicate<std::invoke_result_t<Proj, const_reference>> Pred>
  constexpr size_type remove_if(Pred, Proj = {})
      noexcept(s_has_nothrow_extractor &&
               util::is_nothrow_proj_invocable<const_reference, Proj, Pred>);

  constexpr void reverse() noexcept(s_has_nothrow_extractor);

  template <std::invocable<const_reference> Proj = std::identity,
            std::equivalence_relation<
              std::invoke_result_t<Proj, const_reference>,
              std::invoke_result_t<Proj, const_reference>
            > EqRelation = std::ranges::equal_to>
  constexpr void unique(EqRelation = {}, Proj = {})
    noexcept(s_has_nothrow_extractor &&
             util::is_nothrow_proj_relation<const_reference, Proj, EqRelation>);

  template <std::invocable<const_reference> Proj = std::identity,
            std::strict_weak_order<
              std::invoke_result_t<Proj, const_reference>,
              std::invoke_result_t<Proj, const_reference>
	    > Compare = std::ranges::less>
  constexpr void sort(Compare = {}, Proj = {})
      noexcept(s_has_nothrow_extractor &&
               util::is_nothrow_proj_relation<const_reference, Proj, Compare>);

private:
  template <typename T2, slist_entry_extractor<T2>, optional_size, typename>
  friend class slist_base;

  template <util::derived_from_template<slist_fwd_head> T2,
            slist_entry_extractor<typename T2::value_type>>
  friend class slist_proxy;

  template <typename T2, slist_entry_extractor<T2>, optional_size>
  friend class slist_head;

  template <typename L, typename C, typename P, std::unsigned_integral S>
  friend CSG_TYPENAME L::const_iterator
      detail::forward_list_merge_sort(CSG_TYPENAME L::const_iterator,
                                      CSG_TYPENAME L::const_iterator,
                                      C, P, S) noexcept;

  using entry_ref_codec = detail::entry_ref_codec<entry_type, T, EntryEx>;
  using entry_ref_type = entry_ref_union<entry_type, T>;

  constexpr slist_fwd_head<T, SizeMember> &getHeadData() noexcept {
    return static_cast<Derived *>(this)->getHeadData();
  }

  constexpr const slist_fwd_head<T, SizeMember> &getHeadData() const noexcept {
    return const_cast<slist_base *>(this)->getHeadData();
  }

  constexpr entry_extractor_type &get_entry_extractor_mutable() const noexcept {
    return const_cast<slist_base *>(this)->get_entry_extractor();
  }

  constexpr entry_type *refToEntry(entry_ref_type ref)
      noexcept(s_has_nothrow_extractor) {
    return entry_ref_codec::get_entry(get_entry_extractor(), ref);
  }

  constexpr static entry_type *iterToEntry(iterator_t i)
      noexcept(s_has_nothrow_extractor) {
    auto &entryEx = i.m_rEntryExtractor.get_invocable();
    return entry_ref_codec::get_entry(entryEx, i.m_current);
  }

  constexpr static entry_type *iterToEntry(const_iterator_t i)
      noexcept(s_has_nothrow_extractor) {
    auto &entryEx = i.m_rEntryExtractor.get_invocable();
    return entry_ref_codec::get_entry(entryEx, i.m_current);
  }

  constexpr static entry_ref_type iterToEntryRef(const_iterator_t i) noexcept {
    return i.m_current;
  }

  template <typename QueueIt>
  constexpr static iterator
  insert_range_after(const_iterator pos, QueueIt first, QueueIt last)
      noexcept(s_has_nothrow_extractor &&
               QueueIt::container::s_has_nothrow_extractor);
};

template <typename T, slist_entry_extractor<T> EntryEx,
          optional_size SizeMember, typename Derived>
class slist_base<T, EntryEx, SizeMember, Derived>::iterator {
public:
  using value_type = slist_base::value_type;
  using reference = slist_base::reference;
  using pointer = slist_base::pointer;
  using difference_type = slist_base::difference_type;
  using iterator_category = std::forward_iterator_tag;
  using invocable_ref = compressed_invocable_ref<EntryEx, reference>;

  // This is default constructible even when invocable_ref is not stateless
  // because, even though it doesn't leave us with a useable iterator, being
  // a std::input_range requires it (a useable iterator can still be assigned
  // to it).
  constexpr iterator() noexcept : m_current{}, m_rEntryExtractor{} {}
  constexpr iterator(const iterator &) = default;
  constexpr iterator(iterator &&) = default;

  constexpr iterator(std::nullptr_t) noexcept
      requires stateless<entry_extractor_type>
      : m_current{nullptr}, m_rEntryExtractor{} {}

  constexpr iterator(std::nullptr_t, entry_extractor_type &fn) noexcept
      : m_current{nullptr}, m_rEntryExtractor{fn} {}

  constexpr iterator(pointer p) noexcept
      requires stateless<entry_extractor_type>
      : m_current{entry_ref_codec::create_item_entry_ref(p)},
        m_rEntryExtractor{} {}

  constexpr iterator(pointer p, entry_extractor_type &fn) noexcept
      : m_current{entry_ref_codec::create_item_entry_ref(p)},
        m_rEntryExtractor{fn} {}

  ~iterator() = default;

  constexpr iterator &operator=(const iterator &) = default;
  constexpr iterator &operator=(iterator &&) = default;

  constexpr reference operator*() const noexcept {
    return slist_base::entry_ref_codec::get_value(m_current);
  }

  constexpr pointer operator->() const noexcept {
    return std::addressof(this->operator*());
  }

  constexpr iterator &operator++() noexcept(s_has_nothrow_extractor) {
    m_current = slist_base::iterToEntry(*this)->next;
    return *this;
  }

  constexpr iterator operator++(int) noexcept(s_has_nothrow_extractor) {
    const iterator i{*this};
    this->operator++();
    return i;
  }

  // FIXME [C++20] could use = default for some of these, but then
  // invocable_ref needs operator==
  constexpr bool operator==(const iterator &rhs) const noexcept {
    return m_current == rhs.m_current;
  }

  constexpr bool operator==(const const_iterator &rhs) const noexcept {
    return m_current == rhs.m_current;
  }

private:
  template <typename T2, slist_entry_extractor<T2>, optional_size, typename>
  friend class slist_base;

  friend slist_base::const_iterator;

  using container = slist_base;

  constexpr iterator(entry_ref_type ref, entry_extractor_type &fn) noexcept
      : m_current{ref}, m_rEntryExtractor{fn} {}

  entry_ref_type m_current;
  [[no_unique_address]] invocable_ref m_rEntryExtractor;
};

template <typename T, slist_entry_extractor<T> EntryEx,
          optional_size SizeMember, typename Derived>
class slist_base<T, EntryEx, SizeMember, Derived>::const_iterator {
public:
  using value_type = slist_base::value_type;
  using reference = slist_base::const_reference;
  using pointer = slist_base::const_pointer;
  using difference_type = slist_base::difference_type;
  using iterator_category = std::forward_iterator_tag;
  using invocable_ref = compressed_invocable_ref<EntryEx, slist_base::reference>;

  constexpr const_iterator() noexcept : m_current{}, m_rEntryExtractor{} {}
  constexpr const_iterator(const const_iterator &) = default;
  constexpr const_iterator(const_iterator &&) = default;
  constexpr const_iterator(const iterator &i) noexcept
      : m_current{i.m_current}, m_rEntryExtractor{i.m_rEntryExtractor} {}

  constexpr const_iterator(std::nullptr_t) noexcept
      requires stateless<entry_extractor_type>
      : m_current{nullptr}, m_rEntryExtractor{} {}

  constexpr const_iterator(std::nullptr_t, entry_extractor_type &fn) noexcept
      : m_current{nullptr}, m_rEntryExtractor{fn} {}

  constexpr const_iterator(const_pointer p) noexcept
      requires stateless<entry_extractor_type>
      : m_current{entry_ref_codec::create_item_entry_ref(p)},
        m_rEntryExtractor{} {}

  constexpr const_iterator(const_pointer p, entry_extractor_type &fn) noexcept
      : m_current{entry_ref_codec::create_item_entry_ref(p)},
        m_rEntryExtractor{fn} {}

  ~const_iterator() = default;

  constexpr const_iterator &operator=(const const_iterator &) = default;
  constexpr const_iterator &operator=(const_iterator &&) = default;

  constexpr reference operator*() const noexcept {
    return slist_base::entry_ref_codec::get_value(m_current);
  }

  constexpr pointer operator->() const noexcept {
    return std::addressof(this->operator*());
  }

  constexpr const_iterator &operator++() noexcept(s_has_nothrow_extractor) {
    m_current = slist_base::iterToEntry(*this)->next;
    return *this;
  }

  constexpr const_iterator operator++(int) noexcept(s_has_nothrow_extractor) {
    const const_iterator i{*this};
    this->operator++();
    return i;
  }

  // FIXME [C++20]: maybe = default, as above?
  constexpr bool operator==(const iterator &rhs) const noexcept {
    return m_current == rhs.m_current;
  }

  constexpr bool operator==(const const_iterator &rhs) const noexcept {
    return m_current == rhs.m_current;
  }

private:
  template <typename T2, slist_entry_extractor<T2>, optional_size, typename>
  friend class slist_base;

  friend slist_base::iterator;

  using container = slist_base;

  constexpr const_iterator(entry_ref_type ref, entry_extractor_type &fn) noexcept
      : m_current{ref}, m_rEntryExtractor{fn} {}

  entry_ref_type m_current;
  [[no_unique_address]] invocable_ref m_rEntryExtractor;
};

template <util::derived_from_template<slist_fwd_head> FwdHead,
          slist_entry_extractor<typename FwdHead::value_type> EntryEx>
class slist_proxy : public slist_base<typename FwdHead::value_type, EntryEx,
    typename FwdHead::size_member_type, slist_proxy<FwdHead, EntryEx>> {
  using size_member_type = CSG_TYPENAME FwdHead::size_member_type;
  using base_type = slist_base<typename FwdHead::value_type, EntryEx,
                               size_member_type, slist_proxy>;

public:
  using fwd_head_type = FwdHead;
  using pointer = CSG_TYPENAME base_type::pointer;
  using entry_extractor_type = EntryEx;

  template <optional_size S, typename D>
  using other_list_t = CSG_TYPENAME base_type::template other_list_t<S, D>;

  slist_proxy() = delete;

  slist_proxy(const slist_proxy &) = delete;

  slist_proxy(slist_proxy &&) = delete;

  constexpr slist_proxy(fwd_head_type &h)
      noexcept(std::is_nothrow_default_constructible_v<entry_extractor_type>)
      requires std::default_initializable<entry_extractor_type>
      : m_head{h} {}

  template<util::can_direct_initialize<entry_extractor_type> U>
  constexpr explicit slist_proxy(fwd_head_type &h, U &&u)
      noexcept(std::is_nothrow_constructible_v<entry_extractor_type, U>)
      : m_head{h}, m_entryExtractor{std::forward<U>(u)}
  {
  }

  constexpr slist_proxy(fwd_head_type &h, slist_proxy &&other)
      noexcept(std::is_nothrow_move_constructible_v<entry_extractor_type>)
      requires std::move_constructible<entry_extractor_type>
      : m_head{h}, m_entryExtractor{std::move(other.get_entry_extractor())} {
    m_head = std::move(other.getHeadData());
  }

  template <compatible_slist<slist_proxy> O>
  constexpr slist_proxy(fwd_head_type &h, O &&other)
      noexcept(std::is_nothrow_move_constructible_v<entry_extractor_type> &&
               (std::same_as<typename fwd_head_type::size_member_type, no_size>
                || noexcept(other.size())))
      requires std::move_constructible<entry_extractor_type>
      : m_head{h}, m_entryExtractor{std::move(other.get_entry_extractor())} {
    base_type::clear();
    if constexpr (std::integral<typename fwd_head_type::size_member_type>)
      m_head.swap_with(other.getHeadData(), other.size(), 0);
    else
      m_head.swap_with(other.getHeadData(), 0, 0);
  }

  template <std::input_iterator InputIt, std::sentinel_for<InputIt> Sentinel>
      requires std::default_initializable<entry_extractor_type> &&
               std::constructible_from<pointer, std::iter_reference_t<InputIt>>
  constexpr slist_proxy(fwd_head_type &h, InputIt first, Sentinel last)
      noexcept(std::is_nothrow_default_constructible_v<entry_extractor_type> &&
               noexcept(base_type::assign(first, last)))
      : m_head{h} {
    base_type::assign(first, last);
  }

  template <std::input_iterator InputIt, std::sentinel_for<InputIt> Sentinel,
            util::can_direct_initialize<entry_extractor_type> U>
      requires std::constructible_from<pointer, std::iter_reference_t<InputIt>>
  constexpr slist_proxy(fwd_head_type &h, InputIt first, Sentinel last, U &&u)
      noexcept(std::is_nothrow_constructible_v<entry_extractor_type, U> &&
               noexcept(base_type::assign(first, last)))
      : m_head{h}, m_entryExtractor{std::forward<U>(u)} {
    base_type::assign(first, last);
  }

  template <std::ranges::input_range Range>
      requires std::default_initializable<entry_extractor_type> &&
               std::constructible_from<pointer, std::ranges::range_reference_t<Range>>
  constexpr slist_proxy(fwd_head_type &h, Range &&r)
      noexcept(std::is_nothrow_default_constructible_v<entry_extractor_type> &&
               noexcept(base_type::assign(r)))
      : m_head{h} {
    base_type::assign(r);
  }

  template <std::ranges::input_range Range,
            util::can_direct_initialize<entry_extractor_type> U>
      requires std::constructible_from<pointer, std::ranges::range_reference_t<Range>>
  constexpr slist_proxy(fwd_head_type &h, Range &&r, U &&u)
      noexcept(std::is_nothrow_constructible_v<entry_extractor_type, U> &&
               noexcept(base_type::assign(r)))
      : m_head{h}, m_entryExtractor{std::forward<U>(u)} {
    base_type::assign(r);
  }

  constexpr slist_proxy(fwd_head_type &h, std::initializer_list<pointer> ilist)
      CSG_NOEXCEPT(std::is_nothrow_default_constructible_v<entry_extractor_type> &&
               noexcept(base_type::assign(ilist)))
      requires std::default_initializable<entry_extractor_type>
      : m_head{h} {
    base_type::assign(ilist);
  }

  template <util::can_direct_initialize<entry_extractor_type> U>
  constexpr slist_proxy(fwd_head_type &h, std::initializer_list<pointer> ilist,
                        U &&u)
      noexcept(std::is_nothrow_constructible_v<entry_extractor_type, U> &&
               noexcept(base_type::assign(ilist)))
      : m_head{h}, m_entryExtractor{std::forward<U>(u)} {
    base_type::assign(ilist);
  }

  ~slist_proxy() = default;

  slist_proxy &operator=(const slist_proxy &) = delete;

  constexpr slist_proxy &operator=(slist_proxy &&rhs)
      noexcept(std::is_nothrow_move_assignable_v<entry_extractor_type>) {
    m_head = std::move(rhs.getHeadData());
    m_entryExtractor = std::move(rhs.get_entry_extractor());
    return *this;
  }

  template <compatible_slist<slist_proxy> O>
  constexpr slist_proxy &operator=(O &&rhs)
      noexcept(std::is_nothrow_move_assignable_v<entry_extractor_type> &&
               (std::same_as<typename fwd_head_type::size_member_type, no_size>
		|| noexcept(rhs.size()))) {
    base_type::clear();
    if constexpr (std::integral<typename fwd_head_type::size_member_type>)
      m_head.swap_with(rhs.getHeadData(), rhs.size(), 0);
    else
      m_head.swap_with(rhs.getHeadData(), 0, 0);
    m_entryExtractor = std::move(rhs.get_entry_extractor());
    return *this;
  }

  template <std::ranges::input_range Range>
      requires std::constructible_from<pointer, std::ranges::range_reference_t<Range>>
  constexpr slist_proxy &operator=(Range &&r)
      noexcept(noexcept(base_type::assign(r))) {
    base_type::assign(r);
    return *this;
  }

  constexpr slist_proxy &operator=(std::initializer_list<pointer> ilist)
      CSG_NOEXCEPT(noexcept(base_type::assign(ilist))) {
    base_type::assign(ilist);
    return *this;
  }

private:
  template <typename T2, slist_entry_extractor<T2>, optional_size, typename>
  friend class slist_base;

  template <util::derived_from_template<slist_fwd_head> T2,
            slist_entry_extractor<typename T2::value_type>>
  friend class slist_proxy;

  template <typename T2, slist_entry_extractor<T2>, optional_size>
  friend class slist_head;

  constexpr fwd_head_type &getHeadData() noexcept { return m_head; }

  constexpr entry_extractor_type &getEntryExtractor() noexcept {
    return m_entryExtractor;
  }

  fwd_head_type &m_head;
  [[no_unique_address]] entry_extractor_type m_entryExtractor;
};

template <typename T, slist_entry_extractor<T> EntryEx,
          optional_size SizeMember>
class slist_head : public slist_base<T, EntryEx, SizeMember,
                                     slist_head<T, EntryEx, SizeMember>> {
  using fwd_head_type = slist_fwd_head<T, SizeMember>;
  using base_type = slist_base<T, EntryEx, SizeMember, slist_head>;

public:
  using pointer = CSG_TYPENAME base_type::pointer;
  using entry_extractor_type = EntryEx;

  template <optional_size S, typename D>
  using other_list_t = CSG_TYPENAME base_type::template other_list_t<S, D>;

  constexpr slist_head()
      requires std::default_initializable<entry_extractor_type> = default;

  slist_head(const slist_head &) = delete;

  constexpr slist_head(slist_head &&other)
      noexcept(std::is_nothrow_move_constructible_v<entry_extractor_type>)
      requires std::move_constructible<entry_extractor_type>
      : m_head{std::move(other.m_head)},
        m_entryExtractor{std::move(other.m_entryExtractor)} {}

  template <compatible_slist<slist_head> O>
  constexpr slist_head(O &&other)
      noexcept(std::is_nothrow_move_constructible_v<entry_extractor_type> &&
               (std::same_as<SizeMember, no_size> || noexcept(other.size())))
      : m_entryExtractor{std::move(other.get_entry_extractor())} {
    if constexpr (std::integral<SizeMember>)
      m_head.swap_with(other.getHeadData(), other.size(), 0);
    else
      m_head.swap_with(other.getHeadData(), 0, 0);
  }

  template <util::can_direct_initialize<entry_extractor_type> U>
  constexpr explicit slist_head(U &&u)
      noexcept(std::is_nothrow_constructible_v<entry_extractor_type, U>)
      : m_entryExtractor{std::forward<U>(u)} {}

  template <std::input_iterator InputIt, std::sentinel_for<InputIt> Sentinel>
      requires std::default_initializable<entry_extractor_type> &&
               std::constructible_from<pointer, std::iter_reference_t<InputIt>>
  constexpr slist_head(InputIt first, Sentinel last)
      noexcept(std::is_nothrow_default_constructible_v<entry_extractor_type> &&
               noexcept(base_type::assign(first, last))) {
    base_type::assign(first, last);
  }

  template <std::input_iterator InputIt, std::sentinel_for<InputIt> Sentinel,
            util::can_direct_initialize<entry_extractor_type> U>
      requires std::constructible_from<pointer, std::iter_reference_t<InputIt>>
  constexpr slist_head(InputIt first, Sentinel last, U &&u)
      noexcept(std::is_nothrow_constructible_v<entry_extractor_type, U> &&
               noexcept(base_type::assign(first, last)))
      : m_entryExtractor{std::forward<U>(u)} {
    base_type::assign(first, last);
  }

  template <std::ranges::input_range Range>
      requires std::default_initializable<entry_extractor_type> &&
          std::constructible_from<pointer, std::ranges::range_reference_t<Range>>
  constexpr slist_head(Range &&r)
      noexcept( std::is_nothrow_default_constructible_v<entry_extractor_type> &&
                noexcept(base_type::assign(r)) ) {
    base_type::assign(r);
  }

  template <std::ranges::input_range Range,
            util::can_direct_initialize<entry_extractor_type> U>
      requires std::constructible_from<pointer, std::ranges::range_reference_t<Range>>
  constexpr slist_head(Range &r, U &&u)
      noexcept(std::is_nothrow_constructible_v<entry_extractor_type, U> &&
               noexcept(base_type::assign(r)))
      : m_entryExtractor{std::forward<U>(u)} {
    base_type::assign(r);
  }

  constexpr slist_head(std::initializer_list<pointer> ilist)
      CSG_NOEXCEPT(std::is_nothrow_default_constructible_v<entry_extractor_type> &&
               noexcept(base_type::assign(ilist)))
      requires std::default_initializable<entry_extractor_type> {
    base_type::assign(ilist);
  }

  template <util::can_direct_initialize<entry_extractor_type> U>
  constexpr slist_head(std::initializer_list<pointer> ilist, U &&u)
      noexcept(std::is_nothrow_constructible_v<entry_extractor_type, U> &&
               noexcept(base_type::assign(ilist)))
      : m_entryExtractor{std::forward<U>(u)} {
    base_type::assign(ilist);
  }

  ~slist_head() = default;

  slist_head &operator=(const slist_head &) = delete;

  constexpr slist_head &operator=(slist_head &&rhs)
      noexcept(std::is_nothrow_move_assignable_v<entry_extractor_type>)
      requires std::is_move_assignable_v<entry_extractor_type> {
    m_head = std::move(rhs.m_head);
    m_entryExtractor = std::move(rhs.m_entryExtractor);
    return *this;
  }

  template <compatible_slist<slist_head> O>
  constexpr slist_head &operator=(O &&rhs)
      noexcept(std::is_nothrow_move_assignable_v<entry_extractor_type> &&
               (std::same_as<SizeMember, no_size> || noexcept(rhs.size())))
      requires std::is_move_assignable_v<entry_extractor_type> {
    base_type::clear();
    if constexpr (std::integral<SizeMember>)
      m_head.swap_with(rhs.getHeadData(), rhs.size(), 0);
    else
      m_head.swap_with(rhs.getHeadData(), 0, 0);
    m_entryExtractor = std::move(rhs.get_entry_extractor());
    return *this;
  }

  template <std::ranges::input_range Range>
      requires std::constructible_from<pointer, std::ranges::range_reference_t<Range>>
  constexpr slist_head &operator=(Range &&r)
      noexcept(noexcept(base_type::assign(r))) {
    base_type::assign(r);
    return *this;
  }

  constexpr slist_head &operator=(std::initializer_list<pointer> ilist)
      CSG_NOEXCEPT(noexcept(base_type::assign(ilist))) {
    base_type::assign(ilist);
    return *this;
  }

private:
  template <typename T2, slist_entry_extractor<T2>, optional_size, typename>
  friend class slist_base;

  template <util::derived_from_template<slist_fwd_head> T2,
            slist_entry_extractor<typename T2::value_type>>
  friend class slist_proxy;

  template <typename T2, slist_entry_extractor<T2>, optional_size>
  friend class slist_head;

  constexpr fwd_head_type &getHeadData() noexcept { return m_head; }

  constexpr entry_extractor_type &getEntryExtractor() noexcept {
    return m_entryExtractor;
  }

  fwd_head_type m_head;
  [[no_unique_address]] entry_extractor_type m_entryExtractor;
};

template <typename T, slist_entry_extractor<T> E, optional_size S, typename D>
constexpr CSG_TYPENAME slist_base<T, E, S, D>::iterator
slist_base<T, E, S, D>::before_begin() noexcept {
  auto *const headEntry = &getHeadData().m_headEntry;
  const auto u = entry_ref_codec::create_direct_entry_ref(headEntry);
  return {u, get_entry_extractor()};
}

template <typename T, slist_entry_extractor<T> E, optional_size S, typename D>
constexpr CSG_TYPENAME slist_base<T, E, S, D>::size_type
slist_base<T, E, S, D>::size() const
    noexcept(std::integral<S> || s_has_nothrow_extractor)
{
  if constexpr (std::same_as<S, no_size>)
    return static_cast<size_type>(std::ranges::distance(begin(), end()));
  else
    return getHeadData().m_sz;
}

template <typename T, slist_entry_extractor<T> E, optional_size S, typename D>
constexpr void slist_base<T, E, S, D>::clear() noexcept {
  auto &head = getHeadData();
  head.m_headEntry.next = nullptr;

  if constexpr (std::integral<S>)
    head.m_sz = 0;
}

template <typename T, slist_entry_extractor<T> E, optional_size S, typename D>
constexpr CSG_TYPENAME slist_base<T, E, S, D>::iterator
slist_base<T, E, S, D>::insert_after(const_iterator pos, pointer value)
    noexcept(s_has_nothrow_extractor)
{
  CSG_ASSERT(pos != end(), "end() iterator passed to insert_after");

  const entry_ref_type itemRef = entry_ref_codec::create_item_entry_ref(value);
  entry_type *const posEntry = iterToEntry(pos);
  entry_type *const insertEntry = refToEntry(itemRef);

  insertEntry->next = posEntry->next;
  posEntry->next = itemRef;

  if constexpr (std::integral<S>)
    ++getHeadData().m_sz;

  return {itemRef, get_entry_extractor()};
}

template <typename T, slist_entry_extractor<T> E, optional_size S, typename D>
template <std::input_iterator InputIt, std::sentinel_for<InputIt> Sentinel>
    requires std::constructible_from<T *, std::iter_reference_t<InputIt>>
constexpr CSG_TYPENAME slist_base<T, E, S, D>::iterator
slist_base<T, E, S, D>::insert_after(const_iterator pos, InputIt first,
    Sentinel last) noexcept(noexcept(*first++) && noexcept(first != last) &&
    s_has_nothrow_extractor)
{
  while (first != last)
    pos = insert_after(pos, *first++);

  return {pos.m_current, get_entry_extractor()};
}

template <typename T, slist_entry_extractor<T> E, optional_size S, typename D>
constexpr CSG_TYPENAME slist_base<T, E, S, D>::iterator
slist_base<T, E, S, D>::erase_after(const_iterator pos)
    noexcept(s_has_nothrow_extractor)
{
  CSG_ASSERT(pos != end(), "end() iterator passed to erase_after");

  entry_type *const posEntry = iterToEntry(pos);
  const bool isLastEntry = !posEntry->next;

  if (isLastEntry)
    return end();

  if constexpr (std::integral<S>)
    --getHeadData().m_sz;

  entry_type *const erasedEntry = refToEntry(posEntry->next);
  posEntry->next = erasedEntry->next;

  return {posEntry->next, get_entry_extractor()};
}

template <typename T, slist_entry_extractor<T> E, optional_size S, typename D>
constexpr CSG_TYPENAME slist_base<T, E, S, D>::iterator
slist_base<T, E, S, D>::erase_after(const_iterator first,
                                    const_iterator last)
    noexcept(s_has_nothrow_extractor) {
  if (first == end())
    return {first.m_current, get_entry_extractor()};

  // Adjust the inline size before modifying any entries; the element after
  // `first` will become unreachable once we edit the entries.
  if constexpr (std::integral<S>)
    getHeadData().m_sz -= std::ranges::distance(std::ranges::next(first), last);

  // Remove the open range (first, last) by linking first directly to last,
  // thereby removing all the internal elements.
  entry_type *const firstEntry = iterToEntry(first);
  firstEntry->next = last.m_current;

  return {last.m_current, get_entry_extractor()};
}

template <typename T, slist_entry_extractor<T> E, optional_size S, typename D>
template <optional_size S2, typename D2>
constexpr void slist_base<T, E, S, D>::swap(other_list_t<S2, D2> &other)
    noexcept(std::is_nothrow_swappable_v<entry_extractor_type> &&
             noexcept(size()) && noexcept(other.size())) {
  getHeadData().swap_with(other.getHeadData(), other.size(), size());
  std::ranges::swap(get_entry_extractor(), other.get_entry_extractor());
}

template <typename T, slist_entry_extractor<T> E, optional_size S, typename D>
constexpr CSG_TYPENAME slist_base<T, E, S, D>::iterator
slist_base<T, E, S, D>::find_predecessor(const_iterator scan,
                                         const_iterator last,
                                         const_iterator pos) const
                                         noexcept(s_has_nothrow_extractor) {
  while (scan != last) {
    if (auto prev = scan++; scan == pos)
      return {prev.m_current, get_entry_extractor_mutable()};
  }

  return {nullptr, get_entry_extractor_mutable()};
}

template <typename T, slist_entry_extractor<T> E, optional_size S, typename D>
// FIXME: when clang is given the qualified name
// `slist_base<T, E, S, D>::const_reference`, it doesn't think this declaration
// matches the original one in the class body. We must use the `const T &`
// form directly instead. This used to be the case in gcc also until recently,
// so presumably gcc is correct. For now, these #if 0 blocks appear in a bunch
// of places to make clang happy.
#if 0
template <std::invocable<typename slist_base<T, E, S, D>::const_reference> Proj,
          std::predicate<std::invoke_result_t<Proj,
              typename slist_base<T, E, S, D>::const_reference>> Pred>
#else
template <std::invocable<const T&> Proj,
          std::predicate<std::invoke_result_t<Proj, const T&>> Pred>
#endif	      
constexpr std::pair<typename slist_base<T, E, S, D>::iterator, bool>
slist_base<T, E, S, D>::find_predecessor_if(const_iterator prev,
                                            const_iterator last,
                                            Pred pred, Proj proj) const
    noexcept(s_has_nothrow_extractor &&
             util::is_nothrow_proj_invocable<const_reference, Proj, Pred>)
{
  if (prev == last)
    return {iterator{nullptr, get_entry_extractor_mutable()}, false};

  const_iterator scan = std::ranges::next(prev);

  while (scan != last) {
    if (std::invoke(pred, std::invoke(proj, *scan)))
      return {iterator{prev.m_current, get_entry_extractor_mutable()}, true};
    prev = scan++;
  }

  return {iterator{prev.m_current, get_entry_extractor_mutable()}, false};
}

template <typename T, slist_entry_extractor <T> E, optional_size S1, typename D1>
#if 0
template <optional_size S2, typename D2,
          std::invocable<typename slist_base<T, E, S1, D1>::const_reference> Proj,
          std::strict_weak_order<
            std::invoke_result_t<Proj,
                typename slist_base<T, E, S1, D1>::const_reference>,
            std::invoke_result_t<Proj,
                typename slist_base<T, E, S1, D1>::const_reference>
          > Compare>
#else
template <optional_size S2, typename D2,
          std::invocable<const T&> Proj,
          std::strict_weak_order<
            std::invoke_result_t<Proj, const T&>,
            std::invoke_result_t<Proj, const T&>
          > Compare>
#endif
constexpr void slist_base<T, E, S1, D1>::merge(other_list_t<S2, D2> &other,
                                               Compare comp, Proj proj)
    noexcept(s_has_nothrow_extractor &&
             util::is_nothrow_proj_relation<const_reference, Proj, Compare>)
{
  if (this == &other)
    return;

  // The range [f1, e1) represents the range that has been merged so far. As
  // the algorithm progresses, f1 will move. Originally [f1, e1) is just equal
  // to the current list. p1 is the predecessor to f1, which we need to keep
  // track of to call insert_range_after.
  auto p1 = cbefore_begin();
  auto f1 = std::ranges::next(p1);
  auto e1 = cend();

  // [f2, e1) represents the range of items in the other list that haven't
  // been merged yet. It will eventually be empty.
  auto f2 = other.cbegin();
  auto e2 = other.cend();

  if constexpr (std::integral<S1>)
    getHeadData().m_sz += std::size(other);

  while (f1 != e1 && f2 != e2) {
    // While *f1 is less than *f2, keep advancing *f1.
    if (util::projection_is_ordered_before(proj, comp, f1, f2)) {
      p1 = f1++;
      continue;
    }

    // Scan the range of items where *f2 < *f1. When we're done, [f2, mergeEnd]
    // will be the closed range of elements that needs to merged after p1
    // (before f1).
    auto mergeEnd = f2;

    for (auto scan = std::ranges::next(mergeEnd); scan != e2 &&
         util::projection_is_ordered_before(proj, comp, scan, f1); ++scan)
      mergeEnd = scan;

    f2 = insert_range_after(p1, f2, mergeEnd);

    p1 = mergeEnd;
    f1 = std::ranges::next(mergeEnd);
  }

  if (f2 != e2) {
    // Merge the remaining range, [f2, e2), at the end of the list.
    iterToEntry(p1)->next = f2.m_current;
  }

  other.clear();
}

template <typename T, slist_entry_extractor<T> E, optional_size S1, typename D1>
template <optional_size S2, typename D2>
constexpr void slist_base<T, E, S1, D1>::splice_after(
    const_iterator pos, other_list_t<S2, D2> &other)
    noexcept(s_has_nothrow_extractor)
{
  if (other.empty())
    return;

  CSG_ASSERT(pos.m_current, "end() iterator passed as pos");

  iterToEntry(pos)->next = other.begin().m_current;

  if constexpr (std::integral<S1>)
    getHeadData().m_sz += std::size(other);

  other.clear();
}

template <typename T, slist_entry_extractor <T> E, optional_size S1, typename D1>
template <optional_size S2, typename D2>
constexpr void slist_base<T, E, S1, D1>::splice_after(
    const_iterator pos, other_list_t<S2, D2> &other,
    typename other_list_t<S2, D2>::const_iterator first,
    typename other_list_t<S2, D2>::const_iterator last)
    noexcept(s_has_nothrow_extractor)
{
  if (first == last)
    return;

  // When the above is false, first++ must be legal.
  CSG_ASSERT(first.m_current, "first is end() but last was not end()?");

  // Remove the open range (first, last) from `other`, by directly linking
  // first to last. Also post-increment `first`, so that it will point to the
  // start of the closed range [first + 1, last - 1] that we're inserting.
  other.iterToEntry(first++)->next = last.m_current;

  if (first == last)
    return;

  // Find the last element in the closed range we're inserting, then use
  // insert_range_after to insert the closed range after pos. The size
  // count (sz) starts at one because the closed range is definitely not
  // empty or we would have returned in the above statement.
  auto lastInsert = first;
  std::common_type_t<difference_type, typename D2::difference_type> sz = 1;

  for (auto scan = std::ranges::next(lastInsert); scan != last; ++scan, ++sz)
    lastInsert = scan;

  if constexpr (std::integral<S1>)
    getHeadData().m_sz += sz;

  if constexpr (std::integral<S2>)
    other.getHeadData().m_sz -= sz;

  insert_range_after(pos, first, lastInsert);
}

template <typename T, slist_entry_extractor<T> E, optional_size S, typename D>
#if 0
template <std::invocable<typename slist_base<T, E, S, D>::const_reference> Proj,
          std::predicate<std::invoke_result_t<Proj,
              typename slist_base<T, E, S, D>::const_reference>> Pred>
#else
template <std::invocable<const T&> Proj,
          std::predicate<std::invoke_result_t<Proj, const T&>> Pred>
#endif
constexpr CSG_TYPENAME slist_base<T, E, S, D>::size_type
slist_base<T, E, S, D>::remove_if(Pred pred, Proj proj)
    noexcept(s_has_nothrow_extractor &&
             util::is_nothrow_proj_invocable<const_reference, Proj, Pred>) {
  size_type nRemoved = 0;
  const_iterator prev = cbefore_begin();
  const_iterator i = std::ranges::next(prev);
  const const_iterator end = cend();

  while (i != end) {
    if (!std::invoke(pred, std::invoke(proj, *i))) {
      // Not removing i, advance to the next element and restart.
      prev = i++;
      continue;
    }

    // We need to remove i. It is slightly more efficient to bulk remove a
    // range of elements if we have contiguous sequences where the predicate
    // matches. Build the open range (prev, i), where all contained elements
    // are to be removed.
    ++i;
    ++nRemoved;

    while (i != end && std::invoke(pred, std::invoke(proj, *i))) {
      ++i;
      ++nRemoved;
    }

    prev = erase_after(prev, i);
    i = (prev != end) ? std::ranges::next(prev) : end;
  }

  return nRemoved;
}

template <typename T, slist_entry_extractor<T> E, optional_size S, typename D>
constexpr void slist_base<T, E, S, D>::reverse() noexcept(s_has_nothrow_extractor) {
  const const_iterator end = cend();
  const_iterator i = cbegin();
  const_iterator prev = end;

  while (i != end) {
    const auto current = i;
    entry_type *const entry = iterToEntry(i++);
    entry->next = prev.m_current;
    prev = current;
  }

  getHeadData().m_headEntry.next = prev.m_current;
}

template <typename T, slist_entry_extractor<T> E, optional_size S, typename D>
#if 0
template <std::invocable<typename slist_base<T, E, S, D>::const_reference> Proj,
          std::equivalence_relation<
            std::invoke_result_t<Proj,
                typename slist_base<T, E, S, D>::const_reference>,
            std::invoke_result_t<Proj,
                typename slist_base<T, E, S, D>::const_reference>
          > EqRelation>
#else
template <std::invocable<const T&> Proj,
          std::equivalence_relation<
            std::invoke_result_t<Proj, const T&>,
            std::invoke_result_t<Proj, const T&>
          > EqRelation>
#endif
constexpr void slist_base<T, E, S, D>::unique(EqRelation eq, Proj proj)
    noexcept(s_has_nothrow_extractor &&
             util::is_nothrow_proj_relation<const_reference, Proj, EqRelation>)
{
  if (empty())
    return;

  const_iterator prev = cbegin();
  const_iterator i = std::ranges::next(prev);
  const const_iterator end = cend();

  while (i != end) {
    if (!util::projections_are_equivalent(proj, eq, prev, i)) {
      // Adjacent items are distinct, keep scanning.
      prev = i++;
      continue;
    }

    // `i` is the start of a list of duplicates. Scan for the end of the
    // duplicate range and erase the open range (prev, scanEnd)
    auto scanEnd = std::ranges::next(i);
    while (scanEnd != end &&
           util::projections_are_equivalent(proj, eq, prev, scanEnd))
      ++scanEnd;

    prev = erase_after(prev, scanEnd);
    i = (prev != end) ? std::ranges::next(prev) : end;
  }
}

template <typename T, slist_entry_extractor<T> E, optional_size S, typename D>
#if 0
template <std::invocable<typename slist_base<T, E, S, D>::const_reference> Proj,
          std::strict_weak_order<
            std::invoke_result_t<Proj,
                typename slist_base<T, E, S, D>::const_reference>,
            std::invoke_result_t<Proj,
                typename slist_base<T, E, S, D>::const_reference>
          > Compare>
#else
template <std::invocable<const T&> Proj,
          std::strict_weak_order<
            std::invoke_result_t<Proj, const T&>,
            std::invoke_result_t<Proj, const T&>
          > Compare>
#endif
constexpr void slist_base<T, E, S, D>::sort(Compare comp, Proj proj)
    noexcept(s_has_nothrow_extractor &&
             util::is_nothrow_proj_relation<const_reference, Proj, Compare>) {
  detail::forward_list_merge_sort<slist_base<T, E, S, D>>(cbefore_begin(), cend(),
      std::ref(comp), std::ref(proj), std::size(*this));
}

template <typename T, slist_entry_extractor<T> E, optional_size S, typename D>
template <typename QueueIt>
constexpr CSG_TYPENAME slist_base<T, E, S, D>::iterator
slist_base<T, E, S, D>::insert_range_after(const_iterator pos, QueueIt first,
                                           QueueIt last)
    noexcept(s_has_nothrow_extractor &&
             QueueIt::container::s_has_nothrow_extractor)
{
  // Inserts the closed range [first, last] after pos and returns
  // the successor to last.
  CSG_ASSERT(pos.m_current && last.m_current,
             "end() iterator passed as pos or last");

  entry_type *const posEntry = iterToEntry(pos);
  entry_type *const lastEntry = QueueIt::container::iterToEntry(last);

  const auto oldNext = lastEntry->next;

  lastEntry->next = posEntry->next;
  posEntry->next = first.m_current;

  return iterator{oldNext, last.m_rEntryExtractor.get_invocable()};
}

} // End of namespace csg

#endif
