#ifndef CSD_LIST_TEST_UTIL_H
#define CSD_LIST_TEST_UTIL_H

#include <algorithm>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <random>
#include <ranges>
#include <utility>
#include <vector>

#include <catch2/catch.hpp>

#include <csg/core/listfwd.h>
#include <csg/core/utility.h>

template <typename T, typename O>
concept compatible_list = std::ranges::input_range<T> && csg::linked_list<T> &&
    std::same_as<typename T::value_type, typename O::value_type> &&
    std::same_as<typename T::entry_extractor_type, typename O::entry_extractor_type>;

// A list whose linkage entry is a directly-accessible member variable of the
// list.
template <template <typename> class EntryType>
struct DirectEntryList {
  std::int64_t i;
  EntryType<DirectEntryList> next;

  std::weak_ordering operator<=>(const DirectEntryList &other) const noexcept {
    return i <=> other.i;
  }

  bool operator==(const DirectEntryList &other) const noexcept {
    return (*this <=> other) == std::weak_ordering::equivalent;
  }
};

// A list whose linkage entry is extracted by calling the "next" accessor
// member function
template <template <typename> class EntryType>
class AccessorEntryList {
public:
  AccessorEntryList(std::int64_t i) noexcept : _i{i} {}

  std::int64_t &i() noexcept { return _i; }
  const std::int64_t &i() const noexcept { return _i; }
  EntryType<AccessorEntryList> &next() noexcept { return _next; }

  std::weak_ordering operator<=>(const AccessorEntryList&other) const noexcept {
    return _i <=> other._i;
  }

  bool operator==(const AccessorEntryList &other) const noexcept {
    return (*this <=> other) == std::weak_ordering::equivalent;
  }

private:
  std::int64_t _i;
  EntryType<AccessorEntryList> _next;
};

// A list
template <template <typename> class>
class StatefulExtractorList;

template <template <typename> class EntryType>
class StatefulExtractor {
public:
  StatefulExtractor() noexcept : numAccesses{0}, movedFrom{false}, alive{true} {}

  StatefulExtractor(const StatefulExtractor &) = delete;

  StatefulExtractor(StatefulExtractor &&other) noexcept
      : movedFrom{false}, alive{true} {
    numAccesses = other.numAccesses;
    other.numAccesses = 0;
    other.movedFrom = true;
  }

  ~StatefulExtractor() { alive = false; }

  StatefulExtractor &operator=(const StatefulExtractor &) = delete;

  StatefulExtractor &operator=(StatefulExtractor &&other) noexcept {
    numAccesses = other.numAccesses;
    other.numAccesses = 0;
    other.movedFrom = true;
    return *this;
  }

  EntryType<StatefulExtractorList<EntryType>>
  &operator()(StatefulExtractorList<EntryType> &u) noexcept {
    // The odd `if(!alive) REQUIRE(alive);` construction is so that we still
    // fail the test if we're accessing a dangling iterator but if we're not,
    // then we won't add to the number of counted assertions. Otherwise catch
    // will report something like "1 million assertions in 100 test cases".
    if (!alive)
      REQUIRE( alive );
    ++numAccesses;
    return u._next;
  }

  std::uint64_t numAccesses = 0;
  bool movedFrom;
  bool alive;
};

template <template <typename> class EntryType>
class StatefulExtractorList {
public:
  using extractor_type = StatefulExtractor<EntryType>;

  StatefulExtractorList(std::int64_t i) noexcept : _i{i} {}
  std::int64_t &i() noexcept { return _i; }
  const std::int64_t &i() const noexcept { return _i; }

  std::weak_ordering operator<=>(const StatefulExtractorList &other) const noexcept {
    return _i <=> other._i;
  }

  bool operator==(const StatefulExtractorList &other) const noexcept {
    return (*this <=> other) == std::weak_ordering::equivalent;
  }

private:
  friend class StatefulExtractor<EntryType>;
  std::int64_t _i;
  EntryType<StatefulExtractorList> _next;
};

// Ordinarily a "fwd_head" type (e.g., tailq_fwd_head) is used to declare a
// list head when the entry extractor cannot be not defined yet. Somewhere
// else in the code (after the extractor can be defined), the corresponding
// proxy type is used. To ease the testing of the "fwd_head" pattern, we
// make it more like the "head" style classes, in that a single declaration
// declares both the fwd_head object and exposes the interface of the proxy,
// via inheritance. This allows most test cases to be implemented as template
// functions taking a single type parameter, which will either be the "_head"
// type or this class.
template <typename ListProxyType>
struct list_test_proxy : public ListProxyType {
public:
  using fwd_head_type = CSG_TYPENAME ListProxyType::fwd_head_type;
  using entry_extractor_type = CSG_TYPENAME ListProxyType::entry_extractor_type;
  using list_proxy_type = ListProxyType;

  list_test_proxy() noexcept : ListProxyType{ctorFwdHead()} {}

  list_test_proxy(const list_test_proxy &) = delete;

  list_test_proxy(list_test_proxy &&other) noexcept
      : ListProxyType{ctorFwdHead(), std::move(other)} {}

  template <compatible_list<ListProxyType> O>
  list_test_proxy(O &&other) noexcept
      : ListProxyType{ctorFwdHead(), std::move(other)} {}

  template <std::input_iterator InputIt, std::sentinel_for<InputIt> Sentinel>
  list_test_proxy(InputIt first, Sentinel last) noexcept
      : ListProxyType{ctorFwdHead(), first, last} {}

  template <std::ranges::input_range Range, typename... Ts>
  list_test_proxy(Range &&r, Ts &&...vs) noexcept
      : ListProxyType{ctorFwdHead(), r, std::forward<Ts>(vs)...} {}

  list_test_proxy(std::initializer_list<typename ListProxyType::pointer> ilist) noexcept
      : ListProxyType{ctorFwdHead(), ilist} {}

  template <std::input_iterator InputIt, std::sentinel_for<InputIt> Sentinel,
            csg::util::can_direct_initialize<entry_extractor_type> U>
  list_test_proxy(InputIt first, Sentinel last, U &&u) noexcept
      : ListProxyType{ctorFwdHead(), first, last, std::forward<U>(u)} {}

  template <csg::util::can_direct_initialize<entry_extractor_type> U>
  list_test_proxy(std::initializer_list<typename ListProxyType::pointer> ilist,
                  U &&u) noexcept
      : ListProxyType{ctorFwdHead(), ilist, std::forward<U>(u)} {}

  ~list_test_proxy() {
    auto *const pFwdHead = reinterpret_cast<fwd_head_type *>(fwdHeadBuf);
    pFwdHead->~fwd_head_type();
  }

  using ListProxyType::operator=;

  list_test_proxy &operator=(const list_test_proxy &) = delete;

  list_test_proxy &operator=(list_test_proxy &&other) noexcept {
    this->ListProxyType::operator=(std::move(other));
    return *this;
  }

  template <std::ranges::input_range Range>
      requires std::constructible_from<typename ListProxyType::pointer,
                                       std::ranges::range_reference_t<Range>>
  list_test_proxy &operator=(Range &&r) noexcept {
    this->ListProxyType::operator=(r);
    return *this;
  }

  list_test_proxy &operator=(std::initializer_list<typename ListProxyType::pointer> ilist) noexcept {
    this->ListProxyType::operator=(ilist);
    return *this;
  }

private:
  // fwd_head lives in a char buffer so we can run its default constructor
  // (which will clear the head pointers) prior to running our base class
  // constructor, which may invoke a move operation that assumes an
  // initialized list (i.e., default constructed == empty).
  alignas(fwd_head_type) std::byte fwdHeadBuf[sizeof(fwd_head_type)];

  fwd_head_type &ctorFwdHead() noexcept {
    return *new (fwdHeadBuf) fwd_head_type{};
  }
};

template <typename T>
concept test_proxy = csg::linked_list<T> && requires {
  typename T::fwd_head_type;
  typename T::list_proxy_type;
};

// FIXME [docs]: explain why we can't use push_front here in impl. docs
template <typename ListType, typename S>
    requires csg::singly_linked_list<std::remove_reference_t<ListType>>
auto insert_front(ListType &&L, S *s) noexcept {
  return L.insert_after(L.before_begin(), s);
}

template <typename ListType, std::input_iterator InputIt,
          std::sentinel_for<InputIt> Sentinel>
    requires csg::singly_linked_list<std::remove_reference_t<ListType>>
auto insert_front(ListType &&L, InputIt begin, Sentinel end) noexcept {
  return L.insert_after(L.before_begin(), begin, end);
}

template <typename ListType, std::ranges::input_range Range>
    requires csg::singly_linked_list<std::remove_reference_t<ListType>>
auto insert_front(ListType &&L, Range &&r) noexcept {
  return L.insert_after(L.before_begin(), r);
}

template <typename ListType>
    requires csg::singly_linked_list<std::remove_reference_t<ListType>>
auto insert_front(ListType &&L,
                  std::initializer_list<
                      typename std::remove_reference_t<ListType>::pointer
                  > i) noexcept {
  return L.insert_after(L.before_begin(), i);
}

template <typename ListType, typename S>
    requires csg::tailq<std::remove_reference_t<ListType>>
auto insert_front(ListType &&L, S *s) noexcept {
  return L.insert(L.begin(), s);
}

template <typename ListType, std::input_iterator InputIt,
          std::sentinel_for<InputIt> Sentinel>
    requires csg::tailq<std::remove_reference_t<ListType>>
auto insert_front(ListType &&L, InputIt begin, Sentinel end) noexcept {
  return L.insert(L.begin(), begin, end);
}

template <typename ListType, std::ranges::input_range Range>
    requires csg::tailq<std::remove_reference_t<ListType>>
auto insert_front(ListType &&L, Range &&r) noexcept {
  return L.insert(L.begin(), r);
}

template <typename ListType>
    requires csg::tailq<std::remove_reference_t<ListType>>
auto insert_front(ListType &&L,
                  std::initializer_list<
                      typename std::remove_reference_t<ListType>::pointer
                  > i) noexcept {
  return L.insert(L.begin(), i);
}

template <typename ListType>
    requires csg::singly_linked_list<std::remove_reference_t<ListType>>
auto insert_after(ListType &&L,
                  CSG_TYPENAME std::remove_reference_t<ListType>::const_iterator i,
                  CSG_TYPENAME std::remove_reference_t<ListType>::pointer s) noexcept {
  return L.insert_after(i, s);
}

template <typename ListType>
    requires csg::tailq<std::remove_reference_t<ListType>>
auto insert_after(ListType &&L,
                  CSG_TYPENAME std::remove_reference_t<ListType>::const_iterator i,
                  CSG_TYPENAME std::remove_reference_t<ListType>::pointer s) noexcept {
  return L.insert(std::next(i), s);
}

template <typename ListType>
    requires csg::singly_linked_list<std::remove_reference_t<ListType>>
auto erase_front(ListType &&L) noexcept {
  return L.erase_after(L.before_begin());
}

template <typename ListType>
    requires csg::tailq<std::remove_reference_t<ListType>>
auto erase_front(ListType &&L) noexcept {
  return L.erase(L.begin());
}

template <typename ListType>
    requires csg::singly_linked_list<std::remove_reference_t<ListType>>
auto erase_after(ListType &&L,
                 CSG_TYPENAME std::remove_reference_t<ListType>::const_iterator begin,
                 CSG_TYPENAME std::remove_reference_t<ListType>::const_iterator end) noexcept {
  return L.erase_after(begin, end);
}

template <typename ListType>
    requires csg::tailq<std::remove_reference_t<ListType>>
auto erase_after(ListType &&L,
                 CSG_TYPENAME std::remove_reference_t<ListType>::const_iterator begin,
                 CSG_TYPENAME std::remove_reference_t<ListType>::const_iterator end) noexcept {
  return L.erase(++begin, end);
}

template <typename ListType>
    requires csg::singly_linked_list<std::remove_reference_t<ListType>>
auto erase_item(ListType &&L,
                CSG_TYPENAME std::remove_reference_t<ListType>::const_iterator pos) {
  return L.find_erase(pos);
}

template <typename ListType>
    requires csg::tailq<std::remove_reference_t<ListType>>
auto erase_item(ListType &&L,
                CSG_TYPENAME std::remove_reference_t<ListType>::const_iterator pos) {
  return L.erase(pos);
}

template <typename T>
std::int64_t get_value(const T &t) {
  if constexpr (requires {t.i();})
    return t.i();
  else
    return t.i;
}

// Generate a random sequence of std::int64_t values of the given size and
// having a uniform distribution in the internal [min, max]
template <typename OutputIt>
std::random_device::result_type
generateRandomInput(OutputIt o, std::size_t size, std::int64_t min,
                    std::int64_t max) noexcept {
  std::random_device trueRandom;
  const auto seed = trueRandom();
  std::default_random_engine engine{seed};
  std::uniform_int_distribution random_dist{min, max};

  for (std::size_t i = 0; i < size; ++i)
    *o++ = random_dist(engine);

  return seed;
}

// Given a sequence of values v \in S, construct items `ListType::value_type{v}`
// using the new operator, buffer their addresses up in a vector, and call
// insert_front on the vector's values, thereby populating the list.
template <typename ListType, typename ForwardIt>
void populateListFromSequence(ListType &&list, ForwardIt begin,
                              ForwardIt end) noexcept {
  using value_type = CSG_TYPENAME std::remove_reference_t<ListType>::value_type;
  using pointer = CSG_TYPENAME std::remove_reference_t<ListType>::pointer;

  std::vector<pointer> items;

  while (begin != end) {
    pointer const p = new value_type{*begin++};
    items.push_back(p);
  }

  insert_front(list, std::begin(items), std::end(items));
}

template <typename ListType>
std::random_device::result_type
populateRandomList(ListType &&list, std::size_t size, std::int64_t min,
                   std::int64_t max) noexcept {
  using value_type = CSG_TYPENAME std::remove_reference_t<ListType>::value_type;
  std::random_device trueRandom;
  const auto seed = trueRandom();
  std::default_random_engine engine{seed};
  std::uniform_int_distribution random_dist{min, max};

  for (std::size_t i = 0; i < size; ++i)
    list.push_front(new value_type{random_dist(engine)});

  return seed;
}

template <typename ListType>
std::random_device::result_type
populateSortedList(ListType &&list, std::size_t size, std::int64_t min,
                   std::int64_t max) noexcept {
  std::unique_ptr<std::int64_t []> values{ new std::int64_t[size] };
  const auto seed = generateRandomInput(values.get(), size, min, max);
  std::sort(values.get(), values.get() + size);
  populateListFromSequence(list, values.get(), values.get() + size);

  return seed;
}

template <typename ListType>
void destroyList(ListType &&list) noexcept {
  csg::for_each_safe(list, [](auto &v) { delete std::addressof(v); });
}

// <algorithm> offers `std::ranges::is_sorted` to check if elements are sorted,
// but this is not enough for correctness testing; mistakes in algorithms like
// in-place list merge sort often leave the resulting list sorted, but elements
// are missing (i.e., the links were not properly maintained). This counts the
// items in the list to verify no elements were "lost." It returns of a tuple
// of (in_order, correct_size).
template <std::ranges::forward_range R, class Proj = std::identity,
          std::indirect_strict_weak_order<
            std::projected<std::ranges::iterator_t<R>, Proj>
          > Compare = std::ranges::less>
std::pair<bool, bool>
is_sorted_check(R &&r, std::size_t size, Compare comp = {}, Proj proj = {}) {
  if (std::ranges::empty(r))
    return {true, size == 0};

  auto cur = std::ranges::begin(r);
  const auto end = std::ranges::end(r);
  std::size_t count = 1;

  auto prev = cur;
  while (++cur != end) {
    if (std::invoke(comp, std::invoke(proj, *cur), std::invoke(proj, *prev)))
      return {false, false};

    ++count;
    prev = cur;
  }

  return {true, count == size};
}

#endif
