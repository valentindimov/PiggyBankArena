/// PiggyBankArena: Allocate memory into a block which you free all at once.
/// This is the C++ version of the library.

#pragma once

namespace PiggyBankArena {

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

public:
    // The arena object is not created using constructors or destructors, but using a factory method
    PiggyBankArena() = delete;
    virtual ~PiggyBankArena() = default;

    /// @brief Initialize an arena in some user-provided memory.
    /// @param memory A pointer to the memory that will be used by the arena.
    /// @param size The size of the memory in bytes.
    /// @returns a pointer to an initialized arena, or NULL if the provided memory is not large enough.
    static inline struct PiggyBankArena *init(void* memory, unsigned long size) {
        if (size <= sizeof (struct PiggyBankArena)) {
            return (struct PiggyBankArena*) nullptr;
        }

        struct PiggyBankArena* result = (struct PiggyBankArena*) memory;
        result->heapTop = (unsigned char*) memory + sizeof (struct PiggyBankArena);
        result->end = ((unsigned char*)result) + size;
        result->cleanupActionsBottom = (struct _PiggyBankArenaCleanupAction*) result->end;
        return result;
    }

    /// @brief Query how much space is remaining in an arena.
    /// @returns the total space in bytes remaining in the arena (for both cleanup actions and the heap).
    inline unsigned long remainingSpace() {
        return ((unsigned char*) this->cleanupActionsBottom) - this->heapTop;
    }

    /// @brief Allocate an memory from an arena.
    /// @param size The amount of memory in bytes.
    /// @returns a pointer to the allocated memory, or NULL if there is not enough space in the arena for the given size.
    inline void* alloc(unsigned long size) {
        if (this->remainingSpace() < size) {
            return nullptr;
        }

        void* result = this->heapTop;
        this->heapTop += size;
        return result;
    }

    /// @brief Schedule a cleanup function to be run when the arena is cleaned up.
    /// @param cleanupFunction The function that will be called when the arena is cleaned up.
    /// @param argument An argument that will be passed to the cleanup function.
    /// @returns a pointer to the cleanup action that was scheduled on success, or NULL if there is not enough space in the arena for the given size.
    inline struct _PiggyBankArenaCleanupAction* scheduleCleanup(void (*cleanupFunction)(void*), void* argument) {
        if (this->remainingSpace() < sizeof (struct _PiggyBankArenaCleanupAction)) {
            return (struct _PiggyBankArenaCleanupAction*) nullptr;
        }

        this->cleanupActionsBottom--;
        this->cleanupActionsBottom->func = cleanupFunction;
        this->cleanupActionsBottom->argument = argument;
        return this->cleanupActionsBottom;
    }

    /// @brief Allocate space for a C++ object onto the arena.
    /// @tparam T The type of the object to allocate.
    /// @param callDestructorOnCleanup Whether the destructor should be called when the arena is cleaned up.
    /// @returns a pointer to the allocated object, or NULL if there is insufficient space.
    template <typename T> inline T* allocObject(bool callDestructorOnCleanup = true) {
        T* result = (T*) this->alloc(sizeof(T));

        if (result == nullptr) {
            return nullptr;
        }

        if (callDestructorOnCleanup) {
            if (!this->scheduleCleanup(&PiggyBankArena::_destroyObject<T>, result)) {
                this->heapTop -= sizeof(T);
                return nullptr;
            }
        }

        return result;
    }

    /// @brief Clean up the given arena by calling all registered cleanup functions and resetting the arena to an empty state. 
    /// @remark After this, the arena can be reused again, as if it was just created.
    /// @param arena A pointer to the arena.
    inline void cleanup() {
        struct _PiggyBankArenaCleanupAction* action = this->cleanupActionsBottom;
        
        while (action < this->end) {
            action->func(action->argument);
            action++;
        }

        this->heapTop = this->start;
        this->cleanupActionsBottom = (struct _PiggyBankArenaCleanupAction*) this->end;
    }

private:
    template <typename T> static inline void _destroyObject(void* obj) {
        if (obj != nullptr) {
            ((T*)obj)->~T();
        }
    }
};

}
