#include "cse.hpp"
#include <unordered_map>
namespace miniptx {
void runCSE(std::vector<IRFunction>& fns) {
    for (auto& fn : fns) {
        for (auto& b : fn.blocks) {
            std::unordered_map<std::string, std::string> exprMap;
            for (auto& inst : b->instructions) {
                if (inst.dead || inst.dst.empty()) continue;
                if (inst.kind != InstKind::BINOP && inst.kind != InstKind::ICMP) continue;
                std::string key = inst.op + ":" + inst.lhs.ssa() + ":" + inst.rhs.ssa();
                auto it = exprMap.find(key);
                if (it != exprMap.end()) {
                    inst.kind = InstKind::ASSIGN;
                    inst.lhs = Value(it->second, inst.dst.type);
                    inst.rhs = {}; inst.op.clear();
                } else {
                    exprMap[key] = inst.dst.ssa();
                }
            }
        }
    }
}
} // namespace miniptx
