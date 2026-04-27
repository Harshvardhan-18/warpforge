#include "regalloc.hpp"
#include <algorithm>
#include <stack>
#include <cassert>
#include <sstream>

namespace miniptx {

// ═══════════════════════════════════════════════════════════════════
//  Public API
// ═══════════════════════════════════════════════════════════════════

void RegAlloc::allocate(std::vector<IRFunction>& fns) {
    colorMap_.clear();
    maxS32_ = maxF32_ = maxU64_ = maxPred_ = 0;
    for (auto& fn : fns) allocFunction(fn);
}

std::string RegAlloc::getPhysReg(const std::string& ssaName) const {
    auto it = colorMap_.find(ssaName);
    return (it != colorMap_.end()) ? it->second : ssaName;
}

std::vector<RegAlloc::RegDecl> RegAlloc::getRegDecls() const {
    std::vector<RegDecl> decls;
    if (maxS32_ > 0)  decls.push_back({RegClass::S32,  maxS32_});
    if (maxF32_ > 0)  decls.push_back({RegClass::F32,  maxF32_});
    if (maxU64_ > 0)  decls.push_back({RegClass::U64,  maxU64_});
    if (maxPred_ > 0) decls.push_back({RegClass::PRED, maxPred_});
    return decls;
}

void RegAlloc::dump(std::ostream& os) const {
    os << "=== Register Allocation ===\n";
    os << "Interference graph (" << ig_.size() << " nodes):\n";
    for (auto& [name, node] : ig_) {
        os << "  " << name << " [" << regClassName(node.rc)
           << ", uses=" << node.useCount;
        if (node.spilled) os << ", SPILLED";
        else if (node.color >= 0) os << ", color=" << node.color;
        os << "] neighbors: {";
        bool first = true;
        for (auto& n : node.neighbors) {
            if (!first) os << ", ";
            os << n; first = false;
        }
        os << "}\n";
    }
    os << "\nColoring:\n";
    for (auto& [ssa, phys] : colorMap_) {
        os << "  " << ssa << " → " << phys << "\n";
    }
    os << "Registers: s32=" << maxS32_ << " f32=" << maxF32_
       << " u64=" << maxU64_ << " pred=" << maxPred_ << "\n";
}

// ═══════════════════════════════════════════════════════════════════
//  Per-function allocation
// ═══════════════════════════════════════════════════════════════════

void RegAlloc::allocFunction(IRFunction& fn) {
    ig_.clear();
    spilledSet_.clear();

    // Iterate: build IG → color → spill → retry
    for (int iter = 0; iter < 10; iter++) {
        ig_.clear();

        // Step 1: Liveness
        std::unordered_map<BasicBlock*, LiveInfo> liveMap;
        computeLiveness(fn, liveMap);

        // Step 2: Build interference graph
        buildInterferenceGraph(fn, liveMap);

        // Step 3: Color
        bool ok = colorGraph();
        if (ok) break;

        // Step 4: Insert spill code for spilled nodes and retry
        insertSpillCode(fn);
    }

    // Step 5: Build final color map
    buildColorMap();
}

// ═══════════════════════════════════════════════════════════════════
//  Step 1: Liveness Analysis (iterative backward dataflow)
// ═══════════════════════════════════════════════════════════════════

void RegAlloc::collectDefUse(BasicBlock* bb, LiveInfo& info) {
    info.def.clear();
    info.use.clear();

    // Walk instructions forward; use before def means "use"
    for (auto& inst : bb->instructions) {
        if (inst.dead) continue;

        // Collect uses (operands read)
        auto useVal = [&](const Value& v) {
            if (!v.empty() && !info.def.count(v.ssa()))
                info.use.insert(v.ssa());
        };

        switch (inst.kind) {
        case InstKind::ASSIGN:     useVal(inst.lhs); break;
        case InstKind::BINOP:      useVal(inst.lhs); useVal(inst.rhs); break;
        case InstKind::ICMP:       useVal(inst.lhs); useVal(inst.rhs); break;
        case InstKind::LOAD:       useVal(inst.ptr); useVal(inst.idx); break;
        case InstKind::STORE:      useVal(inst.val); useVal(inst.ptr); useVal(inst.idx); break;
        case InstKind::MOV_BUILTIN: break;
        case InstKind::BRANCH:     useVal(inst.cond); break;
        case InstKind::RET:        useVal(inst.lhs); break;
        case InstKind::PHI:
            for (auto& e : inst.phi_entries) useVal(e.value);
            break;
        default: break;
        }

        // Collect defs
        if (!inst.dst.empty()) {
            info.def.insert(inst.dst.ssa());
        }
    }
}

void RegAlloc::computeLiveness(IRFunction& fn,
    std::unordered_map<BasicBlock*, LiveInfo>& liveMap)
{
    // Initialize def/use sets
    for (auto& bptr : fn.blocks) {
        auto* bb = bptr.get();
        liveMap[bb] = {};
        collectDefUse(bb, liveMap[bb]);
    }

    // Params are implicitly defined at entry and live throughout
    if (!fn.blocks.empty()) {
        auto* entry = fn.blocks[0].get();
        for (auto& p : fn.params) {
            liveMap[entry].def.insert(p.ssa());
        }
    }

    // Iterative backward dataflow until stable
    bool changed = true;
    while (changed) {
        changed = false;
        // Process blocks in reverse order
        for (int i = (int)fn.blocks.size() - 1; i >= 0; i--) {
            auto* bb = fn.blocks[i].get();
            auto& info = liveMap[bb];

            // liveOut = union of liveIn of successors
            std::unordered_set<std::string> newOut;
            if (bb->true_succ)  for (auto& v : liveMap[bb->true_succ].liveIn)  newOut.insert(v);
            if (bb->false_succ) for (auto& v : liveMap[bb->false_succ].liveIn) newOut.insert(v);

            // liveIn = use ∪ (liveOut - def)
            std::unordered_set<std::string> newIn = info.use;
            for (auto& v : newOut) {
                if (!info.def.count(v)) newIn.insert(v);
            }

            if (newIn != info.liveIn || newOut != info.liveOut) {
                info.liveIn  = std::move(newIn);
                info.liveOut = std::move(newOut);
                changed = true;
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
//  Step 2: Build Interference Graph
// ═══════════════════════════════════════════════════════════════════

void RegAlloc::collectAllValues(IRFunction& fn,
    std::unordered_map<std::string, IRType>& valTypes)
{
    for (auto& p : fn.params) valTypes[p.ssa()] = p.type;
    for (auto& bptr : fn.blocks) {
        for (auto& inst : bptr->instructions) {
            if (inst.dead) continue;
            if (!inst.dst.empty()) valTypes[inst.dst.ssa()] = inst.dst.type;
            auto add = [&](const Value& v) {
                if (!v.empty() && !valTypes.count(v.ssa()))
                    valTypes[v.ssa()] = v.type;
            };
            add(inst.lhs); add(inst.rhs); add(inst.cond);
            add(inst.ptr); add(inst.idx); add(inst.val);
            for (auto& e : inst.phi_entries) add(e.value);
        }
    }
}

void RegAlloc::buildInterferenceGraph(IRFunction& fn,
    std::unordered_map<BasicBlock*, LiveInfo>& liveMap)
{
    // Collect all values and their types
    std::unordered_map<std::string, IRType> valTypes;
    collectAllValues(fn, valTypes);

    // Create IG nodes
    for (auto& [name, type] : valTypes) {
        IGNode node;
        node.name = name;
        node.type = type;
        node.rc = irTypeToRegClass(type);
        node.useCount = 0;
        ig_[name] = node;
    }

    // Count uses for spill cost
    for (auto& bptr : fn.blocks) {
        for (auto& inst : bptr->instructions) {
            if (inst.dead) continue;
            auto cnt = [&](const Value& v) {
                if (!v.empty() && ig_.count(v.ssa())) ig_[v.ssa()].useCount++;
            };
            cnt(inst.lhs); cnt(inst.rhs); cnt(inst.cond);
            cnt(inst.ptr); cnt(inst.idx); cnt(inst.val);
        }
    }

    // Build interference edges: two values interfere if they are
    // simultaneously live at some program point
    for (auto& bptr : fn.blocks) {
        auto* bb = bptr.get();
        auto& info = liveMap[bb];

        // Start with liveOut set, walk backward through instructions
        std::unordered_set<std::string> live = info.liveOut;

        for (int i = (int)bb->instructions.size() - 1; i >= 0; i--) {
            auto& inst = bb->instructions[i];
            if (inst.dead) continue;

            // At a def point: dst interferes with everything else live
            if (!inst.dst.empty()) {
                std::string d = inst.dst.ssa();
                if (ig_.count(d)) {
                    RegClass drc = ig_[d].rc;
                    for (auto& lv : live) {
                        if (lv == d) continue;
                        if (!ig_.count(lv)) continue;
                        // Only interfere within same register class
                        if (ig_[lv].rc != drc) continue;
                        ig_[d].neighbors.insert(lv);
                        ig_[lv].neighbors.insert(d);
                    }
                }
                live.erase(d); // remove def from live
            }

            // Add uses to live set
            auto addUse = [&](const Value& v) {
                if (!v.empty()) live.insert(v.ssa());
            };
            switch (inst.kind) {
            case InstKind::ASSIGN:     addUse(inst.lhs); break;
            case InstKind::BINOP:      addUse(inst.lhs); addUse(inst.rhs); break;
            case InstKind::ICMP:       addUse(inst.lhs); addUse(inst.rhs); break;
            case InstKind::LOAD:       addUse(inst.ptr); addUse(inst.idx); break;
            case InstKind::STORE:      addUse(inst.val); addUse(inst.ptr); addUse(inst.idx); break;
            case InstKind::BRANCH:     addUse(inst.cond); break;
            case InstKind::RET:        addUse(inst.lhs); break;
            case InstKind::PHI:
                for (auto& e : inst.phi_entries) addUse(e.value);
                break;
            default: break;
            }
        }
    }

    // All function parameters interfere with each other (simultaneously live at entry)
    for (size_t i = 0; i < fn.params.size(); i++) {
        std::string pi = fn.params[i].ssa();
        if (!ig_.count(pi)) continue;
        RegClass rci = ig_[pi].rc;
        for (size_t j = i + 1; j < fn.params.size(); j++) {
            std::string pj = fn.params[j].ssa();
            if (!ig_.count(pj)) continue;
            if (ig_[pj].rc != rci) continue;  // same class only
            ig_[pi].neighbors.insert(pj);
            ig_[pj].neighbors.insert(pi);
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
//  Step 3: Chaitin-Briggs Graph Coloring
// ═══════════════════════════════════════════════════════════════════

bool RegAlloc::colorGraph() {
    // Work on a copy of neighbor sets (we'll remove nodes during simplification)
    struct WorkNode {
        std::string name;
        RegClass rc;
        int degree;
        int useCount;
        bool removed = false;
    };

    std::unordered_map<std::string, WorkNode> work;
    for (auto& [name, node] : ig_) {
        work[name] = {name, node.rc, (int)node.neighbors.size(), node.useCount, false};
    }

    // Simplification stack
    std::stack<std::string> stack;
    std::unordered_set<std::string> spills;
    int remaining = (int)work.size();

    while (remaining > 0) {
        bool pushed = false;

        // Try to find a node with degree < K
        for (auto& [name, wn] : work) {
            if (wn.removed) continue;
            // Compute actual degree (non-removed same-class neighbors)
            int deg = 0;
            for (auto& nb : ig_[name].neighbors) {
                if (!work[nb].removed && ig_[nb].rc == wn.rc) deg++;
            }
            if (deg < K) {
                stack.push(name);
                wn.removed = true;
                remaining--;
                pushed = true;
                break;
            }
        }

        if (!pushed) {
            // All nodes have degree >= K — pick spill candidate
            // Spill the node with lowest use count (cheapest to spill)
            std::string victim;
            int minCost = INT32_MAX;
            for (auto& [name, wn] : work) {
                if (wn.removed) continue;
                if (wn.useCount < minCost) {
                    minCost = wn.useCount;
                    victim = name;
                }
            }
            if (victim.empty()) break;

            // Optimistic: push it anyway (Briggs improvement)
            stack.push(victim);
            work[victim].removed = true;
            remaining--;
        }
    }

    // Select: pop and assign colors
    bool allColored = true;
    while (!stack.empty()) {
        std::string name = stack.top(); stack.pop();
        if (!ig_.count(name)) continue;
        auto& node = ig_[name];

        // Find colors used by (non-spilled) neighbors
        std::set<int> usedColors;
        for (auto& nb : node.neighbors) {
            if (ig_.count(nb) && ig_[nb].color >= 0 && ig_[nb].rc == node.rc) {
                usedColors.insert(ig_[nb].color);
            }
        }

        // Assign lowest available color
        int c = 0;
        while (usedColors.count(c)) c++;

        if (c < K) {
            node.color = c;
        } else {
            // Actual spill
            node.spilled = true;
            spilledSet_.insert(name);
            allColored = false;
        }
    }

    return allColored;
}

// ═══════════════════════════════════════════════════════════════════
//  Step 4: Spill Code Insertion
// ═══════════════════════════════════════════════════════════════════

void RegAlloc::insertSpillCode(IRFunction& fn) {
    // For spilled values, insert store after def and load before use
    // (Simplified: PTX has unlimited .local memory)
    // In practice, with K=128 and typical kernels, spilling is rare
    for (auto& bptr : fn.blocks) {
        std::vector<Instruction> newInsts;
        for (auto& inst : bptr->instructions) {
            if (inst.dead) { newInsts.push_back(inst); continue; }

            // If dst is spilled, add a store after
            bool dstSpilled = !inst.dst.empty() && spilledSet_.count(inst.dst.ssa());

            newInsts.push_back(inst);

            if (dstSpilled) {
                Instruction st(InstKind::STORE);
                st.val = inst.dst;
                st.ptr = Value("%spill_" + inst.dst.ssa(), IRType::INT_PTR);
                st.idx = Value();
                newInsts.push_back(st);
            }
        }
        bptr->instructions = std::move(newInsts);
    }
}

// ═══════════════════════════════════════════════════════════════════
//  Step 5: Build Color Map
// ═══════════════════════════════════════════════════════════════════

void RegAlloc::buildColorMap() {
    for (auto& [name, node] : ig_) {
        if (node.spilled) {
            colorMap_[name] = "%spill_" + name;
            continue;
        }
        if (node.color < 0) {
            colorMap_[name] = name; // fallback
            continue;
        }

        std::string phys = regClassPrefix(node.rc) + std::to_string(node.color);
        colorMap_[name] = phys;

        // Track max register numbers for declarations
        switch (node.rc) {
            case RegClass::S32:  maxS32_ = std::max(maxS32_, node.color + 1); break;
            case RegClass::F32:  maxF32_ = std::max(maxF32_, node.color + 1); break;
            case RegClass::U64:  maxU64_ = std::max(maxU64_, node.color + 1); break;
            case RegClass::PRED: maxPred_ = std::max(maxPred_, node.color + 1); break;
        }
    }
}

} // namespace miniptx
