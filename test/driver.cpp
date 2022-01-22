#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <cstdarg>
#include <cstdio>
#include <exception>

#include <csg/core/assert.h>

extern "C" [[noreturn]] void
csg_assert_function(const csg::assert_info &info, const char *format, ...) noexcept {
  std::fprintf(stderr, "assertion failed: %s <%s@%s:%d>\n", info.test,
               info.function, info.file, info.line);

  if (format) {
    std::fprintf(stderr, "cause: ");

    std::va_list args;
    va_start(args, format);
    std::vfprintf(stderr, format, args);
    va_end(args);

    std::fprintf(stderr, "\n");
  }

  std::terminate();
}
