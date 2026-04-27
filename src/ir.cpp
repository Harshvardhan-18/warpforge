#include "ir.hpp"
#include "ast.hpp"
#include <algorithm>
#include <cassert>
#include <iomanip>
#include <queue>
#include <sstream>

namespace miniptx {

// ═══════════════════════════════════════════════════════════════════
//  Instruction::dump
// ═══════════════════════════════════════════════════════════════════

void Instruction::dump(std::ostream& os) const {
    if (dead) return;
    switch (kind) {
    case InstKind::ASSIGN:
        os << "  " << dst.ssa() << " = " << lhs.ssa() << "\n"; break;
    case InstKind::BINOP:
        os << "  " << dst.ssa() << " = " << op << " " << lhs.ssa() << ", " << rhs.ssa() << "\n"; break;
    case InstKind::ICMP:
        os << "  " << dst.ssa() << " = icmp." << op << " " << lhs.ssa() << ", " << rhs.ssa() << "\n"; break;
    case InstKind::LOAD:
        os << "  " << dst.ssa() << " = load " << ptr.ssa() << "[" << idx.ssa() << "]\n"; break;
    case InstKind::STORE:
        os << "  store " << val.ssa() << " -> " << ptr.ssa() << "[" << idx.ssa() << "]\n"; break;
    case InstKind::MOV_BUILTIN:
        os << "  " << dst.ssa() << " = " << builtin << "\n"; break;
    case InstKind::BRANCH:
        os << "  br " << cond.ssa() << " " << true_label << " " << false_label << "\n"; break;
    case InstKind::JUMP:
        os << "  jmp " << label << "\n"; break;
    case InstKind::LABEL:
        os << label << ":\n"; break;
    case InstKind::PHI:
        os << "  " << dst.ssa() << " = phi";
        for (auto& e : phi_entries) os << " [" << e.value.ssa() << ", " << e.label << "]";
        os << "\n"; break;
    case InstKind::RET:
        os << "  ret\n"; break;
    case InstKind::CONST_INT:
        os << "  " << dst.ssa() << " = const.i32 " << imm_int << "\n"; break;
    case InstKind::CONST_FLOAT:
        os << "  " << dst.ssa() << " = const.f32 " << imm_float << "\n"; break;
    }
}

// ═══════════════════════════════════════════════════════════════════
//  BasicBlock::dump
// ═══════════════════════════════════════════════════════════════════

void BasicBlock::dump(std::ostream& os) const {
    os << label << ":";
    if (!predecessors.empty()) {
        os << "  ; preds:";
        for (auto* p : predecessors) os << " " << p->label;
    }
    os << "\n";
    for (auto& inst : instructions) inst.dump(os);
}

// ═══════════════════════════════════════════════════════════════════
//  IRFunction
// ═══════════════════════════════════════════════════════════════════

BasicBlock* IRFunction::createBlock(const std::string& lbl) {
    blocks.push_back(std::make_unique<BasicBlock>(lbl));
    return blocks.back().get();
}

BasicBlock* IRFunction::getBlock(const std::string& lbl) const {
    for (auto& b : blocks) if (b->label == lbl) return b.get();
    return nullptr;
}

void IRFunction::dump(std::ostream& os) const {
    os << "function @" << name << "(";
    for (size_t i = 0; i < params.size(); i++) {
        if (i) os << ", ";
        os << irTypeName(params[i].type) << " " << params[i].ssa();
    }
    os << ") {\n";
    for (auto& b : blocks) b->dump(os);
    os << "}\n";
}

// ═══════════════════════════════════════════════════════════════════
//  IRBuilder helpers
// ═══════════════════════════════════════════════════════════════════

Value IRBuilder::freshTemp(IRType type) {
    return Value("%t" + std::to_string(tempCounter_++), type);
}

std::string IRBuilder::freshLabel(const std::string& prefix) {
    return prefix + std::to_string(labelCounter_++);
}

void IRBuilder::emit(Instruction inst) {
    inst.line = curLine_;
    bb_->addInst(std::move(inst));
}

void IRBuilder::setBlock(BasicBlock* blk) {
    bb_ = blk;
}

void IRBuilder::linkBlocks(BasicBlock* from, BasicBlock* to, bool isTrueBranch) {
    if (isTrueBranch) from->true_succ = to;
    else              from->false_succ = to;
    // Add predecessor (avoid duplicates)
    auto& preds = to->predecessors;
    if (std::find(preds.begin(), preds.end(), from) == preds.end())
        preds.push_back(from);
}

IRType IRBuilder::typeFromStr(const std::string& t) {
    if (t == "int")    return IRType::INT;
    if (t == "float")  return IRType::FLOAT;
    if (t == "int*")   return IRType::INT_PTR;
    if (t == "float*") return IRType::FLOAT_PTR;
    return IRType::INT;
}

Value IRBuilder::nextVersion(const std::string& varName, IRType type) {
    int v = ++versionMap_[varName];
    Value val("%" + varName, type, v);
    varMap_[varName] = val;
    return val;
}

// ═══════════════════════════════════════════════════════════════════
//  Top-level lowering
// ═══════════════════════════════════════════════════════════════════

std::vector<IRFunction> IRBuilder::lowerProgram(Program& prog) {
    std::vector<IRFunction> fns;
    for (auto& kd : prog.kernels_) {
        fns.push_back(lowerKernel(*kd));
    }
    return fns;
}

IRFunction IRBuilder::lowerKernel(KernelDecl& kd) {
    IRFunction fn;
    fn_ = &fn;
    fn.name = kd.name_;
    tempCounter_ = 0;
    labelCounter_ = 0;
    varMap_.clear();
    versionMap_.clear();

    // Create entry block
    auto* entry = fn.createBlock("entry");
    setBlock(entry);

    // Lower parameters
    for (auto& p : kd.params_) {
        IRType pt = typeFromStr(p->fullType());
        Value pval("%" + p->name_, pt);
        fn.params.push_back(pval);
        varMap_[p->name_] = pval;
    }

    // Lower body
    lowerBlock(*kd.body_);

    // Ensure ret at end
    Instruction ret(InstKind::RET);
    emit(ret);

    // Build CFG edges and insert PHI nodes
    insertPhiNodes();

    fn_ = nullptr;
    return fn;
}

// ═══════════════════════════════════════════════════════════════════
//  Block / Statement lowering
// ═══════════════════════════════════════════════════════════════════

void IRBuilder::lowerBlock(BlockStmt& blk) {
    for (auto& s : blk.stmts_) lowerNode(*s);
}

void IRBuilder::lowerNode(ASTNode& node) {
    if (node.line_ > 0) curLine_ = node.line_;
    if (auto* vd = dynamic_cast<VarDeclStmt*>(&node)) {
        IRType vt = typeFromStr(vd->type_);
        if (vd->init_) {
            Value v = lowerExpr(*vd->init_);
            Value named = nextVersion(vd->name_, vt);
            Instruction assign(InstKind::ASSIGN);
            assign.dst = named;
            assign.lhs = v;
            emit(assign);
        } else {
            Value named = nextVersion(vd->name_, vt);
            Instruction ci(InstKind::CONST_INT);
            ci.dst = named;
            ci.imm_int = 0;
            emit(ci);
        }
        return;
    }

    if (auto* es = dynamic_cast<ExprStmt*>(&node)) {
        if (es->expr_) lowerExpr(*es->expr_);
        return;
    }

    if (auto* is = dynamic_cast<IfStmt*>(&node)) {
        Value cond = lowerExpr(*is->cond_);

        std::string thenL = freshLabel("if.then");
        std::string elseL = freshLabel("if.else");
        std::string endL  = freshLabel("if.end");

        // Branch
        Instruction br(InstKind::BRANCH);
        br.cond = cond;
        br.true_label = thenL;
        br.false_label = is->else_block_ ? elseL : endL;
        emit(br);

        auto* brBlock = bb_;

        // Then block
        auto* thenBB = fn_->createBlock(thenL);
        linkBlocks(brBlock, thenBB, true);
        setBlock(thenBB);
        if (is->then_block_) lowerBlock(*is->then_block_);
        Instruction j1(InstKind::JUMP); j1.label = endL; emit(j1);
        auto* thenEnd = bb_;

        // Else block
        BasicBlock* elseEnd = nullptr;
        if (is->else_block_) {
            auto* elseBB = fn_->createBlock(elseL);
            linkBlocks(brBlock, elseBB, false);
            setBlock(elseBB);
            lowerBlock(*is->else_block_);
            Instruction j2(InstKind::JUMP); j2.label = endL; emit(j2);
            elseEnd = bb_;
        }

        // End (join) block
        auto* endBB = fn_->createBlock(endL);
        linkBlocks(thenEnd, endBB, true);
        if (elseEnd) linkBlocks(elseEnd, endBB, true);
        else linkBlocks(brBlock, endBB, false);
        setBlock(endBB);
        return;
    }

    if (auto* fs = dynamic_cast<ForStmt*>(&node)) {
        // Init
        if (fs->init_) lowerNode(*fs->init_);

        std::string condL = freshLabel("for.cond");
        std::string bodyL = freshLabel("for.body");
        std::string endL  = freshLabel("for.end");

        Instruction jc(InstKind::JUMP); jc.label = condL; emit(jc);
        auto* preCondBB = bb_;

        // Condition block
        auto* condBB = fn_->createBlock(condL);
        linkBlocks(preCondBB, condBB, true);
        setBlock(condBB);
        Value cond = lowerExpr(*fs->cond_);
        Instruction br(InstKind::BRANCH);
        br.cond = cond; br.true_label = bodyL; br.false_label = endL;
        emit(br);

        // Body block
        auto* bodyBB = fn_->createBlock(bodyL);
        linkBlocks(condBB, bodyBB, true);
        setBlock(bodyBB);
        if (fs->body_) lowerBlock(*fs->body_);
        if (fs->update_) lowerExpr(*fs->update_);
        Instruction jb(InstKind::JUMP); jb.label = condL; emit(jb);
        linkBlocks(bb_, condBB, true); // back edge

        // End block
        auto* endBB = fn_->createBlock(endL);
        linkBlocks(condBB, endBB, false);
        setBlock(endBB);
        return;
    }

    if (auto* rs = dynamic_cast<ReturnStmt*>(&node)) {
        Instruction ret(InstKind::RET);
        if (rs->value_) ret.lhs = lowerExpr(*rs->value_);
        emit(ret);
        return;
    }

    if (auto* bs = dynamic_cast<BlockStmt*>(&node)) {
        lowerBlock(*bs);
        return;
    }

    // Fallback: treat as expression
    lowerExpr(node);
}

// ═══════════════════════════════════════════════════════════════════
//  Expression lowering
// ═══════════════════════════════════════════════════════════════════

Value IRBuilder::lowerExpr(ASTNode& node) {
    if (node.line_ > 0) curLine_ = node.line_;
    if (auto* il = dynamic_cast<IntLiteral*>(&node)) {
        Value dst = freshTemp(IRType::INT);
        Instruction ci(InstKind::CONST_INT);
        ci.dst = dst; ci.imm_int = il->value_; emit(ci);
        return dst;
    }
    if (auto* fl = dynamic_cast<FloatLiteral*>(&node)) {
        Value dst = freshTemp(IRType::FLOAT);
        Instruction cf(InstKind::CONST_FLOAT);
        cf.dst = dst; cf.imm_float = fl->value_; emit(cf);
        return dst;
    }
    if (auto* ve = dynamic_cast<VarExpr*>(&node)) {
        auto it = varMap_.find(ve->name_);
        if (it != varMap_.end()) return it->second;
        return Value("%" + ve->name_, IRType::INT);
    }
    if (auto* be = dynamic_cast<BuiltinExpr*>(&node)) {
        Value dst = freshTemp(IRType::INT);
        Instruction mb(InstKind::MOV_BUILTIN);
        mb.dst = dst; mb.builtin = be->name_; emit(mb);
        return dst;
    }
    if (auto* bin = dynamic_cast<BinaryExpr*>(&node)) {
        Value l = lowerExpr(*bin->left_);
        Value r = lowerExpr(*bin->right_);
        bool isFloat = (node.resolved_type_ == "float");

        // Comparison operators → ICMP
        if (bin->op_ == "<" || bin->op_ == "<=" || bin->op_ == ">" ||
            bin->op_ == ">=" || bin->op_ == "==" || bin->op_ == "!=") {
            Value dst = freshTemp(IRType::BOOL);
            Instruction ic(InstKind::ICMP);
            ic.dst = dst; ic.lhs = l; ic.rhs = r;
            if      (bin->op_ == "<")  ic.op = "lt";
            else if (bin->op_ == "<=") ic.op = "le";
            else if (bin->op_ == ">")  ic.op = "gt";
            else if (bin->op_ == ">=") ic.op = "ge";
            else if (bin->op_ == "==") ic.op = "eq";
            else if (bin->op_ == "!=") ic.op = "ne";
            emit(ic);
            return dst;
        }

        // Arithmetic → BINOP
        Value dst = freshTemp(isFloat ? IRType::FLOAT : IRType::INT);
        Instruction bo(InstKind::BINOP);
        bo.dst = dst; bo.lhs = l; bo.rhs = r;
        if      (bin->op_ == "+") bo.op = isFloat ? "fadd" : "add";
        else if (bin->op_ == "-") bo.op = isFloat ? "fsub" : "sub";
        else if (bin->op_ == "*") bo.op = isFloat ? "fmul" : "mul";
        else if (bin->op_ == "/") bo.op = isFloat ? "fdiv" : "div";
        else if (bin->op_ == "&&") bo.op = "and";
        else if (bin->op_ == "||") bo.op = "or";
        else bo.op = bin->op_;
        emit(bo);
        return dst;
    }
    if (auto* un = dynamic_cast<UnaryExpr*>(&node)) {
        Value operand = lowerExpr(*un->operand_);
        Value dst = freshTemp(operand.type);
        Instruction bo(InstKind::BINOP);
        bo.dst = dst; bo.op = "not"; bo.lhs = operand;
        emit(bo);
        return dst;
    }
    if (auto* aa = dynamic_cast<ArrayAccessExpr*>(&node)) {
        Value arr = lowerExpr(*aa->array_);
        Value idx = lowerExpr(*aa->index_);
        bool isFloat = (node.resolved_type_ == "float");
        Value dst = freshTemp(isFloat ? IRType::FLOAT : IRType::INT);
        Instruction ld(InstKind::LOAD);
        ld.dst = dst; ld.ptr = arr; ld.idx = idx;
        emit(ld);
        return dst;
    }
    if (auto* ae = dynamic_cast<AssignExpr*>(&node)) {
        Value rhs = lowerExpr(*ae->rhs_);
        // Array store
        if (auto* lhsArr = dynamic_cast<ArrayAccessExpr*>(ae->lhs_.get())) {
            Value arr = lowerExpr(*lhsArr->array_);
            Value idx = lowerExpr(*lhsArr->index_);
            Instruction st(InstKind::STORE);
            st.val = rhs; st.ptr = arr; st.idx = idx;
            emit(st);
            return rhs;
        }
        // Variable assignment (creates new SSA version)
        if (auto* lhsVar = dynamic_cast<VarExpr*>(ae->lhs_.get())) {
            Value named = nextVersion(lhsVar->name_, rhs.type);
            Instruction assign(InstKind::ASSIGN);
            assign.dst = named; assign.lhs = rhs;
            emit(assign);
            return named;
        }
        return rhs;
    }
    if (auto* ce = dynamic_cast<CallExpr*>(&node)) {
        (void)ce;
        return freshTemp(IRType::INT); // stub
    }
    return Value("%undef", IRType::INT);
}

Value IRBuilder::lowerLValue(ASTNode& node) {
    if (auto* aa = dynamic_cast<ArrayAccessExpr*>(&node)) {
        Value arr = lowerExpr(*aa->array_);
        Value idx = lowerExpr(*aa->index_);
        Value dst = freshTemp(IRType::INT_PTR);
        Instruction bo(InstKind::BINOP);
        bo.dst = dst; bo.op = "add"; bo.lhs = arr; bo.rhs = idx;
        emit(bo);
        return dst;
    }
    return Value("%undef", IRType::INT);
}

// ═══════════════════════════════════════════════════════════════════
//  PHI insertion (dominance frontier algorithm)
// ═══════════════════════════════════════════════════════════════════

void IRBuilder::computeDominators(
    std::unordered_map<BasicBlock*, BasicBlock*>& idom)
{
    if (fn_->blocks.empty()) return;
    auto* entry = fn_->blocks[0].get();

    // Initialize: entry dominates itself
    idom[entry] = entry;

    // Iterative dominator computation (Cooper, Harvey, Kennedy)
    bool changed = true;
    while (changed) {
        changed = false;
        for (size_t i = 1; i < fn_->blocks.size(); i++) {
            auto* b = fn_->blocks[i].get();
            BasicBlock* newIdom = nullptr;

            for (auto* p : b->predecessors) {
                if (idom.find(p) == idom.end()) continue;
                if (!newIdom) { newIdom = p; continue; }

                // Intersect
                auto* f1 = p;
                auto* f2 = newIdom;
                auto idx = [&](BasicBlock* bb) -> int {
                    for (size_t j = 0; j < fn_->blocks.size(); j++)
                        if (fn_->blocks[j].get() == bb) return (int)j;
                    return -1;
                };
                while (f1 != f2) {
                    while (idx(f1) > idx(f2)) f1 = idom[f1];
                    while (idx(f2) > idx(f1)) f2 = idom[f2];
                }
                newIdom = f1;
            }

            if (newIdom && idom[b] != newIdom) {
                idom[b] = newIdom;
                changed = true;
            }
        }
    }
}

void IRBuilder::computeDominanceFrontier(
    std::unordered_map<BasicBlock*, std::unordered_set<BasicBlock*>>& df)
{
    std::unordered_map<BasicBlock*, BasicBlock*> idom;
    computeDominators(idom);

    for (auto& bptr : fn_->blocks) {
        auto* b = bptr.get();
        if (b->predecessors.size() >= 2) {
            for (auto* p : b->predecessors) {
                auto* runner = p;
                while (runner != idom[b]) {
                    df[runner].insert(b);
                    runner = idom[runner];
                }
            }
        }
    }
}

void IRBuilder::insertPhiNodes() {
    // Find variables defined in each block
    std::unordered_map<std::string, std::unordered_set<BasicBlock*>> defBlocks;
    std::unordered_map<std::string, IRType> varTypes;

    for (auto& bptr : fn_->blocks) {
        for (auto& inst : bptr->instructions) {
            if (!inst.dst.empty() && inst.dst.name.size() > 1 &&
                inst.dst.name[0] == '%' && inst.dst.version > 0) {
                std::string varName = inst.dst.name.substr(1);
                defBlocks[varName].insert(bptr.get());
                varTypes[varName] = inst.dst.type;
            }
        }
    }

    // Compute dominance frontier
    std::unordered_map<BasicBlock*, std::unordered_set<BasicBlock*>> df;
    computeDominanceFrontier(df);

    // Place PHI nodes
    for (auto& [varName, defs] : defBlocks) {
        std::queue<BasicBlock*> worklist;
        std::unordered_set<BasicBlock*> hasPhi;
        for (auto* d : defs) worklist.push(d);

        while (!worklist.empty()) {
            auto* b = worklist.front(); worklist.pop();
            for (auto* dfBlock : df[b]) {
                if (hasPhi.count(dfBlock)) continue;
                hasPhi.insert(dfBlock);

                // Create PHI instruction
                Instruction phi(InstKind::PHI);
                phi.dst = Value("%" + varName, varTypes[varName],
                                ++versionMap_[varName]);

                // Add entries from predecessors
                for (auto* pred : dfBlock->predecessors) {
                    PhiEntry entry;
                    entry.label = pred->label;
                    // Use the variable's value reaching from this pred
                    // (simplified: use the variable name with version 0)
                    entry.value = Value("%" + varName, varTypes[varName]);
                    phi.phi_entries.push_back(entry);
                }

                // Insert at beginning of block
                dfBlock->instructions.insert(
                    dfBlock->instructions.begin(), phi);

                if (!defs.count(dfBlock)) worklist.push(dfBlock);
            }
        }
    }
}

} // namespace miniptx
