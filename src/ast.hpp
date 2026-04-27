#pragma once

#include <string>
#include <vector>
#include <memory>
#include <iostream>

namespace miniptx {

// ═══════════════════════════════════════════════════════════════════
//  Base class
// ═══════════════════════════════════════════════════════════════════

class ASTNode {
public:
    int line_ = 0;
    std::string resolved_type_;  // filled by sema

    virtual ~ASTNode() = default;
    virtual void dump(int indent = 0) const = 0;

protected:
    void indent(int n) const {
        for (int i = 0; i < n; ++i) std::cout << "  ";
    }
};

using NodePtr = std::unique_ptr<ASTNode>;

// Forward declaration
class BlockStmt;

// ═══════════════════════════════════════════════════════════════════
//  Expressions
// ═══════════════════════════════════════════════════════════════════

class IntLiteral : public ASTNode {
public:
    int value_;
    IntLiteral(int v, int line) : value_(v) { line_ = line; }
    void dump(int ind = 0) const override {
        indent(ind); std::cout << "IntLiteral: " << value_ << "\n";
    }
};

class FloatLiteral : public ASTNode {
public:
    float value_;
    FloatLiteral(float v, int line) : value_(v) { line_ = line; }
    void dump(int ind = 0) const override {
        indent(ind); std::cout << "FloatLiteral: " << value_ << "\n";
    }
};

class VarExpr : public ASTNode {
public:
    std::string name_;
    VarExpr(std::string name, int line) : name_(std::move(name)) { line_ = line; }
    void dump(int ind = 0) const override {
        indent(ind); std::cout << "VarExpr: " << name_ << "\n";
    }
};

class BuiltinExpr : public ASTNode {
public:
    std::string name_;  // e.g. "threadIdx.x"
    BuiltinExpr(std::string name, int line) : name_(std::move(name)) { line_ = line; }
    void dump(int ind = 0) const override {
        indent(ind); std::cout << "BuiltinExpr: " << name_ << "\n";
    }
};

class BinaryExpr : public ASTNode {
public:
    std::string op_;
    NodePtr left_;
    NodePtr right_;
    BinaryExpr(std::string op, NodePtr l, NodePtr r, int line)
        : op_(std::move(op)), left_(std::move(l)), right_(std::move(r)) { line_ = line; }
    void dump(int ind = 0) const override {
        indent(ind); std::cout << "BinaryExpr: " << op_ << "\n";
        if (left_)  left_->dump(ind + 1);
        if (right_) right_->dump(ind + 1);
    }
};

class UnaryExpr : public ASTNode {
public:
    std::string op_;
    NodePtr operand_;
    UnaryExpr(std::string op, NodePtr operand, int line)
        : op_(std::move(op)), operand_(std::move(operand)) { line_ = line; }
    void dump(int ind = 0) const override {
        indent(ind); std::cout << "UnaryExpr: " << op_ << "\n";
        if (operand_) operand_->dump(ind + 1);
    }
};

class ArrayAccessExpr : public ASTNode {
public:
    NodePtr array_;
    NodePtr index_;
    ArrayAccessExpr(NodePtr arr, NodePtr idx, int line)
        : array_(std::move(arr)), index_(std::move(idx)) { line_ = line; }
    void dump(int ind = 0) const override {
        indent(ind); std::cout << "ArrayAccessExpr\n";
        if (array_) array_->dump(ind + 1);
        if (index_) index_->dump(ind + 1);
    }
};

class AssignExpr : public ASTNode {
public:
    NodePtr lhs_;
    NodePtr rhs_;
    AssignExpr(NodePtr lhs, NodePtr rhs, int line)
        : lhs_(std::move(lhs)), rhs_(std::move(rhs)) { line_ = line; }
    void dump(int ind = 0) const override {
        indent(ind); std::cout << "AssignExpr\n";
        if (lhs_) lhs_->dump(ind + 1);
        if (rhs_) rhs_->dump(ind + 1);
    }
};

class CallExpr : public ASTNode {
public:
    std::string callee_;
    std::vector<NodePtr> args_;
    CallExpr(std::string callee, std::vector<NodePtr> args, int line)
        : callee_(std::move(callee)), args_(std::move(args)) { line_ = line; }
    void dump(int ind = 0) const override {
        indent(ind); std::cout << "CallExpr: " << callee_ << "\n";
        for (auto& a : args_) if (a) a->dump(ind + 1);
    }
};

// ═══════════════════════════════════════════════════════════════════
//  Statements
// ═══════════════════════════════════════════════════════════════════

class BlockStmt : public ASTNode {
public:
    std::vector<NodePtr> stmts_;
    explicit BlockStmt(int line) { line_ = line; }
    BlockStmt(std::vector<NodePtr> stmts, int line)
        : stmts_(std::move(stmts)) { line_ = line; }
    void dump(int ind = 0) const override {
        indent(ind); std::cout << "BlockStmt\n";
        for (auto& s : stmts_) if (s) s->dump(ind + 1);
    }
};

class VarDeclStmt : public ASTNode {
public:
    std::string type_;       // "int", "float", "float*"
    std::string name_;
    NodePtr init_;           // may be null
    VarDeclStmt(std::string type, std::string name, NodePtr init, int line)
        : type_(std::move(type)), name_(std::move(name)), init_(std::move(init))
    { line_ = line; }
    void dump(int ind = 0) const override {
        indent(ind); std::cout << "VarDeclStmt: " << type_ << " " << name_ << "\n";
        if (init_) init_->dump(ind + 1);
    }
};

class ExprStmt : public ASTNode {
public:
    NodePtr expr_;
    ExprStmt(NodePtr expr, int line) : expr_(std::move(expr)) { line_ = line; }
    void dump(int ind = 0) const override {
        indent(ind); std::cout << "ExprStmt\n";
        if (expr_) expr_->dump(ind + 1);
    }
};

class IfStmt : public ASTNode {
public:
    NodePtr cond_;
    std::unique_ptr<BlockStmt> then_block_;
    std::unique_ptr<BlockStmt> else_block_;  // may be null
    IfStmt(NodePtr cond, std::unique_ptr<BlockStmt> tb,
           std::unique_ptr<BlockStmt> eb, int line)
        : cond_(std::move(cond)), then_block_(std::move(tb)),
          else_block_(std::move(eb)) { line_ = line; }
    void dump(int ind = 0) const override {
        indent(ind); std::cout << "IfStmt\n";
        indent(ind + 1); std::cout << "Condition:\n";
        if (cond_) cond_->dump(ind + 2);
        indent(ind + 1); std::cout << "Then:\n";
        if (then_block_) then_block_->dump(ind + 2);
        if (else_block_) {
            indent(ind + 1); std::cout << "Else:\n";
            else_block_->dump(ind + 2);
        }
    }
};

class ForStmt : public ASTNode {
public:
    NodePtr init_;
    NodePtr cond_;
    NodePtr update_;
    std::unique_ptr<BlockStmt> body_;
    ForStmt(NodePtr init, NodePtr cond, NodePtr update,
            std::unique_ptr<BlockStmt> body, int line)
        : init_(std::move(init)), cond_(std::move(cond)),
          update_(std::move(update)), body_(std::move(body)) { line_ = line; }
    void dump(int ind = 0) const override {
        indent(ind); std::cout << "ForStmt\n";
        if (init_)   { indent(ind+1); std::cout << "Init:\n";   init_->dump(ind+2); }
        if (cond_)   { indent(ind+1); std::cout << "Cond:\n";   cond_->dump(ind+2); }
        if (update_) { indent(ind+1); std::cout << "Update:\n"; update_->dump(ind+2); }
        if (body_)   { indent(ind+1); std::cout << "Body:\n";   body_->dump(ind+2); }
    }
};

class ReturnStmt : public ASTNode {
public:
    NodePtr value_;  // may be null (void return)
    ReturnStmt(NodePtr val, int line) : value_(std::move(val)) { line_ = line; }
    void dump(int ind = 0) const override {
        indent(ind); std::cout << "ReturnStmt\n";
        if (value_) value_->dump(ind + 1);
    }
};

// ═══════════════════════════════════════════════════════════════════
//  Top-level declarations
// ═══════════════════════════════════════════════════════════════════

class ParamDecl : public ASTNode {
public:
    std::string type_;       // "int", "float"
    bool is_pointer_;
    std::string name_;
    ParamDecl(std::string type, bool isPtr, std::string name, int line)
        : type_(std::move(type)), is_pointer_(isPtr), name_(std::move(name))
    { line_ = line; }
    std::string fullType() const { return is_pointer_ ? type_ + "*" : type_; }
    void dump(int ind = 0) const override {
        indent(ind); std::cout << "ParamDecl: " << fullType() << " " << name_ << "\n";
    }
};

class KernelDecl : public ASTNode {
public:
    std::string name_;
    std::vector<std::unique_ptr<ParamDecl>> params_;
    std::unique_ptr<BlockStmt> body_;
    KernelDecl(std::string name,
               std::vector<std::unique_ptr<ParamDecl>> params,
               std::unique_ptr<BlockStmt> body, int line)
        : name_(std::move(name)), params_(std::move(params)),
          body_(std::move(body)) { line_ = line; }
    void dump(int ind = 0) const override {
        indent(ind); std::cout << "KernelDecl: " << name_ << "\n";
        for (auto& p : params_) if (p) p->dump(ind + 1);
        if (body_) body_->dump(ind + 1);
    }
};

class Program : public ASTNode {
public:
    std::vector<std::unique_ptr<KernelDecl>> kernels_;
    Program() { line_ = 0; }
    void dump(int ind = 0) const override {
        indent(ind); std::cout << "Program (" << kernels_.size() << " kernel(s))\n";
        for (auto& k : kernels_) if (k) k->dump(ind + 1);
    }
};

} // namespace miniptx
