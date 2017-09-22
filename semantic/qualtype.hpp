#ifndef QUALTYPE_HPP
#define QUALTYPE_HPP

#include <cstdint>

namespace Compiler {

namespace Semantic {

class Type;

class QualType {
    private:
        using self = QualType;
    private:
        uintptr_t m_ptr;
    public:
        QualType() noexcept;
        QualType(Type *type, uint32_t qual = 0) noexcept;
        QualType(const self &other) noexcept;
        
        uint32_t    qual() const noexcept;
        Type*       get()        noexcept;
        const Type* get()  const noexcept;
        
        void setQual(uint32_t) noexcept;
        void addQual(uint32_t) noexcept;
        void setBase(Type*)    noexcept;
        
        Type* operator->() noexcept;
        const Type* operator->() const noexcept;
        
        void reset(Type *type = nullptr, uint32_t qual = 0) noexcept;
        
        bool isNull()     const noexcept;
        bool isConst()    const noexcept;
        bool isVolatile() const noexcept;
        bool isRestrict() const noexcept;
        
        /* C99 6.3.2.1 Lvalues, arrays, and function designators
         * 
         * Except when it is the operand of the sizeof operator or the unary & operator, or is a
         * string literal used to initialize an array, an expression that has type "array of type" is
         * converted to an expression with type "pointer to type" that points to the initial element of
         * the array object and is not an lvalue. If the array object has register storage class, the
         * behavior is undefined.
         *
         * A function designator is an expression that has function type. Except when it is the
         * operand of the sizeof operator or the unary & operator,a function designator with
         * type "function returning type" is converted to an expression that has type "pointer to
         * function returning type".
         */
        self decay() noexcept;
        
        explicit operator bool() const noexcept;
        bool operator==(const self &o) const noexcept;
        bool operator!=(const self &o) const noexcept;
};

extern const QualType QualNull;

} // namespace Semantic
} // namespace Compiler

#endif // QUALTYPE_HPP
