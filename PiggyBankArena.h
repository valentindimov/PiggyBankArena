/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>
*/

// PiggyBankArena: Arena where you can allocate memory as normal, but have to free it all at once.
// It also supports adding cleanup actions (e.g. releasing locks or file handles) and performing them when the arena is freed.

#ifndef TEMP_ARENA_H
#define TEMP_ARENA_H

#ifndef NULL
#define NULL ((void*)0)
#endif

// The arena allows you to register cleanup actions to be called when it is reset (freed).
// Each action is a function pointer that accepts a single argument and returns nothing.
struct _PiggyBankArenaCleanupAction {
    void (*func)(void*);
    void *argument;
};

// The arena is initialized in a free chunk of memory provided by the user. It looks as follows:
// [{Header} {Heap (grows up) ->} ... {<- Cleanup Action Stack (grows down)}]
struct PiggyBankArena {
    void *end;
    unsigned char *heapTop;
    struct _PiggyBankArenaCleanupAction *cleanupActionsBottom;
    unsigned char start[];
};

// Create a new arena inside the given memory block. Returns a pointer to the same block of memory (now to be interpreted as an Arena pointer) on success, or NULL on failure.
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

// Allocate a new object of the given size. Returns a pointer to the allocated memory on success, or NULL on failure.
static inline void* PiggyBankArenaAlloc(struct PiggyBankArena* arena, unsigned long size) {
    unsigned long remainingSpace = ((unsigned char*) arena->cleanupActionsBottom) - arena->heapTop;
    
    if (remainingSpace < size) {
        return NULL;
    }

    void* result = arena->heapTop;
    arena->heapTop += size;
    return result;
}

// Allocate a cleanup action. Returns a pointer to the action struct on success, or NULL on failure.
static inline struct _PiggyBankArenaCleanupAction* PiggyBankArenaScheduleCleanup(struct PiggyBankArena* arena, void (*cleanupFunction)(void*), void* argument) {
    unsigned long remainingSpace = ((unsigned char*) arena->cleanupActionsBottom) - arena->heapTop;
    
    if (remainingSpace < sizeof (struct _PiggyBankArenaCleanupAction)) {
        return (struct _PiggyBankArenaCleanupAction*) NULL;
    }

    arena->cleanupActionsBottom--;
    arena->cleanupActionsBottom->func = cleanupFunction;
    arena->cleanupActionsBottom->argument = argument;
    return arena->cleanupActionsBottom;
}

// Call all registered cleanup functions and reset the arena to an empty state. After this, the arena can be reused again, as if it was just created.
static inline void PiggyBankArenaCleanup(struct PiggyBankArena* arena) {
    struct _PiggyBankArenaCleanupAction* action = arena->cleanupActionsBottom;
    
    while (action < arena->end) {
        action->func(action->argument);
        action++;
    }

    arena->heapTop = arena->start;
    arena->cleanupActionsBottom = (struct _PiggyBankArenaCleanupAction*) arena->end;
}


#ifdef __cplusplus
// Internal function that wraps the destructor of a given class as a function useable in a cleanup action.
template <typename T> void _PiggyBankArenaDestroyObject(void* obj) {
    if (obj != nullptr) {
        ((T*)obj)->~T();
    }
}

// Allocate space for an object of class T. Optionally, you can avoid calling the class destructor when the arena is cleaned up.
template <typename T> T* PiggyBankArenaAllocObject(PiggyBankArena* arena, bool callDestructorOnCleanup = true) {
    T* result = (T*) PiggyBankArenaAlloc(arena, sizeof(T));

    if (result == nullptr) {
        return nullptr;
    }

    if (callDestructorOnCleanup) {
        if (!PiggyBankArenaScheduleCleanup(arena, &_PiggyBankArenaDestroyObject<T>, result)) {
            arena->heapTop -= sizeof(T);
            return nullptr;
        }
    }

    return result;
}
#endif // __cplusplus

#endif // TEMP_ARENA_H