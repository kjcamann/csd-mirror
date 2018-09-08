//==-- bds/list.h - intrusive linked-list implementation --------*- C++ -*-==//
//
//                     BSD Data Structures (BDS) Library
//
// This file is distributed under the 2-clause BSD Open Source License. See
// LICENSE.TXT for details.
//==-----------------------------------------------------------------------==//

/**
 * @file
 * @brief Included only for "API discovery" purposes; users should not include
 *     this file.
 *
 * Unlike in BSD's queue(3) library, the libbds list is the same as the tailq,
 * and the "list" types are just type aliases for the "tailq" types. This is
 * because the list cannot be made STL-compatible without giving it the same
 * implementation as the tailq.
 *
 * The user guide contains a detailed explanation of this, in the section
 * covering differences from the BSD implementation. Users should prefer
 * tailq over list, and all list-related types are marked as deprecated.
 */

#ifndef BSD_LIST_H
#define BSD_LIST_H

#include <bds/tailq.h>

namespace bds {

template <typename SizeType>
using list_fwd_head BDS_DEPRECATE_LIST_ATTR = tailq_fwd_head<SizeType>;

template <typename T, typename EntryAccessor, SizeMember SizeType>
using list_container BDS_DEPRECATE_LIST_ATTR =
    tailq_container<T, EntryAccessor, SizeType>;

template <typename T, typename EntryAccessor, SizeMember SizeType>
using list_head BDS_DEPRECATE_LIST_ATTR =
    tailq_head<T, EntryAccessor, SizeType>;

} // End of namespace bsd

#endif
