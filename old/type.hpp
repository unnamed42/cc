#ifndef __COMPILER_TYPE__
#define __COMPILER_TYPE__

#include <list>
#include <string>
#include <cassert>
#include <cstdint>

namespace compiler {

/*
 * Each unqualified type has several qualified versions of its type,
 * corresponding to the combinations of one, two, or all
 * three of the const, volatile, and restrict qualifiers. The qualified or unqualified
 * versions of a type are distinct types that belong to the same type category and have the
 * same representation and alignment requirements.
 */
enum type_mask: uint32_t {
    Void = 0x01,
    Bool = Void << 1,
    Char = Bool << 1,
    Short = Char << 1,
    Int = Short << 1,
    Long = Int << 1,
    LLong = Long << 1,
    Float = LLong << 1,
    Double = Float << 1,
    // long double  == Long | Double
    Complex = Double << 1,
    Unsigned = Complex << 1,
    Signed   = Unsigned << 1,
    
    Const = 1,
    Volatile = Const << 1,
    Restrict = Volatile << 1, // pointer only
    
    Base = 
        Void | Bool | Char | Short | Int | Long | LLong | 
        Float | Double | Complex | Signed | Unsigned,
    Qual = 
        Const | Volatile | Restrict,
    Sign = 
        Signed | Unsigned,
    Integer = 
        Bool | Char | Short | Int | Long | LLong | Signed | Unsigned,
    Floating = // better name?
        Float | Double,
};

enum storage_class: uint8_t {
    Typedef = 1,
    Static = Typedef << 1,
    Inline = Static << 1,
    Register = Inline << 1,
    Extern = Register << 1,
    //Auto, // TODO: reversed keyword used for type deduction
};

class scope;

struct ast_ident;
struct ast_object;
struct stmt_decl;

class qual_type;

#define ITERATE_TYPES(func) \
    func(type);\
    func(type_void);\
    func(type_arith);\
    func(type_derived);\
    func(type_pointer);\
    func(type_array);\
    func(type_struct);\
    func(type_enum);\
    func(type_func)

class type;
class type_void;
class type_arith;
class type_derived;
class type_pointer;
class type_array;
class type_struct;
class type_enum;
class type_func;

typedef std::list<ast_object*> param_list;
typedef std::list<ast_object*> member_list;

// as long as the size of underlying object is greater than 8 bytes,
// the lower 3 bits of its memory address is always 0
// so it's safe to store extra information in pointer

// class `qual_type` acts like smart pointer of class `type`
class qual_type {
    private:
        uintptr_t m_ptr;
    public:
        constexpr qual_type()
            :m_ptr(0) {}
        explicit qual_type(type *base, uint8_t qual = 0)
            :m_ptr(reinterpret_cast<uintptr_t>(base) | qual) {
            assert(!(reinterpret_cast<uintptr_t>(base) & Qual));
        }
        
        uint8_t qual() const {return m_ptr & Qual;}
        uintptr_t ptr() const {return m_ptr;}
        
        type* get() {return reinterpret_cast<type*>(m_ptr & ~Qual);}
        const type* get() const {return reinterpret_cast<const type*>(m_ptr & ~Qual);}
        
        void reset(type *base, uint8_t qual = 0) {m_ptr = reinterpret_cast<uintptr_t>(base) | qual;}
        
        bool is_null() const {return !(m_ptr & ~Qual);}
        
        bool is_const() const {return m_ptr & Const;}
        bool is_volatile() const {return m_ptr & Volatile;}
        bool is_restrict() const {return m_ptr & Restrict;}
        
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
        qual_type decay() const;
        
        void set_qual(uint8_t qual) {m_ptr &= ~Qual; m_ptr |= qual;}
        void add_qual(uint8_t qual) {m_ptr |= qual;}
        
        void set_base(type *tp) {
            m_ptr = reinterpret_cast<uintptr_t>(tp) | qual();
        }
        
        std::string to_string() const;
        
        qual_type copy() const;
        
        explicit operator bool() const {return get();}
        
        type& operator*() {return *get();}
        const type& operator*() const {return *get();}
        type* operator->() {return get();}
        const type* operator->() const {return get();}
        
        bool operator==(const qual_type &o) const {return m_ptr == o.m_ptr;}
        bool operator!=(const qual_type &o) const {return !operator==(o);}
};

extern const qual_type qual_null;

/* C99 6.2.5 Types
 * 
 * Types are partitioned into 
 *   object types (types that fully describe objects), 
 *   function types (types that describe functions), 
 *   and incomplete types (types that describe objects 
 *     but lack information needed to determine their sizes).

 * Arithmetic types and pointer types are collectively called **scalar types**. Array and
 * structure types are collectively called **aggregate types**.
 */
class type {
    protected:
        enum tag_t: uint8_t {
            VOID = 1,
            ARITH = VOID << 1, // arithmetic
            POINTER = ARITH << 1,
            ARRAY = POINTER << 1,
            STRUCT = ARRAY << 1,
            UNION = STRUCT << 1,
            ENUM = UNION << 1,
            FUNC = ENUM << 1,
        };
    private:
        const uint8_t m_tag;
    protected:
        type(uint8_t t): m_tag(t) {}
    public:
        virtual ~type() = default;
        
        bool is_void() const {return m_tag & VOID;}
        bool is_arith() const {return m_tag & ARITH;}
        bool is_pointer() const {return m_tag & POINTER;}
        bool is_array() const {return m_tag & ARRAY;}
        bool is_struct() const {return m_tag & STRUCT;}
        bool is_union() const {return m_tag & UNION;}
        bool is_enum() const {return m_tag & ENUM;}
        bool is_func() const {return m_tag & FUNC;}
        
        bool is_scalar() const {return m_tag & (ARITH | POINTER);}
        bool is_aggregate() const {return m_tag & (ARRAY|STRUCT);}
        
        virtual bool is_complete() const {return false;}
        
        /* C99 6.2.7 Compatible type and composite type
         *
         * Two types have compatible type if their types are the **same**. Additional rules for
         * determining whether two types are compatible are described in 6.7.2 for type specifiers,
         * in 6.7.3 for type qualifiers, and in 6.7.5 for declarators. 
         * 
         * ...
         * 
         * All declarations that refer to the same object or function shall have compatible type;
         * otherwise, the behavior is undefined.
         */
        virtual bool compatible(const type &t) const {return this == &t;}
        
        bool compatible(const qual_type &q) const {
            auto tp = q.get();
            return tp && this->compatible(*tp);
        }
        
        // avoid dynamic_cast and reinterpret_cast
        virtual type_derived*       to_derived() {return nullptr;}
        virtual const type_derived* to_derived() const {return nullptr;}
        virtual const type_void*    to_void() const {return nullptr;}
        virtual const type_arith*   to_arith() const {return nullptr;}
        virtual type_pointer*       to_pointer() {return nullptr;}
        virtual const type_pointer* to_pointer() const {return nullptr;}
        virtual type_array*         to_array() {return nullptr;}
        virtual const type_array*   to_array() const {return nullptr;}
        virtual type_struct*        to_struct() {return nullptr;}
        virtual const type_struct*  to_struct() const {return nullptr;}
        virtual type_struct*        to_union() {return nullptr;}
        virtual const type_struct*  to_union() const {return nullptr;}
        virtual type_enum*          to_enum() {return nullptr;}
        virtual const type_enum*    to_enum() const {return nullptr;}
        virtual type_func*          to_func() {return nullptr;}
        virtual const type_func*    to_func() const {return nullptr;}
        
        virtual unsigned int size() const {return 0;}
        virtual unsigned int align() const {return 0;}
        
        virtual std::string to_string() const {return "";}
        
        virtual type* copy() const {return const_cast<type*>(this);}
};

/* C99 6.2.5 Types
 * 
 * The void type comprises an empty set of values; it is an incomplete type 
 * that cannot be completed.
 */
class type_void: public type {
    public:
        type_void(): type(VOID) {}
        
        const type_void* to_void() const override {return this;}
        
        std::string to_string() const override {return "void";}
};

/* C99 6.2.5 Types
 * 
 * There are five standard signed integer types, 
 * designated as `signed char`, `short int`, `int`, `long int`, and `long long int`.
 * The standard and extended signed integer types are collectively called `signed integer types`
 *
 * For each of the signed integer types, there is a corresponding (but different)
 * unsigned integer type (designated with the keyword unsigned).
 * The standard and extended unsigned integer types are collectively called `unsigned integer types`.
 *
 * The standard signed integer types and standard unsigned integer types are collectively
 * called the standard integer types, the extended signed integer types and extended
 * unsigned integer types are collectively called the extended integer types.
 *
 * For any two integer types with the same signedness and different integer conversion rank
 * (see 6.3.1.1), the range of values of the type with **smaller** integer conversion rank is a
 * subrange of the values of the other type.
 *
 * There are three real floating types, designated as float, double, and long double.
 * The set of values of the type float is a subset of the set of values of the
 * type double; the set of values of the type double is a subset of the set of values of the
 * type long double.
 *
 * The type char, the signed and unsigned integer types, and the floating types are
 * collectively called the basic types.
 *
 * The type char, the signed and unsigned integer types, and the enumerated types are
 * collectively called **integer types**. The integer and real floating types are collectively called
 * **real types**.
 *
 * Integer and floating types are collectively called **arithmetic types**.
 */
class type_arith: public type {
    private:
        const uint32_t m_type;
    public:
        type_arith(uint32_t tp)
            :type(ARITH), m_type(tp) {}
        
        unsigned int size() const override;
        unsigned int align() const override;
        
        bool is_complete() const override {return true;}
        
        bool is_bool() const {return m_type & Bool;}
        bool is_char() const {return m_type & Char;}
        bool is_integer() const {return m_type & Integer;}
        bool is_float() const {return m_type & Floating;}
        bool is_unsigned() const {return m_type & Unsigned;}
        bool is_signed() const {return !is_unsigned();}
        
        const type_arith* to_arith() const override {return this;}
        
        std::string to_string() const override;
        
        uint32_t rank() const;
        
        type_arith* promote() const;
};

/* C99 6.2.5 Types
 * 
 * Any number of derived types can be constructed from the object, function, and
 * incomplete types, as follows:
 *
 * — An array type describes a contiguously allocated nonempty set of objects with a
 *   particular member object type, called the element type.
 * 
 *   Array types are characterized by their element type and by the 
 *   number of elements in the array. An array type is said to be derived from its element type, 
 *   and if its element type is T, the array type is sometimes called "array of T". 
 * 
 *   The construction of an array type from an element type is called "array type derivation".
 *
 * — A structure type describes a sequentially allocated nonempty set of member objects
 *   (and, in certain circumstances, an incomplete array), each of which has an optionally
 *   specified name and possibly distinct type.
 *
 * — A union type describes an overlapping nonempty set of member objects, each of
 *   which has an optionally specified name and possibly distinct type.
 *
 * — A function type describes a function with specified return type. A function type is
 *   characterized by its return type and the number and types of its parameters. 
 *   
 *   A function type is said to be derived from its return type, and if its return type is T, the
 *   function type is sometimes called "function returning T". The construction of a
 *   function type from a return type is called "function type derivation".
 *
 * — A pointer type may be derived from a function type, an object type, or an incomplete
 *   type, called the referenced type. A pointer type describes an object whose value
 *   provides a reference to an entity of the referenced type. 
 * 
 *   A pointer type derived from the referenced type T is sometimes called "pointer to T".
 *   The construction of a pointer type from a referenced type is called "pointer type derivation".
 *
 * These methods of constructing derived types can be applied recursively.
 *
 * Array, function, and pointer types are collectively called derived declarator types. A
 * declarator type derivation from a type T is the construction of a derived declarator type
 * from T by the application of an array-type, a function-type, or a pointer-type derivation to T.
 *
 * A type is characterized by its type category, which is either the outermost derivation of a
 * derived type (as noted above in the construction of derived types), or the type itself if the
 * type consists of no derived types.
 */
class type_derived: public type {
    protected:
        qual_type m_base;
    protected:
        type_derived(uint8_t tag, qual_type base)
            :type(tag), m_base(base) {}
    public:
        type_derived* to_derived() override {return this;}
        const type_derived* to_derived() const override {return this;}
        
        unsigned int size() const override {return m_base ? m_base->size() : 0;}
        unsigned int align() const override {return m_base ? m_base->align() : 0;}
        
        qual_type get() const {return m_base;}
        
        void set_base(qual_type b) {m_base = b;}
};

class type_array: public type_derived {
    private:
        unsigned int m_len;
    public:
        type_array(qual_type base, unsigned int l = 0)
            :type_derived(ARRAY, base), m_len(l) {}
        
        bool compatible(const type &t) const override {
            auto ptr = t.to_array();
            return ptr && m_len == ptr->m_len && m_base->compatible(*ptr);
        }
        
        // copy an incomplete array type, it may be completed by initializer
        type_array* copy() const override;
        
        bool is_complete() const override {return m_len != 0;}
        
        unsigned int size() const override {return m_len * m_base->size();}
        
        type_array*       to_array() override {return this;}
        const type_array* to_array() const override {return this;}
        
        std::string to_string() const override;
        
        void set_len(unsigned int l) {m_len = l;}
        
        unsigned int length() const {return m_len;}
};

class type_pointer: public type_derived {
    public:
        explicit type_pointer(qual_type base)
            :type_derived(POINTER, base) {}
        type_pointer(type *tp, uint8_t qual = 0)
            :type_derived(POINTER, qual_type(tp, qual)) {}
        
        bool compatible(const type &t) const override {
            auto ptr = t.to_pointer();
            return ptr && m_base->compatible(ptr->m_base);
        }
        
        bool is_complete() const override {return true;}
        
        unsigned int size() const override;
        unsigned int align() const override;
        
        type_pointer* to_pointer() override {return this;}
        const type_pointer* to_pointer() const override {return this;}
        
        std::string to_string() const override {return m_base.to_string() + '*';}
        
        bool is_voidptr() const {
            auto tp = m_base.get();
            while(tp && tp->is_pointer()) tp = tp->to_pointer();
            return tp->is_void();
        }
};

class type_struct: public type {
    private:
        scope      *m_scope;
        member_list m_members;
        
        uint32_t    m_align;
    public:
        explicit type_struct(scope *s) 
            :type(STRUCT), m_scope(s), m_members() {}
        type_struct(scope *s, member_list &&m)
            :type(STRUCT), m_scope(s), m_members(std::move(m)) {}
        
        bool compatible(const type &t) const override;
        
        bool is_complete() const override {return m_scope && !m_members.empty();}
        
        unsigned int size() const override;
        unsigned int align() const override {return m_align;}
        
        type_struct* to_struct() override {return this;}
        const type_struct* to_struct() const override {return this;}
        
        std::string to_string() const override {return "struct:" + std::to_string(size());}
        
        // TODO: update information
        void set_scope(scope *s) {m_scope = s;}
        void set_members(member_list &&m) {m_members = std::move(m); /*m_align = size()*/;}
        
        scope* get_scope() {return m_scope;}
        member_list& get_members() {return m_members;}
};

// not going to implement
//class type_union: public type {};

class type_enum: public type {
    private:
        bool complete;
    public:
        type_enum()
            :type(ENUM), complete(false) {}
        
        bool is_complete() const override {return complete;}
        
        unsigned int size() const override;
        unsigned int align() const override;
        
        type_enum* to_enum() override {return this;}
        const type_enum* to_enum() const override {return this;}
        
        void set_complete(bool b) {complete = b;}
};

class type_func: public type_derived {
    private:
        param_list m_params; // parameter list
        bool       variadic; // is variadic parameter function
        bool       unspec;   // is param_list unspecified
    public:
        type_func(qual_type ret, param_list &&par, bool v = false, bool c = true)
            :type_derived(FUNC, ret), m_params(std::move(par)), variadic(v), unspec(c) {}
        
        bool compatible(const type&) const override;
        
        unsigned int size() const override;
        
        type_func* to_func() override {return this;}
        const type_func* to_func() const override {return this;}
        
        std::string to_string() const override;
        
        void set_return(qual_type r) {m_base = r;}
        
        qual_type return_type() const {return m_base;}
        param_list& params() {return m_params;}
        bool is_vaarg() const {return variadic;}
};

uint32_t attr_to_spec(uint32_t);

uint32_t calc_padding(uint32_t offset, uint32_t align);
uint32_t padded_offset(uint32_t offset, uint32_t align);

// rhs is expected to have only single specifier/qualifier
uint8_t apply_qual(uint8_t lhs, uint32_t rhs);
uint8_t apply_storage(uint8_t lhs, uint32_t rhs);
uint32_t apply_spec(uint32_t lhs, uint32_t rhs);

qual_type make_qual(type*, uint8_t = 0);

qual_type make_void(); // qualified void is meaningless

type_arith* make_arith(uint32_t);
qual_type   qual_arith(uint32_t tp, uint8_t qual = 0);

qual_type max_type(type_arith*, type_arith*);

// qualifier applied to array is always applied to its derived type // wtf?
qual_type make_array(qual_type, unsigned int = 0);

type_pointer* make_pointer(type *base, uint8_t base_qual = 0);
qual_type     qual_pointer(qual_type base, uint8_t self_qual = 0);

type_struct* make_struct(scope* = nullptr);
type_struct* make_struct(scope*, member_list&&);

type_enum* make_enum();

qual_type make_func(qual_type, param_list&&, bool = false, bool = true);


#define STATIC_ASSERT(type) static_assert(!(sizeof(type) % 8), "")
ITERATE_TYPES(STATIC_ASSERT);
#undef STATIC_ASSERT
} // namespace compiler

#endif // __COMPILER_TYPE__
