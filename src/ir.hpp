#pragma once

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

namespace miniptx {

// Forward declarations from ast.hpp
class Program;
class KernelDecl;
class ASTNode;
class BlockStmt;

// ═══════════════════════════════════════════════════════════════════
//  IR Types
// ═══════════════════════════════════════════════════════════════════

enum class IRType { INT, FLOAT, INT_PTR, FLOAT_PTR, BOOL };

inline const char* irTypeName(IRType t) {
    switch (t) {
        case IRType::INT:       return "i32";
        case IRType::FLOAT:     return "f32";
        case IRType::INT_PTR:   return "i32*";
        case IRType::FLOAT_PTR: return "f32*";
        case IRType::BOOL:      return "pred";
    }
    return "?";
}

// ═══════════════════════════════════════════════════════════════════
//  Value (SSA virtual register)
// ═══════════════════════════════════════════════════════════════════

struct Value {
    std::string name;    // e.g. "%t0", "%a"
    IRType      type = IRType::INT;
    int         version = 0; // SSA version

    Value() = default;
    Value(std::string n, IRType t, int v = 0)
        : name(std::move(n)), type(t), version(v) {}

    bool empty() const { return name.empty(); }
    std::string ssa() const {
        if (version > 0) return name + "." + std::to_string(version);
        return name;
    }
};

// ═══════════════════════════════════════════════════════════════════
//  Instruction
// ═══════════════════════════════════════════════════════════════════

enum class InstKind {
    ASSIGN,       // %dst = %src
    BINOP,        // %dst = %lhs op %rhs
    ICMP,         // %dst = icmp op %lhs %rhs
    LOAD,         // %dst = load %ptr[%idx]
    STORE,        // store %val -> %ptr[%idx]
    MOV_BUILTIN,  // %dst = threadIdx.x
    BRANCH,       // br %cond %true_label %false_label
    JUMP,         // jmp %label
    LABEL,        // label:
    PHI,          // %dst = phi [%v1, L1] [%v2, L2]
    RET,          // ret
    CONST_INT,    // %dst = const i32 imm
    CONST_FLOAT,  // %dst = const f32 imm
};

struct PhiEntry {
    Value       value;
    std::string label;
};

struct Instruction {
    InstKind kind;

    // Destination (for ASSIGN, BINOP, ICMP, LOAD, MOV_BUILTIN, PHI, CONST_*)
    Value dst;

    // Operands (for BINOP, ICMP, ASSIGN)
    Value lhs, rhs;

    // Operator string: BINOP → "add","sub","mul","div","fadd","fsub","fmul","fdiv"
    //                  ICMP  → "lt","le","gt","ge","eq","ne"
    std::string op;

    // LOAD/STORE
    Value ptr, idx, val;

    // MOV_BUILTIN
    std::string builtin;  // "threadIdx.x", "blockIdx.x", etc.

    // BRANCH
    Value cond;
    std::string true_label;
    std::string false_label;

    // JUMP / LABEL
    std::string label;

    // PHI
    std::vector<PhiEntry> phi_entries;

    // CONST_INT / CONST_FLOAT
    int   imm_int   = 0;
    float imm_float = 0.0f;

    // Source line for debugging
    int line = 0;
    bool dead = false;

    Instruction() : kind(InstKind::RET) {}
    explicit Instruction(InstKind k) : kind(k) {}

    void dump(std::ostream& os) const;
};

// ═══════════════════════════════════════════════════════════════════
//  BasicBlock (node in the CFG)
// ═══════════════════════════════════════════════════════════════════

struct BasicBlock {
    std::string              label;
    std::vector<Instruction> instructions;

    // CFG edges
    BasicBlock* true_succ  = nullptr;
    BasicBlock* false_succ = nullptr;
    std::vector<BasicBlock*> predecessors;

    explicit BasicBlock(std::string lbl) : label(std::move(lbl)) {}

    void addInst(Instruction inst) { instructions.push_back(std::move(inst)); }
    void dump(std::ostream& os) const;
};

// ═══════════════════════════════════════════════════════════════════
//  IRFunction
// ═══════════════════════════════════════════════════════════════════

struct IRFunction {
    std::string                             name;
    std::vector<Value>                      params;
    std::vector<std::unique_ptr<BasicBlock>> blocks;

    BasicBlock* createBlock(const std::string& label);
    BasicBlock* getBlock(const std::string& label) const;

    void dump(std::ostream& os = std::cout) const;
};

// ═══════════════════════════════════════════════════════════════════
//  IRBuilder — lowers AST to IR
// ═══════════════════════════════════════════════════════════════════

class IRBuilder {
public:
    /// Lower a full program to a list of IRFunctions.
    std::vector<IRFunction> lowerProgram(Program& prog);

    /// Lower a single kernel.
    IRFunction lowerKernel(KernelDecl& kd);

private:
    IRFunction* fn_  = nullptr;
    BasicBlock* bb_  = nullptr;
    int tempCounter_ = 0;
    int labelCounter_ = 0;
    int curLine_ = 0;  // current source line for debug info

    // Variable → current SSA value
    std::unordered_map<std::string, Value> varMap_;
    // Variable → SSA version counter
    std::unordered_map<std::string, int> versionMap_;

    // ── Helpers ──
    Value freshTemp(IRType type);
    std::string freshLabel(const std::string& prefix = "L");
    void emit(Instruction inst);
    void setBlock(BasicBlock* blk);
    void linkBlocks(BasicBlock* from, BasicBlock* to, bool isTrueBranch = true);
    IRType typeFromStr(const std::string& t);
    Value nextVersion(const std::string& varName, IRType type);

    // ── AST lowering ──
    void lowerBlock(BlockStmt& blk);
    void lowerNode(ASTNode& node);
    Value lowerExpr(ASTNode& node);
    Value lowerLValue(ASTNode& node);

    // ── PHI insertion ──
    void insertPhiNodes();
    void computeDominanceFrontier(
        std::unordered_map<BasicBlock*, std::unordered_set<BasicBlock*>>& df);
    void computeDominators(
        std::unordered_map<BasicBlock*, BasicBlock*>& idom);
};

} // namespace miniptx
