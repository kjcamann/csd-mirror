//==-- bds/list_common.h - Utilities for queue(3)-style lists ---*- C++ -*-==//
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

#include <cstdint>
#include <functional>
#include <iterator>
#include <type_traits>

#include "intrusive.h"

namespace bds {

#define BDS_DEPRECATE_LIST_ATTR                                                \
  [[deprecated("tailq should always be used instead of list, see the BDS "     \
               "documentation")]]

struct slist_entry;
struct stailq_entry;
struct tailq_entry;

using list_entry BDS_DEPRECATE_LIST_ATTR = tailq_entry;

template <std::size_t Offset>
using slist_entry_offset = offset_extractor<slist_entry, Offset>;

template <std::size_t Offset>
using stailq_entry_offset = offset_extractor<stailq_entry, Offset>;

template <std::size_t Offset>
using tailq_entry_offset = offset_extractor<tailq_entry, Offset>;

template <std::size_t Offset>
using list_entry_offset BDS_DEPRECATE_LIST_ATTR = tailq_entry_offset<Offset>;

template <typename EntryAccess, typename T>
concept bool SListEntryAccessor =
    InvocableEntryAccessor<slist_entry, EntryAccess, T>;

template <typename EntryAccess, typename T>
concept bool STailQEntryAccessor =
    InvocableEntryAccessor<stailq_entry, EntryAccess, T>;

template <typename EntryAccess, typename T>
concept bool TailQEntryAccessor =
    InvocableEntryAccessor<tailq_entry, EntryAccess, T>;

template <typename EntryAccess, typename T>
concept bool ListEntryAccessor =
    InvocableEntryAccessor<list_entry, EntryAccess, T>;

template <typename T, typename EntryAccess, typename Derived>
    requires SListEntryAccessor<EntryAccess, T>
class slist_base;

template <typename T, typename EntryAccess, typename Derived>
    requires STailQEntryAccessor<EntryAccess, T>
class stailq_base;

template <typename T, typename EntryAccess, typename Derived>
    requires TailQEntryAccessor<EntryAccess, T>
class tailq_base;

template <typename T, typename EntryAccess, typename Derived>
    requires ListEntryAccessor<EntryAccess, T>
using list_base BDS_DEPRECATE_LIST_ATTR = tailq_base<T, EntryAccess, Derived>;

// FIXME [C++20] <concepts> use std::DerivedFrom instead
template <template <typename, typename, typename> typename ListBase,
          typename ListType>
concept bool BDSListBase =
    std::is_base_of_v<ListBase<typename ListType::value_type,
                               typename ListType::entry_access_type,
                               typename ListType::derived_type>,
                      ListType>;

template <typename ListType>
concept bool SList = BDSListBase<slist_base, ListType>;

template <typename ListType>
concept bool STailQ = BDSListBase<stailq_base, ListType>;

template <typename ListType>
concept bool TailQ = BDSListBase<tailq_base, ListType>;

template <typename ListType>
concept bool List BDS_DEPRECATE_LIST_ATTR = BDSListBase<list_base, ListType>;

template <typename ListType>
concept bool SListOrQueue = SList<ListType> || STailQ<ListType>;

template <typename ListType>
concept bool BDSList =
    SList<ListType> || STailQ<ListType> || TailQ<ListType> || List<ListType>;

// Encoding is the integer address of `T *` if the low bit is tagged; if the
// low bit is not tagged, encoding is the integer address of a sentinel entry
// structure, because it is the only entry that doesn't have a corresponding
// `T` value
template <typename EntryType, typename EntryAccess, typename T>
    requires InvocableEntryAccessor<EntryType, EntryAccess, T>
struct link_encoder {
  static_assert(alignof(T) > 1);

  using link_access_type = compressed_invocable_ref<EntryAccess, T &>;

  static EntryType *getEntry(link_access_type &fn, T *t) noexcept {
    return std::addressof(std::invoke(fn, *t));
  }

  static EntryType *getEntry(link_access_type &fn, std::uintptr_t p) noexcept {
    return (p & ValueTag) ? getEntry(fn, getValue(p))
                          : reinterpret_cast<EntryType *>(p);
  }

  static T *getValue(std::uintptr_t p) noexcept {
    return reinterpret_cast<T *>(p & ~ValueTag);
  }

  static std::uintptr_t encode(const T *t) noexcept {
    return reinterpret_cast<std::uintptr_t>(t) | ValueTag;
  }

  constexpr static std::uintptr_t ValueTag = 1;
};

// Encoding is the integer address of the entry structure; encoding it from
// `T *` (or recovering `T *` from the entry address) is done via known
// structure offsets and reinterpret_cast. Usable for all types for which
// offsetof is allowed by the implementation.
template <typename EntryType, std::size_t Offset, typename T>
struct link_encoder<EntryType, offset_extractor<EntryType, Offset>, T> {
  using link_access_type =
      compressed_invocable_ref<offset_extractor<EntryType, Offset>, T &>;

  static EntryType *getEntry([[maybe_unused]] link_access_type &,
                             T *t) noexcept {
    return reinterpret_cast<EntryType *>(encode(t));
  }

  static EntryType *getEntry([[maybe_unused]] link_access_type &,
                             std::uintptr_t p) noexcept {
    return reinterpret_cast<EntryType *>(p);
  }

  static T *getValue(std::uintptr_t p) noexcept {
    return reinterpret_cast<T *>(p - Offset);
  }

  static std::uintptr_t encode(const T *t) noexcept {
    return reinterpret_cast<std::uintptr_t>(t) + Offset;
  }
};

template <typename Iter, typename Visitor>
void for_each_safe(Iter first, const Iter last, Visitor v) noexcept {
  while (first != last)
    v(*first++);
}

template <typename C, typename Visitor>
void for_each_safe(C &c, Visitor v) noexcept {
  for_each_safe(std::begin(c), std::end(c), v);
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
    requires std::is_integral_v<SizeType>
typename ListType::const_iterator
forward_list_merge_sort(typename ListType::const_iterator p1,
                        typename ListType::const_iterator e2,
                        Compare comp,
                        SizeType n) noexcept {
#endif

template <typename ListType, typename Compare, typename SizeType>
typename ListType::const_iterator
forward_list_merge_sort(typename ListType::const_iterator p1,
                        typename ListType::const_iterator e2, Compare comp,
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
