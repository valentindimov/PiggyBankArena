#define CONFIG_CATCH_MAIN

#include "catch2/catch_amalgamated.hpp"
#include "PiggyBankArenaCPP.hpp"

namespace PiggyBankArena {

struct _TestStruct {
    unsigned long value;

    ~_TestStruct() {
        value = 42;
    }
};

void logCleanupFunction(void *log) {
    *(int*)log = 42;
}

TEST_CASE( "Arena refuses to initialize if the buffer is too small (C++)" ) {
    char memBuffer[sizeof(PiggyBankArena)] = {0};

    REQUIRE(!PiggyBankArena::init(memBuffer, 0));
    REQUIRE(!PiggyBankArena::init(memBuffer, 1));
    REQUIRE(!PiggyBankArena::init(memBuffer, sizeof(PiggyBankArena) - 1));
    REQUIRE(!PiggyBankArena::init(memBuffer, sizeof(PiggyBankArena)));
}

TEST_CASE( "Arena space allocation and deallocation (C++)" ) {
    char memBuffer[sizeof(PiggyBankArena) + 3*sizeof(_TestStruct)];
    PiggyBankArena *arena = PiggyBankArena::init(memBuffer, sizeof(memBuffer));
    REQUIRE(arena != nullptr);

    REQUIRE(arena->alloc(sizeof(_TestStruct)) == arena->start);
    REQUIRE(arena->heapTop == arena->start + sizeof(_TestStruct));
    REQUIRE(arena->alloc(sizeof(_TestStruct)) == arena->start + sizeof(_TestStruct));
    REQUIRE(arena->heapTop == arena->start + 2*sizeof(_TestStruct));
    REQUIRE(arena->alloc(sizeof(_TestStruct)) == arena->start + 2*sizeof(_TestStruct));
    REQUIRE(arena->heapTop == arena->start + 3*sizeof(_TestStruct));
    REQUIRE(arena->alloc(sizeof(_TestStruct)) == nullptr);
    REQUIRE(arena->heapTop == arena->start + 3*sizeof(_TestStruct));

    arena->cleanup();

    REQUIRE(arena->start == (unsigned char*) arena + sizeof(PiggyBankArena));
    REQUIRE(arena->heapTop == arena->start);
    REQUIRE(arena->end == arena->start + 3*sizeof(_TestStruct));
    REQUIRE(arena->cleanupActionsBottom == arena->end);
}

TEST_CASE( "Arena cleanup actions (C++)" ) {
    char memBuffer[sizeof(PiggyBankArena) + 3*sizeof(_PiggyBankArenaCleanupAction)];
    PiggyBankArena *arena = PiggyBankArena::init(memBuffer, sizeof(memBuffer));
    REQUIRE(arena != nullptr);

    int cleanup1 = 0;
    int cleanup2 = 0;
    int cleanup3 = 0;
    int cleanup4 = 0;

    REQUIRE(arena->scheduleCleanup(&logCleanupFunction, &cleanup1) == (_PiggyBankArenaCleanupAction*)arena->end - 1);
    REQUIRE(arena->cleanupActionsBottom == (_PiggyBankArenaCleanupAction*)arena->end - 1);
    REQUIRE(arena->scheduleCleanup(&logCleanupFunction, &cleanup2) == (_PiggyBankArenaCleanupAction*)arena->end - 2);
    REQUIRE(arena->cleanupActionsBottom == (_PiggyBankArenaCleanupAction*)arena->end - 2);
    REQUIRE(arena->scheduleCleanup(&logCleanupFunction, &cleanup3) == (_PiggyBankArenaCleanupAction*)arena->end - 3);
    REQUIRE(arena->cleanupActionsBottom == (_PiggyBankArenaCleanupAction*)arena->end - 3);
    REQUIRE(arena->scheduleCleanup(&logCleanupFunction, &cleanup4) == (_PiggyBankArenaCleanupAction*)NULL);
    REQUIRE(arena->cleanupActionsBottom == (_PiggyBankArenaCleanupAction*)arena->end - 3);

    arena->cleanup();

    REQUIRE(cleanup1 == 42);
    REQUIRE(cleanup2 == 42);
    REQUIRE(cleanup3 == 42);
    REQUIRE(cleanup4 == 0);

    REQUIRE(arena->start == (unsigned char*) arena + sizeof(PiggyBankArena));
    REQUIRE(arena->heapTop == arena->start);
    REQUIRE(arena->end == arena->start + 3*sizeof(_PiggyBankArenaCleanupAction));
    REQUIRE(arena->cleanupActionsBottom == arena->end);
}

TEST_CASE( "Arena allocation and deallocation (C++, without destructors)" ) {
    char memBuffer[sizeof(PiggyBankArena) + 3*sizeof(_TestStruct)];
    PiggyBankArena *arena = PiggyBankArena::init(memBuffer, sizeof(memBuffer));
    REQUIRE(arena != nullptr);
    
    REQUIRE(arena->allocObject<_TestStruct>(false) == (_TestStruct*) arena->start);
    REQUIRE(arena->heapTop == arena->start + sizeof(_TestStruct));
    REQUIRE(arena->allocObject<_TestStruct>(false) == (_TestStruct*) arena->start + 1);
    REQUIRE(arena->heapTop == arena->start + 2*sizeof(_TestStruct));
    REQUIRE(arena->allocObject<_TestStruct>(false) == (_TestStruct*) arena->start + 2);
    REQUIRE(arena->heapTop == arena->start + 3*sizeof(_TestStruct));
    REQUIRE(arena->allocObject<_TestStruct>(false) == (_TestStruct*) nullptr);
    REQUIRE(arena->heapTop == arena->start + 3*sizeof(_TestStruct));

    arena->cleanup();

    REQUIRE(arena->start == (unsigned char*) arena + sizeof(PiggyBankArena));
    REQUIRE(arena->heapTop == arena->start);
    REQUIRE(arena->end == arena->start + 3*sizeof(_TestStruct));
    REQUIRE(arena->cleanupActionsBottom == arena->end);
}

TEST_CASE( "Arena allocation and deallocation (C++, with destructors)" ) {
    char memBuffer[sizeof(PiggyBankArena) + 3*sizeof(_TestStruct) + 3*sizeof(_PiggyBankArenaCleanupAction)];
    PiggyBankArena *arena = PiggyBankArena::init(memBuffer, sizeof(memBuffer));
    REQUIRE(arena != nullptr);

    REQUIRE(arena->allocObject<_TestStruct>(true) == (_TestStruct*) arena->start);
    REQUIRE(arena->cleanupActionsBottom == (_PiggyBankArenaCleanupAction*)arena->end - 1);
    REQUIRE(arena->heapTop == arena->start + sizeof(_TestStruct));
    REQUIRE(arena->allocObject<_TestStruct>(true) == (_TestStruct*) arena->start + 1);
    REQUIRE(arena->cleanupActionsBottom == (_PiggyBankArenaCleanupAction*)arena->end - 2);
    REQUIRE(arena->heapTop == arena->start + 2*sizeof(_TestStruct));
    REQUIRE(arena->allocObject<_TestStruct>(true) == (_TestStruct*) arena->start + 2);
    REQUIRE(arena->cleanupActionsBottom == (_PiggyBankArenaCleanupAction*)arena->end - 3);
    REQUIRE(arena->heapTop == arena->start + 3*sizeof(_TestStruct));
    REQUIRE(arena->allocObject<_TestStruct>(true) == (_TestStruct*) nullptr);
    REQUIRE(arena->heapTop == arena->start + 3*sizeof(_TestStruct));
    REQUIRE(arena->cleanupActionsBottom == (_PiggyBankArenaCleanupAction*)arena->end - 3);

    arena->cleanup();

    REQUIRE(((_TestStruct *) arena->start)->value == 42);
    REQUIRE(((_TestStruct *) arena->start + 1)->value == 42);
    REQUIRE(((_TestStruct *) arena->start + 2)->value == 42);

    REQUIRE(arena->start == (unsigned char*) arena + sizeof(PiggyBankArena));
    REQUIRE(arena->heapTop == arena->start);
    REQUIRE(arena->end == arena->start + 3*sizeof(_TestStruct) + 3*sizeof(_PiggyBankArenaCleanupAction));
    REQUIRE(arena->cleanupActionsBottom == arena->end);
}

TEST_CASE( "Arena allocation of a C++ object with destructors fails properly if there's no space for the cleanup action" ) {
    char memBuffer[sizeof(PiggyBankArena) + 2*sizeof(_TestStruct) + sizeof(_PiggyBankArenaCleanupAction)];
    PiggyBankArena *arena = PiggyBankArena::init(memBuffer, sizeof(memBuffer));
    REQUIRE(arena != nullptr);

    REQUIRE(arena->allocObject<_TestStruct>(true) == (_TestStruct*) arena->start);
    REQUIRE(arena->cleanupActionsBottom == (_PiggyBankArenaCleanupAction*)arena->end - 1);
    REQUIRE(arena->heapTop == arena->start + sizeof(_TestStruct));
    REQUIRE(arena->allocObject<_TestStruct>(true) == (_TestStruct*) NULL);
    REQUIRE(arena->cleanupActionsBottom == (_PiggyBankArenaCleanupAction*)arena->end - 1);
    REQUIRE(arena->heapTop == arena->start + sizeof(_TestStruct));

    arena->cleanup();

    REQUIRE(((_TestStruct *) arena->start)->value == 42);
    REQUIRE(arena->start == (unsigned char*) arena + sizeof(PiggyBankArena));
    REQUIRE(arena->heapTop == arena->start);
    REQUIRE(arena->end == arena->start + 2*sizeof(_TestStruct) + sizeof(_PiggyBankArenaCleanupAction));
    REQUIRE(arena->cleanupActionsBottom == arena->end);
}

}