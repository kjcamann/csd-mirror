//==-- csg/core/tailq.h - tail queue intrusive list impl. -------*- C++ -*-==//
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

#ifndef CSG_CORE_TAILQ_H
#define CSG_CORE_TAILQ_H

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <ranges>
#include <type_traits>

#include <csg/core/assert.h>
#include <csg/core/listfwd.h>
#include <csg/core/utility.h>

namespace csg {

template <typename T>
struct tailq_entry {
  entry_ref_union<tailq_entry, T> next;
  entry_ref_union<tailq_entry, T> prev;
};

template <typename T, tailq_entry_extractor<T> EntryEx,
          optional_size SizeMember, typename Derived>
class tailq_base;

template <typename T, typename O>
concept compatible_tailq = tailq<T> &&
    std::same_as<typename T::value_type, typename O::value_type> &&
    std::same_as<typename T::entry_extractor_type, typename O::entry_extractor_type>;

template <typename T, optional_size SizeMember>
class tailq_fwd_head {
public:
  using value_type = T;
  using size_type = std::conditional_t<std::same_as<SizeMember, no_size>,
                                       std::size_t, SizeMember>;

  constexpr tailq_fwd_head() noexcept : m_sz{} {
    m_endEntry.next.offset = m_endEntry.prev.offset = &m_endEntry;
  }

  tailq_fwd_head(const tailq_fwd_head &) = delete;

  tailq_fwd_head(tailq_fwd_head &&) = delete;

  ~tailq_fwd_head() = default;

  tailq_fwd_head &operator=(const tailq_fwd_head &) = delete;

  tailq_fwd_head &operator=(tailq_fwd_head &&) = delete;

private:
  template <typename T2, tailq_entry_extractor<T2>, optional_size, typename>
  friend class tailq_base;

  template <util::derived_from_template<tailq_fwd_head> FH2,
            tailq_entry_extractor<typename FH2::value_type>>
  friend class tailq_proxy;

  using size_member_type = SizeMember;

  tailq_entry<T> m_endEntry;
  [[no_unique_address]] SizeMember m_sz;
};

template <typename T, tailq_entry_extractor<T> EntryEx,
          optional_size SizeMember, typename Derived>
class tailq_base {
protected:
  constexpr static bool s_has_nothrow_extractor =
      std::is_nothrow_invocable_v<EntryEx, T &>;

public:
  using value_type = T;
  using reference = T &;
  using const_reference = const T &;
  using pointer = T *;
  using const_pointer = const T *;
  using size_type = CSG_TYPENAME tailq_fwd_head<T, SizeMember>::size_type;
  using difference_type = std::make_signed_t<size_type>;
  using entry_type = tailq_entry<T>;
  using entry_extractor_type = EntryEx;
  using size_member_type = SizeMember;

  template <optional_size S, typename D>
  using other_list_t = tailq_base<T, EntryEx, S, D>;

  constexpr entry_extractor_type &get_entry_extractor() noexcept {
    return static_cast<Derived *>(this)->getEntryExtractor();
  }

  constexpr const entry_extractor_type &get_entry_extractor() const noexcept {
    return const_cast<tailq_base *>(this)->get_entry_extractor();
  }

  template <std::input_iterator InputIt, std::sentinel_for<InputIt> Sentinel>
      requires std::constructible_from<pointer, std::iter_reference_t<InputIt>>
  constexpr void assign(InputIt first, Sentinel last)
      CSG_NOEXCEPT(noexcept(insert(cbegin(), first, last))) {
    clear();
    insert(cbegin(), first, last);
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

  constexpr reference front() noexcept { return *begin(); }

  constexpr const_reference front() const noexcept { *begin(); }

  constexpr reference back() noexcept(s_has_nothrow_extractor) {
    return *--end();
  }

  constexpr const_reference back() const noexcept(s_has_nothrow_extractor) {
    return *--end();
  }

  class iterator;
  class const_iterator;

  using iterator_t = std::type_identity_t<iterator>;
  using const_iterator_t = std::type_identity_t<const_iterator>;

  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  constexpr iterator begin() noexcept {
    return {getHeadData().m_endEntry.next, get_entry_extractor()};
  }

  constexpr const_iterator begin() const noexcept {
    return const_cast<tailq_base *>(this)->begin();
  }

  constexpr const_iterator cbegin() const noexcept { return begin(); }

  constexpr iterator end() noexcept {
    auto *const endEntry = &getHeadData().m_endEntry;
    const auto u = entry_ref_codec::create_direct_entry_ref(endEntry);
    return {u, get_entry_extractor()};
  }

  constexpr const_iterator end() const noexcept {
    return const_cast<tailq_base *>(this)->end();
  }

  constexpr const_iterator cend() const noexcept { return end(); }

  constexpr reverse_iterator rbegin() noexcept {
    return reverse_iterator{end()};
  }

  constexpr const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator{end()};
  }

  constexpr const_reverse_iterator crbegin() const noexcept {
    return rbegin();
  }

  constexpr reverse_iterator rend() noexcept {
    return reverse_iterator{begin()};
  }

  constexpr const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator{begin()};
  }

  constexpr const_reverse_iterator crend() const noexcept { return rend(); }

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
    const entry_type &endEntry = getHeadData().m_endEntry;
    return getEntry(get_entry_extractor_mutable(), endEntry.next) == &endEntry;
  }

  constexpr size_type size() const
      noexcept(std::integral<SizeMember> || s_has_nothrow_extractor);

  constexpr static size_type max_size() noexcept {
    return std::numeric_limits<size_type>::max();
  }

  constexpr void clear() noexcept;

  constexpr iterator insert(const_iterator, pointer)
      noexcept(s_has_nothrow_extractor);

  template <std::input_iterator InputIt, std::sentinel_for<InputIt> Sentinel>
      requires std::constructible_from<pointer, std::iter_reference_t<InputIt>>
  constexpr iterator
  insert(const_iterator, InputIt first, Sentinel last)
      noexcept(noexcept(*first++) && noexcept(first != last) &&
               s_has_nothrow_extractor);

  template <std::ranges::input_range Range>
      requires std::constructible_from<pointer, std::ranges::range_reference_t<Range>>
  constexpr iterator insert(const_iterator_t pos, Range &&r)
      noexcept(noexcept(insert(pos, std::ranges::begin(r), std::ranges::end(r)))) {
    return insert(pos, std::ranges::begin(r), std::ranges::end(r));
  }

  constexpr iterator
  insert(const_iterator_t pos, std::initializer_list<pointer> i)
      noexcept(s_has_nothrow_extractor) {
    return insert(pos, std::ranges::begin(i), std::ranges::end(i));
  }

  constexpr iterator erase(const_iterator) noexcept(s_has_nothrow_extractor);

  constexpr iterator erase(const_iterator first, const_iterator last)
      noexcept(s_has_nothrow_extractor);

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

  constexpr void push_back(pointer p) noexcept(s_has_nothrow_extractor) {
    insert(cend(), p);
  }

  constexpr void pop_back() noexcept(s_has_nothrow_extractor) {
    erase(--end());
  }

  constexpr void push_front(pointer p) noexcept(s_has_nothrow_extractor) {
    insert(cbegin(), p);
  }

  constexpr void pop_front() noexcept(s_has_nothrow_extractor) {
    erase(begin());
  }

  template <optional_size S2, typename D2>
  constexpr void swap(other_list_t<S2, D2> &other)
      noexcept(std::is_nothrow_swappable_v<entry_extractor_type>);

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
  constexpr void splice(const_iterator pos, other_list_t<S2, D2> &other)
      noexcept(s_has_nothrow_extractor);

  template <optional_size S2, typename D2>
  constexpr void splice(const_iterator_t pos, other_list_t<S2, D2> &&other)
      noexcept(noexcept(splice(pos, other))) {
    return splice(pos, other);
  }

  template <optional_size S2, typename D2>
  constexpr void splice(const_iterator_t pos, other_list_t<S2, D2> &other,
                        CSG_TYPENAME other_list_t<S2, D2>::const_iterator it)
      noexcept(noexcept(splice(pos, other, it, other.end()))) {
    return splice(pos, other, it, other.cend());
  }

  template <optional_size S2, typename D2>
  constexpr void splice(const_iterator_t pos, other_list_t<S2, D2> &&other,
                        CSG_TYPENAME other_list_t<S2, D2>::const_iterator it)
      noexcept(noexcept(splice(pos, other, it))) {
    return splice(pos, other, it);
  }

  template <optional_size S2, typename D2>
  constexpr void splice(const_iterator pos, other_list_t<S2, D2> &other,
                        CSG_TYPENAME other_list_t<S2, D2>::const_iterator first,
                        CSG_TYPENAME other_list_t<S2, D2>::const_iterator last)
      noexcept(s_has_nothrow_extractor);

  template <optional_size S2, typename D2>
  constexpr void splice(const_iterator_t pos, other_list_t<S2, D2> &&other,
                        CSG_TYPENAME other_list_t<S2, D2>::const_iterator first,
                        CSG_TYPENAME other_list_t<S2, D2>::const_iterator last)
      noexcept(noexcept(splice(pos, other, first, last))) {
    return splice(pos, other, first, last);
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
  constexpr void sort(Compare comp = {}, Proj proj = {})
      noexcept(s_has_nothrow_extractor &&
               util::is_nothrow_proj_relation<const_reference, Proj, Compare>) {
    merge_sort(cbegin(), cend(), std::ref(comp), std::ref(proj),
               std::size(*this));
  }

protected:
  template <optional_size S2, typename D2>
  constexpr void swap_lists(other_list_t<S2, D2> &other)
      noexcept(s_has_nothrow_extractor);

private:
  template <typename T2, tailq_entry_extractor<T2>, optional_size, typename>
  friend class tailq_base;

  using entry_ref_codec = detail::entry_ref_codec<entry_type, T, EntryEx>;
  using entry_ref_type = entry_ref_union<entry_type, T>;

  constexpr tailq_fwd_head<T, SizeMember> &getHeadData() noexcept {
    return static_cast<Derived *>(this)->getHeadData();
  }

  constexpr const tailq_fwd_head<T, SizeMember> &getHeadData() const noexcept {
    return const_cast<tailq_base *>(this)->getHeadData();
  }

  constexpr entry_extractor_type &get_entry_extractor_mutable() const noexcept {
    return const_cast<tailq_base *>(this)->get_entry_extractor();
  }

  constexpr static entry_type *
  getEntry(entry_extractor_type &ex, entry_ref_type ref)
      noexcept(s_has_nothrow_extractor) {
    return entry_ref_codec::get_entry(ex, ref);
  }

  constexpr static reference getValue(entry_ref_type ref) noexcept {
    return entry_ref_codec::get_value(ref);
  }


  constexpr entry_type *refToEntry(entry_ref_type ref)
      noexcept(s_has_nothrow_extractor) {
    return getEntry(get_entry_extractor(), ref);
  }

  constexpr static entry_type *iterToEntry(iterator_t i)
      noexcept(s_has_nothrow_extractor) {
    return getEntry(i.m_rEntryExtractor.get_invocable(), i.m_current);
  }

  constexpr static entry_type *iterToEntry(const_iterator_t i)
      noexcept(s_has_nothrow_extractor) {
    return getEntry(i.m_rEntryExtractor.get_invocable(), i.m_current);
  }

  template <typename QueueIt>
  constexpr static void insert_range(const_iterator pos, QueueIt first,
                                     QueueIt last) noexcept(s_has_nothrow_extractor);

  constexpr static void remove_range(const_iterator first, const_iterator last)
      noexcept(s_has_nothrow_extractor);

  template <typename Compare, typename Proj, std::unsigned_integral SizeType>
  constexpr const_iterator merge_sort(const_iterator f1, const_iterator e2,
                                      Compare comp, Proj proj, SizeType n)
      noexcept(s_has_nothrow_extractor);
};

template <typename T, tailq_entry_extractor<T> EntryEx,
          optional_size SizeMember, typename Derived>
class tailq_base<T, EntryEx, SizeMember, Derived>::iterator {
public:
  using value_type = tailq_base::value_type;
  using reference = tailq_base::reference;
  using pointer = tailq_base::pointer;
  using difference_type = tailq_base::difference_type;
  using iterator_category = std::bidirectional_iterator_tag;
  using invocable_ref = compressed_invocable_ref<EntryEx, reference>;

  constexpr iterator() noexcept : m_current{}, m_rEntryExtractor{} {}
  constexpr iterator(const iterator &) = default;
  constexpr iterator(iterator &&) = default;

  constexpr iterator(std::nullptr_t) noexcept
      requires stateless<entry_extractor_type>
      : m_current{nullptr}, m_rEntryExtractor{} {}

  constexpr iterator(std::nullptr_t, entry_extractor_type &fn) noexcept
      requires stateless<entry_extractor_type>
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
    return tailq_base::getValue(m_current);
  }

  constexpr pointer operator->() const noexcept {
    return std::addressof(this->operator*());
  }

  constexpr iterator &operator++() noexcept(s_has_nothrow_extractor) {
    m_current = tailq_base::iterToEntry(*this)->next;
    return *this;
  }

  constexpr iterator operator++(int) noexcept(s_has_nothrow_extractor) {
    const iterator i{*this};
    this->operator++();
    return i;
  }

  constexpr iterator operator--() noexcept(s_has_nothrow_extractor) {
    m_current = tailq_base::iterToEntry(*this)->prev;
    return *this;
  }

  constexpr iterator operator--(int) noexcept(s_has_nothrow_extractor) {
    iterator i{*this};
    this->operator--();
    return i;
  }

  constexpr bool operator==(const iterator &rhs) const noexcept {
    return m_current == rhs.m_current;
  }

  constexpr bool operator==(const const_iterator &rhs) const noexcept {
    return m_current == rhs.m_current;
  }

private:
  template <typename T2, tailq_entry_extractor<T2>, optional_size, typename>
  friend class tailq_base;

  friend tailq_base::const_iterator;

  using container = tailq_base;

  constexpr iterator(entry_ref_type ref, entry_extractor_type &fn) noexcept
      : m_current{ref}, m_rEntryExtractor{fn} {}

  entry_ref_type m_current;
  [[no_unique_address]] invocable_ref m_rEntryExtractor;
};

template <typename T, tailq_entry_extractor<T> EntryEx,
          optional_size SizeMember, typename Derived>
class tailq_base<T, EntryEx, SizeMember, Derived>::const_iterator {
public:
  using value_type = tailq_base::value_type;
  using reference = tailq_base::const_reference;
  using pointer = tailq_base::const_pointer;
  using difference_type = tailq_base::difference_type;
  using iterator_category = std::bidirectional_iterator_tag;
  using invocable_ref = compressed_invocable_ref<EntryEx, tailq_base::reference>;

  constexpr const_iterator() noexcept : m_current{}, m_rEntryExtractor{} {}
  constexpr const_iterator(const const_iterator &) = default;
  constexpr const_iterator(const_iterator &&) = default;
  constexpr const_iterator(const iterator &i) noexcept
      : m_current{i.m_current}, m_rEntryExtractor{i.m_rEntryExtractor} {}

  constexpr const_iterator(std::nullptr_t) noexcept
      requires stateless<entry_extractor_type>
      : m_current{nullptr}, m_rEntryExtractor{} {}

  constexpr const_iterator(std::nullptr_t, entry_extractor_type &fn) noexcept
      requires stateless<entry_extractor_type>
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
    return tailq_base::getValue(m_current);
  }

  constexpr pointer operator->() const noexcept {
    return std::addressof(this->operator*());
  }

  constexpr const_iterator &operator++() noexcept(s_has_nothrow_extractor) {
    m_current = tailq_base::iterToEntry(*this)->next;
    return *this;
  }

  constexpr const_iterator operator++(int) noexcept(s_has_nothrow_extractor) {
    const_iterator i{*this};
    this->operator++();
    return i;
  }

  constexpr const_iterator operator--() noexcept(s_has_nothrow_extractor) {
    m_current = tailq_base::iterToEntry(*this)->prev;
    return *this;
  }

  constexpr const_iterator operator--(int) noexcept(s_has_nothrow_extractor) {
    const const_iterator i{*this};
    this->operator--();
    return i;
  }

  constexpr bool operator==(const iterator &rhs) const noexcept {
    return m_current == rhs.m_current;
  }

  constexpr bool operator==(const const_iterator &rhs) const noexcept {
    return m_current == rhs.m_current;
  }

private:
  template <typename T2, tailq_entry_extractor<T2>, optional_size, typename>
  friend class tailq_base;

  friend tailq_base::iterator;

  using container = tailq_base;

  constexpr const_iterator(entry_ref_type ref, entry_extractor_type &fn) noexcept
      : m_current{ref}, m_rEntryExtractor{fn} {}

  entry_ref_type m_current;
  [[no_unique_address]] invocable_ref m_rEntryExtractor;
};

template <util::derived_from_template<tailq_fwd_head> FwdHead,
          tailq_entry_extractor<typename FwdHead::value_type> EntryEx>
class tailq_proxy : public tailq_base<typename FwdHead::value_type, EntryEx,
    typename FwdHead::size_member_type, tailq_proxy<FwdHead, EntryEx>> {
  using size_member_type = CSG_TYPENAME FwdHead::size_member_type;
  using base_type = tailq_base<typename FwdHead::value_type, EntryEx,
                               size_member_type, tailq_proxy>;

public:
  using fwd_head_type = FwdHead;
  using pointer = CSG_TYPENAME base_type::pointer;
  using entry_extractor_type = EntryEx;

  template <optional_size S, typename D>
  using other_list_t = CSG_TYPENAME base_type::template other_list_t<S, D>;

  tailq_proxy() = delete;

  tailq_proxy(const tailq_proxy &) = delete;

  tailq_proxy(tailq_proxy &&) = delete;

  constexpr tailq_proxy(fwd_head_type &h)
      noexcept(std::is_nothrow_default_constructible_v<entry_extractor_type>)
      requires std::default_initializable<entry_extractor_type>
      : m_head{h} {}

  template <util::can_direct_initialize<entry_extractor_type> U>
  constexpr explicit tailq_proxy(fwd_head_type &h, U &&u)
      noexcept(std::is_nothrow_constructible_v<entry_extractor_type, U>)
      : m_head{h}, m_entryExtractor{std::forward<U>(u)} {}

  template <compatible_tailq<tailq_proxy> O>
  constexpr tailq_proxy(fwd_head_type &h, O &&other)
      noexcept(std::is_nothrow_move_assignable_v<entry_extractor_type> &&
               base_type::s_has_nothrow_extractor)
      : m_head{h} {
    base_type::clear();
    base_type::swap_lists(other);
    m_entryExtractor = std::move(other.get_entry_extractor());
  }

  template <std::input_iterator InputIt, std::sentinel_for<InputIt> Sentinel>
      requires std::default_initializable<entry_extractor_type> &&
               std::constructible_from<pointer, std::iter_reference_t<InputIt>>
  constexpr tailq_proxy(fwd_head_type &h, InputIt first, Sentinel last)
      noexcept(std::is_nothrow_default_constructible_v<entry_extractor_type> &&
               noexcept(base_type::assign(first, last)))
      : m_head{h} {
    base_type::assign(first, last);
  }

  template <std::input_iterator InputIt, std::sentinel_for<InputIt> Sentinel,
            util::can_direct_initialize<entry_extractor_type> U>
      requires std::constructible_from<pointer, std::iter_reference_t<InputIt>>
  constexpr tailq_proxy(fwd_head_type &h, InputIt first, Sentinel last, U &&u)
      noexcept(std::is_nothrow_constructible_v<entry_extractor_type, U> &&
               noexcept(base_type::assign(first, last)))
      : m_head{h}, m_entryExtractor{std::forward<U>(u)} {
    base_type::assign(first, last);
  }

  template <std::ranges::input_range Range>
      requires std::default_initializable<entry_extractor_type> &&
               std::constructible_from<pointer, std::ranges::range_reference_t<Range>>
  constexpr tailq_proxy(fwd_head_type &h, Range &&r)
      noexcept(std::is_nothrow_default_constructible_v<entry_extractor_type> &&
               noexcept(base_type::assign(r)))
      : m_head{h} {
    base_type::assign(r);
  }

  template <std::ranges::input_range Range,
            util::can_direct_initialize<entry_extractor_type> U>
      requires std::constructible_from<pointer, std::ranges::range_reference_t<Range>>
  constexpr tailq_proxy(fwd_head_type &h, Range &&r, U &&u)
      noexcept(std::is_nothrow_constructible_v<entry_extractor_type, U> &&
               noexcept(base_type::assign(r)))
      : m_head{h}, m_entryExtractor{std::forward<U>(u)} {
    base_type::assign(r);
  }

  constexpr tailq_proxy(fwd_head_type &h, std::initializer_list<pointer> ilist)
      CSG_NOEXCEPT(std::is_nothrow_default_constructible_v<entry_extractor_type> &&
               noexcept(base_type::assign(ilist)))
      requires std::default_initializable<entry_extractor_type>
      : m_head{h} {
    base_type::assign(ilist);
  }

  template <util::can_direct_initialize<entry_extractor_type> U>
  constexpr tailq_proxy(fwd_head_type &h, std::initializer_list<pointer> ilist,
                        U &&u)
      noexcept(std::is_nothrow_constructible_v<entry_extractor_type, U> &&
               noexcept(base_type::assign(ilist)))
      : m_head{h}, m_entryExtractor{std::forward<U>(u)} {
    base_type::assign(ilist);
  }

  ~tailq_proxy() = default;

  tailq_proxy &operator=(const tailq_proxy &) = delete;

  constexpr tailq_proxy &operator=(tailq_proxy &&rhs)
      noexcept(std::is_nothrow_move_assignable_v<entry_extractor_type> &&
               base_type::s_has_nothrow_extractor) {
    base_type::clear();
    base_type::swap_lists(rhs);
    m_entryExtractor = std::move(rhs.get_entry_extractor());
    return *this;
  }

  template <compatible_tailq<tailq_proxy> O>
  constexpr tailq_proxy &operator=(O &&rhs)
      noexcept(std::is_nothrow_move_assignable_v<entry_extractor_type> &&
               base_type::s_has_nothrow_extractor) {
    base_type::clear();
    base_type::swap_lists(rhs);
    m_entryExtractor = std::move(rhs.get_entry_extractor());
    return *this;
  }

  template <std::ranges::input_range Range>
      requires std::constructible_from<pointer, std::ranges::range_reference_t<Range>>
  constexpr tailq_proxy &operator=(Range &&r)
      noexcept(noexcept(base_type::assign(r))) {
    base_type::assign(r);
    return *this;
  }

  constexpr tailq_proxy &operator=(std::initializer_list<pointer> ilist)
      CSG_NOEXCEPT(noexcept(base_type::assign(ilist))) {
    base_type::assign(ilist);
    return *this;
  }

private:
  template <typename T2, tailq_entry_extractor<T2>, optional_size, typename>
  friend class tailq_base;

  constexpr fwd_head_type &getHeadData() noexcept { return m_head; }

  constexpr entry_extractor_type &getEntryExtractor() noexcept {
    return m_entryExtractor;
  }

  fwd_head_type &m_head;
  [[no_unique_address]] entry_extractor_type m_entryExtractor;
};

template <typename T, tailq_entry_extractor<T> EntryEx,
          optional_size SizeMember>
class tailq_head : public tailq_base<T, EntryEx, SizeMember,
                                     tailq_head<T, EntryEx, SizeMember>> {
  using fwd_head_type = tailq_fwd_head<T, SizeMember>;
  using base_type = tailq_base<T, EntryEx, SizeMember, tailq_head>;

public:
  using pointer = CSG_TYPENAME base_type::pointer;
  using entry_extractor_type = EntryEx;

  template <optional_size S, typename D>
  using other_list_t = CSG_TYPENAME base_type::template other_list_t<S, D>;

  constexpr tailq_head()
      requires std::default_initializable<entry_extractor_type> = default;

  tailq_head(const tailq_head &) = delete;

  constexpr tailq_head(tailq_head &&other)
      noexcept(std::is_nothrow_move_assignable_v<entry_extractor_type>)
      requires std::is_move_assignable_v<entry_extractor_type> {
    base_type::swap_lists(other);
    m_entryExtractor = std::move(other.m_entryExtractor);
  }

  template <compatible_tailq<tailq_head> O>
  constexpr tailq_head(O &&other)
      noexcept(std::is_nothrow_move_assignable_v<entry_extractor_type>)
      requires std::is_move_assignable_v<entry_extractor_type> {
    base_type::swap_lists(other);
    m_entryExtractor = std::move(other.get_entry_extractor());
  }

  template <util::can_direct_initialize<entry_extractor_type> U>
  constexpr explicit tailq_head(U &&u)
      noexcept(std::is_nothrow_constructible_v<entry_extractor_type, U>)
      : m_entryExtractor{std::forward<U>(u)} {}

  template <std::input_iterator InputIt, std::sentinel_for<InputIt> Sentinel>
      requires std::default_initializable<entry_extractor_type> &&
               std::constructible_from<pointer, std::iter_reference_t<InputIt>>
  constexpr tailq_head(InputIt first, Sentinel last)
      noexcept(std::is_nothrow_default_constructible_v<entry_extractor_type> &&
               noexcept(base_type::assign(first, last))) {
    base_type::assign(first, last);
  }

  template <std::input_iterator InputIt, std::sentinel_for<InputIt> Sentinel,
            util::can_direct_initialize<entry_extractor_type> U>
      requires std::constructible_from<pointer, std::iter_reference_t<InputIt>>
  constexpr tailq_head(InputIt first, Sentinel last, U &&u)
      noexcept(std::is_nothrow_constructible_v<entry_extractor_type, U> &&
               noexcept(base_type::assign(first, last)))
      : m_entryExtractor{std::forward<U>(u)} {
    base_type::assign(first, last);
  }

  template <std::ranges::input_range Range>
      requires std::default_initializable<entry_extractor_type> &&
               std::constructible_from<pointer, std::ranges::range_reference_t<Range>>
  constexpr tailq_head(Range &&r)
      noexcept(std::is_nothrow_default_constructible_v<entry_extractor_type> &&
               noexcept(base_type::assign(r))) {
    base_type::assign(r);
  }

  template <std::ranges::input_range Range,
            util::can_direct_initialize<entry_extractor_type> U>
      requires std::constructible_from<pointer, std::ranges::range_reference_t<Range>>
  constexpr tailq_head(Range &&r, U &&u)
      noexcept(std::is_nothrow_constructible_v<entry_extractor_type, U> &&
               noexcept(base_type::assign(r)))
      : m_entryExtractor{std::forward<U>(u)} {
    base_type::assign(r);
  }

  constexpr tailq_head(std::initializer_list<pointer> ilist)
      CSG_NOEXCEPT(std::is_nothrow_default_constructible_v<entry_extractor_type> &&
               noexcept(base_type::assign(ilist)))
      requires std::default_initializable<entry_extractor_type> {
    base_type::assign(ilist);
  }

  template <util::can_direct_initialize<entry_extractor_type> U>
  constexpr tailq_head(std::initializer_list<pointer> ilist, U &&u)
      noexcept(std::is_nothrow_constructible_v<entry_extractor_type, U> &&
               noexcept(base_type::assign(ilist)))
      : m_entryExtractor{std::forward<U>(u)} {
    base_type::assign(ilist);
  }

  ~tailq_head() = default;

  tailq_head &operator=(const tailq_head &) = delete;

  constexpr tailq_head &operator=(tailq_head &&rhs)
      noexcept(std::is_nothrow_move_assignable_v<entry_extractor_type>)
      requires std::is_move_assignable_v<entry_extractor_type> {
    base_type::clear();
    base_type::swap_lists(rhs);
    m_entryExtractor = std::move(rhs.get_entry_extractor());
    return *this;
  }

  template <compatible_tailq<tailq_head> O>
  constexpr tailq_head &operator=(O &&rhs)
      noexcept(std::is_nothrow_move_assignable_v<entry_extractor_type>)
      requires std::is_move_assignable_v<entry_extractor_type> {
    base_type::clear();
    base_type::swap_lists(rhs);
    m_entryExtractor = std::move(rhs.get_entry_extractor());
    return *this;
  }

  template <std::ranges::input_range Range>
      requires std::constructible_from<pointer, std::ranges::range_reference_t<Range>>
  constexpr tailq_head &operator=(Range &&r)
      noexcept(noexcept(base_type::assign(r))) {
    base_type::assign(r);
    return *this;
  }

  constexpr tailq_head &operator=(std::initializer_list<pointer> ilist)
      CSG_NOEXCEPT(noexcept(base_type::assign(ilist))) {
    base_type::assign(ilist);
    return *this;
  }

private:
  template <typename T2, tailq_entry_extractor<T2>, optional_size, typename>
  friend class tailq_base;

  constexpr fwd_head_type &getHeadData() noexcept { return m_head; }

  constexpr entry_extractor_type &getEntryExtractor() noexcept {
    return m_entryExtractor;
  }

  fwd_head_type m_head;
  [[no_unique_address]] entry_extractor_type m_entryExtractor;
};

template <typename T, tailq_entry_extractor<T> E, optional_size S, typename D>
constexpr CSG_TYPENAME tailq_base<T, E, S, D>::size_type
tailq_base<T, E, S, D>::size() const
    noexcept(std::integral<S> || s_has_nothrow_extractor) {
  if constexpr (std::same_as<S, no_size>)
    return static_cast<size_type>(std::ranges::distance(begin(), end()));
  else
    return getHeadData().m_sz;
}

template <typename T, tailq_entry_extractor<T> E, optional_size S, typename D>
constexpr void tailq_base<T, E, S, D>::clear() noexcept {
  auto &endEntry = getHeadData().m_endEntry;
  endEntry.next = endEntry.prev =
      entry_ref_codec::create_direct_entry_ref(&endEntry);

  if constexpr (std::integral<S>)
    getHeadData().m_sz = 0;
}

template <typename T, tailq_entry_extractor<T> E, optional_size S, typename D>
constexpr CSG_TYPENAME tailq_base<T, E, S, D>::iterator
tailq_base<T, E, S, D>::insert(const_iterator pos, pointer value)
    noexcept(s_has_nothrow_extractor)
{
  entry_type *const posEntry = iterToEntry(pos);
  entry_type *const prevEntry = refToEntry(posEntry->prev);

  const entry_ref_type itemRef =
      entry_ref_codec::create_item_entry_ref(value);
  entry_type *const insertEntry = refToEntry(itemRef);

  insertEntry->prev = posEntry->prev;
  insertEntry->next = pos.m_current;
  prevEntry->next = posEntry->prev = itemRef;

  if constexpr (std::integral<S>)
    ++getHeadData().m_sz;

  return {itemRef, get_entry_extractor()};
}

template <typename T, tailq_entry_extractor<T> E, optional_size S, typename D>
template <std::input_iterator InputIt, std::sentinel_for<InputIt> Sentinel>
    requires std::constructible_from<T *, std::iter_reference_t<InputIt>>
constexpr CSG_TYPENAME tailq_base<T, E, S, D>::iterator
tailq_base<T, E, S, D>::insert(const_iterator pos, InputIt first,
    Sentinel last) noexcept(noexcept(*first++) && noexcept(first != last) &&
    s_has_nothrow_extractor)
{
  if (first == last)
    return {pos.m_current, get_entry_extractor()};

  const iterator firstInsert = insert(pos, *first++);
  pos = firstInsert;

  while (first != last)
    pos = insert(++pos, *first++);

  return firstInsert;
}

template <typename T, tailq_entry_extractor<T> E, optional_size S, typename D>
constexpr CSG_TYPENAME tailq_base<T, E, S, D>::iterator
tailq_base<T, E, S, D>::erase(const_iterator pos)
    noexcept(s_has_nothrow_extractor)
{
  entry_type *const erasedEntry = iterToEntry(pos);
  entry_type *const nextEntry = refToEntry(erasedEntry->next);
  entry_type *const prevEntry = refToEntry(erasedEntry->prev);

  CSG_ASSERT(erasedEntry != &getHeadData().m_endEntry,
             "end() iterator passed to erase");

  prevEntry->next = erasedEntry->next;
  nextEntry->prev = erasedEntry->prev;

  if constexpr (std::integral<S>)
    --getHeadData().m_sz;

  return {erasedEntry->next, get_entry_extractor()};
}

template <typename T, tailq_entry_extractor<T> E, optional_size S, typename D>
constexpr CSG_TYPENAME tailq_base<T, E, S, D>::iterator
tailq_base<T, E, S, D>::erase(const_iterator first, const_iterator last)
    noexcept(s_has_nothrow_extractor) {
  if (first == last)
    return {last.m_current, get_entry_extractor()};

  // FIXME [C++20]: why not std::ranges:prev?
  remove_range(first, std::prev(last));

  if constexpr (std::integral<S>)
    getHeadData().m_sz -= static_cast<size_type>(std::ranges::distance(first, last));

  return {last.m_current, get_entry_extractor()};
}

template <typename T, tailq_entry_extractor<T> E, optional_size S, typename D>
template <optional_size S2, typename D2>
constexpr void tailq_base<T, E, S, D>::swap(other_list_t<S2, D2> &other)
      noexcept(std::is_nothrow_swappable_v<entry_extractor_type>) {
  swap_lists(other);
  std::ranges::swap(get_entry_extractor(), other.get_entry_extractor());
}

template <typename T, tailq_entry_extractor<T> E, optional_size S1, typename D1>
// FIXME: see the comment in slist.h for an explanation of these #if 0 blocks.
#if 0
template <optional_size S2, typename D2,
          std::invocable<typename tailq_base<T, E, S1, D1>::const_reference> Proj,
          std::strict_weak_order<
            std::invoke_result_t<Proj,
                typename tailq_base<T, E, S1, D1>::const_reference>,
            std::invoke_result_t<Proj,
                typename tailq_base<T, E, S1, D1>::const_reference>
          > Compare>
#else
template <optional_size S2, typename D2,
          std::invocable<const T&> Proj,
          std::strict_weak_order<
            std::invoke_result_t<Proj, const T&>,
            std::invoke_result_t<Proj, const T&>
          > Compare>
#endif
constexpr void tailq_base<T, E, S1, D1>::merge(other_list_t<S2, D2> &other,
                                               Compare comp, Proj proj)
    noexcept(s_has_nothrow_extractor &&
             util::is_nothrow_proj_relation<const_reference, Proj, Compare>)
{
  if (this == &other)
    return;

  auto f1 = cbegin();
  auto e1 = cend();
  auto f2 = other.cbegin();
  auto e2 = other.cend();

  if constexpr (std::integral<S1>)
    getHeadData().m_sz += std::size(other);

  if constexpr (std::integral<S2>)
    other.getHeadData().m_sz = 0;

  while (f1 != e1 && f2 != e2) {
    if (util::projection_is_ordered_before(proj, comp, f1, f2)) {
      ++f1;
      continue;
    }

    // f2 < f1, scan the range [f2, mEnd) containing all items smaller than
    // f1 (the "merge range"), which are out of order. Unlike most generic
    // algorithms, remove_range uses an inclusive range rather than a
    // half-open one, so keep track of the previous element.
    auto mPrev = f2;
    auto mEnd = std::ranges::next(mPrev);
    while (mEnd != e2 &&
           util::projection_is_ordered_before(proj, comp, mEnd, f1))
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

template <typename T, tailq_entry_extractor<T> E, optional_size S1, typename D1>
template <optional_size S2, typename D2>
constexpr void tailq_base<T, E, S1, D1>::splice(const_iterator pos,
    other_list_t<S2, D2> &other) noexcept(s_has_nothrow_extractor) {
  if (other.empty())
    return;

  auto first = other.cbegin();
  auto last = --other.cend();

  if constexpr (std::integral<S1>)
    getHeadData().m_sz += std::size(other);

  if constexpr (std::integral<S2>)
    getHeadData().m_sz = 0;

  other.remove_range(first, last);
  insert_range(pos, first, last);
}

template <typename T, tailq_entry_extractor<T> E, optional_size S1, typename D1>
template <optional_size S2, typename D2>
constexpr void tailq_base<T, E, S1, D1>::splice(
    const_iterator pos, other_list_t<S2, D2> &other,
    typename other_list_t<S2, D2>::const_iterator first,
    typename other_list_t<S2, D2>::const_iterator last)
    noexcept(s_has_nothrow_extractor) {
  if (first == last)
    return;

  if constexpr (std::integral<S1> || std::integral<S2>) {
    const auto n = std::ranges::distance(first, last);

    if constexpr (std::integral<S1>)
      getHeadData().m_sz += n;

    if constexpr (std::integral<S2>)
      other.getHeadData().m_sz -= n;
  }

  --last;
  other.remove_range(first, last);
  insert_range(pos, first, last);
}

template <typename T, tailq_entry_extractor<T> E, optional_size S, typename D>
#if 0
template <std::invocable<typename tailq_base<T, E, S, D>::const_reference> Proj,
          std::predicate<std::invoke_result_t<Proj,
              typename tailq_base<T, E, S, D>::const_reference>> Pred>
#else
template <std::invocable<const T&> Proj,
          std::predicate<std::invoke_result_t<Proj, const T&>> Pred>
#endif
constexpr CSG_TYPENAME tailq_base<T, E, S, D>::size_type
tailq_base<T, E, S, D>::remove_if(Pred pred, Proj proj)
    noexcept(s_has_nothrow_extractor &&
             util::is_nothrow_proj_invocable<const_reference, Proj, Pred>) {
  size_type nRemoved = 0;
  const const_iterator e = cend();
  const_iterator i = cbegin();

  while (i != e) {
    if (!std::invoke(pred, std::invoke(proj, *i))) {
      // Not removing *i, advance and continue.
      ++i;
      continue;
    }

    // Removing *i; it is slightly more efficient to scan for a contiguous
    // range and call range erase, than to call single-element erase one
    // at a time. Otherwise we're patching list linkage for elements that
    // will eventually be removed anyway.
    const_iterator scanEnd = std::ranges::next(i);
    ++nRemoved;

    while (scanEnd != e && std::invoke(pred, std::invoke(proj, *scanEnd))) {
      ++scanEnd;
      ++nRemoved;
    }

    i = erase(i, scanEnd);
    if (i != e)
      ++i; // i != e, so i == scanEnd; we know !pred(*i) already; advance i
  }

  return nRemoved;
}

template <typename T, tailq_entry_extractor<T> E, optional_size S, typename D>
constexpr void tailq_base<T, E, S, D>::reverse() noexcept(s_has_nothrow_extractor) {
  entry_type *const endEntry = &getHeadData().m_endEntry;
  entry_type *curEntry = endEntry;

  do {
    std::swap(curEntry->next, curEntry->prev);
    curEntry = refToEntry(curEntry->next);
  } while (curEntry != endEntry);
}

template <typename T, tailq_entry_extractor<T> E, optional_size S, typename D>
#if 0
template <std::invocable<typename tailq_base<T, E, S, D>::const_reference> Proj,
          std::equivalence_relation<
            std::invoke_result_t<Proj,
                typename tailq_base<T, E, S, D>::const_reference>,
            std::invoke_result_t<Proj,
                typename tailq_base<T, E, S, D>::const_reference>
          > EqRelation>
#else
template <std::invocable<const T&> Proj,
          std::equivalence_relation<
            std::invoke_result_t<Proj, const T&>,
            std::invoke_result_t<Proj, const T&>
          > EqRelation>
#endif
constexpr void tailq_base<T, E, S, D>::unique(EqRelation eq, Proj proj)
    noexcept(s_has_nothrow_extractor &&
             util::is_nothrow_proj_relation<const_reference, Proj, EqRelation>)
{
  const const_iterator e = cend();
  const_iterator scanStart = cbegin();

  while (scanStart != e) {
    // scanStart is potentially the start of a range of unique items; it always
    // remains in the list but its adjacent duplicates will be removed, if they
    // exist.
    const_iterator scanEnd = std::ranges::next(scanStart);
    while (scanEnd != e &&
           util::projections_are_equivalent(proj, eq, scanStart, scanEnd))
      ++scanEnd;

    // If the range has more than than one item in it, erase all but the first.
    if (++scanStart != scanEnd)
      scanStart = erase(scanStart, scanEnd);
  }
}

template <typename T, tailq_entry_extractor<T> E, optional_size S, typename D>
template <typename Compare, typename Proj, std::unsigned_integral SizeType>
constexpr CSG_TYPENAME tailq_base<T, E, S, D>::const_iterator
tailq_base<T, E, S, D>::merge_sort(const_iterator f1, const_iterator e2,
                                   Compare comp, Proj proj, SizeType n)
    noexcept(s_has_nothrow_extractor) {
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
    --e2; // Move e2 backward to the second element so we can compare.
    if (util::projection_is_ordered_before(proj, comp, f1, e2))
      return f1; // Already sorted
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
  const const_iterator e1 = std::ranges::next(f1, pivot);

  // Recursively sort the two subranges. Because f2 and e1 point to the same
  // element and all links are maintained, the merge sort of the "right half"
  // finds both the value for new f2 and the new value of the e1 iterator.
  // Because f2 and e1 are always equal, e1 is not modified during the merge
  // algorithm. The end iterator for the right half, e2, is not part of
  // either range so it is also never modified.
  f1 = merge_sort(f1, e1, std::ref(comp), std::ref(proj), pivot);
  auto f2 = merge_sort(e1, e2, std::ref(comp), std::ref(proj), n - pivot);

  // The iterator `min` will point to the first element in the merged range
  // (the smallest element in both lists), which we need to return.
  const const_iterator mergedMin =
      util::projection_is_ordered_before(proj, comp, f1, f2) ? f1 : f2;

  // Merge step between the two sorted sublists. Like the `merge` member
  // function, the sorted right half is "merged into" the sorted left half.
  // The invariant we want to maintain is that *f1 < *f2. As long as this
  // is true, f1 is advanced. Whenever it is not true, we scan the maximum
  // subsequence of [f2, e) such that the elements are less than *f1, then
  // unlink that range from its current location and relink it in front of *f1.
  while (f1 != f2 && f2 != e2) {
    if (util::projection_is_ordered_before(proj, comp, f1, f2)) {
      ++f1;
      continue;
    }

    // This code is mostly the same as in the `merge` member function.
    auto mPrev = f2;
    auto mEnd = std::ranges::next(mPrev);
    while (mEnd != e2 &&
           util::projection_is_ordered_before(proj, comp, mEnd, f1))
      mPrev = mEnd++;

    remove_range(f2, mPrev);
    insert_range(f1, f2, mPrev);

    ++f1;
    f2 = mEnd;
  }

  return mergedMin;
}

template <typename T, tailq_entry_extractor<T> E, optional_size S1, typename D1>
template <optional_size S2, typename D2>
constexpr void tailq_base<T, E, S1, D1>::swap_lists(other_list_t<S2, D2> &other)
    noexcept(s_has_nothrow_extractor) {
  // Swap the inline sizes first; this is done prior to swapping the end entries
  // in case one of the lists has a computed size and needs to be scanned.
  if constexpr (std::integral<S1> && std::integral<S2>) {
    std::swap<std::common_type_t<S1, S2>>(getHeadData().m_sz,
                                          other.getHeadData().m_sz);
  }
  else if constexpr (std::integral<S1>)
    getHeadData().m_sz = static_cast<S1>(std::size(other));
  else if constexpr (std::integral<S2>)
    other.getHeadData().m_sz = static_cast<S2>(std::size(*this));

  entry_type *const lhsEndEntry = &getHeadData().m_endEntry;
  entry_type *const lhsFirstEntry = refToEntry(lhsEndEntry->next);
  entry_type *const lhsLastEntry = refToEntry(lhsEndEntry->prev);

  entry_type *const rhsEndEntry = &other.getHeadData().m_endEntry;
  entry_type *const rhsFirstEntry = other.refToEntry(rhsEndEntry->next);
  entry_type *const rhsLastEntry = other.refToEntry(rhsEndEntry->prev);

  // Fix the linkage at the beginning and end of each list into
  // the end entries.
  lhsFirstEntry->prev = lhsLastEntry->next =
      entry_ref_codec::create_direct_entry_ref(rhsEndEntry);

  rhsFirstEntry->prev = rhsLastEntry->next =
      entry_ref_codec::create_direct_entry_ref(lhsEndEntry);

  // Swap the end entries.
  std::swap(*lhsEndEntry, *rhsEndEntry);
}

template <typename T, tailq_entry_extractor<T> E, optional_size S, typename D>
template <typename QueueIt>
constexpr void tailq_base<T, E, S, D>::insert_range(const_iterator pos,
                                                    QueueIt first, QueueIt last)
    noexcept(s_has_nothrow_extractor) {
  // Inserts the closed range [first, last] before pos.
  entry_type *const posEntry = iterToEntry(pos);
  entry_type *const firstEntry = QueueIt::container::iterToEntry(first);
  entry_type *const lastEntry = QueueIt::container::iterToEntry(last);
  entry_type *const beforePosEntry =
      getEntry(pos.m_rEntryExtractor.get_invocable(), posEntry->prev);

  firstEntry->prev = posEntry->prev;
  beforePosEntry->next = first.m_current;
  lastEntry->next = pos.m_current;
  posEntry->prev = last.m_current;
}

template <typename T, tailq_entry_extractor<T> E, optional_size S, typename D>
constexpr void tailq_base<T, E, S, D>::remove_range(const_iterator first,
                                                    const_iterator last)
    noexcept(s_has_nothrow_extractor) {
  // Removes the closed range [first, last].
  entry_type *const firstEntry = iterToEntry(first);
  entry_type *const lastEntry = iterToEntry(last);

  entry_type *const beforeFirstEntry =
      getEntry(first.m_rEntryExtractor.get_invocable(), firstEntry->prev);

  entry_type *const afterLastEntry =
      getEntry(last.m_rEntryExtractor.get_invocable(), lastEntry->next);

  beforeFirstEntry->next = lastEntry->next;
  afterLastEntry->prev = firstEntry->prev;
}

} // End of namespace csg

#endif
