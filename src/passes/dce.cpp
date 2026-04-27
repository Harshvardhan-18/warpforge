#include "dce.hpp"
#include <unordered_set>
namespace miniptx {
void runDCE(std::vector<IRFunction>& fns) {
    for (auto& fn : fns) {
        std::unordered_set<std::string> used;
        // Collect used values
        for (auto& b : fn.blocks) for (auto& i : b->instructions) {
            if (!i.lhs.empty())  used.insert(i.lhs.ssa());
            if (!i.rhs.empty())  used.insert(i.rhs.ssa());
            if (!i.cond.empty()) used.insert(i.cond.ssa());
            if (!i.ptr.empty())  used.insert(i.ptr.ssa());
            if (!i.idx.empty())  used.insert(i.idx.ssa());
            if (!i.val.empty())  used.insert(i.val.ssa());
            for (auto& e : i.phi_entries) used.insert(e.value.ssa());
        }
        // Mark unused dests as dead
        for (auto& b : fn.blocks) for (auto& i : b->instructions) {
            if (i.dst.empty()) continue;
            if (i.kind == InstKind::STORE || i.kind == InstKind::BRANCH ||
                i.kind == InstKind::JUMP || i.kind == InstKind::RET) continue;
            if (!used.count(i.dst.ssa())) i.dead = true;
        }
    }
}
} // namespace miniptx
