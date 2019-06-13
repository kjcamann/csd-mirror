.. csd-readme-include-begin

What is CSD?
============

CSD contains C++23 implementations of several well-known data structures from the `BSD family of operating systems <https://en.wikipedia.org/wiki/Berkeley_Software_Distribution>`_. [1]_

It provides the following:

* STL-like implementation of BSD's `queue(3) <https://man.openbsd.org/queue.3>`_ intrusive linked list library (`link <https://kjcamann.github.io/doc/csd/lists-main.html>`_)
* STL-like implementation of BSD's intrusive, chained hash tables, similar to `hashinit(9) <https://man.openbsd.org/hashinit>`_
* `std::pmr::memory_resource <https://en.cppreference.com/w/cpp/memory/memory_resource>`_-compatible implementation of the `vmem(9) <https://www.freebsd.org/cgi/man.cgi?query=vmem&sektion=9>`_ general purpose memory allocator
* `std::allocator <https://en.cppreference.com/w/cpp/memory/allocator>`_-compatible implementation of the `uma(9) "zone" <https://www.freebsd.org/cgi/man.cgi?query=uma&sektion=9>`_ pool allocator
* STL-like ring buffers based on FreeBSD's `buf_ring(9) <https://www.freebsd.org/cgi/man.cgi?query=buf_ring>`_ and DPDK's `librte_ring <https://doc.dpdk.org/guides/prog_guide/ring_lib.html>`_
* A ring based shared memory allocator based on DPDK's `librte_mempool <https://doc.dpdk.org/guides/prog_guide/mempool_lib.html>`_
* A read/writer mutex that keeps track of all readers for debugging and robust deadlock detection, based on FreeBSD's `rmlock <https://www.freebsd.org/cgi/man.cgi?query=rmlock&sektion=9>`_ and conforming to the C++17 threading library concepts.

CSD is an acronym for "Cyril Software Data Structures"; it is used in other open source releases from the Cyril Software Group (CSG). Because it provides core functionality to other CSG projects, its classes are defined directly in ``namespace csg`` so they can be referenced via unqualified names in other CSG code.

When C++20 modules support improves in both gcc and clang, the name CSD will go away and it will be distributed as the module ``csg.core``.

.. warning::

   This is an alpha release of CSD, made so that other early-stage projects could source it from github. Some of the libraries described above are not included in this alpha release, and the libraries that *are* included do not have stable APIs or complete documentation.

.. warning::
   This alpha release of CSD uses C++20 concepts. For this reason, the reference documentation is often incorrect or missing, since the concept declarations cannot be parsed by doxygen.

Why should I care about this library?
=====================================

Consider this sad-but-true observation from `The Architecture of Open Source Applications <https://www.aosabook.org/en/index.html>`_:

   Architects look at thousands of buildings during their training, and study critiques of those buildings written by masters. In contrast, most software developers only ever get to know a handful of large programs well -- usually programs they wrote themselves -- and never study the great programs of history. As a result, they repeat one another's mistakes rather than building on one another's successes.

Most programmers are terrible software architects because they have never studied a truly great program designed by engineers who have mastered the art. If *you* have not done this, you should do it! One "great program of history" you might want to study is the FreeBSD kernel, because it has such excellent documentation.

.. sidebar:: Why BSD and and why UNIX?

   `The Design and Implementation of the FreeBSD Operating System <https://books.google.com/books?isbn=0321968972>`_ -- one of the great works in computer science literature -- walks through designs for all major subsystems needed for a modern OS. You will finish this book not just understanding a single great program, but understanding the brilliance of the original UNIX architecture.

   This architecture is peerless in computer science -- where there were once a million and one competitors, over time UNIX vanquished them all. This happened in large part because the clean, well-documented design made it possible to teach the architecture to the next generation of engineers. Every decade or so, a hefty tome would be published describing an updated UNIX design to the next class of operating system acolytes. This was true since the `very beginning <https://books.google.com/books?isbn=1573980137>`_, through the `critical years of its adoption <https://books.google.com/books?id=BxZpQgAACAAJ&dq=editions:6KhaBvAZBMMC>`_, during the years `it made the Internet possible <https://books.google.com/books?id=6rjd2ZxE1vYC>`_ and when it replaced everything in `commercial computing <https://books.google.com/books/about/Solaris_Internals.html?id=Aq9QAAAAMAAJ>`_, right up until `modern times <https://books.google.com/books?id=3MWRMYRwulIC>`_ where it is essentially one of only two designs left in the world -- and its only remaining competitor is slowly `embracing its programming interface. <https://en.wikipedia.org/wiki/Windows_Subsystem_for_Linux>`_

What CSD offers you
-------------------

In my experience, the effect of studying any great program is two-fold. First, your skills are *vastly* increased, elevating you from a mere programmer to a true "software engineer." Once you have seen what a great program looks like, you can stand on the shoulders of giants and credibly try to emulate it.

A second, more subtle effect is that the "patterns" and "structure" of the code begin to rub off on you. Just like artists are able to cite their "artistic influences," your own programming style becomes influenced by the great programs you study.

When this happens, there is a typically a problem.

All "large" programs (> 100,000 lines of code) are full of smaller helper libraries and attendant design patterns that help organize their code. Because the "great programs of history" are, by definition, very cleanly designed, you find yourself reaching for their libraries and design patterns in your own projects.

Although such code is typically useful *outside* of the original project, it often isn't easy to repackage into a free-standing library. Sometimes it's full of dependencies on unwanted interfaces (e.g., header files). Other times, you want to reuse the *core ideas* but in a different programming language.

CSD contains high quality, free-standing re-implementations of several such libraries from BSD-licensed code bases. The original libraries were mostly written in C, whereas CSD rewrites them using C++20 idioms.

If you haven't studied any of the programs that this code is derived from, you might not see the appeal. However, if you *have* witnessed the engineering excellence of these programs, you just might find yourself reaching for these powerful abstractions in your C++ projects.

And finally, here they are!

How do I install CSD?
=====================

CSD is a header-only library, so there is little to do. The one exception is if you enable assertions in the library by ``#define``\ ing ``CSG_DEBUG_LEVEL`` to ``1`` via the C preprocessor; in that case, the library will emit calls to an ``extern "C"`` free function called ``csg_assert_function`` which you must provide to the linker. No default definition is provided because the "weak linkage" concept is not portable to Windows. See the ``driver.cpp`` file in the test suite for an example of a simple assertion function that prints to stderr, then calls `std::terminate <https://en.cppreference.com/w/cpp/error/terminate>`_.

The included CMake build system is only needed to build the test suite and the Sphinx documentation, but it does include an ``install`` target which will copy the CSD headers, if you wish to use that. To build the Sphinx documentation, you must also install `doxygen <https://www.doxygen.org>`_, `breathe <https://breathe.readthedocs.io>`_, the `"Read the Docs" Sphinx theme <https://sphinx-rtd-theme.readthedocs.io/en/latest>`_, and `Sphinx itself <https://www.sphinx-doc.org/en/stable/>`_. Building the test suite will use CMake's `ExternalProject <https://cmake.org/cmake/help/latest/module/ExternalProject.html>`_ command to fetch the `Catch2 <https://github.com/catchorg/Catch2>`_ unit testing framework from Github, so it requires an Internet connection.

.. csd-readme-include-end

How do I start using CSD?
=========================

On the `main documentation page <https://kjcamann.github.io/doc/csd>`_ you will find links to the documentation for each library (lists, memory allocators, etc.). Each library includes a "quick start guide," reference API documentation, and implementation notes if you want to hack on the library itself.

.. csd-readme-footnote-begin

.. [1] CSD was originally called "BDS" -- for "BSD Data Structures" -- but the similarity between *BSD* and *BDS* made the documentation difficult to read. The old name was also slightly misleading, because a few of the designs come from `OpenSolaris <https://en.wikipedia.org/wiki/OpenSolaris>`_ and `DPDK <https://en.wikipedia.org/wiki/Data_Plane_Development_Kit>`_.

.. csd-readme-footnote-end
