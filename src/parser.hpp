#pragma once

#include "ast.hpp"
#include "token.hpp"
#include <vector>
#include <string>
#include <stdexcept>

namespace miniptx {

/// Parser error with source location.
class ParseError : public std::runtime_error {
public:
    int line, col;
    ParseError(const std::string& msg, int ln, int c)
        : std::runtime_error(msg), line(ln), col(c) {}
};

/// Hand-written recursive-descent parser.
///
/// Grammar:
///   program        → kernel_decl*
///   kernel_decl    → 'kernel' 'void' IDENT '(' param_list ')' block
///   param_list     → param (',' param)*
///   param          → type_spec IDENT
///   type_spec      → ('int' | 'float') '*'?
///   block          → '{' stmt* '}'
///   stmt           → var_decl | if_stmt | for_stmt | return_stmt | expr_stmt
///   var_decl       → type_spec IDENT ('=' expr)? ';'
///   if_stmt        → 'if' '(' expr ')' block ('else' block)?
///   for_stmt       → 'for' '(' stmt expr ';' expr ')' block
///   return_stmt    → 'return' expr? ';'
///   expr_stmt      → expr ';'
///   expr           → assignment
///   assignment     → logical_or ('=' assignment)?   // right-assoc
///   logical_or     → logical_and ('||' logical_and)*
///   logical_and    → equality ('&&' equality)*
///   equality       → relational (('=='|'!=') relational)*
///   relational     → additive (('<'|'<='|'>'|'>=') additive)*
///   additive       → multiplicative (('+'|'-') multiplicative)*
///   multiplicative → unary (('*'|'/') unary)*
///   unary          → '!' unary | primary
///   primary        → INT_LIT | FLOAT_LIT | BUILTIN
///                  | IDENT '[' expr ']' | IDENT | '(' expr ')'
///
class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    /// Parse a complete program (zero or more kernels).
    std::unique_ptr<Program> parseProgram();

private:
    std::vector<Token> tokens_;
    size_t pos_;

    // ── Token helpers ──
    const Token& peek() const;
    const Token& peekAt(size_t offset) const;
    const Token& previous() const;
    Token advance();
    bool  check(TokenType t) const;
    bool  match(TokenType t);
    Token expect(TokenType t, const std::string& context);
    [[noreturn]] void error(const std::string& msg);
    [[noreturn]] void errorAt(const Token& tok, const std::string& msg);

    // ── Grammar rules ──
    std::unique_ptr<KernelDecl>  parseKernelDecl();
    std::unique_ptr<ParamDecl>   parseParam();
    std::string                  parseTypeSpec();   // returns "int", "float", "int*", "float*"
    std::unique_ptr<BlockStmt>   parseBlock();
    NodePtr                      parseStmt();
    NodePtr                      parseVarDecl();
    NodePtr                      parseIfStmt();
    NodePtr                      parseForStmt();
    NodePtr                      parseReturnStmt();
    NodePtr                      parseExprStmt();

    // ── Expression rules (precedence climbing) ──
    NodePtr parseExpr();
    NodePtr parseAssignment();
    NodePtr parseLogicalOr();
    NodePtr parseLogicalAnd();
    NodePtr parseEquality();
    NodePtr parseRelational();
    NodePtr parseAdditive();
    NodePtr parseMultiplicative();
    NodePtr parseUnary();
    NodePtr parsePrimary();
};

} // namespace miniptx
