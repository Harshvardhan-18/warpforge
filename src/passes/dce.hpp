#pragma once
#include "../ir.hpp"
namespace miniptx {
/// Dead Code Elimination on the new IR.
void runDCE(std::vector<IRFunction>& fns);
} // namespace miniptx
