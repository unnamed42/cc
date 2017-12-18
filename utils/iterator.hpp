#ifndef ITERATOR_HPP
#define ITERATOR_HPP

#include <iterator>

namespace Compiler {
namespace Utils {

class IteratorCore {
    template <class I, class V, class T, class P, class R, class D> 
    friend class IteratorFacade;
    
    template <class It>
    static It& increment(It &i) { i.increment(); return i; }
    
    template <class It>
    static It& decrement(It &i) { i.decrement(); return i; }
    
    template <class It>
    static typename It::pointer get(It &i) { return i.get(); }
    
    template <class It>
    static typename It::difference_type compare(const It &lhs, const It &rhs) { return lhs.compare(rhs); }
    
    template <class It>
    static It& advance(It &it, typename It::difference_type diff) { it.advance(diff); return it; }
};

template <class It, class ValueT, 
          class Tag = std::forward_iterator_tag, 
          class PtrT = ValueT*, class RefT = ValueT&, class DiffT = std::ptrdiff_t>
class IteratorFacade {
    public:
        using value_type        = ValueT;
        using pointer           = PtrT;
        using reference         = RefT;
        using iterator_category = Tag;
        using difference_type   = DiffT;
    public:
        It& operator++()    { return IteratorCore::increment(iterator()); }
        It  operator++(int) { It &i = iterator(), copy{i}; IteratorCore::increment(i); return copy; }

        It& operator--()    { return IteratorCore::decrement(iterator()); }
        It  operator--(int) { It &i = iterator(), copy{i}; IteratorCore::decrement(i); return copy; }

        It& operator+=(difference_type d) { return IteratorCore::advance(iterator(), d);}
        It  operator+(difference_type d)  { It copy = iterator(); return IteratorCore::advance(copy, d); }

        It& operator-=(difference_type d) { return IteratorCore::advance(iterator(), -d); }
        It  operator-(difference_type d)  { It copy = iterator(); return IteratorCore::advance(copy, -d); }

        difference_type operator-(const It &o) const { return IteratorCore::compare(iterator(), o); }

        pointer   operator->() { return IteratorCore::get(iterator()); }
        reference operator*()  { return *IteratorCore::get(iterator()); }

#define DEPLOY_OPERATOR(op) \
    bool operator op(const It &o) const { return IteratorCore::compare(iterator(), o) op 0; }
        DEPLOY_OPERATOR(==)
        DEPLOY_OPERATOR(!=)
        DEPLOY_OPERATOR(<)
        DEPLOY_OPERATOR(>)
        DEPLOY_OPERATOR(<=)
        DEPLOY_OPERATOR(>=)
#undef DEPLOY_OPERATOR
    private:
        It&       iterator()       noexcept { return *static_cast<It*>(this); }
        const It& iterator() const noexcept { return *static_cast<const It*>(this); }
};

} // namespace Utils
} // namespace Compiler

#endif // ITERATOR_HPP
