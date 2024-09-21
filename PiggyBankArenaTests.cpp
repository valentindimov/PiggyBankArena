#define CONFIG_CATCH_MAIN

#include "catch2/catch_amalgamated.hpp"
#include "PiggyBankArena.h"

struct _TestStruct {
    unsigned long value;

    ~_TestStruct() {
        value = 42;
    }
};

void logCleanupFunction(void *log) {
    *(int*)log = 42;
}

TEST_CASE( "Arena refuses to initialize if the buffer is too small" ) {
    char memBuffer[sizeof(PiggyBankArena)] = {0};

    REQUIRE(!PiggyBankArenaInit(memBuffer, 0));
    REQUIRE(!PiggyBankArenaInit(memBuffer, 1));
    REQUIRE(!PiggyBankArenaInit(memBuffer, sizeof(PiggyBankArena) - 1));
    REQUIRE(!PiggyBankArenaInit(memBuffer, sizeof(PiggyBankArena)));
}

TEST_CASE( "Arena allocation and deallocation" ) {
    char memBuffer[sizeof(PiggyBankArena) + 3*sizeof(_TestStruct)];
    PiggyBankArena *arena = PiggyBankArenaInit(memBuffer, sizeof(memBuffer));
    REQUIRE(arena != nullptr);

    SECTION( "C" ) {
        REQUIRE(PiggyBankArenaAlloc(arena, sizeof(_TestStruct)) == arena->start);
        REQUIRE(arena->heapTop == arena->start + sizeof(_TestStruct));
        REQUIRE(PiggyBankArenaAlloc(arena, sizeof(_TestStruct)) == arena->start + sizeof(_TestStruct));
        REQUIRE(arena->heapTop == arena->start + 2*sizeof(_TestStruct));
        REQUIRE(PiggyBankArenaAlloc(arena, sizeof(_TestStruct)) == arena->start + 2*sizeof(_TestStruct));
        REQUIRE(arena->heapTop == arena->start + 3*sizeof(_TestStruct));
        REQUIRE(PiggyBankArenaAlloc(arena, sizeof(_TestStruct)) == nullptr);
        REQUIRE(arena->heapTop == arena->start + 3*sizeof(_TestStruct));
    }
    
    SECTION( "C++, without destructors" ) {
        REQUIRE(PiggyBankArenaAllocObject<_TestStruct>(arena, false) == (_TestStruct*) arena->start);
        REQUIRE(arena->heapTop == arena->start + sizeof(_TestStruct));
        REQUIRE(PiggyBankArenaAllocObject<_TestStruct>(arena, false) == (_TestStruct*) arena->start + 1);
        REQUIRE(arena->heapTop == arena->start + 2*sizeof(_TestStruct));
        REQUIRE(PiggyBankArenaAllocObject<_TestStruct>(arena, false) == (_TestStruct*) arena->start + 2);
        REQUIRE(arena->heapTop == arena->start + 3*sizeof(_TestStruct));
        REQUIRE(PiggyBankArenaAllocObject<_TestStruct>(arena, false) == (_TestStruct*) nullptr);
        REQUIRE(arena->heapTop == arena->start + 3*sizeof(_TestStruct));
    }

    PiggyBankArenaCleanup(arena);

    REQUIRE(arena->start == (unsigned char*) arena + sizeof(PiggyBankArena));
    REQUIRE(arena->heapTop == arena->start);
    REQUIRE(arena->end == arena->start + 3*sizeof(_TestStruct));
    REQUIRE(arena->cleanupActionsBottom == arena->end);
}

TEST_CASE( "Arena cleanup actions (C)" ) {
    char memBuffer[sizeof(PiggyBankArena) + 3*sizeof(_PiggyBankArenaCleanupAction)];
    PiggyBankArena *arena = PiggyBankArenaInit(memBuffer, sizeof(memBuffer));
    REQUIRE(arena != nullptr);

    int cleanup1 = 0;
    int cleanup2 = 0;
    int cleanup3 = 0;
    int cleanup4 = 0;

    REQUIRE(PiggyBankArenaScheduleCleanup(arena, &logCleanupFunction, &cleanup1) == (_PiggyBankArenaCleanupAction*)arena->end - 1);
    REQUIRE(arena->cleanupActionsBottom == (_PiggyBankArenaCleanupAction*)arena->end - 1);
    REQUIRE(PiggyBankArenaScheduleCleanup(arena, &logCleanupFunction, &cleanup2) == (_PiggyBankArenaCleanupAction*)arena->end - 2);
    REQUIRE(arena->cleanupActionsBottom == (_PiggyBankArenaCleanupAction*)arena->end - 2);
    REQUIRE(PiggyBankArenaScheduleCleanup(arena, &logCleanupFunction, &cleanup3) == (_PiggyBankArenaCleanupAction*)arena->end - 3);
    REQUIRE(arena->cleanupActionsBottom == (_PiggyBankArenaCleanupAction*)arena->end - 3);
    REQUIRE(PiggyBankArenaScheduleCleanup(arena, &logCleanupFunction, &cleanup4) == (_PiggyBankArenaCleanupAction*)NULL);
    REQUIRE(arena->cleanupActionsBottom == (_PiggyBankArenaCleanupAction*)arena->end - 3);

    PiggyBankArenaCleanup(arena);
    REQUIRE(cleanup1 == 42);
    REQUIRE(cleanup2 == 42);
    REQUIRE(cleanup3 == 42);
    REQUIRE(cleanup4 == 0);

    REQUIRE(arena->start == (unsigned char*) arena + sizeof(PiggyBankArena));
    REQUIRE(arena->heapTop == arena->start);
    REQUIRE(arena->end == arena->start + 3*sizeof(_PiggyBankArenaCleanupAction));
    REQUIRE(arena->cleanupActionsBottom == arena->end);
}

TEST_CASE( "Arena allocation and deallocation (C++, with destructors)" ) {
    char memBuffer[sizeof(PiggyBankArena) + 3*sizeof(_TestStruct) + 3*sizeof(_PiggyBankArenaCleanupAction)];
    PiggyBankArena *arena = PiggyBankArenaInit(memBuffer, sizeof(memBuffer));
    REQUIRE(arena != nullptr);

    REQUIRE(PiggyBankArenaAllocObject<_TestStruct>(arena, true) == (_TestStruct*) arena->start);
    REQUIRE(arena->cleanupActionsBottom == (_PiggyBankArenaCleanupAction*)arena->end - 1);
    REQUIRE(arena->heapTop == arena->start + sizeof(_TestStruct));
    REQUIRE(PiggyBankArenaAllocObject<_TestStruct>(arena, true) == (_TestStruct*) arena->start + 1);
    REQUIRE(arena->cleanupActionsBottom == (_PiggyBankArenaCleanupAction*)arena->end - 2);
    REQUIRE(arena->heapTop == arena->start + 2*sizeof(_TestStruct));
    REQUIRE(PiggyBankArenaAllocObject<_TestStruct>(arena, true) == (_TestStruct*) arena->start + 2);
    REQUIRE(arena->cleanupActionsBottom == (_PiggyBankArenaCleanupAction*)arena->end - 3);
    REQUIRE(arena->heapTop == arena->start + 3*sizeof(_TestStruct));
    REQUIRE(PiggyBankArenaAllocObject<_TestStruct>(arena, true) == (_TestStruct*) nullptr);
    REQUIRE(arena->heapTop == arena->start + 3*sizeof(_TestStruct));
    REQUIRE(arena->cleanupActionsBottom == (_PiggyBankArenaCleanupAction*)arena->end - 3);

    PiggyBankArenaCleanup(arena);
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
    PiggyBankArena *arena = PiggyBankArenaInit(memBuffer, sizeof(memBuffer));
    REQUIRE(arena != nullptr);

    REQUIRE(PiggyBankArenaAllocObject<_TestStruct>(arena, true) == (_TestStruct*) arena->start);
    REQUIRE(arena->cleanupActionsBottom == (_PiggyBankArenaCleanupAction*)arena->end - 1);
    REQUIRE(arena->heapTop == arena->start + sizeof(_TestStruct));
    REQUIRE(PiggyBankArenaAllocObject<_TestStruct>(arena, true) == (_TestStruct*) NULL);
    REQUIRE(arena->cleanupActionsBottom == (_PiggyBankArenaCleanupAction*)arena->end - 1);
    REQUIRE(arena->heapTop == arena->start + sizeof(_TestStruct));

    PiggyBankArenaCleanup(arena);
    REQUIRE(((_TestStruct *) arena->start)->value == 42);
    REQUIRE(arena->start == (unsigned char*) arena + sizeof(PiggyBankArena));
    REQUIRE(arena->heapTop == arena->start);
    REQUIRE(arena->end == arena->start + 2*sizeof(_TestStruct) + sizeof(_PiggyBankArenaCleanupAction));
    REQUIRE(arena->cleanupActionsBottom == arena->end);
}
