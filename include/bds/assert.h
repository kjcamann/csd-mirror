//==-- bds/assert.h - Assert macros used in libbds --------------*- C++ -*-==//
//
//                     BSD Data Structures (BDS) Library
//
// This file is distributed under the 2-clause BSD Open Source License. See
// LICENSE.TXT for details.
//==-----------------------------------------------------------------------==//

/**
 * @file
 * @brief Defines assertion macros that will soon be replaced with C++20
 *     contract assertions.
 */

#ifndef BDS_ASSERT_H
#define BDS_ASSERT_H

#if !defined(BDS_DISABLE_ASSERTS)

#include <cstdio>
#include <cstdlib>

#if defined(__cpp_lib_experimental_source_location)
#include <experimental/source_location>

#define BDS_ASSERT(Test, ...)                                                  \
  if (!(Test)) {                                                               \
    constexpr auto srcloc = std::experimental::source_location::current();     \
    std::fprintf(stderr, "Assertion failed in %s@%s:%d:%d: " #Test,            \
                 srcloc.function_name(), srcloc.file_name(), srcloc.line(),    \
                 srcloc.column());                                             \
    std::fprintf(stderr, "\nCause: " __VA_ARGS__);                             \
    std::abort();                                                              \
  }

#else // defined(__cpp_lib_experimental_source_location)

#define BDS_ASSERT(Test, ...)                                                  \
  if (!(Test)) {                                                               \
    std::fprintf(stderr, "Assertion failed in %s@%s:%d: " #Test, __FUNCTION__, \
                 __FILE__, __LINE__);                                          \
    std::fprintf(stderr, "\nCause: " __VA_ARGS__);                             \
    std::abort();                                                              \
  }

#endif

#else // !defined(BDS_DISABLE_ASSERTS)

#define BDS_ASSERT(Test, ...)

#endif

#endif
