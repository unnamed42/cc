#include "utils/mempool.hpp"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace impl = Compiler::Utils;

using namespace Compiler::Utils;

MemPool impl::pool{};

struct MemPool::MemBlock {
    union {
        /**< next free list node */
        MemBlock  *next;
        /**< owner of this free list node */
        PoolBlock *owner;
    };
    uint8_t data[0];
};

struct MemPool::ChunkBlock {
    /**< next chunk */
    ChunkBlock *next;
    /**< chunk data */
    uint8_t data[0];
};

static inline void* align8Pointer(void *ptr) {
    auto &uintptr = reinterpret_cast<uintptr_t&>(ptr);
    if(uintptr & 7) {
        uintptr &= ~7;
        uintptr += 8;
    }
    return ptr;
}

MemPool::MemBlock* MemPool::addChunk(PoolBlock &owner, unsigned size) {
    auto count = block_size / size;
    auto *chunk = static_cast<ChunkBlock*>(malloc(count * sizeof(MemBlock) + block_size));
    // save my life from casting
    union {
        uint8_t   *as_uint8;
        MemBlock *as_mem;
    } first = {chunk->data};
    owner.first = first.as_mem;
    owner.chunk = chunk;
    while(count--) {
        auto *next = reinterpret_cast<MemBlock*>(first.as_uint8 + size + sizeof(MemBlock));
        first.as_mem->next = next;
        first.as_mem = next;
    }
    first.as_mem->next = owner.first;
    chunk->next = owner.chunk;
    return reinterpret_cast<MemBlock*>(chunk->data);
}

MemPool::MemPool() {
    unsigned size = 8;
    
    for(auto &p: chunks) {
        addChunk(p, size);
        size += 8;
    }
}

MemPool::~MemPool() {
    for(auto &&p: chunks) {
        ChunkBlock *ptr = p.chunk, *save;
        while (ptr) {
            save = ptr->next;
            ::free(ptr);
            ptr = save;
        }
    }
}

void* MemPool::allocate(unsigned size) {
    auto index = (size + 7) >> 3;
    assert(size <= sizes);
    auto *block = chunks[index].first;
    
    if(!block)
        block = addChunk(chunks[index], index << 3);
    chunks[index].first = block->next;
    block->owner = chunks + index;
    return block->data;
}

void* MemPool::align8Allocate(unsigned size) {
    auto index = (size >> 3) + ((size & 0x7) != 0);
    assert(size <= sizes);
    auto *first = chunks[index].first;
    static MemBlock dummy;
    dummy.next = first;
    auto *block = &dummy;
    while(block->next) {
        auto *next = block->next;
        if(reinterpret_cast<uintptr_t>(next->data) & 7)
            block = next;
        else {
            block->next = next->next;
            next->owner = chunks + index;
            return next->data;
        }
    }
    return align8Pointer(allocate(size + 8));
}

void* MemPool::reallocate(void *orig, unsigned size) {
    auto *mem = reinterpret_cast<MemBlock*>(*(reinterpret_cast<void**>(orig) - 1));
    auto curr_size = ((mem->owner - chunks) + 1) << 3;
    if(curr_size >= size)
        return orig;
    // data inside `orig` after `free` is not destroyed
    deallocate(orig);
    return memcpy(allocate(size), mem, curr_size);
}

void* MemPool::copy(void *src) {
    auto *mem = reinterpret_cast<MemBlock*>(*(reinterpret_cast<void**>(src) - 1));
    auto size = ((mem->owner - chunks) + 1) << 3;
    return memcpy(allocate(size), src, size);
}

void MemPool::deallocate(void *block) {
    auto *mem = reinterpret_cast<MemBlock*>(*(reinterpret_cast<void**>(block) - 1));
    auto *chunk = mem->owner;
    mem->next = chunk->first;
    chunk->first = mem;
}
