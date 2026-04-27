#include "sema.hpp"
#include <sstream>

namespace miniptx {

void Sema::pushScope() { scopes_.emplace_back(); }
void Sema::popScope()  { scopes_.pop_back(); }

void Sema::declare(const std::string& name, const std::string& type, int line) {
    auto& scope = scopes_.back();
    if (scope.count(name)) {
        throw SemaError("redeclaration of '" + name + "'", line);
    }
    scope[name] = {name, type};
}

const Sema::Symbol* Sema::lookup(const std::string& name) const {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto f = it->find(name);
        if (f != it->end()) return &f->second;
    }
    return nullptr;
}

void Sema::analyse(Program& prog) {
    for (auto& kd : prog.kernels_) analyseKernel(*kd);
}

void Sema::analyseKernel(KernelDecl& kd) {
    pushScope();
    for (auto& p : kd.params_) {
        declare(p->name_, p->fullType(), p->line_);
    }
    analyseBlock(*kd.body_);
    popScope();
}

void Sema::analyseBlock(BlockStmt& blk) {
    pushScope();
    for (auto& s : blk.stmts_) analyseNode(*s);
    popScope();
}

void Sema::analyseNode(ASTNode& node) {
    if (auto* vd = dynamic_cast<VarDeclStmt*>(&node)) {
        if (vd->init_) analyseExpr(*vd->init_);
        declare(vd->name_, vd->type_, vd->line_);
        return;
    }
    if (auto* es = dynamic_cast<ExprStmt*>(&node)) {
        if (es->expr_) analyseExpr(*es->expr_);
        return;
    }
    if (auto* is = dynamic_cast<IfStmt*>(&node)) {
        analyseExpr(*is->cond_);
        if (is->then_block_) analyseBlock(*is->then_block_);
        if (is->else_block_) analyseBlock(*is->else_block_);
        return;
    }
    if (auto* fs = dynamic_cast<ForStmt*>(&node)) {
        pushScope();
        if (fs->init_)   analyseNode(*fs->init_);
        if (fs->cond_)   analyseExpr(*fs->cond_);
        if (fs->update_) analyseExpr(*fs->update_);
        if (fs->body_)   analyseBlock(*fs->body_);
        popScope();
        return;
    }
    if (auto* rs = dynamic_cast<ReturnStmt*>(&node)) {
        if (rs->value_) analyseExpr(*rs->value_);
        return;
    }
    if (auto* bs = dynamic_cast<BlockStmt*>(&node)) {
        analyseBlock(*bs);
        return;
    }
    // Expression used as statement
    analyseExpr(node);
}

std::string Sema::analyseExpr(ASTNode& node) {
    if (auto* il = dynamic_cast<IntLiteral*>(&node)) {
        node.resolved_type_ = "int";
        (void)il;
        return "int";
    }
    if (auto* fl = dynamic_cast<FloatLiteral*>(&node)) {
        node.resolved_type_ = "float";
        (void)fl;
        return "float";
    }
    if (auto* ve = dynamic_cast<VarExpr*>(&node)) {
        auto* sym = lookup(ve->name_);
        if (!sym) throw SemaError("undeclared '" + ve->name_ + "'", ve->line_);
        node.resolved_type_ = sym->type;
        return sym->type;
    }
    if (auto* be = dynamic_cast<BuiltinExpr*>(&node)) {
        node.resolved_type_ = "int";
        (void)be;
        return "int";
    }
    if (auto* bin = dynamic_cast<BinaryExpr*>(&node)) {
        auto lt = analyseExpr(*bin->left_);
        auto rt = analyseExpr(*bin->right_);
        std::string res = (lt == "float" || rt == "float") ? "float" : "int";
        if (bin->op_ == "<" || bin->op_ == "<=" || bin->op_ == ">" ||
            bin->op_ == ">=" || bin->op_ == "==" || bin->op_ == "!=")
            res = "int"; // comparisons return int (pred)
        node.resolved_type_ = res;
        return res;
    }
    if (auto* un = dynamic_cast<UnaryExpr*>(&node)) {
        auto t = analyseExpr(*un->operand_);
        node.resolved_type_ = t;
        return t;
    }
    if (auto* aa = dynamic_cast<ArrayAccessExpr*>(&node)) {
        auto bt = analyseExpr(*aa->array_);
        analyseExpr(*aa->index_);
        // Dereference pointer type: "float*" → "float"
        std::string elem = bt;
        if (!elem.empty() && elem.back() == '*') elem.pop_back();
        node.resolved_type_ = elem;
        return elem;
    }
    if (auto* ae = dynamic_cast<AssignExpr*>(&node)) {
        analyseExpr(*ae->lhs_);
        auto rt = analyseExpr(*ae->rhs_);
        node.resolved_type_ = rt;
        return rt;
    }
    if (auto* ce = dynamic_cast<CallExpr*>(&node)) {
        for (auto& a : ce->args_) analyseExpr(*a);
        node.resolved_type_ = "int";
        return "int";
    }
    return "";
}

} // namespace miniptx
