min11
=====
min11 provides a subset of C++11 functionality to older compilers.

C++11 support:
-------------
* std::mutex (no timed wait)
* std::unique\_lock
* std::condition\_variable (no timed wait)
* std::future (no timed wait)
* std::shared\_future (no timed wait)
* std::promise

Compilers supported:
--------------------
* Visual Studio 2009 (vc9)
* Visual Studio 2010 (vc10)
* Visual Studio 2012 (vs11) - Not needed, vc11 has support
* Visual Studio 2013 (vs12) - Not needed, vc12 has support

Limitations
-----------
Type inference (via the `auto` keyword) does not work with the
`promise<T>::get_future` function. `get_future` actually returns
a `min11::detail::movable_future<T>` to allow the psuedo-move
semantics needed to return a future by value, but disallow copy
semantics for the `future<T>` type. As a result, the following
will not work:
`auto ftr = promise.get_future();
ftr.wait(); // compiler error here T__T`

Instead, you will need to bind the result of `get_future` to an
explicitly named `future<T>` name:
`min11::future<T> ftr = promise.get_future();
ftr.wait();`


There are no timed wait functions. In order to preserver some
semblance of compatibility, this requires larger support for
at least an `std::chrono::duration<Rep, Period>` type. On the todo
list.

Contact
-------
[@_boblan_](https://twitter.com/#!/_boblan_)  
<https://github.com/mendsley/min11>

License
-------
Copyright 2014 Matthew Endsley

This project is governed by the BSD 2-clause license. For details see the file
titled LICENSE in the project root folder.
