//==-- csd/assert.h - Assert macros used in libcsd --------------*- C++ -*-==//
//
//                Cyril Software Data Structures (CSD) Library
//
// This file is distributed under the 2-clause BSD Open Source License. See
// LICENSE.TXT for details.
//==-----------------------------------------------------------------------==//

/**
 * @file
 * @brief Defines assertion macros that will soon be replaced with C++20
 *     contract assertions.
 */

#ifndef CSD_ASSERT_H
#define CSD_ASSERT_H

#if !defined(CSD_DISABLE_ASSERTS)

#include <cstdio>
#include <cstdlib>

#if defined(__cpp_lib_experimental_source_location)
#include <experimental/source_location>

#define CSD_ASSERT(Test, ...)                                                  \
  if (!(Test)) {                                                               \
    constexpr auto srcloc = std::experimental::source_location::current();     \
    std::fprintf(stderr, "Assertion failed in %s@%s:%d:%d: " #Test,            \
                 srcloc.function_name(), srcloc.file_name(), srcloc.line(),    \
                 srcloc.column());                                             \
    std::fprintf(stderr, "\nCause: " __VA_ARGS__);                             \
    std::abort();                                                              \
  }

#else // defined(__cpp_lib_experimental_source_location)

#define CSD_ASSERT(Test, ...)                                                  \
  if (!(Test)) {                                                               \
    std::fprintf(stderr, "Assertion failed in %s@%s:%d: " #Test, __FUNCTION__, \
                 __FILE__, __LINE__);                                          \
    std::fprintf(stderr, "\nCause: " __VA_ARGS__);                             \
    std::abort();                                                              \
  }

#endif

#else // !defined(CSD_DISABLE_ASSERTS)

#define CSD_ASSERT(Test, ...)

#endif

#endif
