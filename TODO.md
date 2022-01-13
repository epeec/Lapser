
TODO
====

(Note: also see TODOs & FIXMEs on the code)

  - lapser_set/lapser_get
    + [X] Just local version
    + [X] Basic PULL version (always gaspi_read, don't ask if its good to go)
    + [X] Basic PUSH version (same but gaspi_write)
    + [X] Smart PUSH version (at least clocks + checksum)
    + [X] Smart PULL version (idem)

  - Make actually working tests
    + [X] Write googletest fixtures for GASPI
    + [X] Write script to launch tests
    + [X] Write some lapser app code to see if it works somehow
    + [X] Translate the src/main.c into a test case
    + [ ] Figure out if there are better ways to integrate googletest in CMake

  - [X] Do something better (in CMake) about the auxiliary gaspi functions
  - [X] Better code style, see Roman example
  - [X] Add sample app folder, to debug failing test cases (use the exp test)

  - [X] Fix linking stuff (between Lapser and GASPI)
    + [ ] Fix failing find_library() call on test CMakeList.txt -> use LD_PATHS
    + [X] Try to understand what CMake is doing when going the find_library() route
        ```
        ----------------------------------------------------------------------
        Libraries have been installed in:
           /home/rafael/local/lib64

        If you ever happen to want to link against installed libraries
        in a given directory, LIBDIR, you must either use libtool, and
        specify the full pathname of the library, or use the '-LLIBDIR'
        flag during linking and do at least one of the following:
           - add LIBDIR to the 'LD_LIBRARY_PATH' environment variable
             during execution (script)
           - add LIBDIR to the 'LD_RUN_PATH' environment variable
             during linking <- doing this
           - use the '-Wl,-rpath -Wl,LIBDIR' linker flag
           - have your system administrator add LIBDIR to '/etc/ld.so.conf'

        See any operating system documentation about shared libraries for
        more information, such as the ld(1) and ld.so(8) manual pages.
        ----------------------------------------------------------------------
        ```

  - [ ] Configurable place to put the logs (maybe a separate function?) (consider gaspi_printf?)
    + [ ] Debug levels (check Amândio's work), and prepend the various levels with the timestamp

  - [ ] Better hash function / checksum strategy ?
    + Check if it actually consumes a measurable fraction of time in implementation
    + Check if checksum is not preferable, may be faster?
    + Check https://github.com/aappleby/smhasher/ and https://github.com/ztanml/fast-hash

  - Sparser datastructures?
    (right now, we are assuming Key belong in the range [0, total_items[)
    + [ ] metadata lookup
    + [ ] item lookup
    + [ ] consumers offset lookup
    + [ ] Key assignment (possibly already done, but not testable without the others)

  - [X] Allow more than 64 processes
    + Actually check that this works up to 64 processes
    + [X] Change item_metadata fields - consumers, consumers_offsets

  - Make it thread-safe
    + [ ] Extern variables inside struct
    + [ ] Functions receive extra argument somehow pointing to the said struct
    + [ ] Think about problems in multiple concurrent Lapser initializations

  - [ ] Finish writing README

