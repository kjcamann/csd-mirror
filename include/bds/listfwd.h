//==-- bds/listfwd.h - Utilities for queue(3)-style lists -------*- C++ -*-==//
//
//                     BSD Data Structures (BDS) Library
//
// This file is distributed under the 2-clause BSD Open Source License. See
// LICENSE.TXT for details.
//==-----------------------------------------------------------------------==//

/**
 * @file
 * @brief Contains forward declarations, free functions, and implementation
 *     utilies for the slist, stailq, and tailq classes.
 */

#ifndef BDS_LIST_COMMON_H
#define BDS_LIST_COMMON_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <type_traits>

#include <bds/intrusive.h>
#include <bds/utility.h>

namespace bds {

#define BDS_DEPRECATE_LIST_ATTR                                                \
  [[deprecated("tailq should always be used instead of list, see the BDS "     \
               "documentation")]]

/*
 * SList
 */

template <typename T>
struct slist_entry;

template <std::size_t Offset>
struct slist_entry_offset;

template <typename T, std::size_t Offset>
struct entry_access_traits<T, slist_entry_offset<Offset>> {
  using entry_access_type = offset_extractor<slist_entry<T>, Offset>;
};

template <typename T, CompressedSize SizeMember = no_size>
struct slist_fwd_head;

template <typename EntryAccess, typename T>
concept bool SListEntryAccessor =
    InvocableEntryAccessor<slist_entry<T>, EntryAccess, T>;

template <typename T, typename EntryAccess, CompressedSize SizeMember = no_size>
    requires SListEntryAccessor<entry_access_helper_t<T, EntryAccess>, T>
class slist_head;

template <typename FwdHead, typename EntryAccess>
    requires SListEntryAccessor<
        entry_access_helper_t<typename FwdHead::value_type, EntryAccess>,
        typename FwdHead::value_type>
class slist_proxy;

template <typename ListType>
concept bool SList = util::DerivedFromTemplate<ListType, slist_head> ||
    util::DerivedFromTemplate<ListType, slist_proxy>;

#define BDS_SLIST_HEAD_OFFSET_T(TYPE, MEMBER, ...) \
  bds::slist_head<TYPE, bds::slist_entry_offset<offsetof(TYPE, MEMBER)> \
                 __VA_OPT__(,) __VA_ARGS__>

#define BDS_SLIST_PROXY_OFFSET_T(TYPE, MEMBER, ...) \
  bds::slist_proxy<bds::slist_fwd_head<TYPE __VA_OPT__(,) __VA_ARGS__>, \
                   bds::slist_entry_offset<offsetof(TYPE, MEMBER)>>

template <auto Invocable, CompressedSize SizeMember = no_size>
using slist_head_cinvoke_t = slist_head<
    std::remove_cvref_t<typename cinvoke_traits_t<Invocable>::argument_type>,
    constexpr_invocable<Invocable>, SizeMember>;

template <auto Invocable, CompressedSize SizeMember = no_size>
using slist_proxy_cinvoke_t = slist_proxy<
    slist_fwd_head<typename cinvoke_traits_t<Invocable>::argument_type, SizeMember>,
    constexpr_invocable<Invocable>>;

/*
 * STailQ
 */

template <typename T>
struct stailq_entry;

template <std::size_t Offset>
struct stailq_entry_offset;

template <typename T, std::size_t Offset>
struct entry_access_traits<T, stailq_entry_offset<Offset>> {
  using entry_access_type = offset_extractor<stailq_entry<T>, Offset>;
};

template <typename T, CompressedSize SizeMember = no_size>
struct stailq_fwd_head;

template <typename EntryAccess, typename T>
concept bool STailQEntryAccessor =
    InvocableEntryAccessor<stailq_entry<T>, EntryAccess, T>;

template <typename T, typename EntryAccess, CompressedSize SizeMember = no_size>
    requires STailQEntryAccessor<entry_access_helper_t<T, EntryAccess>, T>
class stailq_head;

template <typename FwdHead, typename EntryAccess>
    requires STailQEntryAccessor<
        entry_access_helper_t<typename FwdHead::value_type, EntryAccess>,
        typename FwdHead::value_type>
class stailq_proxy;

template <typename ListType>
concept bool STailQ = util::DerivedFromTemplate<ListType, stailq_head> ||
    util::DerivedFromTemplate<ListType, stailq_proxy>;

#define BDS_STAILQ_HEAD_OFFSET_T(TYPE, MEMBER, ...) \
  bds::stailq_head<TYPE, bds::stailq_entry_offset<offsetof(TYPE, MEMBER)> \
                   __VA_OPT__(,) __VA_ARGS__>;

#define BDS_STAILQ_PROXY_OFFSET_T(TYPE, MEMBER, ...) \
  bds::stailq_proxy<bds::stailq_fwd_head<TYPE __VA_OPT__(,) __VA_ARGS__>, \
                    bds::stailq_entry_offset<offsetof(TYPE, MEMBER)>>

template <auto Invocable, CompressedSize SizeMember = no_size>
using stailq_head_cinvoke_t = stailq_head<
    std::remove_cvref_t<typename cinvoke_traits_t<Invocable>::argument_type>,
    constexpr_invocable<Invocable>, SizeMember>;

template <auto Invocable, CompressedSize SizeMember = no_size>
using stailq_proxy_cinvoke_t = stailq_proxy<
    stailq_fwd_head<typename cinvoke_traits_t<Invocable>::argument_type, SizeMember>,
    constexpr_invocable<Invocable>>;

/*
 * TailQ
 */

template <typename T>
struct tailq_entry;

template <std::size_t Offset>
struct tailq_entry_offset;

template <typename T, std::size_t Offset>
struct entry_access_traits<T, tailq_entry_offset<Offset>> {
  using entry_access_type = offset_extractor<tailq_entry<T>, Offset>;
};

template <typename T, CompressedSize SizeMember = no_size>
struct tailq_fwd_head;

template <typename EntryAccess, typename T>
concept bool TailQEntryAccessor =
    InvocableEntryAccessor<tailq_entry<T>, EntryAccess, T>;

template <typename T, typename EntryAccess, CompressedSize SizeMember = no_size>
    requires TailQEntryAccessor<entry_access_helper_t<T, EntryAccess>, T>
class tailq_head;

template <typename FwdHead, typename EntryAccess>
    requires TailQEntryAccessor<
        entry_access_helper_t<typename FwdHead::value_type, EntryAccess>,
        typename FwdHead::value_type>
class tailq_proxy;

template <typename ListType>
concept bool TailQ = util::DerivedFromTemplate<ListType, tailq_head> ||
    util::DerivedFromTemplate<ListType, tailq_proxy>;

#define BDS_TAILQ_HEAD_OFFSET_T(TYPE, MEMBER, ...) \
  bds::tailq_head<TYPE, bds::tailq_entry_offset<offsetof(TYPE, MEMBER)> \
                  __VA_OPT__(,) __VA_ARGS__>

#define BDS_TAILQ_PROXY_OFFSET_T(TYPE, MEMBER, ...) \
  bds::tailq_proxy<bds::tailq_fwd_head<TYPE __VA_OPT__(,) __VA_ARGS__>, \
                   bds::tailq_entry_offset<offsetof(TYPE, MEMBER)>>

template <auto Invocable, CompressedSize SizeMember = no_size>
using tailq_head_cinvoke_t = tailq_head<
    std::remove_cvref_t<typename cinvoke_traits_t<Invocable>::argument_type>,
    constexpr_invocable<Invocable>, SizeMember>;

template <auto Invocable, CompressedSize SizeMember = no_size>
using tailq_proxy_cinvoke_t = tailq_proxy<
    tailq_fwd_head<typename cinvoke_traits_t<Invocable>::argument_type, SizeMember>,
    constexpr_invocable<Invocable>>;

/*
 * List (deprecated)
 */

template <typename T>
using list_entry BDS_DEPRECATE_LIST_ATTR = tailq_entry<T>;

template <std::size_t Offset>
using list_entry_offset BDS_DEPRECATE_LIST_ATTR = tailq_entry_offset<Offset>;

template <typename ListType>
concept bool List BDS_DEPRECATE_LIST_ATTR = TailQ<ListType>;

/*
 * Concepts / helper functions used for multiple list types
 */

template <typename T>
concept bool LinkedListEntry =
    util::InstantiatedFrom<T, slist_entry> ||
    util::InstantiatedFrom<T, stailq_entry> ||
    util::InstantiatedFrom<T, tailq_entry>;

template <typename ListType>
concept bool SListOrQueue = SList<ListType> || STailQ<ListType>;

template <typename ListType>
concept bool LinkedList =
    SList<ListType> || STailQ<ListType> || TailQ<ListType> || List<ListType>;

template <typename Iter, typename Visitor>
void for_each_safe(Iter first, const Iter last, Visitor v) noexcept {
  while (first != last)
    v(*first++);
}

template <bds::LinkedList C, typename Visitor>
void for_each_safe(C &c, Visitor v) noexcept {
  for_each_safe(std::begin(c), std::end(c), v);
}

template <bds::LinkedList C, typename T>
C::size_type erase(C &c, const T &value) noexcept {
  return c.remove(value);
}

template <bds::LinkedList C, typename UnaryPredicate>
C::size_type erase_if(C &c, UnaryPredicate pred) noexcept {
  return c.remove_if(pred);
}

namespace detail {

// FIXME [C++20]: for the moment we remove the constrained function declaration
// of detail::forward_list_merge_sort because (due to a bug?) we cannot make it
// a friend of stailq_base and slist_base (the friend declaration is considered
// a new declaration).
#if 0
template <SListOrQueue ListType, typename Compare, typename SizeType>
    // With <concepts>, last param should be `std::Integral SizeType``, also
    // need a <concepts> constraint on Compare. Any changes also need to made
    // in the friend declarations in stailq and slist.
    requires std::is_integral_v<SizeMember>
ListType::const_iterator
forward_list_merge_sort(ListType::const_iterator p1,
                        ListType::const_iterator e2,
                        Compare comp,
                        SizeType n) noexcept {
#endif

template <typename ListType, typename Compare, typename SizeType>
ListType::const_iterator
forward_list_merge_sort(ListType::const_iterator p1,
                        ListType::const_iterator e2, Compare comp,
                        SizeType n) noexcept {

  // In-place merge sort for slist and stailq; those classes cannot reuse
  // their `merge` member function because that merges two different lists,
  // leaving the second list empty. Here, the merge operation is "in place"
  // so the code has a different structure.
  //
  // This algorithm was designed by adapting the tailq's merge_sort to use
  // a forward list's "insert_after" semantics. Although it works, its use
  // of open ranges makes it somewhat hard to follow.
  //
  // This function is called to sort the open range (p1, e2). It splits this
  // input range into the subranges (p1, e1) and (p2, e2), where
  // std::next(p2) == e1, and each subrange is recursively sorted. The
  // two sorted subranges are then merged.
  //
  // This function returns an iterator pointing to the last element of the
  // sorted range, i.e., it returns the iterator to the element prior to e2,
  // after being sorted (in general, the last element in the open input range
  // (p1, e2) will not be the same prior to sorting vs. after sorting).

  // Base cases for recursion: manually sort small lists.
  switch (n) {
  case 0:
  case 1:
    return std::next(p1);

  case 2:
    auto f1 = std::next(p1);
    auto f2 = std::next(f1);
    if (comp(*f1, *f2))
      return f2;
    else {
      // Two element list in reversed order; swap order of the elements.
      ListType::getEntry(p1)->next = ListType::getEncoding(f2);
      ListType::getEntry(f2)->next = ListType::getEncoding(f1);
      ListType::getEntry(f1)->next = ListType::getEncoding(e2);
      return f1;
    }
  }

  // Explicitly form the ranges (p1, e1) and (p2, e2) from (p1, e2) by selecting
  // a pivot element based on the range length to compute p2. Since
  // e1 == std::next(p2) always holds, we omit e1 as a separate variable.
  SizeType pivot = n / 2;
  typename ListType::const_iterator p2 = std::next(p1, pivot);

  p2 = forward_list_merge_sort<ListType>(p1, std::next(p2), comp, pivot);
  const auto pEnd = forward_list_merge_sort<ListType>(p2, e2, comp, n - pivot);

  auto f1 = std::next(p1);
  auto f2 = std::next(p2);

  while (f1 != f2 && f2 != e2) {
    if (comp(*f1, *f2)) {
      p1 = f1++;
      continue;
    }

    // *f2 < *f1, scan the "merge range" [f2, pScan] that will merged by linking
    // it in front f1 (or equivalently, after p1).
    typename ListType::const_iterator pScan = f2;
    typename ListType::const_iterator scan = std::next(pScan);
    while (scan != e2 && comp(*scan, *f1))
      pScan = scan++;

    // Remove the scanned merge range, [f2, pScan], from its current linkage
    // point after p2, by linking p2 directly to scan (the successor to pScan).
    ListType::getEntry(p2)->next = ListType::getEncoding(scan);

    // Link the scanned merge range, [f2, pScan], after p1 (in front of f1).
    ListType::insert_range_after(p1, f2, pScan);

    // Prepare f2 for the next iteration. p2 is already the predecessor of
    // scan.
    f2 = scan;

    // Advance f1, because we already know that !comp(*scan, *f1) by the scan
    // loop termination.
    p1 = f1++;
  }

  if (std::next(pEnd) == e2)
    return pEnd;
  else {
    while (std::next(p2) != e2)
      ++p2;
  }

  return p2;
}

} // End of namespace detail

} // End of namespace bds

#endif
