//==-- csg/core/assert.h - Assert macros used in CSG ------------*- C++ -*-==//
//
//                Cyril Software Data Structures (CSD) Library
//
// This file is distributed under the 2-clause BSD Open Source License. See
// LICENSE.TXT for details.
//==-----------------------------------------------------------------------==//

/**
 * @file
 * @brief Defines assertion macros that will be replaced with contract
 *     assertions once they are standardized.
 */

#ifndef CSG_CORE_ASSERT_H
#define CSG_CORE_ASSERT_H

#if !defined(CSG_DEBUG_LEVEL)
#define CSG_DEBUG_LEVEL 0
#endif

#if CSG_DEBUG_LEVEL == 0
#define CSG_ASSERT(...) ((void)0)
#else

#define CSG_ASSERT(TEST, ...) ((TEST) ? (void)0 :                              \
    csg_assert_function(csg::assert_info{__FUNCTION__, __FILE__, __LINE__,     \
                        #TEST} __VA_OPT__(,) __VA_ARGS__))

#endif

namespace csg {

struct assert_info {
  constexpr assert_info() : file{nullptr}, line{-1}, test{nullptr} {}

  constexpr assert_info(const char *fn, const char *f, int l,
                        const char *t)
  : function{fn}, file{f}, line{l}, test{t} {}

  const char *function;
  const char *file;
  int line;
  const char *test;
};

} // End of namespace csg

extern "C" void csg_assert_function(const csg::assert_info &,
                                    const char *format, ...);

inline void csg_assert_function(const csg::assert_info &i) {
  csg_assert_function(i, nullptr);
}

#endif
