#pragma once
#include "ir.hpp"
#include "regalloc.hpp"
#include <ostream>
#include <string>

namespace miniptx {

/// PTX Code Generator — takes IRFunctions + RegAlloc map, emits PTX assembly.
class CodeGen {
public:
    explicit CodeGen(const RegAlloc& ra) : ra_(ra) {}

    /// Set the source filename for .file / .loc debug directives.
    void setSourceFile(const std::string& filename) { srcFile_ = filename; }

    /// Emit a complete .ptx file for all functions.
    void generate(const std::vector<IRFunction>& fns, std::ostream& os);

    /// Count total instructions across all functions (excluding dead).
    int countInstructions(const std::vector<IRFunction>& fns) const;

private:
    const RegAlloc& ra_;
    std::string srcFile_;
    int lastLocLine_ = -1;  // avoid duplicate .loc directives

    /// Map an SSA Value to its physical PTX register.
    std::string r(const Value& v) const { return ra_.getPhysReg(v.ssa()); }

    void emitHeader(std::ostream& os);
    void emitFunction(const IRFunction& fn, std::ostream& os);
    void emitParamDecls(const IRFunction& fn, std::ostream& os);
    void emitRegDecls(std::ostream& os);
    void emitParamLoads(const IRFunction& fn, std::ostream& os);
    void emitInst(const Instruction& inst, const IRFunction& fn, std::ostream& os);
    void emitLoc(int line, std::ostream& os);

    static const char* ptxParamType(IRType t);
    static std::string ptxParamName(const IRFunction& fn, size_t idx);
};

} // namespace miniptx
