#ifndef BDS_LIST_MODIFIER_TESTS_H
#define BDS_LIST_MODIFIER_TESTS_H

#include <catch2/catch.hpp>
#include "list_test_util.h"

template <typename T>
constexpr bool is_base_u = false;

template <template <typename> class LinkType>
constexpr bool is_base_u<BaseU<LinkType>> = true;

template <bds::LinkedList ListType>
void basic_tests() {
  // Test the basic modifiers (single element insert and erase) and accessors
  // (size, empty, front, and back) of the list; also checks the functioning
  // of the iterators and begin(), end().
  using E = ListType::value_type;
  E e[] = { {0}, {1} };

  ListType head;

  REQUIRE( std::size(head) == 0 );
  REQUIRE( head.empty() );
  REQUIRE( head.begin() == head.end() );

  // Perform first insertion, and check its effects
  auto it1 = insert_front(head, &e[0]);

  REQUIRE( std::size(head) == 1 );
  REQUIRE( !head.empty() );
  REQUIRE( std::addressof(head.front()) == &e[0] );
  if constexpr (bds::TailQ<ListType> || bds::STailQ<ListType>)
    REQUIRE( std::addressof(head.back()) == &e[0] );

  // Check iterator comparison with begin(), end() and the iter() method
  REQUIRE( it1 == head.begin() );
  REQUIRE( it1 != head.end() );
  REQUIRE( it1 == head.iter(&e[0]) );
  if constexpr (bds::STailQ<ListType>)
    REQUIRE( it1 == head.before_end() );

  // Check pre-increment iterator operators (and pre-decrement, for tailq's)
  REQUIRE( ++it1 == head.end() );
  if constexpr (bds::TailQ<ListType>)
    REQUIRE( --it1 == head.begin() );

  // Check post-{inc,dec}rement iterator operators
  it1 = head.begin();
  REQUIRE( it1++ == head.begin() );
  REQUIRE( it1 == head.end() );
  if constexpr (bds::TailQ<ListType>) {
    REQUIRE( it1-- == head.end() );
    REQUIRE( it1 == head.begin() );
  }

  // Check operator* and operator->
  it1 = head.begin();
  REQUIRE( std::addressof(*it1) == &e[0] );
  REQUIRE( it1.operator->() == &e[0] );
  if constexpr (bds::STailQ<ListType>)
    REQUIRE( std::addressof(*head.before_end()) == &e[0] );

  // Perform second insertion after the first element, and check its effects.
  auto it2 = insert_after(head, it1, &e[1]);

  REQUIRE( std::size(head) == 2 );
  REQUIRE( !head.empty() );
  REQUIRE( std::addressof(head.front()) == &e[0] );
  REQUIRE( std::addressof(*++head.begin()) == &e[1] );
  REQUIRE( it1 != it2 );
  REQUIRE( it2 == head.iter(&e[1]) );

  if constexpr (bds::TailQ<ListType> || bds::STailQ<ListType>) {
    REQUIRE( std::addressof(head.back()) == &e[1] );

    if constexpr (bds::STailQ<ListType>) {
      REQUIRE( it2 == head.before_end() );
      REQUIRE( std::addressof(*head.before_end()) == &e[1] );
    }
  }

  // Check iterator relationships
  REQUIRE( ++it1 == it2 );
  if constexpr (bds::TailQ<ListType>) {
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

  if constexpr (!bds::TailQ<ListType>)
    REQUIRE( head.before_begin() == head.cbefore_begin() );

  if constexpr (bds::STailQ<ListType>)
    REQUIRE( head.before_end() == head.cbefore_end() );

  // Remove first element, and check its effects
  auto after_it1 = erase_front(head);

  REQUIRE( std::size(head) == 1 );
  REQUIRE( !head.empty() );
  REQUIRE( std::addressof(head.front()) == &e[1] );
  REQUIRE( after_it1 == it2 );
  REQUIRE( it2 == head.begin() );
  REQUIRE( head.iter(&e[1]) == head.begin() );

  if constexpr (bds::TailQ<ListType> || bds::STailQ<ListType>) {
    REQUIRE( std::addressof(head.back()) == &e[1] );

    if constexpr (bds::STailQ<ListType>)
      REQUIRE( it2 == head.before_end() );
    else
      REQUIRE( it2 == --head.end() );
  }

  if constexpr (!bds::TailQ<ListType>) {
    // Erase the element after the last one; this should not damage the list
    // and should return an iterator to the end.
    auto after_it2 = head.erase_after(it2);
    REQUIRE( after_it2 == head.end() );
    REQUIRE( std::size(head) == 1 );
    REQUIRE( std::addressof(head.front()) == &e[1] );
    if constexpr (bds::STailQ<ListType>) {
      REQUIRE( std::addressof(head.back()) == &e[1] );
    }
  }

  // Remove second element, and check its effects
  auto after_it2 = erase_front(head);

  REQUIRE( std::size(head) == 0 );
  REQUIRE( head.empty() );
  REQUIRE( head.begin() == head.end() );
  REQUIRE( after_it2 == head.end() );
  if constexpr (bds::STailQ<ListType>)
    REQUIRE( head.before_end() == head.end() );

  // Perform a final insertion to check that the list is still usable after
  // being returned to an empty state.
  insert_front(head, &e[0]);

  REQUIRE( std::size(head) == 1 );
  REQUIRE( !std::empty(head) );
  REQUIRE( std::addressof(*head.begin()) == &e[0] );
  REQUIRE( head.begin() != head.end() );
  if constexpr (bds::STailQ<ListType>)
    REQUIRE( head.before_end() == head.begin() );
}

template <bds::LinkedList ListType>
void clear_tests() {
  using E = ListType::value_type;
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
  if constexpr (bds::STailQ<ListType>)
    REQUIRE( head.before_end() == head.end() );

  // Ensure that the list is still in a working state (items can be
  // successfully added) even after being cleared.
  insert_front(head, &e);
  REQUIRE( std::size(head) == 1 );
  REQUIRE( !head.empty() );
  REQUIRE( std::addressof(head.front()) == &e );
  if constexpr (bds::TailQ<ListType> || bds::STailQ<ListType>)
    REQUIRE( std::addressof(head.back()) == &e );
}

template <bds::LinkedList ListType1, bds::LinkedList ListType2>
void move_ctor_tests() {
  using E = ListType1::value_type;
  static_assert(std::is_same_v<E, typename ListType2::value_type>);

  ListType1 head;
  E e{0};

  insert_front(head, &e);
  REQUIRE( std::size(head) == 1 );

  ListType2 movedHead{std::move(head)};

  REQUIRE( std::size(movedHead) == 1 );
  REQUIRE( std::addressof(movedHead.front()) == &e );
  REQUIRE( !movedHead.empty() );
  if constexpr (bds::TailQ<ListType2> || bds::STailQ<ListType2>)
    REQUIRE( std::addressof(movedHead.back()) == &e );

  REQUIRE( head.empty() );
  if constexpr (is_base_u<E>) {
    REQUIRE( head.get_entry_accessor().movedFrom == true );
    REQUIRE( movedHead.get_entry_accessor().movedFrom == false );
  }

  movedHead.clear();

  if constexpr (bds::SListOrQueue<ListType1> && bds::SListOrQueue<ListType2> &&
                TestProxy<ListType1> && TestProxy<ListType2>) {
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
    REQUIRE( !std::empty(newHead) );

    REQUIRE( std::size(oldHead) == 0 );
    REQUIRE( std::empty(oldHead) );
    REQUIRE( oldHead.begin() == oldHead.end() );
  }
}

template <typename ListType1, typename ListType2>
void move_assign_tests() {
  using E = ListType1::value_type;
  static_assert(std::is_same_v<E, typename ListType2::value_type>);

  ListType1 oldHead;
  ListType2 newHead;
  E e{0};

  insert_front(oldHead, &e);
  REQUIRE( std::size(oldHead) == 1 );

  newHead = std::move(oldHead);

  REQUIRE( std::size(newHead) == 1 );
  REQUIRE( std::addressof(newHead.front()) == &e );
  REQUIRE( !newHead.empty() );
  if constexpr (bds::TailQ<ListType2> || bds::STailQ<ListType2>)
    REQUIRE( std::addressof(newHead.back()) == &e );

  REQUIRE( std::size(oldHead) == 0 );
  REQUIRE( oldHead.empty() );
  REQUIRE( oldHead.begin() == oldHead.end() );
  if constexpr (bds::STailQ<ListType1>)
    REQUIRE( oldHead.before_end() == oldHead.end() );
  if constexpr (is_base_u<E>) {
    REQUIRE( oldHead.get_entry_accessor().movedFrom == true );
    REQUIRE( newHead.get_entry_accessor().movedFrom == false );
  }

  if constexpr (bds::SListOrQueue<ListType1> && bds::SListOrQueue<ListType2> &&
                TestProxy<ListType1> && TestProxy<ListType2>) {
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
    REQUIRE( !std::empty(newHead) );

    REQUIRE( std::size(oldHead) == 0 );
    REQUIRE( std::empty(oldHead) );
    REQUIRE( oldHead.begin() == oldHead.end() );
  }
}

template <bds::LinkedList ListHeadType, bds::LinkedList ListProxyType>
void move_tests() {
  SECTION("move_ctor.head_head") { move_ctor_tests<ListHeadType, ListHeadType>(); }
  SECTION("move_ctor.cont_cont") { move_ctor_tests<ListProxyType, ListProxyType>(); }
  SECTION("move_ctor.head_cont") { move_ctor_tests<ListHeadType, ListProxyType>(); }
  SECTION("move_ctor.cont_head") { move_ctor_tests<ListProxyType, ListHeadType>(); }

  SECTION("move_assign.head_head") { move_assign_tests<ListHeadType, ListHeadType>(); }
  SECTION("move_assign.cont_cont") { move_assign_tests<ListProxyType, ListProxyType>(); }
  SECTION("move_assign.head_cont") { move_assign_tests<ListHeadType, ListProxyType>(); }
  SECTION("move_assign.cont_head") { move_assign_tests<ListProxyType, ListHeadType>(); }
}

template <bds::LinkedList ListType>
void extra_ctor_tests() {
  using E = ListType::value_type;

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

    if constexpr (bds::STailQ<ListType>)
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

    if constexpr (bds::STailQ<ListType>)
      REQUIRE( std::addressof(*head.before_end()) == &e[2] );
  }
}

template <bds::LinkedList ListType>
void bulk_insert_tests() {
  using E = ListType::value_type;

  ListType head;
  E e[] = { {0}, {1}, {2} };

  SECTION("bulk_insert.iterator_range") {
    // Insert a single element, then use bulk insert to prepend elements in
    // front of it; this is slightly less trivial than bulk insertion into
    // an empty list.
    insert_front(head, &e[2] );
    E *insert[] = { &e[0], &e[1] };

    // Tail queues return the first element of the insertion, whereas the
    // singly-linked lists return the last element inserted. This behavior
    // matches std::list and std::forward_list.
    auto i = insert_front(head, std::begin(insert), std::end(insert));
    REQUIRE( std::size(head) == 3 );

    if constexpr (bds::TailQ<ListType>)
      REQUIRE( i == head.begin() );
    else {
      REQUIRE( std::addressof(*i) == &e[1] );
      if constexpr (bds::STailQ<ListType>)
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

    if constexpr (bds::TailQ<ListType>)
      REQUIRE( i == head.begin() );
    else {
      REQUIRE( std::addressof(*i) == &e[1] );
      if constexpr (bds::STailQ<ListType>)
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
    insert_front(head, &e[2] );
    E *insert[] = { &e[0], &e[1] };

    head.assign(std::begin(insert), std::end(insert));
    REQUIRE( std::size(head) == 2 );

    auto i = head.begin();

    REQUIRE( std::addressof(*i++) == &e[0] );
    REQUIRE( std::addressof(*i++) == &e[1] );
    REQUIRE( i == head.end() );

    if constexpr (bds::STailQ<ListType>)
      REQUIRE( std::addressof(*head.before_end()) == &e[1] );
  }

  SECTION("assign.initializer_list") {
    insert_front(head, &e[2] );

    head.assign({ &e[0], &e[1] });
    REQUIRE( std::size(head) == 2 );

    auto i = head.begin();

    REQUIRE( std::addressof(*i++) == &e[0] );
    REQUIRE( std::addressof(*i++) == &e[1] );
    REQUIRE( i == head.end() );

    if constexpr (bds::STailQ<ListType>)
      REQUIRE( std::addressof(*head.before_end()) == &e[1] );
  }

  SECTION("assign.operator_form") {
    // operator=(initializer_list<T *>) is syntactic sugar for assign, so we
    // also test that here.
    insert_front(head, &e[2] );

    head = { &e[0], &e[1] };
    REQUIRE( std::size(head) == 2 );

    auto i = head.begin();

    REQUIRE( std::addressof(*i++) == &e[0] );
    REQUIRE( std::addressof(*i++) == &e[1] );
    REQUIRE( i == head.end() );

    if constexpr (bds::STailQ<ListType>)
      REQUIRE( std::addressof(*head.before_end()) == &e[1] );
  }
}

template <bds::LinkedList ListHeadType>
void for_each_safe_tests() {
  using E = ListHeadType::value_type;
  ListHeadType head;
  E *e[] = { new E{0}, new E{1}, new E{2} };

  insert_front(head, std::begin(e), std::end(e));
  head.for_each_safe([&head] (E &item) {
    erase_front(head);
    E *const p = std::addressof(item);

    // Placement new a dummy value over the top of the item to simulate
    // it being deleted to verify that it was actually deleted
    new(p) E{123};
    std::memset(&p->next, 0, sizeof(typename ListHeadType::entry_type));
  });

  REQUIRE( head.empty() );

  for (E *item : e) {
    REQUIRE(item->i == 123);
    delete item;
  }
}

template <bds::LinkedList ListType1, bds::LinkedList ListType2>
void swap_lists() {
  using E = ListType1::value_type;
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
    if constexpr (bds::TailQ<ListType1>)
      REQUIRE( std::addressof(*--head1.end()) == &e[2] );
    if constexpr (bds::STailQ<ListType1>)
      REQUIRE( std::addressof(*head1.before_end()) == &e[2] );

    REQUIRE( std::size(head2) == 1 );
    REQUIRE( std::addressof(*head2.begin()) == &e[0] );
    if constexpr (bds::STailQ<ListType2>)
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
    if constexpr (bds::STailQ<ListType1>)
      REQUIRE( std::addressof(*head2.before_end()) == &e[2] );
  }
}

template <bds::LinkedList ListHeadType, bds::LinkedList ListProxyType>
void swap_tests() {
  SECTION("same_type_head") { swap_lists<ListHeadType, ListHeadType>(); }
  SECTION("same_type_fwd") { swap_lists<ListProxyType, ListProxyType>(); }
  SECTION("swap_to_fwd_head") { swap_lists<ListHeadType, ListProxyType>(); }
  SECTION("swap_from_fwd_head") { swap_lists<ListProxyType, ListHeadType>(); }
}

template <bds::SListOrQueue ListType>
void find_predecessor_tests() {
  using E = ListType::value_type;

  ListType head;
  E e[] = { {0}, {1} };

  head.insert_after(head.before_begin(), { &e[0], &e[1] });

  SECTION("predecessor") {
    REQUIRE( head.find_predecessor(head.before_begin()) == head.end() );
    REQUIRE( head.find_predecessor(head.iter(&e[0])) == head.before_begin() );
    REQUIRE( head.find_predecessor(head.iter(&e[1])) == head.iter(&e[0]) );
    REQUIRE( head.find_predecessor(head.end()) == head.iter(&e[1]) );
  }

  SECTION("find_erase") {
    E *erased;
    typename ListType::iterator next;

    std::tie(erased, next) = head.find_erase(head.iter(&e[0]));
    REQUIRE( std::size(head) == 1 );
    REQUIRE( erased == &e[0] );
    REQUIRE( std::addressof(head.front()) == &e[1] );
    REQUIRE( next == head.begin() );
    if constexpr (bds::STailQ<ListType>)
      REQUIRE( std::addressof(head.back()) == &e[1] );
  }
}

template <typename ProxyType>
void proxy_test_helper(ProxyType head, ProxyType::value_type *e) {
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

  using E = ProxyType::value_type;
  E e[] = { {0}, {1} };

  insert_front(head, &e[0]);
  REQUIRE( std::size(head) == 1 );
  proxy_test_helper<ProxyType>(fwdHead, &e[1]);
  REQUIRE( std::size(head) == 2 );
}

#endif
