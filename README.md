# Piggy Bank Arena

A small C or C++ header-only library implementing a memory arena which is freed all at once.
* You can allocate memory from the arena.
* You do not free individual objects from the arena, you need to clean up the entire arena to free all memory at once.
* You can also schedule clean-up actions (such as releasing locks or file handles) to run when the arena is freed.
* If compiling under C++, you can allocate memory for an object (you still need to call its placement new operator). You can choose if the destructor should be called when the arena is freed.

## Usage
Include `PiggyBankArena.h` in your code.

## Tests
The tests use the Catch2 framework, which is included with the repository. Run `build_and_run_tests_windows.cmd` or `build_and_run_tests_linux.sh`, depending on your system, to build and run the tests.

## License
The library (`PiggyBankArena.h`) and the test suite (`PiggyBankArenaTests.cpp`) are released into the public domain. For more details, see `LICENSE.txt`.

Catch2 is redistributed with this repository under the Boost Software License Version 1.0 (see `catch2\LICENSE.txt`). Catch2 and the test suite are not part of the library - you do not need to include them in your projects.