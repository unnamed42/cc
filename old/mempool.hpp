#ifndef __COMPILER_UTIL_MEMPOOL__
#define __COMPILER_UTIL_MEMPOOL__

#include <cstring>
#include <boost/pool/object_pool.hpp>

namespace compiler {

template <class T> class mempool: public boost::object_pool<T> {
    typedef boost::object_pool<T> base_class;
    public:
        using base_class::base_class;
        // mempool();
        // ~mempool();
        // T* malloc(); 
        // void free(const T*);
};

class sizepool {
    struct poolnode {
        poolnode *prev;
        poolnode *next;
    };
    private:
        poolnode head, tail;
    public:
        sizepool():head{nullptr, &tail}, tail{&head, nullptr} {}
        
        ~sizepool() {
            auto ptr = head.next;
            while(ptr != &tail) {
                auto save = ptr->next;
                std::free(ptr);
                ptr = save;
            }
        }
        
        void* allocate(size_t size) {
            auto mem = reinterpret_cast<poolnode*>(malloc(sizeof(poolnode) + size)),
                 before_tail = tail.prev;
            if(!mem) {
                puts("Interal error: insufficient memory");
                throw std::bad_alloc();
            }
            before_tail->next = mem;
            mem->prev = before_tail;
            mem->next = &tail;
            tail.prev = mem;
            
            return reinterpret_cast<void*>(reinterpret_cast<void**>(mem) + 2);
        }
        
        void deallocate(void *mem) {
            auto next = *(reinterpret_cast<poolnode**>(mem) - 1);
            auto curr = next->prev,
                 prev = curr->prev;
            prev->next = next;
            next->prev = prev;
            std::free(curr);
        }
        
        void* reallocate(void *old, size_t old_size, size_t new_size) {
            auto new_mem = allocate(new_size);
            if(!new_mem) {
                puts("Interal error: insufficient memory");
                throw std::bad_alloc();
            }
            std::memmove(new_mem, old, old_size);
            deallocate(old);
            return new_mem;
        }
};

} // namespace compiler

#endif // __COMPILER_UTIL_MEMPOOL__
