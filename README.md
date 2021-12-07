Lapser: LAzy Parameter Server with RDMA
=======================================


Main concepts:
  - Slack, as in Stale Synchronous Parallel (bounded assynchrony)
  - GASPI, an API enabling fast one-sided communication with RDMA


Installation
------------

This project uses the CMake meta-build system, version 3.10 or above.

Critical dependencies: GPI-2, C compiler with C99 standard support
Optional dependencies: ASan & UBSan compiler support, googletest, C++ compiler


Please note that the final linking step with GPI-2 should be done with your
application build step;
to compile this library, having the GASPI header files is enough.

If you choose to compile Lapser as a shared library (the default option),
remember to run `ldconfig` after installing the library, to set up the appropriate
bindings in your system linker.

Testing
-------

If you want to run the tests, you'll need the GPI-2 library installed
in a suitable location (this is, find-able by the linker and
with the `gaspi_run` command present in `PATH`), and also
a local copy of [GoogleTest](https://google.github.io/googletest/),
which is used as a unit testing framework.
The location of the GoogleTest library must be indicated in
the `GTEST_CMAKE_PATH` CMake variable (either by using the option
`-DGTEST_CMAKE_PATH` on the `cmake` command or by using
`make edit_cache` afterwards).

Additionally, you should create a machinefile named `hosts.txt`
at the project source directory, to be used in the `gaspi_run` command.

After that, you can run the tests from your build directory using:

    make check

