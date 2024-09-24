/// PiggyBankArena: Allocate memory into a block which you free all at once.
/// This is the pure-C version of the library.

#pragma once

#ifndef NULL
#define NULL 0
#endif

/// @brief A single cleanup action that the arena calls when cleaned up.
struct _PiggyBankArenaCleanupAction {
    void (*func)(void*);
    void *argument;
};

/// @brief The arena occupies a user-provided chunk of memory. It looks like this: [{Header} {Heap (grows up) ->} ... {<- Cleanup Action Stack (grows down)}]
struct PiggyBankArena {
    void *end;
    unsigned char *heapTop;
    struct _PiggyBankArenaCleanupAction *cleanupActionsBottom;
    unsigned char start[];
};

/// @brief Query how much space is remaining in an arena.
/// @param arena A pointer to the arena.
/// @returns the total space in bytes remaining in the arena (for both cleanup actions and the heap).
inline unsigned long PiggyBankArenaRemainingSpace(struct PiggyBankArena* arena) {
    return ((unsigned char*) arena->cleanupActionsBottom) - arena->heapTop;
}

/// @brief Initialize an arena in some user-provided memory.
/// @param memory A pointer to the memory that will be used by the arena.
/// @param size The size of the memory in bytes.
/// @returns a pointer to an initialized arena, or NULL if the provided memory is not large enough.
static inline struct PiggyBankArena *PiggyBankArenaInit(void* memory, unsigned long size) {
    if (size <= sizeof (struct PiggyBankArena)) {
        return (struct PiggyBankArena*) NULL;
    }

    struct PiggyBankArena* result = (struct PiggyBankArena*) memory;
    result->heapTop = (unsigned char*) memory + sizeof (struct PiggyBankArena);
    result->end = ((unsigned char*)result) + size;
    result->cleanupActionsBottom = (struct _PiggyBankArenaCleanupAction*) result->end;
    return result;
}

/// @brief Allocate an memory from an arena.
/// @param arena A pointer to the arena that will be used for allocation.
/// @param size The amount of memory in bytes.
/// @returns a pointer to the allocated memory, or NULL if there is not enough space in the arena for the given size.
static inline void* PiggyBankArenaAlloc(struct PiggyBankArena* arena, unsigned long size) {
    unsigned long remainingSpace = PiggyBankArenaRemainingSpace(arena);
    
    if (remainingSpace < size) {
        return NULL;
    }

    void* result = arena->heapTop;
    arena->heapTop += size;
    return result;
}

/// @brief Schedule a cleanup function to be run when the arena is cleaned up.
/// @param arena A pointer to the arena.
/// @param cleanupFunction The function that will be called when the arena is cleaned up.
/// @param argument An argument that will be passed to the cleanup function.
/// @returns a pointer to the cleanup action that was scheduled on success, or NULL if there is not enough space in the arena for the given size.
static inline struct _PiggyBankArenaCleanupAction* PiggyBankArenaScheduleCleanup(struct PiggyBankArena* arena, void (*cleanupFunction)(void*), void* argument) {
    unsigned long remainingSpace = PiggyBankArenaRemainingSpace(arena);
    if (remainingSpace < sizeof (struct _PiggyBankArenaCleanupAction)) {
        return (struct _PiggyBankArenaCleanupAction*) NULL;
    }

    arena->cleanupActionsBottom--;
    arena->cleanupActionsBottom->func = cleanupFunction;
    arena->cleanupActionsBottom->argument = argument;
    return arena->cleanupActionsBottom;
}

/// @brief Clean up the given arena by calling all registered cleanup functions and resetting the arena to an empty state. 
/// @remark After this, the arena can be reused again, as if it was just created.
/// @param arena A pointer to the arena.
static inline void PiggyBankArenaCleanup(struct PiggyBankArena* arena) {
    struct _PiggyBankArenaCleanupAction* action = arena->cleanupActionsBottom;
    
    while (action < arena->end) {
        action->func(action->argument);
        action++;
    }

    arena->heapTop = arena->start;
    arena->cleanupActionsBottom = (struct _PiggyBankArenaCleanupAction*) arena->end;
}