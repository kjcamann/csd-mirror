//==-- bds/listfwd.h - Forward decl. for queue(3)-style lists ---*- C++ -*-==//
//
//                     BSD Data Structures (BDS) Library
//
// This file is distributed under the 2-clause BSD Open Source License. See
// LICENSE.TXT for details.
//==-----------------------------------------------------------------------==//

/**
 * @file
 * @brief Contains forward declarations, free functions, and implementation
 *     utilities for the slist, stailq, and tailq classes.
 */

#ifndef BDS_LISTFWD_H
#define BDS_LISTFWD_H

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
concept SListEntryAccessor = requires(entry_access_helper_t<T, EntryAccess> e, T t) {
  { std::invoke(e, t) } -> slist_entry<T> &;
};

template <typename T, SListEntryAccessor<T> EntryAccess,
          CompressedSize SizeMember = no_size>
class slist_head;

template <typename FwdHead,
          SListEntryAccessor<typename FwdHead::value_type> EntryAccess>
class slist_proxy;

template <typename ListType>
concept SList = util::DerivedFromTemplate<ListType, slist_head> ||
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
concept STailQEntryAccessor = requires(entry_access_helper_t<T, EntryAccess> e, T t) {
  { std::invoke(e, t) } -> stailq_entry<T> &;
};

template <typename T, STailQEntryAccessor<T> EntryAccess,
          CompressedSize SizeMember = no_size>
class stailq_head;

template <typename FwdHead,
          STailQEntryAccessor<typename FwdHead::value_type>>
class stailq_proxy;

template <typename ListType>
concept STailQ = util::DerivedFromTemplate<ListType, stailq_head> ||
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
concept TailQEntryAccessor = requires(entry_access_helper_t<T, EntryAccess> e, T t) {
  { std::invoke(e, t) } -> tailq_entry<T> &;
};

template <typename T, TailQEntryAccessor<T> EntryAccess,
          CompressedSize SizeMember = no_size>
class tailq_head;

template <typename FwdHead,
          TailQEntryAccessor<typename FwdHead::value_type> EntryAccess>
class tailq_proxy;

template <typename ListType>
concept TailQ = util::DerivedFromTemplate<ListType, tailq_head> ||
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

// FIXME [C++20] can attributes appear here? This was allowed in gcc but not
// clang
template <typename ListType>
concept List /*BDS_DEPRECATE_LIST_ATTR*/ = TailQ<ListType>;

/*
 * Concepts / helper functions used for multiple list types
 */

template <typename T>
concept LinkedListEntry =
    util::InstantiatedFrom<T, slist_entry> ||
    util::InstantiatedFrom<T, stailq_entry> ||
    util::InstantiatedFrom<T, tailq_entry>;

template <typename ListType>
concept SListOrQueue = SList<ListType> || STailQ<ListType>;

template <typename ListType>
concept LinkedList =
    SList<ListType> || STailQ<ListType> || TailQ<ListType> || List<ListType>;

template <LinkedList C, typename T>
C::size_type erase(C &c, const T &value) noexcept {
  return c.remove(value);
}

template <LinkedList C, typename UnaryPredicate>
C::size_type erase_if(C &c, UnaryPredicate pred) noexcept {
  return c.remove_if(pred);
}

namespace detail {

// FIXME [C++20]: is typename required even after P0634? gcc and clang
// implementations disagree.
// FIXME: we want to constrain ListType to be SListOrQueue, but this is
// defined in terms of inheritance from classes like slist_head or
// slist_proxy, but not slist_base (and this function is only called
// internally by list base class) so the constraint fails.
template <typename ListType, typename Compare, typename SizeType>
    // With <concepts>, last param should be `std::Integral SizeType``, also
    // need a <concepts> constraint on Compare. Any changes also need to made
    // in the friend declarations in stailq and slist.
    requires std::is_integral_v<SizeType>
ListType::const_iterator
forward_list_merge_sort(typename ListType::const_iterator p1,
                        typename ListType::const_iterator e2,
                        Compare comp,
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
