#include "ast.hpp"
#include "type.hpp"
#include "token.hpp"
#include "error.hpp"
#include "codegen.hpp"

#include <set>

using namespace compiler;

static unsigned ret_count;

std::set<ast_func*> func_set{}; // for printing uniqueness

std::string IR::pop() {
    if(stack.empty()) error("IR error: stack empty");
    auto res = stack.back();
    stack.pop_back();
    return res;
}

static std::string make_temp() {
    static unsigned id = 1;
    return 't' + std::to_string(id++);
}

static std::tuple<std::string, std::string, std::string> make_if_id() {
    static unsigned id = 1;
    auto num = std::to_string(id++);
    return {".IF" + num, ".ELSE" + num, ".ENDIF" + num};
}

static std::string make_obj_id(stmt_decl *d) {
    // used for recognizing different variables with same name
    static unsigned id = 1;
    static std::map<stmt_decl*, unsigned> ids{};
    auto it = ids.find(d);
    if(it == ids.end())
        it = ids.emplace(d, id++).first;
    std::string name{};
    auto tok = d->obj->m_tok;
    if(tok) 
        name = tok->to_string();
    else 
        name = "[Anonymous]";
    return name += '_' + std::to_string(it->second);
}

static const char* op_to_string(uint32_t attr) {
    static const std::map<uint32_t, const char*> attr_str {
        {Member, "."},
        {Dereference, "*"},
        {AddressOf, "&"},
        {Subscript, "["},
        {MemberPtr, "->"},
        {PostInc, "++"},
        {PostDec, "--"},
        {ArithmeticOf, "+"}, 
        {Negate, "-"},
//        Cast, // binary
        {Comma, ","},
        {Add, "+"},
        {Sub, "-"},
        {Mul, "*"},
        {Div, "/"},
        {Mod, "%"},
        {BitAnd, "&"}, 
        {BitOr, "|"},
        {BitXor, "^"},
        {BitNot, "~"},
        {LeftShift, "<<"},
        {RightShift, ">>"},
        {LessThan, "<"},
        {LessEqual, "<="},
        {GreaterThan, ">"},
        {GreaterEqual, ">="},
        {Equal, "=="},
        {NotEqual, "!="},
        {LogicalAnd, "&&"},
        {LogicalOr, "||"},
        {LogicalNot, "!"},
        {Inc, "++"},
        {Dec, "--"},
        {Assign, "="},
        {AddAssign, "+="},
        {SubAssign, "-="},
        {MulAssign, "*="},
        {DivAssign, "/="},
        {ModAssign, "%="},
        {BitAndAssign, "&="},
        {BitOrAssign, "|="},
        {BitXorAssign, "^="},
        {LeftShiftAssign, "<<="},
        {RightShiftAssign, ">>="},
    };
    auto it = attr_str.find(attr);
    if(it == attr_str.end())
        error("IR error: unexpected symbol %s", attr_to_string(attr));
    return it->second;
}

void IR::visit_constant(ast_constant *a) {
    auto type = a->m_type->to_arith();
    if(!type) // const char*
        stack.emplace_back('"' + std::string(a->str) + '"');
    else if(type->is_integer())
        stack.emplace_back(std::to_string(a->ival));
    else if(type->is_float())
        stack.emplace_back(std::to_string(a->dval));
}

void IR::visit_object(ast_object *a) {    
    stack.push_back(make_obj_id(a->decl));
}

void IR::visit_enum(ast_enum *a) {
    stack.push_back(std::to_string(a->val.ival));
}

void IR::visit_func(ast_func *a) {
    ret_count = 0;
    
    auto it = func_set.find(a);
    if(it != func_set.end()) return;
    
    file << '\n' << a->m_tok->to_string() << ":\n";
    auto func = a->m_type->to_func();
    
    for(auto &&param: func->params()) {
        file << "\t[PARAM]\t";
        file << make_obj_id(param->decl) << "\t=>\t";
        file << '(' << param->m_type.to_string() << ')' << '\n';
    }
    if(func->is_vaarg())
        file << "\t[VARIADIC PARAMETER]\n";
    auto ret_type = func->return_type();
    file << "\t[RET]\t=>\t(" << ret_type.to_string() << ")\n";
    
    if(a->body) {
        a->body->accept(this);
        
        if(!ret_count) file << "return\n";
    } else 
        file << "[undefined function]\n";
    
    func_set.insert(a);
}

void IR::visit_unary(ast_unary *a) {
    a->operand->accept(this);
    auto str = pop();
    auto op = op_to_string(a->op);
    auto temp = make_temp();
    switch(a->op) {
        case Dereference:
            file << "[DEREF]\t" << a->m_type->size() << '\t' << str << '\t' << temp << '\n';
            break;
        case PostInc: case PostDec:
            file << "[=]\t" << str << "\t\t" << temp << '\n';
            file << '[' << op << "]\t" << str << "\t\t\t\n";
            break;
        default:
            file << '[' << op << "]\t" << str << "\t\t" << temp << '\n';
            break;
    }
    stack.emplace_back(std::move(temp));
}

void IR::visit_cast(ast_cast *a) {
    a->operand->accept(this);
    auto str = pop();
    auto temp = make_temp();
    file << "[CAST]\t" << '(' << a->m_type.to_string() << ')' << '\t';
    file << str << '\t' << temp << '\n';
    stack.emplace_back(std::move(temp));
}

void IR::visit_binary(ast_binary *a) {
    a->lhs->accept(this);
    auto lhs = pop();
    
    if(a->op == Member || a->op == MemberPtr) {
        lhs += op_to_string(a->op);
        lhs += a->rhs->m_tok->to_string(); // member name
        stack.emplace_back(std::move(lhs));
        return;
    } 
    
    a->rhs->accept(this);
    auto rhs = pop();
    
    if(a->op == Subscript) {
        auto addr = make_temp();
        auto deref = make_temp();
        file << "[+]\t" << lhs << '\t' << rhs << '\t' << addr << '\n';
        file << "[DEREF]\t" << a->m_type->size() << '\t' << addr << '\t' << deref << '\n';
        stack.emplace_back(std::move(deref));
    } else if (a->op == Assign) {
        file << "[=]\t" << rhs << "\t\t" << lhs << '\n';
        stack.emplace_back(std::move(lhs));
    } else {
        auto op = op_to_string(a->op);
        auto temp = make_temp();
        file << '[' << op << "]\t" << lhs << "\t" << rhs << "\t" << temp << '\n';
        stack.emplace_back(std::move(temp));
    }
}

void IR::visit_ternary(ast_ternary *a) {
    auto &&tuple = make_if_id();
    a->cond->accept(this);
    a->yes->accept(this);
    a->no->accept(this);
    
    auto n = pop();
    auto y = pop();
    auto c = pop();
    
    auto _if = std::get<0>(tuple);
    auto _else = std::get<1>(tuple);
    auto _endif = std::get<2>(tuple);
    
    auto temp = make_temp();
    
    file << "[IF]\t" << c << "\t[THEN]\t" << _if << '\n';
    file << "[GOTO]\t" << _else << '\n';
    file << _if << ":\n";
    file << "[=]\t" << n << "\t\t" << temp << '\n';
    file << "[GOTO]\t" << _endif << '\n';
    file << _else << ":\n";
    file << "[=]\t" << y << "\t\t" << temp << '\n';
    file << _endif << ":\n";
    
    stack.emplace_back(std::move(temp));
}

void IR::visit_call(ast_call *a) {
    auto name = a->func->m_tok->to_string();
    for(auto &&arg: a->args) 
        arg->accept(this);
    
    bool noret = a->func->m_type->to_func()->return_type()->is_void();
    
    bool nfirst = false;
    file << "[CALL]\t" << name << "{"; 
    for(auto i = a->args.size(); i; --i) {
        if(nfirst) file << ", ";
        file << pop();
        nfirst = true;
    }
    file << "}\t"; 
    if(!noret) {
        auto res = make_temp();
        file << res;
        stack.emplace_back(std::move(res));
    } 
    file << '\n';
}

void IR::visit_stmt(stmt*) {
    // empty statement, do nothing
}

void IR::visit_compound(stmt_compound *a) {
    std::deque<std::string> allocs{};
//    std::swap(allocs, alloc_stack);
    for(auto &&s: a->m_stmt)
        s->accept(this);
//    for(auto rit = alloc_stack.rbegin(); rit != alloc_stack.rend(); ++rit)
//        file << "[FREE]\t" << std::move(*rit) << '\n';
//    std::swap(allocs, alloc_stack);
}

void IR::visit_jump(stmt_jump *a) {
    file << "[GOTO]\t" << ".L" << std::to_string(a->label->id) << '\n';
}

void IR::visit_label(stmt_label *a) {
    file << ".L" << std::to_string(a->id) << ":\n";
}

void IR::visit_return(stmt_return *a) {
    ++ret_count;
    if(a->val) {
        a->val->accept(this);
        auto ret = pop();
        file << "[RET]\t" << ret << '\n';
    } else 
        file << "[RET]\n";
}

void IR::visit_if(stmt_if *a) {
    auto &&tuple = make_if_id();
    a->cond->accept(this);
    auto c = pop();
    
    bool has_else = a->no;
    
    auto _if = std::get<0>(tuple);
    auto _else = std::get<1>(tuple);
    auto _endif = std::get<2>(tuple);
    
    file << "[IF]\t" << c << "\t[THEN]\t" << _if << '\n';
    file << _if << ":\n";
    a->yes->accept(this);
    file << "[GOTO]\t" << _endif << '\n';
    if(has_else) {
        file << _else << ":\n";
        a->no->accept(this);
    }
    file << _endif << ":\n";
}

void IR::visit_expr(stmt_expr *a) {
    if(a->expr) a->expr->accept(this);
}

void IR::visit_decl(stmt_decl *a) {
    auto obj = a->obj->to_obj();
    auto func = a->obj->to_func();
    if(obj) {
        auto name = make_obj_id(obj->decl);
//        file << "[ALLOC]\t" << obj->m_type->size() << '\t' << name << '\n';
//        alloc_stack.emplace_back(std::move(name));
        auto &&inits = obj->decl->inits;
        if(inits.empty()) return;
        if(!obj->m_type->is_aggregate()) {
            inits.front()->accept(this);
            file << "[=]\t" << pop() << "\t\t" << name << '\n';
        }
    } else if(func)
        func->accept(this);
}

