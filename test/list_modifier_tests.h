#ifndef CSD_LIST_MODIFIER_TESTS_H
#define CSD_LIST_MODIFIER_TESTS_H

#include <cstring>
#include <iterator>
#include <memory>
#include <span>
#include <tuple>
#include <catch2/catch.hpp>
#include "list_test_util.h"

template <typename T>
constexpr bool is_stateful_extractor_list = false;

template <template <typename> class LinkType>
constexpr bool is_stateful_extractor_list<StatefulExtractorList<LinkType>> = true;

template <csg::linked_list ListType>
void basic_tests() {
  // Test the basic modifiers (single element insert and erase) and accessors
  // (size, empty, front, and back) of the list; also checks the functioning
  // of the iterators and begin(), end().
  using E = CSG_TYPENAME ListType::value_type;
  E e[] = { {0}, {1} };

  ListType head;

  REQUIRE( std::size(head) == 0 );
  REQUIRE( head.empty() );
  REQUIRE( head.begin() == head.end() );
  if constexpr (csg::stailq<ListType>)
    REQUIRE( head.before_begin() == head.before_end() );

  // Perform first insertion, and check its effects
  auto it1 = insert_front(head, &e[0]);

  REQUIRE( std::size(head) == 1 );
  REQUIRE( !head.empty() );
  REQUIRE( std::addressof(head.front()) == &e[0] );
  if constexpr (csg::tailq<ListType> || csg::stailq<ListType>)
    REQUIRE( std::addressof(head.back()) == &e[0] );

  // Check iterator comparison with begin(), end() and the iter() method
  REQUIRE( it1 == head.begin() );
  REQUIRE( it1 != head.end() );
  REQUIRE( it1 == head.iter(&e[0]) );
  if constexpr (csg::stailq<ListType>)
    REQUIRE( it1 == head.before_end() );

  // Check pre-increment iterator operators (and pre-decrement, for tailq's)
  REQUIRE( ++it1 == head.end() );
  if constexpr (csg::tailq<ListType>)
    REQUIRE( --it1 == head.begin() );

  // Check post-{inc,dec}rement iterator operators
  it1 = head.begin();
  REQUIRE( it1++ == head.begin() );
  REQUIRE( it1 == head.end() );
  if constexpr (csg::tailq<ListType>) {
    REQUIRE( it1-- == head.end() );
    REQUIRE( it1 == head.begin() );
  }

  // Check operator* and operator->
  it1 = head.begin();
  REQUIRE( std::addressof(*it1) == &e[0] );
  REQUIRE( it1.operator->() == &e[0] );
  if constexpr (csg::stailq<ListType>)
    REQUIRE( std::addressof(*head.before_end()) == &e[0] );

  // Perform second insertion after the first element, and check its effects.
  auto it2 = insert_after(head, it1, &e[1]);

  REQUIRE( std::size(head) == 2 );
  REQUIRE( !head.empty() );
  REQUIRE( std::addressof(head.front()) == &e[0] );
  REQUIRE( std::addressof(*++head.begin()) == &e[1] );
  REQUIRE( it1 != it2 );
  REQUIRE( it2 == head.iter(&e[1]) );

  if constexpr (csg::tailq<ListType> || csg::stailq<ListType>) {
    REQUIRE( std::addressof(head.back()) == &e[1] );

    if constexpr (csg::stailq<ListType>) {
      REQUIRE( it2 == head.before_end() );
      REQUIRE( std::addressof(*head.before_end()) == &e[1] );
    }
  }

  // Check iterator relationships
  REQUIRE( ++it1 == it2 );
  if constexpr (csg::tailq<ListType>) {
    REQUIRE( --it1 == head.begin() );
    REQUIRE( --it2 == it1 );
    REQUIRE( ++it2 == --head.end() );
  }
  else {
    REQUIRE( ++it2 == head.end() );
    it1 = head.begin();
    it2 = std::next(it1);
  }

  // Check constant versions of the iterators.
  REQUIRE( head.begin() == head.cbegin() );
  REQUIRE( head.end() == head.cend() );

  if constexpr (!csg::tailq<ListType>)
    REQUIRE( head.before_begin() == head.cbefore_begin() );

  if constexpr (csg::stailq<ListType>)
    REQUIRE( head.before_end() == head.cbefore_end() );

  // Remove first element, and check its effects
  auto after_it1 = erase_front(head);

  REQUIRE( std::size(head) == 1 );
  REQUIRE( !head.empty() );
  REQUIRE( std::addressof(head.front()) == &e[1] );
  REQUIRE( after_it1 == it2 );
  REQUIRE( it2 == head.begin() );
  REQUIRE( head.iter(&e[1]) == head.begin() );

  if constexpr (csg::tailq<ListType> || csg::stailq<ListType>) {
    REQUIRE( std::addressof(head.back()) == &e[1] );

    if constexpr (csg::stailq<ListType>)
      REQUIRE( it2 == head.before_end() );
    else
      REQUIRE( it2 == --head.end() );
  }

  if constexpr (!csg::tailq<ListType>) {
    // Erase the element after the last one; this should not damage the list
    // and should return an iterator to the end.
    auto after_it2 = head.erase_after(it2);
    REQUIRE( after_it2 == head.end() );
    REQUIRE( std::size(head) == 1 );
    REQUIRE( std::addressof(head.front()) == &e[1] );
    if constexpr (csg::stailq<ListType>) {
      REQUIRE( std::addressof(head.back()) == &e[1] );
    }
  }

  // Remove second element, and check its effects
  auto after_it2 = erase_front(head);

  REQUIRE( std::size(head) == 0 );
  REQUIRE( head.empty() );
  REQUIRE( head.begin() == head.end() );
  REQUIRE( after_it2 == head.end() );
  if constexpr (csg::stailq<ListType>)
    REQUIRE( head.before_begin() == head.before_end() );

  // Perform a final insertion to check that the list is still usable after
  // being returned to an empty state.
  insert_front(head, &e[0]);

  REQUIRE( std::size(head) == 1 );
  REQUIRE( !std::ranges::empty(head) );
  REQUIRE( std::addressof(*head.begin()) == &e[0] );
  REQUIRE( head.begin() != head.end() );
  if constexpr (csg::stailq<ListType>)
    REQUIRE( head.before_end() == head.begin() );
}

template <csg::linked_list ListType>
void clear_tests() {
  using E = CSG_TYPENAME ListType::value_type;
  E e{0};

  ListType head;

  // Verify that the list is empty, then insert the item and check the insert.
  REQUIRE( std::size(head) == 0 );
  insert_front(head, &e);
  REQUIRE( std::size(head) == 1 );

  // Check that clear properly resets the list and its size. It may be called
  // multiple times with no adverse effects.
  head.clear();
  head.clear();
  REQUIRE( head.empty() );
  REQUIRE( head.begin() == head.end() );
  REQUIRE( std::size(head) == 0 );
  if constexpr (csg::stailq<ListType>)
    REQUIRE( head.before_begin() == head.before_end() );

  // Ensure that the list is still in a working state (items can be
  // successfully added) even after being cleared.
  insert_front(head, &e);
  REQUIRE( std::size(head) == 1 );
  REQUIRE( !head.empty() );
  REQUIRE( std::addressof(head.front()) == &e );
  if constexpr (csg::tailq<ListType> || csg::stailq<ListType>)
    REQUIRE( std::addressof(head.back()) == &e );
}

template <csg::linked_list ListType1, csg::linked_list ListType2>
void move_ctor_tests() {
  using E = CSG_TYPENAME ListType1::value_type;
  static_assert(std::same_as<E, typename ListType2::value_type>);

  ListType1 head;
  E e{0};

  insert_front(head, &e);
  REQUIRE( std::size(head) == 1 );

  ListType2 movedHead{std::move(head)};

  REQUIRE( std::size(movedHead) == 1 );
  REQUIRE( std::addressof(movedHead.front()) == &e );
  REQUIRE( !movedHead.empty() );
  if constexpr (csg::tailq<ListType2> || csg::stailq<ListType2>)
    REQUIRE( std::addressof(movedHead.back()) == &e );

  REQUIRE( head.empty() );
  if constexpr (is_stateful_extractor_list<E>) {
    REQUIRE( head.get_entry_extractor().movedFrom == true );
    REQUIRE( movedHead.get_entry_extractor().movedFrom == false );
  }

  movedHead.clear();

  if constexpr (csg::singly_linked_list<ListType1> &&
                csg::singly_linked_list<ListType2> &&
                test_proxy<ListType1> && test_proxy<ListType2>) {
    // Test that we can move-construct the fwd_head types; this is only allowed
    // for slist and stailq. tailq_fwd_head has a deleted move constructor
    // as explained in the documentation.
    typename ListType1::fwd_head_type oldFwdHead;
    typename ListType1::list_proxy_type oldHead{oldFwdHead};

    insert_front(oldHead, &e);
    REQUIRE( std::size(oldHead) == 1 );

    typename ListType2::fwd_head_type newFwdHead{std::move(oldFwdHead)};
    typename ListType2::list_proxy_type newHead{newFwdHead};

    REQUIRE( std::addressof(newHead.front()) == &e );
    REQUIRE( std::size(newHead) == 1 );
    REQUIRE( !std::ranges::empty(newHead) );

    REQUIRE( std::size(oldHead) == 0 );
    REQUIRE( std::ranges::empty(oldHead) );
    REQUIRE( oldHead.begin() == oldHead.end() );
  }
}

template <typename ListType1, typename ListType2>
void move_assign_tests() {
  using E = CSG_TYPENAME ListType1::value_type;
  static_assert(std::same_as<E, typename ListType2::value_type>);

  ListType1 oldHead;
  ListType2 newHead;
  E e{0};

  insert_front(oldHead, &e);
  REQUIRE( std::size(oldHead) == 1 );

  newHead = std::move(oldHead);

  REQUIRE( std::size(newHead) == 1 );
  REQUIRE( std::addressof(newHead.front()) == &e );
  REQUIRE( !newHead.empty() );
  if constexpr (csg::tailq<ListType2> || csg::stailq<ListType2>)
    REQUIRE( std::addressof(newHead.back()) == &e );

  REQUIRE( std::size(oldHead) == 0 );
  REQUIRE( oldHead.empty() );
  REQUIRE( oldHead.begin() == oldHead.end() );
  if constexpr (csg::stailq<ListType1>)
    REQUIRE( oldHead.before_begin() == oldHead.before_end() );
  if constexpr (is_stateful_extractor_list<E>) {
    REQUIRE( oldHead.get_entry_extractor().movedFrom == true );
    REQUIRE( newHead.get_entry_extractor().movedFrom == false );
  }

  if constexpr (csg::singly_linked_list<ListType1> &&
                csg::singly_linked_list<ListType2> &&
                test_proxy<ListType1> && test_proxy<ListType2>) {
    // Test that we can move-assign the fwd_head types; the same comments apply
    // as in the move-construction tests.
    typename ListType1::fwd_head_type oldFwdHead;
    typename ListType2::fwd_head_type newFwdHead;

    typename ListType1::list_proxy_type oldHead{oldFwdHead};
    typename ListType2::list_proxy_type newHead{newFwdHead};

    insert_front(oldHead, &e);
    REQUIRE( std::size(oldHead) == 1 );
    newFwdHead = std::move(oldFwdHead);

    REQUIRE( std::addressof(newHead.front()) == &e );
    REQUIRE( std::size(newHead) == 1 );
    REQUIRE( !std::ranges::empty(newHead) );

    REQUIRE( std::size(oldHead) == 0 );
    REQUIRE( std::ranges::empty(oldHead) );
    REQUIRE( oldHead.begin() == oldHead.end() );
  }
}

template <csg::linked_list ListHeadType, csg::linked_list ListProxyType>
void move_tests() {
  SECTION("move_ctor.head_head") { move_ctor_tests<ListHeadType, ListHeadType>(); }
  SECTION("move_ctor.proxy_proxy") { move_ctor_tests<ListProxyType, ListProxyType>(); }
  SECTION("move_ctor.head_proxy") { move_ctor_tests<ListHeadType, ListProxyType>(); }
  SECTION("move_ctor.proxy_head") { move_ctor_tests<ListProxyType, ListHeadType>(); }

  SECTION("move_assign.head_head") { move_assign_tests<ListHeadType, ListHeadType>(); }
  SECTION("move_assign.proxy_proxy") { move_assign_tests<ListProxyType, ListProxyType>(); }
  SECTION("move_assign.head_proxy") { move_assign_tests<ListHeadType, ListProxyType>(); }
  SECTION("move_assign.proxy_head") { move_assign_tests<ListProxyType, ListHeadType>(); }
}

template <csg::linked_list ListType>
void extra_ctor_tests() {
  using E = CSG_TYPENAME ListType::value_type;

  E e[] = { {0}, {1}, {2} };

  SECTION("ctor.iterator") {
    E *insert[] = { &e[0], &e[1], &e[2] };

    ListType head{ std::begin(insert), std::end(insert) };

    REQUIRE( std::size(head) == 3 );

    auto i = head.begin();
    REQUIRE( std::addressof(*i++) == &e[0] );
    REQUIRE( std::addressof(*i++) == &e[1] );
    REQUIRE( std::addressof(*i++) == &e[2] );
    REQUIRE( i == head.end() );

    if constexpr (csg::stailq<ListType>)
      REQUIRE( std::addressof(*head.before_end()) == &e[2] );
  }

  SECTION("ctor.range") {
    E *insert[] = { &e[0], &e[1], &e[2] };

    ListType head{std::span{insert}};

    REQUIRE( std::size(head) == 3 );

    auto i = head.begin();
    REQUIRE( std::addressof(*i++) == &e[0] );
    REQUIRE( std::addressof(*i++) == &e[1] );
    REQUIRE( std::addressof(*i++) == &e[2] );
    REQUIRE( i == head.end() );

    if constexpr (csg::stailq<ListType>)
      REQUIRE( std::addressof(*head.before_end()) == &e[2] );
  }

  SECTION("ctor.initializer_list") {
    ListType head{ { &e[0], &e[1], &e[2] } };

    REQUIRE( std::size(head) == 3 );

    auto i = head.begin();
    REQUIRE( std::addressof(*i++) == &e[0] );
    REQUIRE( std::addressof(*i++) == &e[1] );
    REQUIRE( std::addressof(*i++) == &e[2] );
    REQUIRE( i == head.end() );

    if constexpr (csg::stailq<ListType>)
      REQUIRE( std::addressof(*head.before_end()) == &e[2] );
  }
}

template <csg::linked_list ListType>
void bulk_insert_tests() {
  using E = CSG_TYPENAME ListType::value_type;

  ListType head;
  E e[] = { {0}, {1}, {2} };

  SECTION("bulk_insert.iterator_pair") {
    // Insert a single element, then use bulk insert to prepend elements in
    // front of it; this is slightly less trivial than bulk insertion into
    // an empty list.
    insert_front(head, &e[2]);
    E *insert[] = { &e[0], &e[1] };

    // Tail queues return the first element of the insertion, whereas the
    // singly-linked lists return the last element inserted. This behavior
    // matches std::list and std::forward_list.
    auto i = insert_front(head, std::begin(insert), std::end(insert));
    REQUIRE( std::size(head) == 3 );

    if constexpr (csg::tailq<ListType>)
      REQUIRE( i == head.begin() );
    else {
      REQUIRE( std::addressof(*i) == &e[1] );
      if constexpr (csg::stailq<ListType>)
        REQUIRE( std::addressof(*head.before_end()) == &e[2] );
      i = head.begin();
    }

    REQUIRE( std::addressof(*i++) == &e[0] );
    REQUIRE( std::addressof(*i++) == &e[1] );
    REQUIRE( std::addressof(*i++) == &e[2] );
    REQUIRE( i == head.end() );
  }

  SECTION("bulk_insert.range") {
    // Same test as above, but using bulk insertion of a range.
    insert_front(head, &e[2]);
    std::array<E *, 2> insert = { &e[0], &e[1] };

    auto i = insert_front(head, insert);
    REQUIRE( std::size(head) == 3 );

    if constexpr (csg::tailq<ListType>)
      REQUIRE( i == head.begin() );
    else {
      REQUIRE( std::addressof(*i) == &e[1] );
      if constexpr (csg::stailq<ListType>)
        REQUIRE( std::addressof(*head.before_end()) == &e[2] );
      i = head.begin();
    }

    REQUIRE( std::addressof(*i++) == &e[0] );
    REQUIRE( std::addressof(*i++) == &e[1] );
    REQUIRE( std::addressof(*i++) == &e[2] );
    REQUIRE( i == head.end() );
  }

  SECTION("bulk_insert.initializer_list") {
    // Same as above, but using an initializer_list instead of iterators.
    insert_front(head, &e[2]);
    auto i = insert_front(head, { &e[0], &e[1] });
    REQUIRE( std::size(head) == 3 );

    if constexpr (csg::tailq<ListType>)
      REQUIRE( i == head.begin() );
    else {
      REQUIRE( std::addressof(*i) == &e[1] );
      if constexpr (csg::stailq<ListType>)
        REQUIRE( std::addressof(*head.before_end()) == &e[2] );
      i = head.begin();
    }

    REQUIRE( std::addressof(*i++) == &e[0] );
    REQUIRE( std::addressof(*i++) == &e[1] );
    REQUIRE( std::addressof(*i++) == &e[2] );
    REQUIRE( i == head.end() );
  }

  SECTION("bulk_insert.empty_range") {
    insert_front(head, &e[2]);
    E *insert[] = { &e[0] };
    insert_front(head, std::begin(insert), std::begin(insert));
    REQUIRE( std::size(head) == 1 );
  }

  // The equivalent assign tests are also performed here; assign is much the
  // same as bulk insertion, but clears the list first.

  SECTION("assign.iterator_range") {
    insert_front(head, &e[2]);
    E *insert[] = { &e[0], &e[1] };

    head.assign(std::begin(insert), std::end(insert));
    REQUIRE( std::size(head) == 2 );

    auto i = head.begin();

    REQUIRE( std::addressof(*i++) == &e[0] );
    REQUIRE( std::addressof(*i++) == &e[1] );
    REQUIRE( i == head.end() );

    if constexpr (csg::stailq<ListType>)
      REQUIRE( std::addressof(*head.before_end()) == &e[1] );
  }

  SECTION("assign.range") {
    insert_front(head, &e[2]);
    std::array<E *, 2> insert{ &e[0], &e[1] };

    head.assign(insert);
    REQUIRE( std::size(head) == 2 );

    auto i = head.begin();

    REQUIRE( std::addressof(*i++) == &e[0] );
    REQUIRE( std::addressof(*i++) == &e[1] );
    REQUIRE( i == head.end() );

    if constexpr (csg::stailq<ListType>)
      REQUIRE( std::addressof(*head.before_end()) == &e[1] );
  }

  SECTION("assign.initializer_list") {
    insert_front(head, &e[2]);

    head.assign({ &e[0], &e[1] });
    REQUIRE( std::size(head) == 2 );

    auto i = head.begin();

    REQUIRE( std::addressof(*i++) == &e[0] );
    REQUIRE( std::addressof(*i++) == &e[1] );
    REQUIRE( i == head.end() );

    if constexpr (csg::stailq<ListType>)
      REQUIRE( std::addressof(*head.before_end()) == &e[1] );
  }

  SECTION("assign.assign_operator_with_range") {
    // operator=(range) is syntactic sugar for assign, so we also test that.
    insert_front(head, &e[2]);
    std::array<E *, 2> insert = { &e[0], &e[1] };

    head = insert;
    REQUIRE( std::size(head) == 2 );

    auto i = head.begin();

    REQUIRE( std::addressof(*i++) == &e[0] );
    REQUIRE( std::addressof(*i++) == &e[1] );
    REQUIRE( i == head.end() );

    if constexpr (csg::stailq<ListType>)
      REQUIRE( std::addressof(*head.before_end()) == &e[1] );
  }

  SECTION("assign.assign_operator_with_ilist") {
    // As above, but for operator=(initializer_list<T *>).
    insert_front(head, &e[2]);

    head = { &e[0], &e[1] };
    REQUIRE( std::size(head) == 2 );

    auto i = head.begin();

    REQUIRE( std::addressof(*i++) == &e[0] );
    REQUIRE( std::addressof(*i++) == &e[1] );
    REQUIRE( i == head.end() );

    if constexpr (csg::stailq<ListType>)
      REQUIRE( std::addressof(*head.before_end()) == &e[1] );
  }
}

template <csg::linked_list ListType>
void bulk_erase_tests() {
  using E = CSG_TYPENAME ListType::value_type;

  ListType head;
  E e[] = { {0}, {1}, {2} };
  insert_front(head, { &e[0], &e[1], &e[2] });

  // Remove [e1, end), then check that the list is still valid and == [e0].
  auto i = erase_after(head, head.begin(), head.end());
  REQUIRE( std::size(head) == 1 );
  REQUIRE( i == head.end() );
  REQUIRE( std::addressof(*head.begin()) == &e[0] );
  if constexpr (csg::stailq<ListType>)
    REQUIRE( std::addressof(*head.before_end()) == &e[0] );

  // Remove [e0] and check that the list is empty.
  i = erase_front(head);
  REQUIRE( i == head.end() );
  REQUIRE( head.empty() );

  // Remove the empty range (begin, end) -- this is a no-op, so it must leave
  // the list in a valid state.
  i = erase_after(head, head.begin(), head.end());
  REQUIRE( i == head.end() );
  REQUIRE( head.empty() );
  if constexpr (csg::stailq<ListType>)
    REQUIRE( head.before_begin() == head.before_end() );

  if constexpr (csg::singly_linked_list<ListType>) {
    i = head.erase_after(head.before_begin(), head.end());
    REQUIRE( i == head.end() );
    REQUIRE( head.empty() );
    if constexpr (csg::stailq<ListType>)
      REQUIRE( head.before_begin() == head.before_end() );
  }

  // Check that list is still usable after empty range erase.
  insert_front(head, &e[0]);
  REQUIRE( std::size(head) == 1 );
  REQUIRE( std::addressof(head.front()) == &e[0] );
  if constexpr (requires(ListType L) { L.back(); })
    REQUIRE( std::addressof(head.back()) == &e[0] );
}

template <csg::linked_list ListType>
void for_each_safe_tests() {
  using E = CSG_TYPENAME ListType::value_type;
  ListType head;

  SECTION("for_each_safe.deletion_safe") {
    // Checks that the traversal is actually safe when destroying the current
    // item in the iteration.
    E *e[] = { new E{0}, new E{1}, new E{2} };

    insert_front(head, std::begin(e), std::end(e));
    head.for_each_safe([&head] (E &item) {
      erase_front(head);
      E *const itemAddr = std::addressof(item);

      // Destroy the item, then placement new a dummy value over the top of the
      // item to simulate it being deleted and its space reused. We'll check for
      // this special value to verify that it was actually deleted. Also ensure
      // the entry is reset to verify that it doesn't break list traversal.
      item.~E();
      auto &entry = head.get_entry_extractor()(*new(itemAddr) E{123});
      entry = {};
    });

    REQUIRE( head.empty() );

    for (E *item : e) {
      REQUIRE( get_value(*item) == 123 );
      delete item;
    }
  }

  SECTION("for_each_safe.projection") {
    // Test the range projection feature (this doesn't really test the "safe"
    // aspect of this member like the above).
    E e[] = { {0}, {1}, {2} };
    insert_front(head, { &e[0], &e[1], &e[2] });

    std::int64_t sum = 0;
    head.for_each_safe([&sum] (const auto i) { sum += i; }, get_value<E>);
    REQUIRE( sum == 3 );
  }
}

template <csg::linked_list ListType>
void push_pop_tests() {
  using E = CSG_TYPENAME ListType::value_type;
  ListType head;

  E e[] = { {0}, {1} };

  insert_front(head, &e[1]);
  REQUIRE( std::addressof(*head.begin()) == &e[1] );
  REQUIRE( std::size(head) == 1 );
  REQUIRE( !head.empty() );
  REQUIRE( ++head.begin() == head.end() );
  if constexpr (csg::stailq<ListType>)
    REQUIRE( std::addressof(*head.before_end()) == &e[1] );

  insert_front(head, &e[0]);
  REQUIRE( std::size(head) == 2 );
  REQUIRE( std::addressof(*head.begin()) == &e[0] );
  if constexpr (csg::stailq<ListType>)
    REQUIRE( std::addressof(*head.before_end()) == &e[1] );

  head.pop_front();
  REQUIRE( std::size(head) == 1 );
  REQUIRE( std::addressof(*head.begin()) == &e[1] );
  if constexpr (csg::stailq<ListType>)
    REQUIRE( std::addressof(*head.before_end()) == &e[1] );

  head.pop_front();
  REQUIRE( head.empty() );
  if constexpr (csg::stailq<ListType>)
    REQUIRE( head.before_begin() == head.before_end() );

  if constexpr (csg::stailq<ListType> || csg::tailq<ListType>) {
    head.push_back(&e[0]);
    REQUIRE( std::addressof(*head.begin()) == &e[0] );
    REQUIRE( std::size(head) == 1 );
    REQUIRE( !head.empty() );
    REQUIRE( ++head.begin() == head.end() );
    if constexpr (csg::stailq<ListType>)
      REQUIRE( std::addressof(*head.before_end()) == &e[0] );

    head.push_back(&e[1]);
    REQUIRE( std::addressof(*++head.begin()) == &e[1] );
    REQUIRE( std::size(head) == 2 );
    if constexpr (csg::stailq<ListType>)
      REQUIRE( std::addressof(*head.before_end()) == &e[1] );

    if constexpr (csg::tailq<ListType>) {
      head.pop_back();
      REQUIRE( std::addressof(*head.begin()) == &e[0] );
      REQUIRE( std::size(head) == 1 );
      REQUIRE( !head.empty() );
      REQUIRE( ++head.begin() == head.end() );

      head.pop_back();
      REQUIRE( head.empty() );
      REQUIRE( head.begin() == head.end() );
    }
  }
}

template <csg::linked_list ListType1, csg::linked_list ListType2>
void swap_lists() {
  using E = CSG_TYPENAME ListType1::value_type;
  static_assert(std::is_same_v<E, typename ListType2::value_type>);

  ListType1 head1;
  ListType2 head2;
  E e[] = { {0}, {1}, {2} };

  SECTION("swap_neither_empty") {
    insert_front(head1, &e[0]);
    insert_front(head2, { &e[1], &e[2] });

    head1.swap(head2);

    REQUIRE( std::size(head1) == 2 );
    REQUIRE( std::addressof(*head1.begin()) == &e[1] );
    REQUIRE( std::addressof(*++head1.begin()) == &e[2] );
    if constexpr (csg::tailq<ListType1>)
      REQUIRE( std::addressof(*--head1.end()) == &e[2] );
    if constexpr (csg::stailq<ListType1>)
      REQUIRE( std::addressof(*head1.before_end()) == &e[2] );

    REQUIRE( std::size(head2) == 1 );
    REQUIRE( std::addressof(*head2.begin()) == &e[0] );
    if constexpr (csg::stailq<ListType2>)
      REQUIRE( std::addressof(*head2.before_end()) == &e[0] );
  }

  SECTION("swap_one_empty") {
    insert_front(head1, { &e[0], &e[1], &e[2] });
    head1.swap(head2);

    REQUIRE( head1.empty() );

    REQUIRE( std::size(head2) == 3 );
    auto i = head2.begin();
    REQUIRE( std::addressof(*i++) == &e[0] );
    REQUIRE( std::addressof(*i++) == &e[1] );
    REQUIRE( std::addressof(*i++) == &e[2] );
    REQUIRE( i == head2.end() );
    if constexpr (csg::stailq<ListType1>)
      REQUIRE( std::addressof(*head2.before_end()) == &e[2] );
  }
}

template <csg::linked_list ListHeadType, csg::linked_list ListProxyType>
void swap_tests() {
  SECTION("same_type_head") { swap_lists<ListHeadType, ListHeadType>(); }
  SECTION("same_type_fwd") { swap_lists<ListProxyType, ListProxyType>(); }
  SECTION("swap_to_fwd_head") { swap_lists<ListHeadType, ListProxyType>(); }
  SECTION("swap_from_fwd_head") { swap_lists<ListProxyType, ListHeadType>(); }
}

template <csg::singly_linked_list ListType>
void find_predecessor_tests() {
  using E = CSG_TYPENAME ListType::value_type;

  ListType head;
  E e[] = { {0}, {1} };

  head.insert_after(head.before_begin(), { &e[0], &e[1] });

  SECTION("find_predecessor") {
    REQUIRE( head.find_predecessor(head.before_begin()) == head.end() );
    REQUIRE( head.find_predecessor(head.iter(&e[0])) == head.before_begin() );
    REQUIRE( head.find_predecessor(head.iter(&e[1])) == head.iter(&e[0]) );
    REQUIRE( head.find_predecessor(head.end()) == head.iter(&e[1]) );
  }

  SECTION("find_predecessor_if") {
    typename ListType::iterator i;
    bool found;

    std::tie(i, found) = head.find_predecessor_if(
      [&e] (const E &item) { return &item == &e[0]; });
    REQUIRE( i == head.before_begin() );
    REQUIRE( found == true );

    std::tie(i, found) = head.find_predecessor_if(
      [&e] (const E &item) { return &item == &e[1]; });
    REQUIRE( i == head.begin() );
    REQUIRE( found == true );

    std::tie(i, found) = head.find_predecessor_if(
      [] (const E &item) { return std::addressof(item) == nullptr; });
    REQUIRE( i == ++head.begin() );
    REQUIRE( found == false );
    if constexpr (csg::stailq<ListType>)
      REQUIRE( i == head.before_end() );

    head.clear();

    std::tie(i, found) = head.find_predecessor_if(
      [] (const E &item) { return std::addressof(item) == nullptr; });

    REQUIRE( i == head.before_begin() );
    REQUIRE( found == false );
    if constexpr (csg::stailq<ListType>)
      REQUIRE( i == head.before_end() );
  }

  SECTION("find_erase") {
    E *erased;
    typename ListType::iterator next;

    std::tie(erased, next) = head.find_erase(head.iter(&e[0]));
    REQUIRE( std::size(head) == 1 );
    REQUIRE( erased == &e[0] );
    REQUIRE( std::addressof(head.front()) == &e[1] );
    REQUIRE( next == head.begin() );
    if constexpr (csg::stailq<ListType>)
      REQUIRE( std::addressof(head.back()) == &e[1] );
  }
}

template <typename ProxyType>
void proxy_test_helper(ProxyType head, CSG_TYPENAME ProxyType::pointer e) {
  insert_front(head, e);
  REQUIRE( std::size(head) == 2 );
}

template <typename FwdHeadType, typename ProxyType>
void proxy_tests() {
  // Test that a fwd_head can be bound to another proxy wrapper and it will
  // affect the state of the fwd_head object when accessed through the original
  // wrapper. This also shows the implicit construction of the proxy wrapper
  // from the fwd_head (in the function call), and tests that the destructor of
  // the proxy wrapper (at the end of the called function) does not
  // affect/damage the list when accessed through the original proxy.
  FwdHeadType fwdHead;
  ProxyType head{fwdHead};

  using E = CSG_TYPENAME ProxyType::value_type;
  E e[] = { {0}, {1} };

  insert_front(head, &e[0]);
  REQUIRE( std::size(head) == 1 );
  proxy_test_helper<ProxyType>(fwdHead, &e[1]);
  REQUIRE( std::size(head) == 2 );
}

#endif
