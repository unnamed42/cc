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

struct ListBlock {
    ListBlock *next;
};

/**
 * StandardLayout and layout-compatible with ListBlock,
 * and both have a next field which are semantically equal.
 * so DoubleLinkedBlock* can be safely casted to ListBlock*,
 * so we do not need a freeList(DoubleLinkedBlock*) version
 */
struct DoubleLinkedBlock {
    DoubleLinkedBlock *next;
    DoubleLinkedBlock *prev;
    uint8_t data[0];
};

struct PoolBlock {
    /**< first node of free list */
    ListBlock *freeList;
    /**< points to first node of chunk list */
    ListBlock *chunk;
};

template <class T>
static inline T* managedMalloc(std::size_t additional = 0) noexcept(false) {
    auto ret = malloc(additional + sizeof(T));
    if(!ret)
        throw std::bad_alloc{};
    return static_cast<T*>(ret);
}

static inline void* managedRealloc(void *src, std::size_t newsize) noexcept(false) {
    auto ret = realloc(src, newsize);
    if(!ret)
        throw std::bad_alloc{};
    return ret;
}

static inline void managedFree(void *mem) noexcept {
    free(mem);
}

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

static inline PoolBlock* chunkAt(void *base, unsigned index) noexcept {
    return static_cast<PoolBlock*>(base) + index;
}

/**
 * Allocate a memory block used as chunk, then attach to a PoolBlock
 * @param owner owner of this chunk
 * @param blockSize size of small block
 * @return first available memory block of this new chunk
 */
static ListBlock* addChunk(PoolBlock *owner, unsigned blockSize) {
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
static void* arbitarySizedBlock(PoolBlock *owner, unsigned blockSize) {
    auto block = managedMalloc<DoubleLinkedBlock>(blockSize);
    
    auto first = reinterpret_cast<DoubleLinkedBlock*>(owner->chunk);
    if(first)
        first->prev = block;
    block->next = first;
    block->prev = nullptr;
    
    owner->chunk = reinterpret_cast<ListBlock*>(block);
    return block->data;
}

MemPool impl::pool{};

class MemPool::AlignedTag {};

MemPool::AlignedTag MemPool::aligned{};

MemPool::MemPool() {
    auto chunks = managedMalloc<PoolBlock>(SIZES * sizeof(PoolBlock));
    for(auto i=0; i<SIZES+1; ++i) {
        chunks[i].chunk = nullptr;
        chunks[i].freeList = nullptr;
    }
}

MemPool::~MemPool() noexcept {
    clear();
    managedFree(m_chunks);
}

void* MemPool::allocate(unsigned size) {
    auto index = indexOf(size);
    auto chunk = chunkAt(m_chunks, index);
    
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
    
    auto chunk = chunkAt(m_chunks, index);
    ListBlock *first = chunk->freeList, *prev = nullptr;
    while(first) {
        if(lowest3bitSet(first)) {
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
    if(indexOf(oldSize) == SIZES) 
        return managedRealloc(current, newSize);
    
    auto ret = memcpy(allocate(newSize), current, oldSize);
    deallocate(current, oldSize);
    return ret;
}

void MemPool::deallocate(void *mem, unsigned size) noexcept {
    if(!mem)
        return;
    auto index = indexOf(size);
    auto chunk = chunkAt(m_chunks, index);
    
    if(index == SIZES) {
        auto block = CONTAINER_OF(DoubleLinkedBlock, data, mem);
        auto prev = block->prev;
        auto next = block->next;
        if(prev)
            prev->next = next;
        if(next)
            next->prev = prev;
        managedFree(block);
        return;
    }
    
    auto block = static_cast<ListBlock*>(mem);
    block->next = chunk->freeList;
    chunk->freeList = block;
}

void MemPool::clear() noexcept {
    auto chunks = static_cast<PoolBlock*>(m_chunks);
    for(auto i=0; i<SIZES; ++i, ++chunks) {
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
