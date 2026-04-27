#pragma once

#include "ir.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <iostream>

namespace miniptx {

/// PTX register class (determines register pool).
enum class RegClass { S32, F32, U64, PRED };

inline const char* regClassName(RegClass rc) {
    switch (rc) {
        case RegClass::S32:  return ".s32";
        case RegClass::F32:  return ".f32";
        case RegClass::U64:  return ".u64";
        case RegClass::PRED: return ".pred";
    }
    return "?";
}

inline RegClass irTypeToRegClass(IRType t) {
    switch (t) {
        case IRType::INT:       return RegClass::S32;
        case IRType::BOOL:      return RegClass::PRED;
        case IRType::FLOAT:     return RegClass::F32;
        case IRType::INT_PTR:
        case IRType::FLOAT_PTR: return RegClass::U64;
    }
    return RegClass::S32;
}

inline const char* regClassPrefix(RegClass rc) {
    switch (rc) {
        case RegClass::S32:  return "%r";
        case RegClass::F32:  return "%f";
        case RegClass::U64:  return "%rd";
        case RegClass::PRED: return "%p";
    }
    return "%r";
}

// ─── Liveness info ─────────────────────────────────────────────────

struct LiveInfo {
    std::unordered_set<std::string> liveIn;
    std::unordered_set<std::string> liveOut;
    std::unordered_set<std::string> def;
    std::unordered_set<std::string> use;
};

// ─── Interference graph node ───────────────────────────────────────

struct IGNode {
    std::string   name;     // SSA name (Value::ssa())
    IRType        type;
    RegClass      rc;
    int           color = -1;       // assigned register number (-1 = uncolored)
    bool          spilled = false;
    int           useCount = 0;     // for spill cost heuristic
    std::unordered_set<std::string> neighbors;
};

// ═══════════════════════════════════════════════════════════════════
//  RegAlloc — Chaitin-Briggs graph-coloring register allocator
// ═══════════════════════════════════════════════════════════════════

class RegAlloc {
public:
    /// Allocate registers for all functions.
    void allocate(std::vector<IRFunction>& fns);

    /// Get the PTX physical register for an SSA value name.
    std::string getPhysReg(const std::string& ssaName) const;

    /// Print the interference graph and coloring result.
    void dump(std::ostream& os = std::cout) const;

    /// Get register declarations for the PTX header.
    struct RegDecl { RegClass rc; int count; };
    std::vector<RegDecl> getRegDecls() const;

private:
    // The final mapping: SSA name → PTX register string
    std::unordered_map<std::string, std::string> colorMap_;

    // Per-class register counts (for declarations)
    int maxS32_ = 0, maxF32_ = 0, maxU64_ = 0, maxPred_ = 0;

    // Interference graph (rebuilt per function)
    std::unordered_map<std::string, IGNode> ig_;

    // Spilled values
    std::unordered_set<std::string> spilledSet_;

    static constexpr int K = 128; // registers per class

    // ── Pipeline ──
    void allocFunction(IRFunction& fn);

    // Step 1: Liveness analysis
    void computeLiveness(IRFunction& fn,
                         std::unordered_map<BasicBlock*, LiveInfo>& liveMap);
    void collectDefUse(BasicBlock* bb, LiveInfo& info);

    // Step 2: Build interference graph
    void buildInterferenceGraph(IRFunction& fn,
                                std::unordered_map<BasicBlock*, LiveInfo>& liveMap);
    void collectAllValues(IRFunction& fn,
                          std::unordered_map<std::string, IRType>& valTypes);

    // Step 3: Chaitin-Briggs coloring
    bool colorGraph();

    // Step 4: Spill insertion
    void insertSpillCode(IRFunction& fn);

    // Step 5: Build final color map
    void buildColorMap();
};

} // namespace miniptx
