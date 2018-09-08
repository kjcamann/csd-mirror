#ifndef BDS_LIST_TEST_UTIL_H
#define BDS_LIST_TEST_UTIL_H

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <random>
#include <utility>
#include <vector>

template <typename LinkEntryType>
struct BaseS {
  std::int64_t i;
  LinkEntryType next;
};

template <typename LinkEntryType>
class BaseT {
public:
  BaseT(std::int64_t i) noexcept : _i{i} {}

  std::int64_t &i() noexcept { return _i; }
  LinkEntryType &next() noexcept { return _next; }

private:
  std::int64_t _i;
  LinkEntryType _next;
};

template <typename> class BaseU;

template <typename LinkEntryType>
class StatefulAccessorU {
public:
  StatefulAccessorU() noexcept : numAccesses{0}, movedFrom{false} {}

  StatefulAccessorU(const StatefulAccessorU &) = delete;

  StatefulAccessorU(StatefulAccessorU &&other) noexcept : movedFrom{false} {
    numAccesses = other.numAccesses;
    other.numAccesses = 0;
    other.movedFrom = true;
  }

  ~StatefulAccessorU() = default;

  StatefulAccessorU &operator=(const StatefulAccessorU &) = delete;

  StatefulAccessorU &operator=(StatefulAccessorU &&other) noexcept {
    numAccesses = other.numAccesses;
    other.numAccesses = 0;
    other.movedFrom = true;
    return *this;
  }

  LinkEntryType &operator()(BaseU<LinkEntryType> &u) noexcept {
    ++numAccesses;
    return u._next;
  }

  std::uint64_t numAccesses = 0;
  bool movedFrom;
};

template <typename LinkEntryType>
class BaseU {
public:
  using accessor_type = StatefulAccessorU<LinkEntryType>;

  BaseU(std::int64_t i) noexcept : _i{i} {}

private:
  friend class StatefulAccessorU<LinkEntryType>;
  std::int64_t _i;
  LinkEntryType _next;
};

// Ordinarily a "fwd_head" type (e.g., tailq_fwd_head) is used to declare a
// list head when the entry accessor cannot be not defined yet. Somewhere
// else in the code (after the accessor can be defined), the corresponding
// container type is used. To ease the testing of the "fwd_head" pattern, we
// make it more like the "head" style classes, in that a single declaration
// declares both the fwd_head object and exposes the interface of the container,
// via inheritance. This allows most test cases to be implemented as template
// functions taking a single type parameter, which will either be the "_head"
// type or this class.
template <typename ListContType>
struct list_test_container : public ListContType {
public:
  using fwd_head_type = typename ListContType::fwd_head_type;

  list_test_container() noexcept : ListContType{ctorFwdHead()} {}

  list_test_container(const list_test_container &) = delete;

  list_test_container(list_test_container &&other) noexcept
      : ListContType{ctorFwdHead(), std::move(other)} {}

  template <typename D>
  list_test_container(typename ListContType::template other_list_t<D> &&other) noexcept
      : ListContType{ctorFwdHead(), std::move(other)} {}

  template <typename InputIt, typename... Ts>
  list_test_container(InputIt first, InputIt last, Ts &&...vs) noexcept
      : ListContType{ctorFwdHead(), first, last, std::forward<Ts>(vs)...} {}

  template <typename... Ts>
  list_test_container(std::initializer_list<typename ListContType::value_type *> ilist, Ts &&...vs) noexcept
      : ListContType{ctorFwdHead(), ilist, std::forward<Ts>(vs)...} {}

  ~list_test_container() = default;

  list_test_container &operator=(const list_test_container &) = delete;

  list_test_container &operator=(list_test_container &&other) noexcept {
    this->ListContType::operator=(std::move(other));
    return *this;
  }

  list_test_container &operator=(std::initializer_list<typename ListContType::value_type *> ilist) noexcept {
    this->ListContType::operator=(ilist);
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

// FIXME [docs]: explain why we can't use push_front here
template <bds::SListOrQueue ListType, typename S>
auto insert_front(ListType &L, S *s) noexcept {
  return L.insert_after(L.before_begin(), s);
}

template <bds::SListOrQueue ListType, typename InputIt>
auto insert_front(ListType &L, InputIt begin, InputIt end) noexcept {
  return L.insert_after(L.before_begin(), begin, end);
}

template <bds::SListOrQueue ListType>
auto insert_front(ListType &L, std::initializer_list<typename ListType::pointer> i) noexcept {
  return L.insert_after(L.before_begin(), i);
}

template <bds::TailQ ListType, typename S>
auto insert_front(ListType &L, S *s) noexcept {
  return L.insert(L.begin(), s);
}

template <bds::TailQ ListType, typename InputIt>
auto insert_front(ListType &L, InputIt begin, InputIt end) noexcept {
  return L.insert(L.begin(), begin, end);
}

template <bds::TailQ ListType>
auto insert_front(ListType &L, std::initializer_list<typename ListType::pointer> i) noexcept {
  return L.insert(L.begin(), i);
}

template <bds::SListOrQueue ListType, typename S>
auto insert_after(ListType &L, typename ListType::const_iterator i, S *s) noexcept {
  return L.insert_after(i, s);
}

template <bds::TailQ ListType, typename S>
auto insert_after(ListType &L, typename ListType::const_iterator i, S *s) noexcept {
  return L.insert(std::next(i), s);
}

template <bds::SListOrQueue ListType>
auto erase_front(ListType &L) { return L.erase_after(L.before_begin()); }

template <bds::TailQ ListType>
auto erase_front(ListType &L) { return L.erase(L.begin()); }

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
void populateListFromSequence(ListType &list, ForwardIt begin,
                              ForwardIt end) noexcept {
  std::vector<typename ListType::pointer> items;

  while (begin != end) {
    typename ListType::pointer const p = new typename ListType::value_type{*begin++};
    items.push_back(p);
  }

  insert_front(list, std::begin(items), std::end(items));
}

template <typename ListType>
std::random_device::result_type
populateRandomList(ListType &list, std::size_t size, std::int64_t min,
                   std::int64_t max) noexcept {
  std::random_device trueRandom;
  const auto seed = trueRandom();
  std::default_random_engine engine{seed};
  std::uniform_int_distribution random_dist{min, max};

  for (std::size_t i = 0; i < size; ++i)
    list.push_front(new typename ListType::value_type{random_dist(engine)});

  return seed;
}

template <typename ListType>
std::random_device::result_type
populateSortedList(ListType &list, std::size_t size, std::int64_t min,
                   std::int64_t max) noexcept {
  std::unique_ptr<std::int64_t []> values{ new std::int64_t[size] };
  const auto seed = generateRandomInput(values.get(), size, min, max);
  std::sort(values.get(), values.get() + size);
  populateListFromSequence(list, values.get(), values.get() + size);

  return seed;
}

template <typename ListType>
void destroyList(ListType &list) noexcept {
  bds::for_each_safe(list, [](auto &v) { delete std::addressof(v); });
}

// <algorithm> offers `std::is_sorted` to check if elements are sorted, but
// this is not enough for correctness testing; mistakes in algorithms like
// in-place list merge sort often leave the resulting list sorted, but elements
// are missing (i.e., the links are not properly maintained). This counts the
// items in the list to verify no elements were "lost." It returns of a tuple
// of (in_order, correct_size).
template <typename ForwardIt, typename Compare>
std::pair<bool, bool>
is_sorted_check(ForwardIt begin, ForwardIt end, Compare comp,
                std::size_t size) noexcept {
  if (begin == end)
    return { true, size == 0 };

  ForwardIt prev = begin++;
  std::size_t count = 1;

  while (begin != end) {
    if (comp(*begin, *prev))
      return { false, false };

    ++count;
    prev = begin++;
  }

  return { true, count == size };
}

#endif
