#ifndef MEMPOOL_HPP
#define MEMPOOL_HPP

#include <cstddef>

#include "common.hpp"

namespace Compiler {
namespace Utils {

/**
 * Custom memory allocator.
 *
 * The size of managed objects should be no more than 64 bytes.
 *
 * All objects requiring reallocation must be TriviallyCopyable.
 *
 * All objects allocated by this pool must be TriviallyDestructible,
 * or all resources requiring destruction are also allocated using MemPool.
 * Otherwise, memory will leak.
 */
class MemPool {
    NO_COPY_MOVE(MemPool);
    public:
        class AlignedTag;
    public:
        static AlignedTag aligned;
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
        
        /**
         * Memory allocated by this class should not get deallocate()-ed or reallocate()-ed
         * @param size requested size
         * @return 8-byte aligned memory address
         */
        void* align8Allocate(unsigned size);
        
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
        inline void deallocate(T *mem) noexcept { deallocate(mem, sizeof(T)); }
        
        /**
         * Free all managed memory resouces.
         */
        void clear() noexcept;
};

extern MemPool pool;

} // namespace Utils
} // namespace Compiler

/**
 * Provide a placement new operator for MemPool
 */
void* operator new(std::size_t size, Compiler::Utils::MemPool &pool);
void* operator new(std::size_t size, Compiler::Utils::MemPool &pool, const Compiler::Utils::MemPool::AlignedTag &);

void operator delete(void *mem, std::size_t size, Compiler::Utils::MemPool &pool) noexcept;

#endif // MEMPOOL_HPP
