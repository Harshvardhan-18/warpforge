#include "const_fold.hpp"
#include <unordered_map>
namespace miniptx {
void runConstFold(std::vector<IRFunction>& fns) {
    for (auto& fn : fns) {
        struct CV { bool isFloat; int i; float f; };
        std::unordered_map<std::string, CV> consts;
        for (auto& b : fn.blocks) for (auto& inst : b->instructions) {
            if (inst.dead) continue;
            if (inst.kind == InstKind::CONST_INT) {
                consts[inst.dst.ssa()] = {false, inst.imm_int, 0}; continue;
            }
            if (inst.kind == InstKind::CONST_FLOAT) {
                consts[inst.dst.ssa()] = {true, 0, inst.imm_float}; continue;
            }
            if (inst.kind == InstKind::BINOP) {
                auto a = consts.find(inst.lhs.ssa());
                auto bb = consts.find(inst.rhs.ssa());
                if (a != consts.end() && bb != consts.end() && !a->second.isFloat && !bb->second.isFloat) {
                    int r = 0; bool ok = true;
                    if      (inst.op=="add") r = a->second.i + bb->second.i;
                    else if (inst.op=="sub") r = a->second.i - bb->second.i;
                    else if (inst.op=="mul") r = a->second.i * bb->second.i;
                    else if (inst.op=="div" && bb->second.i!=0) r = a->second.i / bb->second.i;
                    else ok = false;
                    if (ok) {
                        inst.kind = InstKind::CONST_INT; inst.imm_int = r;
                        inst.lhs = {}; inst.rhs = {}; inst.op.clear();
                        consts[inst.dst.ssa()] = {false, r, 0};
                    }
                }
            }
        }
    }
}
} // namespace miniptx
