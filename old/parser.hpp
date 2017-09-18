#ifndef __COMPILER_PARSER__
#define __COMPILER_PARSER__

#include "ast.hpp"
#include "cpp.hpp"
#include "type.hpp"
#include "token.hpp"
#include "scope.hpp"
#include "codegen.hpp"

#include <list>

namespace compiler {

class parser {
    private:
        typedef std::list<std::pair<token*, stmt_jump*>> label_list;
        typedef std::unordered_map<std::string, stmt_label*>  label_map;
    private:
        cpp       m_cpp;
        scope    *m_file;
        scope    *m_curr;
        
        stmt_list m_stmts;
        
        stmt_label *m_break;
        stmt_label *m_continue;
        
        stmt_list  m_tu; // translation unit
        
        ast_func  *m_func; // current being defined function
        label_map  m_lmap;
        label_list m_unresolved;
    private:
        ast_ident* make_identifier(token*);
        ast_ident* get_identifier();
        
        void append_arg(ast_expr*, ast_expr*);
        
        ast_expr* primary_expr();
        ast_expr* postfix_expr();
        ast_expr* assignment_expr();
        arg_list  argument_expr_list();
        ast_expr* expr();
        ast_expr* unary_expr();
        ast_expr* cast_expr();
        // using operator precedence climbing algorithm
        // parses arithmetic, bitwise, comparasion and logical expressions
        ast_expr* binary_expr();
        ast_expr* binary_expr(ast_expr*, unsigned int);
        ast_expr* conditional_expr();
        //ast_expr* constant_expr(); // same as conditional_expr
        
        // handles declaration, returns a list of declaration statement
        void decl(stmt_list&);
        // handles initializer for declaration, called by decl()
        void init_declarators(stmt_list&, uint8_t, qual_type);
        // the real action performer of specifier/qualifier/storage-class list
        // storage-class is optional, turn it off by passing nullptr
        qual_type type_specifier(uint8_t *storage);
        // storage-class, type-specifier, type-qualifier list
        qual_type decl_specifiers(uint8_t &storage);
        // the real action performer of abstract-declarator/declarator
        // returns the name of identifier, if null means no name
        token*    try_declarator(qual_type&);
        // helper function, handles array/function declaration
        qual_type arr_func_declarator(qual_type);
        // handles pointer, nullable
        qual_type pointer(qual_type base);
        // declarator as descriped in standard
        ast_object* declarator(uint8_t stor, qual_type tp);
        ast_object* direct_declarator(uint8_t, qual_type);
        // declarator without name
        qual_type abstract_declarator(qual_type);
        qual_type direct_abstract_declarator(qual_type);
        
        init_list initializer(qual_type);
        void      array_initializer(init_list&, type_array*);
        void      struct_initializer(init_list&, type_struct*);
        // struct/union declaration
        void         struct_decl();
        // a struct-union-specifier does not include its qualifiers
        type_struct* struct_union_specifier();
        // struct declarator has no storage class specifier
        ast_object*  struct_declarator(qual_type);
        scope*       struct_decl_list(member_list&);
        
        type_enum* enum_specifier();
        void       enumerator_list();
        
        qual_type  type_name();
        
        // returns a function type, requires a qual_type as return type
        qual_type  param_type_list(qual_type);
        
        stmt*      statement();
        stmt*      jump_stmt();
        stmt_if*   selection_stmt();
        stmt_compound* label_stmt();
        // parameter is used only when defining a function
        stmt_compound* compound_stmt(qual_type = qual_null);
        
        stmt_compound* for_loop();
        stmt_compound* while_loop();
        stmt_compound* do_while_loop();
        
        stmt_decl* function_definition(token*, qual_type, uint8_t = 0);
        void       translation_unit();
        
    public:
        parser();
        parser(const char*);
        
        void process() {translation_unit();}
        
//        void run() {
//            auto main = m_curr->find("main");
//            if(!main) 
//                error("\"main\" is not declared as function");
//            main->to_func()->body->visit();
//            print_top();
//        }
        
        void print() {
            static IR ir{"/home/h/test.s"};
            for(auto &s:m_tu)
                s->accept(&ir);
        }
        
        parser(const parser&) = delete;
        parser& operator=(const parser&) = delete;
};

} // namespace compiler

#endif // __COMPILER_PARSER__
