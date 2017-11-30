#include "utils/mempool.hpp"

#include <new>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#define MOVE_BYTES(base, offset) \
    reinterpret_cast<decltype(base)>(reinterpret_cast<uint8_t*>(base) + offset)

#define CONTAINER_OF(containerType, memberName, memberAddress) \
    reinterpret_cast<containerType*>(MOVE_BYTES(memberAddress, -offsetof(containerType, memberName)))

namespace impl = Compiler::Utils;

using namespace Compiler::Utils;

static constexpr auto BLOCK_SIZE = 1 << 12; // 4KB
static constexpr auto SIZES = 8; // difference size of memory, 8, 16, 24, 32, 40, 48, 56, 64
static constexpr auto MAX_SIZE = 8 * SIZES;

template <class T>
static inline T* managedMalloc(std::size_t additional = 0) noexcept(false) {
    auto ret = malloc(additional + sizeof(T));
    if(!ret)
        throw std::bad_alloc{};
    return static_cast<T*>(ret);
}

template <class T>
static inline T* managedRealloc(T *src, std::size_t newsize) noexcept(false) {
    auto ret = realloc(src, newsize + sizeof(T));
    if(!ret)
        throw std::bad_alloc{};
    return static_cast<T*>(ret);
}

static inline void managedFree(void *mem) noexcept {
    free(mem);
}

/**
 * Used as node in a singly-linked list that describes a free-list
 * for unallocated blocks whose size is no more than MAX_SIZE in MemPool. 
 * When allocation, pop a node in free-list and node itself is also considered
 * as available memory space. That's why deallocation requires size.
 */
struct ListBlock {
    ListBlock *next;
};

/**
 * Used as node in a doubly-linked list linking across blocks of arbitary size.
 * 
 * Have a ListBlock as first member so DLinkedBlock* can be casted to 
 * ListBlock*, and can share a same deallcation function.
 */
struct DLinkedBlock {
    ListBlock     m_next;
    DLinkedBlock *prev;
    uint8_t       data[0];
    
    inline DLinkedBlock*& next() noexcept {
        return reinterpret_cast<DLinkedBlock*&>(m_next.next);
    }
    
    /**
     * Fix pointer linkage after reallocation.
     */
    inline void fixLinkage() noexcept {
        if(prev)
            prev->next() = this;
        auto next = this->next();
        if(next)
            next->prev = this;
    }
    
    /**
     * Remove this block from a doubly-linked list
     */
    inline void removeThis() noexcept {
        auto next = this->next();
        if(prev)
            prev->next() = next;
        if(next)
            next->prev = prev;
    }
};

/**
 * Controller of MemPool, each has two singly-linked list:
 * - One list maintains free list of fixed size memory block
 * - One list maintains all pre-allocated large chunks
 * When deallocation, just deallcate chunk list.
 */
struct MemPool::PoolBlock {
    /**< first node of free list */
    ListBlock *freeList;
    /**< points to first node of chunk list */
    ListBlock *chunk;
};

class MemPool::AlignedTag {};

/**
 * See if lowest 3 bit of pointer address is not 0
 * @param ptr pointer address
 */
static inline bool lowest3bitSet(void *ptr) noexcept {
    return (reinterpret_cast<uintptr_t>(ptr) & 7);
}

/**
 * Make address 8-byte aligned
 * @param ptr pointer address
 * @return aligned address
 */
static inline void* align8Pointer(void *ptr) noexcept {
    if(lowest3bitSet(ptr)) {
        auto &&uintptr = reinterpret_cast<uintptr_t&>(ptr);
        uintptr &= ~7U;
        uintptr += 8;
    }
    return ptr;
}

/**
 * Calculate size of memory block
 * @param size requested size
 * @return actual size of memory block
 */
static inline unsigned sizeOf(unsigned size) noexcept {
    assert(size != 0);
    return size < MAX_SIZE ? (size + 7) & ~7U : size;
}

/**
 * Calculate index of MemBlock in whole MemPool
 * @param size requested size
 * @return index to chunks array
 */
static inline unsigned indexOf(unsigned size) noexcept {
    assert(size != 0);
    return size < MAX_SIZE ? ((size + 7) >> 3) - 1 : SIZES;
}

/**
 * Allocate a memory block used as chunk, then attach to a PoolBlock
 * @param owner owner of this chunk
 * @param blockSize size of small block
 * @return first available memory block of this new chunk
 */
static ListBlock* addChunk(MemPool::PoolBlock *owner, unsigned blockSize) {
    auto count = BLOCK_SIZE / blockSize;
    auto chunk = managedMalloc<ListBlock>(BLOCK_SIZE);
    
    auto mem = chunk + 1;
    while(count--) {
        auto next = MOVE_BYTES(mem, blockSize);
        mem->next = next;
        mem = next;
    }
    
    chunk->next = owner->chunk;
    owner->chunk = chunk;
    
    mem->next = owner->freeList;
    owner->freeList = chunk + 1;
    
    return chunk + 1;
}

/**
 * Allocate an arbitary size of memory, then attach it to PoolBlock
 * @param owner owner of this memory
 * @param blockSize size of this memory
 * @return available memory of this memory block
 */
static void* arbitarySizedBlock(MemPool::PoolBlock *owner, unsigned blockSize) {
    auto block = managedMalloc<DLinkedBlock>(blockSize);
    
    auto first = reinterpret_cast<DLinkedBlock*>(owner->chunk);
    if(first)
        first->prev = block;
    block->next() = first;
    block->prev = nullptr;
    
    owner->chunk = reinterpret_cast<ListBlock*>(block);
    return block->data;
}

MemPool impl::pool{};

MemPool::AlignedTag MemPool::aligned{};

MemPool::MemPool() {
    auto chunks = managedMalloc<PoolBlock>(SIZES * sizeof(PoolBlock));
    m_chunks = chunks;
    for(auto i=0; i<=SIZES; ++i, ++chunks) {
        chunks->chunk = nullptr;
        chunks->freeList = nullptr;
    }
}

MemPool::~MemPool() noexcept {
    clear();
    managedFree(m_chunks);
}

void* MemPool::allocate(unsigned size) {
    auto index = indexOf(size);
    auto chunk = m_chunks + index;
    
    if(index == SIZES) 
        return arbitarySizedBlock(chunk, size);
    
    auto block = chunk->freeList;
    if(!block) 
        block = addChunk(chunk, sizeOf(size));
    
    chunk->freeList = block->next;
    return block;
}

void* MemPool::align8Allocate(unsigned size) {
    auto index = indexOf(size);
    assert(index < SIZES);
    
    auto chunk = m_chunks + index;
    ListBlock *first = chunk->freeList, *prev = nullptr;
    while(first) {
        if(!lowest3bitSet(first)) {
            if(!prev)
                chunk->freeList = first->next;
            else
                prev->next = first->next;
            return first;
        }
        prev = first;
        first = first->next;
    }
    
    return align8Pointer(allocate(size + 8));
}

void* MemPool::reallocate(void *current, unsigned oldSize, unsigned newSize) {
    if(oldSize >= newSize || sizeOf(oldSize) == sizeOf(newSize))
        return current;
    if(indexOf(oldSize) == SIZES) {
        auto block = CONTAINER_OF(DLinkedBlock, data, current);
        auto newblock = managedRealloc(block, newSize);
        newblock->fixLinkage();
        return newblock->data;
    }
    auto ret = memcpy(allocate(newSize), current, oldSize);
    deallocate(current, oldSize);
    return ret;
}

void MemPool::deallocate(void *mem, unsigned size) noexcept {
    if(!mem)
        return;
    auto index = indexOf(size);
    auto chunk = m_chunks + index;
    
    if(index == SIZES) {
        auto block = CONTAINER_OF(DLinkedBlock, data, mem);
        block->removeThis();
        managedFree(block);
        return;
    }
    
    auto block = static_cast<ListBlock*>(mem);
    block->next = chunk->freeList;
    chunk->freeList = block;
}

void MemPool::clear() noexcept {
    auto chunks = m_chunks;
    for(auto i=0; i<=SIZES; ++i, ++chunks) {
        auto head = chunks->chunk;
        while(head) {
            auto save = head->next;
            managedFree(head);
            head = save;
        }
        chunks->chunk = nullptr;
        chunks->freeList = nullptr;
    }
}

void* operator new(std::size_t size, MemPool &pool) {
    return pool.allocate(size);
}

void* operator new(std::size_t size, MemPool &pool, const MemPool::AlignedTag &) {
    return pool.align8Allocate(size);
}

void operator delete(void* mem, std::size_t size, Compiler::Utils::MemPool& pool) noexcept {
    pool.deallocate(mem, size);
}

void* operator new[](std::size_t size, MemPool &pool) {
    return pool.allocate(size);
}

void operator delete[](void *mem, std::size_t size, MemPool &pool) noexcept {
    pool.deallocate(mem, size);
}
