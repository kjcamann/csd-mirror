//==-- csg/core/stailq.h - singly-linked tail queue impl. -------*- C++ -*-==//
//
//                Cyril Software Data Structures (CSD) Library
//
// This file is distributed under the 2-clause BSD Open Source License. See
// LICENSE.TXT for details.
//==-----------------------------------------------------------------------==//

/**
 * @file
 * @brief Contains an STL-compatible implementation of singly-linked intrusive
 *     tail queues, inspired by BSD's queue(3) STAILQ_ macros.
 */

#ifndef CSG_CORE_STAILQ_H
#define CSG_CORE_STAILQ_H

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
struct stailq_entry {
  entry_ref_union<stailq_entry, T> next;
};

template <typename T, stailq_entry_extractor<T> EntryEx,
          optional_size SizeMember, typename Derived>
class stailq_base;

template <typename T, typename O>
concept compatible_stailq = stailq<T> &&
    std::same_as<typename T::value_type, typename O::value_type> &&
    std::same_as<typename T::entry_extractor_type, typename O::entry_extractor_type>;

template <typename T, optional_size SizeMember>
class stailq_fwd_head {
public:
  using value_type = T;
  using size_type = std::conditional_t<std::same_as<SizeMember, no_size>,
                                       std::size_t, SizeMember>;

  constexpr stailq_fwd_head() noexcept : m_headEntry{nullptr}, m_sz{} {
    m_encodedTail.offset = &m_headEntry;
  }

  stailq_fwd_head(const stailq_fwd_head &) = delete;

  constexpr stailq_fwd_head(stailq_fwd_head &&other) noexcept : stailq_fwd_head{} {
    swap(other);
  }

  constexpr void swap(stailq_fwd_head &other) noexcept {
    std::ranges::swap(m_headEntry, other.m_headEntry);
    std::ranges::swap(m_sz, other.m_sz);

    const auto oldEncodedTail = m_encodedTail;

    if (!m_headEntry.next)
      m_encodedTail.offset = &m_headEntry;
    else
      m_encodedTail = other.m_encodedTail;

    if (!other.m_headEntry.next)
      other.m_encodedTail.offset = &other.m_headEntry;
    else
      other.m_encodedTail = oldEncodedTail;
  }

  ~stailq_fwd_head() = default;

  stailq_fwd_head &operator=(const stailq_fwd_head &) = delete;

  constexpr stailq_fwd_head &operator=(stailq_fwd_head &&other) noexcept {
    return *new(this) stailq_fwd_head{std::move(other)};
  }

private:
  template <typename, optional_size>
  friend class stailq_fwd_head;

  template <typename T2, stailq_entry_extractor<T2>, optional_size, typename>
  friend class stailq_base;

  template <util::derived_from_template<stailq_fwd_head> FH2,
            stailq_entry_extractor<typename FH2::value_type>>
  friend class stailq_proxy;

  template <typename T2, stailq_entry_extractor<T2>, optional_size>
  friend class stailq_head;

  using size_member_type = SizeMember;

  template <optional_size S2>
  constexpr void swap_with(stailq_fwd_head<T, S2> &other,
                           CSG_TYPENAME stailq_fwd_head<T, S2>::size_type otherSize,
                           size_type ourSize) noexcept {
    std::ranges::swap(m_headEntry, other.m_headEntry);

    const auto oldEncodedTail = m_encodedTail;

    if (!m_headEntry.next)
      m_encodedTail.offset = &m_headEntry;
    else
      m_encodedTail = other.m_encodedTail;

    if (!other.m_headEntry.next)
      other.m_encodedTail.offset = &other.m_headEntry;
    else
      other.m_encodedTail = oldEncodedTail;

    if constexpr (std::integral<size_member_type>)
      m_sz = static_cast<size_type>(otherSize);
    if constexpr (std::integral<S2>)
      other.m_sz = static_cast<S2>(ourSize);
  }

  stailq_entry<T> m_headEntry;
  entry_ref_union<stailq_entry<T>, T> m_encodedTail;
  [[no_unique_address]] SizeMember m_sz;
};

template <typename T, stailq_entry_extractor<T> EntryEx,
          optional_size SizeMember, typename Derived>
class stailq_base {
protected:
  constexpr static bool s_has_nothrow_extractor =
      std::is_nothrow_invocable_v<EntryEx, T &>;

public:
  using value_type = T;
  using reference = T &;
  using const_reference = const T &;
  using pointer = T *;
  using const_pointer = const T *;
  using size_type = CSG_TYPENAME stailq_fwd_head<SizeMember>::size_type;
  using difference_type = std::make_signed_t<size_type>;
  using entry_type = stailq_entry<T>;
  using entry_extractor_type = EntryEx;
  using size_member_type = SizeMember;

  template <optional_size S, typename D>
  using other_list_t = stailq_base<T, EntryEx, S, D>;

  constexpr entry_extractor_type &get_entry_extractor() noexcept {
    return static_cast<Derived *>(this)->getEntryExtractor();
  }

  constexpr const entry_extractor_type &get_entry_extractor() const noexcept {
    return const_cast<stailq_base *>(this)->get_entry_extractor();
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

  reference back() noexcept { return *before_end(); }

  const_reference back() const noexcept { return *before_end(); }

  class iterator;
  class const_iterator;

  using iterator_t = std::type_identity_t<iterator>;
  using const_iterator_t = std::type_identity_t<const_iterator>;

  constexpr iterator before_begin() noexcept;

  constexpr const_iterator before_begin() const noexcept {
    return const_cast<stailq_base *>(this)->before_begin();
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

  constexpr iterator before_end() noexcept {
    return {getHeadData().m_encodedTail, get_entry_extractor()};
  }

  constexpr const_iterator before_end() const noexcept {
    return {getHeadData().m_encodedTail, get_entry_extractor_mutable()};
  }

  constexpr const_iterator cbefore_end() const noexcept { return before_end(); }

  constexpr iterator end() noexcept {
    return {nullptr, get_entry_extractor()};
  }

  constexpr const_iterator end() const noexcept {
    return {nullptr, get_entry_extractor_mutable()};
  }

  constexpr const_iterator cend() const noexcept { return end(); }

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
  constexpr iterator insert_after(const_iterator_t pos, Range &r)
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

  void push_back(pointer p) noexcept { insert_after(cbefore_end(), p); }

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
                      Pred pred, Proj = {}) const
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
    merge(other, comp);
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
      noexcept(noexcept(splice_after(pos, other, it))) {
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

  constexpr void unique() noexcept(noexcept(unique(std::equal_to<T>{}))) {
    unique(std::equal_to<T>{});
  }

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
  template <typename T2, stailq_entry_extractor<T2>, optional_size, typename>
  friend class stailq_base;

  template <util::derived_from_template<stailq_fwd_head> T2,
            stailq_entry_extractor<typename T2::value_type>>
  friend class stailq_proxy;

  template <typename T2, stailq_entry_extractor<T2>, optional_size>
  friend class stailq_head;

  template <typename L, typename C, typename P, std::unsigned_integral S>
  friend CSG_TYPENAME L::const_iterator
      detail::forward_list_merge_sort(CSG_TYPENAME L::const_iterator,
                                      CSG_TYPENAME L::const_iterator,
                                      C, P, S) noexcept;

  using entry_ref_codec = detail::entry_ref_codec<entry_type, T, EntryEx>;
  using entry_ref_type = entry_ref_union<entry_type, T>;

  constexpr stailq_fwd_head<T, SizeMember> &getHeadData() noexcept {
    return static_cast<Derived *>(this)->getHeadData();
  }

  constexpr const stailq_fwd_head<T, SizeMember> &getHeadData() const noexcept {
    return const_cast<stailq_base *>(this)->getHeadData();
  }

  constexpr entry_extractor_type &get_entry_extractor_mutable() const noexcept {
    return const_cast<stailq_base *>(this)->get_entry_extractor();
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

template <typename T, stailq_entry_extractor<T> EntryEx,
          optional_size SizeMember, typename Derived>
class stailq_base<T, EntryEx, SizeMember, Derived>::iterator {
public:
  using value_type = stailq_base::value_type;
  using reference = stailq_base::reference;
  using pointer = stailq_base::pointer;
  using difference_type = stailq_base::difference_type;
  using iterator_category = std::forward_iterator_tag;
  using invocable_ref = compressed_invocable_ref<EntryEx, reference>;

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
    return stailq_base::entry_ref_codec::get_value(m_current);
  }

  constexpr pointer operator->() const noexcept {
    return std::addressof(this->operator*());
  }

  constexpr iterator &operator++() noexcept(s_has_nothrow_extractor) {
    m_current = stailq_base::iterToEntry(*this)->next;
    return *this;
  }

  constexpr iterator operator++(int) noexcept(s_has_nothrow_extractor) {
    const iterator i{*this};
    this->operator++();
    return i;
  }

  constexpr bool operator==(const iterator &rhs) const noexcept {
    return m_current == rhs.m_current;
  }

  constexpr bool operator==(const const_iterator &rhs) const noexcept {
    return m_current == rhs.m_current;
  }

private:
  template <typename T2, stailq_entry_extractor<T2>, optional_size, typename>
  friend class stailq_base;

  friend stailq_base::const_iterator;

  using container = stailq_base;

  constexpr iterator(entry_ref_type ref, entry_extractor_type &fn) noexcept
      : m_current{ref}, m_rEntryExtractor{fn} {}

  entry_ref_type m_current;
  [[no_unique_address]] invocable_ref m_rEntryExtractor;
};

template <typename T, stailq_entry_extractor<T> EntryEx,
          optional_size SizeMember, typename Derived>
class stailq_base<T, EntryEx, SizeMember, Derived>::const_iterator {
public:
  using value_type = stailq_base::value_type;
  using reference = stailq_base::const_reference;
  using pointer = stailq_base::const_pointer;
  using difference_type = stailq_base::difference_type;
  using iterator_category = std::forward_iterator_tag;
  using invocable_ref = compressed_invocable_ref<EntryEx, stailq_base::reference>;

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
    return stailq_base::entry_ref_codec::get_value(m_current);
  }

  constexpr pointer operator->() const noexcept {
    return std::addressof(this->operator*());
  }

  constexpr const_iterator &operator++() noexcept(s_has_nothrow_extractor) {
    m_current = stailq_base::iterToEntry(*this)->next;
    return *this;
  }

  constexpr const_iterator operator++(int) noexcept(s_has_nothrow_extractor) {
    const const_iterator i{*this};
    this->operator++();
    return i;
  }

  constexpr bool operator==(const iterator &rhs) const noexcept {
    return m_current == rhs.m_current;
  }

  constexpr bool operator==(const const_iterator &rhs) const noexcept {
    return m_current == rhs.m_current;
  }

private:
  template <typename T2, stailq_entry_extractor<T2>, optional_size, typename>
  friend class stailq_base;

  friend stailq_base::iterator;

  using container = stailq_base;

  constexpr const_iterator(entry_ref_type ref, entry_extractor_type &fn) noexcept
      : m_current{ref}, m_rEntryExtractor{fn} {}

  entry_ref_type m_current;
  [[no_unique_address]] invocable_ref m_rEntryExtractor;
};

template <util::derived_from_template<stailq_fwd_head> FwdHead,
          stailq_entry_extractor<typename FwdHead::value_type> EntryEx>
class stailq_proxy : public stailq_base<typename FwdHead::value_type, EntryEx,
    typename FwdHead::size_member_type, stailq_proxy<FwdHead, EntryEx>> {
  using size_member_type = CSG_TYPENAME FwdHead::size_member_type;
  using base_type = stailq_base<typename FwdHead::value_type, EntryEx,
                                size_member_type, stailq_proxy>;

public:
  using fwd_head_type = FwdHead;
  using pointer = CSG_TYPENAME base_type::pointer;
  using entry_extractor_type = EntryEx;

  template <optional_size S, typename D>
  using other_list_t = CSG_TYPENAME base_type::template other_list_t<S, D>;

  stailq_proxy() = delete;

  stailq_proxy(const stailq_proxy &) = delete;

  stailq_proxy(stailq_proxy &&) = delete;

  constexpr stailq_proxy(fwd_head_type &h)
      noexcept(std::is_nothrow_default_constructible_v<entry_extractor_type>)
      requires std::default_initializable<entry_extractor_type>
      : m_head{h} {}

  template <util::can_direct_initialize<entry_extractor_type> U>
  constexpr explicit stailq_proxy(fwd_head_type &h, U &&u)
      noexcept(std::is_nothrow_constructible_v<entry_extractor_type, U>)
      : m_head{h}, m_entryExtractor{std::forward<U>(u)} {}

  constexpr stailq_proxy(fwd_head_type &h, stailq_proxy &&other)
      noexcept(std::is_nothrow_move_constructible_v<entry_extractor_type>)
      requires std::move_constructible<entry_extractor_type>
      : m_head{h}, m_entryExtractor{std::move(other.get_entry_extractor())} {
    m_head = std::move(other.getHeadData());
  }

  template <compatible_stailq<stailq_proxy> O>
  constexpr stailq_proxy(fwd_head_type &h, O &&other)
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
      requires std::constructible_from<pointer, std::iter_reference_t<InputIt>>
  constexpr stailq_proxy(fwd_head_type &h, InputIt first, Sentinel last)
      noexcept(std::is_nothrow_default_constructible_v<entry_extractor_type> &&
               noexcept(base_type::assign(first, last)))
      : m_head{h} {
    base_type::assign(first, last);
  }

  template <std::input_iterator InputIt, std::sentinel_for<InputIt> Sentinel,
            util::can_direct_initialize<entry_extractor_type> U>
      requires std::constructible_from<pointer, std::iter_reference_t<InputIt>>
  constexpr stailq_proxy(fwd_head_type &h, InputIt first, Sentinel last, U &&u)
      noexcept(std::is_nothrow_constructible_v<entry_extractor_type, U> &&
               noexcept(base_type::assign(first, last)))
      : m_head{h}, m_entryExtractor{std::forward<U>(u)} {
    base_type::assign(first, last);
  }

  template <std::ranges::input_range Range>
      requires std::default_initializable<entry_extractor_type> &&
               std::constructible_from<pointer, std::ranges::range_reference_t<Range>>
  constexpr stailq_proxy(fwd_head_type &h, Range &&r)
      noexcept(std::is_nothrow_default_constructible_v<entry_extractor_type> &&
               noexcept(base_type::assign(r)))
      : m_head{h} {
    base_type::assign(r);
  }

  template <std::ranges::input_range Range,
            util::can_direct_initialize<entry_extractor_type> U>
      requires std::constructible_from<pointer, std::ranges::range_reference_t<Range>>
  constexpr stailq_proxy(fwd_head_type &h, Range &&r, U &&u)
      noexcept(std::is_nothrow_constructible_v<entry_extractor_type, U> &&
               noexcept(base_type::assign(r)))
      : m_head{h}, m_entryExtractor{std::forward<U>(u)} {
    base_type::assign(r);
  }

  constexpr stailq_proxy(fwd_head_type &h, std::initializer_list<pointer> ilist)
      CSG_NOEXCEPT(std::is_nothrow_default_constructible_v<entry_extractor_type> &&
               noexcept(base_type::assign(ilist)))
      requires std::default_initializable<entry_extractor_type>
      : m_head{h} {
    base_type::assign(ilist);
  }

  template <util::can_direct_initialize<entry_extractor_type> U>
  constexpr stailq_proxy(fwd_head_type &h, std::initializer_list<pointer> ilist,
                         U &&u)
      noexcept(std::is_nothrow_constructible_v<entry_extractor_type, U> &&
               noexcept(base_type::assign(ilist)))
      : m_head{h}, m_entryExtractor{std::forward<U>(u)} {
    base_type::assign(ilist);
  }

  ~stailq_proxy() = default;

  stailq_proxy &operator=(const stailq_proxy &) = delete;

  constexpr stailq_proxy &operator=(stailq_proxy &&rhs)
      noexcept(std::is_nothrow_move_assignable_v<entry_extractor_type>) {
    m_head = std::move(rhs.getHeadData());
    m_entryExtractor = std::move(rhs.get_entry_extractor());
    return *this;
  }

  template <compatible_stailq<stailq_proxy> O>
  constexpr stailq_proxy &operator=(O &&rhs)
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
  constexpr stailq_proxy &operator=(Range &&r)
      noexcept(noexcept(base_type::assign(r))) {
    base_type::assign(r);
    return *this;
  }

  constexpr stailq_proxy &operator=(std::initializer_list<pointer> ilist)
      CSG_NOEXCEPT(noexcept(base_type::assign(ilist))) {
    base_type::assign(ilist);
    return *this;
  }

private:
  template <typename T2, stailq_entry_extractor<T2>, optional_size, typename>
  friend class stailq_base;

  template <util::derived_from_template<stailq_fwd_head> T2,
            stailq_entry_extractor<typename T2::value_type>>
  friend class stailq_proxy;

  template <typename T2, stailq_entry_extractor<T2>, optional_size>
  friend class stailq_head;

  constexpr fwd_head_type &getHeadData() noexcept { return m_head; }

  constexpr entry_extractor_type &getEntryExtractor() noexcept {
    return m_entryExtractor;
  }

  fwd_head_type &m_head;
  [[no_unique_address]] entry_extractor_type m_entryExtractor;
};

template <typename T, stailq_entry_extractor<T> EntryEx,
          optional_size SizeMember>
class stailq_head : public stailq_base<T, EntryEx, SizeMember,
                                       stailq_head<T, EntryEx, SizeMember>> {
  using fwd_head_type = stailq_fwd_head<T, SizeMember>;
  using base_type = stailq_base<T, EntryEx, SizeMember, stailq_head>;

public:
  using pointer = CSG_TYPENAME base_type::pointer;
  using entry_extractor_type = EntryEx;

  template <optional_size S, typename D>
  using other_list_t = CSG_TYPENAME base_type::template other_list_t<S, D>;

  constexpr stailq_head()
      requires std::default_initializable<entry_extractor_type> = default;

  stailq_head(const stailq_head &) = delete;

  constexpr stailq_head(stailq_head &&other)
      noexcept(std::is_nothrow_move_constructible_v<entry_extractor_type>)
      requires std::move_constructible<entry_extractor_type>
      : m_head{std::move(other.m_head)},
        m_entryExtractor{std::move(other.m_entryExtractor)} {}

  template <compatible_stailq<stailq_head> O>
  constexpr stailq_head(O &&other)
      noexcept(std::is_nothrow_move_constructible_v<entry_extractor_type> &&
               (std::same_as<SizeMember, no_size> || noexcept(other.size())))
      : m_entryExtractor{std::move(other.get_entry_extractor())} {
    if constexpr (std::integral<SizeMember>)
      m_head.swap_with(other.getHeadData(), other.size(), 0);
    else
      m_head.swap_with(other.getHeadData(), 0, 0);
  }

  template <util::can_direct_initialize<entry_extractor_type> U>
  constexpr explicit stailq_head(U &&u)
      noexcept(std::is_nothrow_constructible_v<entry_extractor_type, U>)
      : m_entryExtractor{std::forward<U>(u)} {}

  template <std::input_iterator InputIt, std::sentinel_for<InputIt> Sentinel>
      requires std::default_initializable<entry_extractor_type> &&
          std::constructible_from<pointer, std::iter_reference_t<InputIt>>
  constexpr stailq_head(InputIt first, Sentinel last)
      noexcept(std::is_nothrow_default_constructible_v<entry_extractor_type> &&
               noexcept(base_type::assign(first, last))) {
    base_type::assign(first, last);
  }

  template <std::input_iterator InputIt, std::sentinel_for<InputIt> Sentinel,
            util::can_direct_initialize<entry_extractor_type> U>
      requires std::constructible_from<entry_extractor_type, U> &&
          std::constructible_from<pointer, std::iter_reference_t<InputIt>>
  constexpr stailq_head(InputIt first, Sentinel last, U &&u)
      noexcept(std::is_nothrow_constructible_v<entry_extractor_type, U> &&
               noexcept(base_type::assign(first, last)))
      : m_entryExtractor{std::forward<U>(u)} {
    base_type::assign(first, last);
  }

  template <std::ranges::input_range Range>
      requires std::default_initializable<entry_extractor_type> &&
          std::constructible_from<pointer, std::ranges::range_reference_t<Range>>
  constexpr stailq_head(Range &&r)
      noexcept(std::is_nothrow_default_constructible_v<entry_extractor_type> &&
               noexcept(base_type::assign(r))) {
    base_type::assign(r);
  }

  template <std::ranges::input_range Range,
            util::can_direct_initialize<entry_extractor_type> U>
      requires std::constructible_from<entry_extractor_type, U> &&
          std::constructible_from<pointer, std::ranges::range_reference_t<Range>>
  constexpr stailq_head(Range &&r, U &&u)
      noexcept(std::is_nothrow_constructible_v<entry_extractor_type, U> &&
               noexcept(base_type::assign(r)))
      : m_entryExtractor{std::forward<U>(u)} {
    base_type::assign(r);
  }

  constexpr stailq_head(std::initializer_list<pointer> ilist)
      CSG_NOEXCEPT(std::is_nothrow_default_constructible_v<entry_extractor_type> &&
               noexcept(base_type::assign(ilist)))
      requires std::default_initializable<entry_extractor_type> {
    base_type::assign(ilist);
  }

  template <util::can_direct_initialize<entry_extractor_type> U>
  constexpr stailq_head(std::initializer_list<pointer> ilist, U &&u)
      noexcept(std::is_nothrow_constructible_v<entry_extractor_type, U> &&
               noexcept(base_type::assign(ilist)))
      : m_entryExtractor{std::forward<U>(u)} {
    base_type::assign(ilist);
  }

  ~stailq_head() = default;

  stailq_head &operator=(const stailq_head &) = delete;

  constexpr stailq_head &operator=(stailq_head &&rhs)
      noexcept(std::is_nothrow_move_assignable_v<entry_extractor_type>)
      requires std::assignable_from<entry_extractor_type &, entry_extractor_type &&> {
    m_head = std::move(rhs.m_head);
    m_entryExtractor = std::move(rhs.m_entryExtractor);
    return *this;
  }

  template <compatible_stailq<stailq_head> O>
  stailq_head &operator=(O &&rhs)
      noexcept(std::is_nothrow_move_constructible_v<entry_extractor_type> &&
               (std::same_as<SizeMember, no_size> || noexcept(rhs.size())))
      requires std::assignable_from<entry_extractor_type &, entry_extractor_type &&> {
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
  constexpr stailq_head &operator=(Range &&r)
      noexcept(noexcept(base_type::assign(r))) {
    base_type::assign(r);
    return *this;
  }

  constexpr stailq_head &operator=(std::initializer_list<pointer> ilist)
      CSG_NOEXCEPT(noexcept(base_type::assign(ilist))) {
    base_type::assign(ilist);
    return *this;
  }

private:
  template <typename T2, stailq_entry_extractor<T2>, optional_size, typename>
  friend class stailq_base;

  template <util::derived_from_template<stailq_fwd_head> T2,
            stailq_entry_extractor<typename T2::value_type>>
  friend class stailq_proxy;

  template <typename T2, stailq_entry_extractor<T2>, optional_size>
  friend class stailq_head;

  constexpr fwd_head_type &getHeadData() noexcept { return m_head; }

  constexpr entry_extractor_type &getEntryExtractor() noexcept {
    return m_entryExtractor;
  }

  fwd_head_type m_head;
  [[no_unique_address]] entry_extractor_type m_entryExtractor;
};

template <typename T, stailq_entry_extractor<T> E, optional_size S, typename D>
constexpr CSG_TYPENAME stailq_base<T, E, S, D>::iterator
stailq_base<T, E, S, D>::before_begin() noexcept {
  auto *const headEntry = &getHeadData().m_headEntry;
  const auto u = entry_ref_codec::create_direct_entry_ref(headEntry);
  return {u, get_entry_extractor()};
}

template <typename T, stailq_entry_extractor<T> E, optional_size S, typename D>
constexpr CSG_TYPENAME stailq_base<T, E, S, D>::size_type
stailq_base<T, E, S, D>::size() const
    noexcept(std::integral<S> || s_has_nothrow_extractor) {
  if constexpr (std::same_as<S, no_size>)
    return static_cast<size_type>(std::ranges::distance(begin(), end()));
  else
    return getHeadData().m_sz;
}

template <typename T, stailq_entry_extractor<T> E, optional_size S, typename D>
constexpr void stailq_base<T, E, S, D>::clear() noexcept {
  auto &head = getHeadData();
  head.m_headEntry.next = nullptr;
  head.m_encodedTail =
      entry_ref_codec::create_direct_entry_ref(&head.m_headEntry);

  if constexpr (std::integral<S>)
    head.m_sz = 0;
}

template <typename T, stailq_entry_extractor<T> E, optional_size S, typename D>
constexpr CSG_TYPENAME stailq_base<T, E, S, D>::iterator
stailq_base<T, E, S, D>::insert_after(const_iterator pos, pointer value)
    noexcept(s_has_nothrow_extractor)
{
  CSG_ASSERT(pos != end(), "end() iterator passed to insert_after");

  const entry_ref_type itemRef = entry_ref_codec::create_item_entry_ref(value);
  entry_type *const posEntry = iterToEntry(pos);
  entry_type *const insertEntry = refToEntry(itemRef);

  insertEntry->next = posEntry->next;
  posEntry->next = itemRef;

  if (!insertEntry->next)
    getHeadData().m_encodedTail = itemRef;

  if constexpr (std::integral<S>)
    ++getHeadData().m_sz;

  return {itemRef, get_entry_extractor()};
}

template <typename T, stailq_entry_extractor<T> E, optional_size S, typename D>
template <std::input_iterator InputIt, std::sentinel_for<InputIt> Sentinel>
    requires std::constructible_from<T *, std::iter_reference_t<InputIt>>
constexpr CSG_TYPENAME stailq_base<T, E, S, D>::iterator
stailq_base<T, E, S, D>::insert_after(const_iterator pos, InputIt first,
    Sentinel last) noexcept(noexcept(*first++) && noexcept(first != last) &&
    s_has_nothrow_extractor)
{
  while (first != last)
    pos = insert_after(pos, *first++);

  return {pos.m_current, get_entry_extractor()};
}

template <typename T, stailq_entry_extractor<T> E, optional_size S, typename D>
constexpr CSG_TYPENAME stailq_base<T, E, S, D>::iterator
stailq_base<T, E, S, D>::erase_after(const_iterator pos)
    noexcept(s_has_nothrow_extractor)
{
  CSG_ASSERT(pos != end(), "end() iterator passed to erase_after");

  entry_type *const posEntry = iterToEntry(pos);
  const bool isLastEntry = !posEntry->next;

  if constexpr (std::integral<S>) {
    if (!isLastEntry)
      --getHeadData().m_sz;
  }

  if (isLastEntry)
    return end();

  entry_type *const erasedEntry = refToEntry(posEntry->next);
  posEntry->next = erasedEntry->next;

  if (!posEntry->next) {
    // erasedEntry was the tail element so now posEntry becomes the tail.
    getHeadData().m_encodedTail = pos.m_current;
  }

  return {posEntry->next, get_entry_extractor()};
}

template <typename T, stailq_entry_extractor<T> E, optional_size S, typename D>
constexpr CSG_TYPENAME stailq_base<T, E, S, D>::iterator
stailq_base<T, E, S, D>::erase_after(const_iterator first,
                                     const_iterator last)
    noexcept(s_has_nothrow_extractor)
{
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

  if (!last.m_current) {
    // last is end(), so first is the new tail element, unless first is
    // before_begin() -- in that case the tail element will be before_begin().
    getHeadData().m_encodedTail = first.m_current;
  }

  return {last.m_current, get_entry_extractor()};
}

template <typename T, stailq_entry_extractor<T> E, optional_size S, typename D>
template <optional_size S2, typename D2>
constexpr void stailq_base<T, E, S, D>::swap(other_list_t<S2, D2> &other)
    noexcept(std::is_nothrow_swappable_v<entry_extractor_type> &&
             noexcept(size()) && noexcept(other.size())) {
  getHeadData().swap_with(other.getHeadData(), other.size(), size());
  std::ranges::swap(get_entry_extractor(), other.get_entry_extractor());
}

template <typename T, stailq_entry_extractor<T> E, optional_size S, typename D>
constexpr CSG_TYPENAME stailq_base<T, E, S, D>::iterator
stailq_base<T, E, S, D>::find_predecessor(const_iterator scan,
                                          const_iterator last,
                                          const_iterator pos) const
                                          noexcept(s_has_nothrow_extractor) {
  while (scan != last) {
    if (auto prev = scan++; scan == pos)
      return {prev.m_current, get_entry_extractor_mutable()};
  }

  return {nullptr, get_entry_extractor_mutable()};
}

template <typename T, stailq_entry_extractor<T> E, optional_size S, typename D>
// FIXME: see the comment in slist.h for an explanation of these #if 0 blocks.
#if 0
template <std::invocable<typename stailq_base<T, E, S, D>::const_reference> Proj,
          std::predicate<std::invoke_result_t<Proj,
              typename stailq_base<T, E, S, D>::const_reference>> Pred>
#else
template <std::invocable<const T&> Proj,
          std::predicate<std::invoke_result_t<Proj, const T&>> Pred>
#endif
constexpr std::pair<typename stailq_base<T, E, S, D>::iterator, bool>
stailq_base<T, E, S, D>::find_predecessor_if(const_iterator prev,
                                             const_iterator last,
                                             Pred pred, Proj proj) const
    noexcept(s_has_nothrow_extractor &&
             util::is_nothrow_proj_invocable<const_reference, Proj, Pred>)
{
  if (prev == last)
    return {iterator{nullptr, get_entry_extractor_mutable()}, false};

  // This algorithm has a more complex structure than the iterator-based
  // find_predecessor because when the list is empty the previous is equal to
  // `before_begin` , the next is `end`, but neither is safe to dereference.
  const_iterator scan = std::ranges::next(prev);

  while (scan != last) {
    if (std::invoke(pred, std::invoke(proj, *scan)))
      return {iterator{prev.m_current, get_entry_extractor_mutable()}, true};
    prev = scan++;
  }

  return {iterator{prev.m_current, get_entry_extractor_mutable()}, false};
}

template <typename T, stailq_entry_extractor<T> E, optional_size S1, typename D1>
#if 0
template <optional_size S2, typename D2,
          std::invocable<typename stailq_base<T, E, S1, D1>::const_reference> Proj,
          std::strict_weak_order<
            std::invoke_result_t<Proj,
                typename stailq_base<T, E, S1, D1>::const_reference>,
            std::invoke_result_t<Proj,
                typename stailq_base<T, E, S1, D1>::const_reference>
          > Compare>
#else
template <optional_size S2, typename D2,
          std::invocable<const T&> Proj,
          std::strict_weak_order<
            std::invoke_result_t<Proj, const T&>,
            std::invoke_result_t<Proj, const T&>
          > Compare>
#endif
constexpr void stailq_base<T, E, S1, D1>::merge(other_list_t<S2, D2> &other,
                                                Compare comp, Proj proj)
    noexcept(s_has_nothrow_extractor &&
             util::is_nothrow_proj_relation<const_reference, Proj, Compare>)
{
  // See the comments in slist.h for a description of this algorithm.
  if (this == &other)
    return;

  auto p1 = cbefore_begin();
  auto f1 = std::ranges::next(p1);
  auto e1 = cend();

  const_iterator mergeEnd;
  auto f2 = other.cbegin();
  auto e2 = other.cend();

  if constexpr (std::integral<S1>)
    getHeadData().m_sz += std::size(other);

  while (f1 != e1 && f2 != e2) {
    if (util::projection_is_ordered_before(proj, comp, f1, f2)) {
      p1 = f1++;
      continue;
    }

    mergeEnd = f2;

    for (auto scan = std::ranges::next(mergeEnd); scan != e2 &&
         util::projection_is_ordered_before(proj, comp, scan, f1); ++scan)
      mergeEnd = scan;

    f2 = insert_range_after(p1, f2, mergeEnd);

    p1 = mergeEnd;
    f1 = std::ranges::next(mergeEnd);
  }

  if (f2 != e2) {
    // Merge the remaining range, [f2, e2) at the end of the list; the tail
    // element must be located in `other` if other is not empty, so take it
    // from there. If `other` *is* empty, then the largest element was
    // already in our list from the beginning, so the tail element is already
    // correct.
    iterToEntry(p1)->next = f2.m_current;
    getHeadData().m_encodedTail = other.getHeadData().m_encodedTail;
  }

  other.clear();
}

template <typename T, stailq_entry_extractor<T> E, optional_size S1, typename D1>
template <optional_size S2, typename D2>
constexpr void stailq_base<T, E, S1, D1>::splice_after(
    const_iterator pos, other_list_t<S2, D2> &other)
    noexcept(s_has_nothrow_extractor)
{
  if (other.empty())
    return;

  CSG_ASSERT(pos.m_current, "end() iterator passed as pos");

  entry_type *const posEntry = iterToEntry(pos);

  if (!posEntry->next) {
    // pos is the tail entry; new tail entry will come from the other list.
    // We can ignore the `before_begin() == before_end()` corner case because
    // we've already returned in the case where the other list is empty.
    getHeadData().m_encodedTail = other.getHeadData().m_encodedTail;
  }

  posEntry->next = other.begin().m_current;

  if constexpr (std::integral<S1>)
    getHeadData().m_sz += std::size(other);

  other.clear();
}

template <typename T, stailq_entry_extractor<T> E, optional_size S1, typename D1>
template <optional_size S2, typename D2>
constexpr void stailq_base<T, E, S1, D1>::splice_after(
    const_iterator pos, other_list_t<S2, D2> &other,
    typename other_list_t<S2, D2>::const_iterator first,
    typename other_list_t<S2, D2>::const_iterator last)
    noexcept(s_has_nothrow_extractor) {
  if (first == last)
    return;

  // When the above is false, iterToEntry(first) and first++ must be legal.
  CSG_ASSERT(first.m_current, "first is end() but last was not end()?");

  if (!last.m_current) {
    // last is end(), so we're removing the tail element -- first points to
    // the new tail element of other.
    other.getHeadData().m_encodedTail = first.m_current;
  }

  // Remove the open range (first, last) from `other`, by directly linking
  // first to last. Also post-increment first, so that it will point to the
  // start of the closed range [first + 1, last - 1] that we're inserting.
  other.iterToEntry(first++)->next = last.m_current;

  if (first == last)
    return;

  // Find the last element in the closed range we're inserting, then use
  // insert_range_after to insert the closed range after pos.
  auto lastInsert = first;
  std::common_type_t<difference_type, typename D2::difference_type> sz = 1;

  for (auto scan = std::ranges::next(lastInsert); scan != last; ++scan, ++sz)
    lastInsert = scan;

  if constexpr (std::integral<S1>)
    getHeadData().m_sz += sz;

  if constexpr (std::integral<S2>)
    other.getHeadData().m_sz -= sz;

  insert_range_after(pos, first, lastInsert);

  if (!refToEntry(lastInsert.m_current)->next) {
    // lastInsert is the new tail element.
    getHeadData().m_encodedTail = lastInsert.m_current;
  }
}

template <typename T, stailq_entry_extractor<T> E, optional_size S, typename D>
#if 0
template <std::invocable<typename stailq_base<T, E, S, D>::const_reference> Proj,
          std::predicate<std::invoke_result_t<Proj,
              typename stailq_base<T, E, S, D>::const_reference>> Pred>
#else
template <std::invocable<const T&> Proj,
          std::predicate<std::invoke_result_t<Proj, const T&>> Pred>
#endif
constexpr CSG_TYPENAME stailq_base<T, E, S, D>::size_type
stailq_base<T, E, S, D>::remove_if(Pred pred, Proj proj)
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

template <typename T, stailq_entry_extractor<T> E, optional_size S, typename D>
constexpr void stailq_base<T, E, S, D>::reverse() noexcept(s_has_nothrow_extractor) {
  const const_iterator end = cend();
  const_iterator i = cbegin();
  const_iterator prev = end;

  getHeadData().m_encodedTail = i.m_current;

  while (i != end) {
    const auto current = i;
    entry_type *const entry = iterToEntry(i++);
    entry->next = prev.m_current;
    prev = current;
  }

  getHeadData().m_headEntry.next = prev.m_current;
}

template <typename T, stailq_entry_extractor<T> E, optional_size S, typename D>
#if 0
template <std::invocable<typename stailq_base<T, E, S, D>::const_reference> Proj,
          std::equivalence_relation<
            std::invoke_result_t<Proj,
                typename stailq_base<T, E, S, D>::const_reference>,
            std::invoke_result_t<Proj,
                typename stailq_base<T, E, S, D>::const_reference>
          > EqRelation>
#else
template <std::invocable<const T&> Proj,
          std::equivalence_relation<
            std::invoke_result_t<Proj, const T&>,
            std::invoke_result_t<Proj, const T&>
          > EqRelation>
#endif
constexpr void stailq_base<T, E, S, D>::unique(EqRelation eq, Proj proj)
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

template <typename T, stailq_entry_extractor<T> E, optional_size S, typename D>
#if 0
template <std::invocable<typename stailq_base<T, E, S, D>::const_reference> Proj,
          std::strict_weak_order<
            std::invoke_result_t<Proj,
                typename stailq_base<T, E, S, D>::const_reference>,
            std::invoke_result_t<Proj,
                typename stailq_base<T, E, S, D>::const_reference>
          > Compare>
#else
template <std::invocable<const T&> Proj,
          std::strict_weak_order<
            std::invoke_result_t<Proj, const T&>,
            std::invoke_result_t<Proj, const T&>
          > Compare>
#endif
constexpr void stailq_base<T, E, S, D>::sort(Compare comp, Proj proj)
    noexcept(s_has_nothrow_extractor &&
             util::is_nothrow_proj_relation<const_reference, Proj, Compare>) {
  const auto pEnd = detail::forward_list_merge_sort<stailq_base<T, E, S, D>>(
      cbefore_begin(), cend(), std::ref(comp), std::ref(proj), std::size(*this));
  getHeadData().m_encodedTail = pEnd.m_current;
}

template <typename T, stailq_entry_extractor<T> E, optional_size S, typename D>
template <typename QueueIt>
constexpr CSG_TYPENAME stailq_base<T, E, S, D>::iterator
stailq_base<T, E, S, D>::insert_range_after(const_iterator pos, QueueIt first,
                                            QueueIt last)
    noexcept(s_has_nothrow_extractor &&
             QueueIt::container::s_has_nothrow_extractor)
{
  // Inserts the closed range [first, last] after pos, and returns the
  // successor to last.
  CSG_ASSERT(pos.m_current && last.m_current,
             "end() iterator passed as pos or last");

  entry_type *const posEntry = iterToEntry(pos);
  entry_type *const lastEntry = QueueIt::container::iterToEntry(last);
  const entry_ref_type oldNext = lastEntry->next;

  lastEntry->next = posEntry->next;
  posEntry->next = first.m_current;

  return iterator{oldNext, last.m_rEntryExtractor.get_invocable()};
}

} // End of namespace csg

#endif
