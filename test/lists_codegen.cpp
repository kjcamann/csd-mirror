#include <algorithm>

#include <csg/core/slist.h>
#include <csg/core/stailq.h>
#include <csg/core/tailq.h>

struct S {
  int i;
  csg::slist_entry<S> next;
};

struct T {
  int i;
  csg::stailq_entry<T> next;
};

struct U {
  int i;
  csg::tailq_entry<U> next;
};

#if defined(CODEGEN_OFFSET)
using slist_t = CSG_SLIST_HEAD_OFFSET_T(S, next);
using stailq_t = CSG_STAILQ_HEAD_OFFSET_T(T, next);
using tailq_t = CSG_TAILQ_HEAD_OFFSET_T(U, next);
#else
using slist_t = csg::slist_head_cinvoke_t<&S::next>;
using stailq_t = csg::stailq_head_cinvoke_t<&T::next>;
using tailq_t = csg::tailq_head_cinvoke_t<&U::next>;
#endif

extern "C" const S *slist_find(slist_t &head, int i) noexcept {
  for (const S &s : head) {
    if (s.i == i)
      return &s;
  }

  return nullptr;
}

extern "C" const S *slist_find_ranges(slist_t &head, int i) noexcept {
  const auto it = std::ranges::find(head, i, &S::i);
  return it == head.end() ? nullptr : std::addressof(*it);
}

extern "C" const T *stailq_find(stailq_t &head, int i) noexcept {
  for (const T &t : head) {
    if (t.i == i)
      return &t;
  }

  return nullptr;
}

extern "C" const T *stailq_find_ranges(stailq_t &head, int i) noexcept {
  const auto it = std::ranges::find(head, i, &T::i);
  return it == head.end() ? nullptr : std::addressof(*it);
}

extern "C" const U *tailq_find(tailq_t &head, int i) noexcept {
  for (const U &u : head) {
    if (u.i == i)
      return &u;
  }

  return nullptr;
}

extern "C" const U *tailq_find_ranges(tailq_t &head, int i) noexcept {
  const auto it = std::ranges::find(head, i, &U::i);
  return it == head.end() ? nullptr : std::addressof(*it);
}
