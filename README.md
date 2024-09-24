# Piggy Bank Arena

A small C or C++ header-only library implementing a memory heap which is freed all at once.
* Arenas are created with a user-supplied memory buffer.
* Chunks of memory can be allocated from the arena.
* Cleanup actions can be scheduled to run when the arena is cleaned up.
* When the arena is cleaned up, all cleanup actions are executed (last-in-first-out order) and the arena's memory is empty again.
* Using the C++ interface, you can allocate memory for a specific type, which you can then use with the placement new operator. You can choose whether or not the class destructor should run when the arena is cleaned up.

## Usage
For a pure C interface, include `PiggyBankArenaC.h` in your code. For a C++ interface, include `PiggyBankArenaCPP.hpp`.

## Tests
The tests use the Catch2 framework, which is included with the repository. Run `build_and_run_tests_windows.cmd` or `build_and_run_tests_linux.sh`, depending on your system, to build and run the tests.

## License
The library (`PiggyBankArenaC.h` and `PiggyBankArenaCPP.hpp`) and the test suite (`PiggyBankArenaTestsC.cpp`, `PiggyBankArenaTestsCPP.cpp`) are released into the public domain. For more details, see `UNLICENSE.txt`.

Catch2 is redistributed with this repository under the Boost Software License Version 1.0 (see `catch2\LICENSE.txt`). Catch2 and the test suite are not part of the library - you do not need to include them in your projects.