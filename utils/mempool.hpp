#ifndef MEMPOOL_HPP
#define MEMPOOL_HPP

#include "common.hpp"

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
    public:
        /**< available memory space for each chunk */
        static constexpr unsigned block_size = 1 << 12; // 4KB
        /**< kinds of different block size */
        static constexpr unsigned sizes = 5; // 8, 16, 24, 32, 64
    private:
        struct MemBlock;
        struct ChunkBlock;
        struct PoolBlock {
            /**< first node of free list */
            MemBlock   *first;
            /**< points to first node of chunk list */
            ChunkBlock *chunk;
        };
    private:
        PoolBlock chunks[sizes];
    public:
        MemPool();
        
        ~MemPool();
        
        /**
         * @param size requested size
         * @return an allocated memory block
         */
        void* allocate(unsigned size);
        
        /**
         * @tparam T type of allocated object
         * @return an allocated memory block of size at least sizeof(T)
         */
        template <class T>
        inline T* allocate() { return static_cast<T*>(allocate(sizeof(T))); }
        
        /**
         * Memory allocated by this class should not get `free()`ed ,`realloc()`ed or `copy()`ed
         * @param size requested size
         * @return an aligned to 8 bytes memory address
         */
        void* align8Allocate(unsigned size);
        
        /**
         * Extend previously allocated memory to at least `size` bytes.
         * @param orig original address previously `malloc`-ed
         * @param size requested size
         * @return memory after reallocation
         */
        void* reallocate(void *orig, unsigned size);
        
        /**
         * Copy a block of memory. Input source must be `malloc()`ed by a same mempool instance
         * @param src memory source
         * @return copied memory
         */
        void* copy(void *src);
        
        /**
         * Free previously allocated memory block.
         * @param block allocated memory previously returned by `malloc`
         */
        void deallocate(void *block);
    
    private:
        static MemBlock* addChunk(PoolBlock &owner, unsigned size);
};

extern MemPool pool;

} // namespace Utils
} // namespace Compiler

#endif // MEMPOOL_HPP
