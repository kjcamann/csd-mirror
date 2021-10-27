//==-- csg/core/listfwd.h - fwd. decl. for queue(3)-style lists -*- C++ -*-==//
//
//                Cyril Software Data Structures (CSD) Library
//
// This file is distributed under the 2-clause BSD Open Source License. See
// LICENSE.TXT for details.
//==-----------------------------------------------------------------------==//

/**
 * @file
 * @brief Contains forward declarations, free functions, and implementation
 *     utilities for the slist, stailq, and tailq classes.
 */

#ifndef CSG_CORE_LISTFWD_H
#define CSG_CORE_LISTFWD_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <ranges>
#include <type_traits>

#include <csg/core/intrusive.h>
#include <csg/core/utility.h>

namespace csg {

#define CSG_DEPRECATE_LIST_ATTR                                                \
  [[deprecated("tailq should always be used instead of list, see the CSD "     \
               "documentation")]]

/*
 * slist
 */

template <typename T>
struct slist_entry;

template <typename T, optional_size SizeMember = no_size>
class slist_fwd_head;

template <typename EntryEx, typename T>
concept slist_entry_extractor = extractor<EntryEx, slist_entry<T>, T>;

template <typename T, slist_entry_extractor<T> EntryEx,
          optional_size SizeMember = no_size>
class slist_head;

template <util::derived_from_template<slist_fwd_head> FwdHead,
          slist_entry_extractor<typename FwdHead::value_type> EntryEx>
class slist_proxy;

template <typename ListType>
concept slist = std::ranges::input_range<ListType> &&
    (util::derived_from_template<ListType, slist_head> ||
     util::derived_from_template<ListType, slist_proxy>);

#define CSG_SLIST_HEAD_OFFSET_T(TYPE, MEMBER, ...) \
  csg::slist_head<TYPE, CSG_OFFSET_EXTRACTOR(TYPE, MEMBER) \
                  __VA_OPT__(,) __VA_ARGS__>

#define CSG_SLIST_PROXY_OFFSET_T(TYPE, MEMBER, ...) \
  csg::slist_proxy<csg::slist_fwd_head<TYPE __VA_OPT__(,) __VA_ARGS__>, \
                   CSG_OFFSET_EXTRACTOR(TYPE, MEMBER)>

template <auto Invocable, optional_size SizeMember = no_size>
using slist_head_cinvoke_t = slist_head<
    std::remove_cvref_t<typename cinvoke_traits_t<Invocable>::argument_type>,
    invocable_constant<Invocable>, SizeMember>;

template <auto Invocable, optional_size SizeMember = no_size>
using slist_proxy_cinvoke_t = slist_proxy<
    slist_fwd_head<typename cinvoke_traits_t<Invocable>::argument_type, SizeMember>,
    invocable_constant<Invocable>>;

/*
 * stailq
 */

template <typename T>
struct stailq_entry;

template <typename T, optional_size SizeMember = no_size>
class stailq_fwd_head;

template <typename EntryEx, typename T>
concept stailq_entry_extractor = extractor<EntryEx, stailq_entry<T>, T>;

template <typename T, stailq_entry_extractor<T> EntryEx,
          optional_size SizeMember = no_size>
class stailq_head;

template <util::derived_from_template<stailq_fwd_head> FwdHead,
          stailq_entry_extractor<typename FwdHead::value_type> EntryEx>
class stailq_proxy;

template <typename ListType>
concept stailq = std::ranges::input_range<ListType> &&
    (util::derived_from_template<ListType, stailq_head> ||
     util::derived_from_template<ListType, stailq_proxy>);

#define CSG_STAILQ_HEAD_OFFSET_T(TYPE, MEMBER, ...) \
  csg::stailq_head<TYPE, CSG_OFFSET_EXTRACTOR(TYPE, MEMBER) \
                   __VA_OPT__(,) __VA_ARGS__>

#define CSG_STAILQ_PROXY_OFFSET_T(TYPE, MEMBER, ...) \
  csg::stailq_proxy<csg::stailq_fwd_head<TYPE __VA_OPT__(,) __VA_ARGS__>, \
                    CSG_OFFSET_EXTRACTOR(TYPE, MEMBER)>

template <auto Invocable, optional_size SizeMember = no_size>
using stailq_head_cinvoke_t = stailq_head<
    std::remove_cvref_t<typename cinvoke_traits_t<Invocable>::argument_type>,
    invocable_constant<Invocable>, SizeMember>;

template <auto Invocable, optional_size SizeMember = no_size>
using stailq_proxy_cinvoke_t = stailq_proxy<
    stailq_fwd_head<typename cinvoke_traits_t<Invocable>::argument_type, SizeMember>,
    invocable_constant<Invocable>>;

/*
 * tailq
 */

template <typename T>
struct tailq_entry;

template <typename T, optional_size SizeMember = no_size>
class tailq_fwd_head;

template <typename EntryEx, typename T>
concept tailq_entry_extractor = extractor<EntryEx, tailq_entry<T>, T>;

template <typename T, tailq_entry_extractor<T> EntryEx,
          optional_size SizeMember = no_size>
class tailq_head;

template <util::derived_from_template<tailq_fwd_head> FwdHead,
          tailq_entry_extractor<typename FwdHead::value_type> EntryEx>
class tailq_proxy;

template <typename ListType>
concept tailq = std::ranges::input_range<ListType> &&
    (util::derived_from_template<ListType, tailq_head> ||
     util::derived_from_template<ListType, tailq_proxy>);

#define CSG_TAILQ_HEAD_OFFSET_T(TYPE, MEMBER, ...) \
  csg::tailq_head<TYPE, CSG_OFFSET_EXTRACTOR(TYPE, MEMBER) \
                  __VA_OPT__(,) __VA_ARGS__>

#define CSG_TAILQ_PROXY_OFFSET_T(TYPE, MEMBER, ...) \
  csg::tailq_proxy<csg::tailq_fwd_head<TYPE __VA_OPT__(,) __VA_ARGS__>, \
                   CSG_OFFSET_EXTRACTOR(TYPE, MEMBER)>

template <auto Invocable, optional_size SizeMember = no_size>
using tailq_head_cinvoke_t = tailq_head<
    std::remove_cvref_t<typename cinvoke_traits_t<Invocable>::argument_type>,
    invocable_constant<Invocable>, SizeMember>;

template <auto Invocable, optional_size SizeMember = no_size>
using tailq_proxy_cinvoke_t = tailq_proxy<
    tailq_fwd_head<typename cinvoke_traits_t<Invocable>::argument_type, SizeMember>,
    invocable_constant<Invocable>>;

/*
 * list (deprecated)
 */

template <typename T>
using list_entry CSG_DEPRECATE_LIST_ATTR = tailq_entry<T>;

template <typename ListType>
concept list /*CSG_DEPRECATE_LIST_ATTR*/ = tailq<ListType>;

/*
 * Concepts / helper functions used for multiple list types
 */

template <typename ListType>
concept singly_linked_list = slist<ListType> || stailq<ListType>;

template <typename ListType>
concept linked_list = slist<ListType> || stailq<ListType> || tailq<ListType>;

template <linked_list L, typename T>
    requires std::constructible_from<typename L::reference, const T &>
CSG_TYPENAME L::size_type erase(L &list, const T &value) noexcept {
  return list.remove(value);
}

template <linked_list L, std::predicate<typename L::reference> Pred>
CSG_TYPENAME L::size_type erase_if(L &list, Pred pred) noexcept {
  return list.remove_if(pred);
}

namespace detail {

// FIXME [C++20]: is typename required even after P0634? gcc and clang
// implementations disagree.
// FIXME: we want to constrain ListType to be singly_linked_list, but this is
// defined in terms of inheritance from classes like slist_head or
// slist_proxy, but not slist_base (and this function is only called
// internally by list base class) so the constraint fails. Also Compare is not
// constrained, but do we really care about constraining implementation details?
template <typename ListType, typename Compare, typename Proj,
          std::unsigned_integral SizeType>
CSG_TYPENAME ListType::const_iterator
forward_list_merge_sort(CSG_TYPENAME ListType::const_iterator p1,
                        CSG_TYPENAME ListType::const_iterator e2,
                        Compare comp,
                        Proj proj,
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
  // std::ranges::next(p2) == e1, and each subrange is recursively sorted. The
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
    return std::ranges::next(p1);

  case 2:
    auto f1 = std::ranges::next(p1);
    auto f2 = std::ranges::next(f1);
    if (util::projection_is_ordered_before(proj, comp, f1, f2))
      return f2;
    else {
      // Two element list in reversed order; swap order of the elements.
      ListType::iterToEntry(p1)->next = ListType::iterToEntryRef(f2);
      ListType::iterToEntry(f2)->next = ListType::iterToEntryRef(f1);
      ListType::iterToEntry(f1)->next = ListType::iterToEntryRef(e2);
      return f1;
    }
  }

  // Explicitly form the ranges (p1, e1) and (p2, e2) from (p1, e2) by selecting
  // a pivot element based on the range length to compute p2. Since
  // e1 == std::next(p2) always holds, we omit e1 as a separate variable.
  SizeType pivot = n / 2;
  typename ListType::const_iterator p2 = std::ranges::next(p1, pivot);

  p2 = forward_list_merge_sort<ListType>(p1, std::ranges::next(p2),
      std::ref(comp), std::ref(proj), pivot);
  const auto pEnd = forward_list_merge_sort<ListType>(p2, e2,
      std::ref(comp), std::ref(proj), n - pivot);

  auto f1 = std::ranges::next(p1);
  auto f2 = std::ranges::next(p2);

  while (f1 != f2 && f2 != e2) {
    if (util::projection_is_ordered_before(proj, comp, f1, f2)) {
      p1 = f1++;
      continue;
    }

    // *f2 < *f1, scan the "merge range" [f2, pScan] that will merged by linking
    // it in front f1 (or equivalently, after p1).
    typename ListType::const_iterator pScan = f2;
    typename ListType::const_iterator scan = std::ranges::next(pScan);
    while (scan != e2 && util::projection_is_ordered_before(proj, comp, scan, f1))
      pScan = scan++;

    // Remove the scanned merge range, [f2, pScan], from its current linkage
    // point after p2, by linking p2 directly to scan (the successor to pScan).
    ListType::iterToEntry(p2)->next = ListType::iterToEntryRef(scan);

    // Link the scanned merge range, [f2, pScan], after p1 (in front of f1).
    ListType::insert_range_after(p1, f2, pScan);

    // Prepare f2 for the next iteration. p2 is already the predecessor of
    // scan.
    f2 = scan;

    // Advance f1, because we already know that !comp(*scan, *f1) by the scan
    // loop termination.
    p1 = f1++;
  }

  if (std::ranges::next(pEnd) == e2)
    return pEnd;
  else {
    while (std::ranges::next(p2) != e2)
      ++p2;
  }

  return p2;
}

} // End of namespace detail

} // End of namespace csg

#endif
