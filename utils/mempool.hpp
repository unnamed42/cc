#ifndef MEMPOOL_HPP
#define MEMPOOL_HPP

#include "common.hpp"

#include <new>

namespace Compiler {
namespace Utils {

/**
 * Allocate some bytes of memory.
 *
 * The size of managed objects should be no more than 64 bytes.
 *
 * All objects requiring reallocation must be TriviallyCopyable.
 *
 * All objects allocated by this pool must be TriviallyDestructible,
 * or all resources requiring destruction are also allocated by this class.
 * Otherwise, memory will leak.
 */
class MemPool {
    NO_COPY_MOVE(MemPool);
    private:
        void *m_chunks;
    public:
        MemPool();
        
        ~MemPool() noexcept;
        
        /**
         * @param size requested size
         * @return an allocated memory block
         */
        void* allocate(unsigned size);
        
        template <class T>
        inline T* allocate() { return static_cast<T*>(allocate(sizeof(T))); }
        
        /**
         * Memory allocated by this class should not get deallocate()-ed or reallocate()-ed
         * @param size requested size
         * @return 8-byte aligned memory address
         */
        void* align8Allocate(unsigned size);
        
        template <class T>
        inline T* align8Allocate() { return static_cast<T*>(align8Allocate(sizeof(T))); }
        
        /**
         * Extend previously allocated memory to at least newSize bytes.
         * @param current previously allocate()-ed memory
         * @param oldSize current size of this memory
         * @param newSize expected new size of memory
         * @return memory after reallocation
         */
        void* reallocate(void *current, unsigned oldSize, unsigned newSize);
        
        /**
         * Free previously allocated memory block.
         * @param block allocated memory previously returned by allocate()
         * @param size size which passed to allocate()
         */
        void deallocate(void *block, unsigned size) noexcept;
        
        template <class T>
        inline void deallocate(T *mem) noexcept { deallocate(mem, sizeof(mem)); }
        
        void clear() noexcept;
};

extern MemPool pool;

} // namespace Utils
} // namespace Compiler

#endif // MEMPOOL_HPP
