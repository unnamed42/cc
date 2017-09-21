#ifndef USTRING_HPP
#define USTRING_HPP

#include "text/uchar.hpp"
#include "utils/vector.hpp"

extern template class Compiler::Utils::Vector<Compiler::Text::UChar>;

namespace Compiler {
namespace Text {

class UString : public Utils::Vector<UChar> {
    private:
        using self = UString;
        using base = Utils::Vector<UChar>;
    public:
        static UString fromUnsigned(unsigned i);
    public:
        using base::base;
        UString(const char*);
        
        self& append(char ch);
        self& append(UChar ch);
        self& append(const char *str);
        self& append(const self &other);
        
        self& operator+=(char);
        self& operator+=(UChar);
        self& operator+=(const self&);
        self& operator+=(const char*);
        self  operator+(char) const;
        self  operator+(UChar) const;
        self  operator+(const self&) const;
        self  operator+(const char*) const;
        
        bool operator==(const self &other) const noexcept;
        bool operator!=(const self &other) const noexcept;
};

} // namespace Text
} // namespace Compiler

#endif // USTRING_HPP
