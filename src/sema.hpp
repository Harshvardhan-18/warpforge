#pragma once

#include "ast.hpp"
#include <string>
#include <vector>
#include <stdexcept>
#include <memory>
#include <unordered_map>

namespace miniptx {

class SemaError : public std::runtime_error {
public:
    int line;
    SemaError(const std::string& msg, int ln)
        : std::runtime_error(msg), line(ln) {}
};

class Sema {
public:
    void analyse(Program& prog);

private:
    struct Symbol {
        std::string name;
        std::string type;  // "int", "float", "float*", etc.
    };

    std::vector<std::unordered_map<std::string, Symbol>> scopes_;

    void pushScope();
    void popScope();
    void declare(const std::string& name, const std::string& type, int line);
    const Symbol* lookup(const std::string& name) const;

    void analyseKernel(KernelDecl& kd);
    void analyseBlock(BlockStmt& blk);
    void analyseNode(ASTNode& node);
    std::string analyseExpr(ASTNode& node);  // returns resolved type
};

} // namespace miniptx
