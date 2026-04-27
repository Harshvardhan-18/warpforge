#include "parser.hpp"
#include <sstream>

namespace miniptx {

Parser::Parser(std::vector<Token> tokens)
    : tokens_(std::move(tokens)), pos_(0) {}

// ═══════════════════════════════════════════════════════════════════
//  Token helpers
// ═══════════════════════════════════════════════════════════════════

const Token& Parser::peek() const     { return tokens_[pos_]; }
const Token& Parser::previous() const { return tokens_[pos_ - 1]; }

const Token& Parser::peekAt(size_t offset) const {
    size_t idx = pos_ + offset;
    if (idx >= tokens_.size()) return tokens_.back(); // EOF
    return tokens_[idx];
}

Token Parser::advance() {
    Token t = tokens_[pos_];
    if (t.type != TokenType::END_OF_FILE) pos_++;
    return t;
}

bool Parser::check(TokenType t) const { return peek().type == t; }

bool Parser::match(TokenType t) {
    if (check(t)) { advance(); return true; }
    return false;
}

Token Parser::expect(TokenType t, const std::string& context) {
    if (check(t)) return advance();
    std::ostringstream oss;
    oss << "expected " << tokenTypeName(t) << " " << context
        << ", but found '" << peek().value << "' ("
        << tokenTypeName(peek().type) << ")";
    errorAt(peek(), oss.str());
}

void Parser::error(const std::string& msg) {
    errorAt(peek(), msg);
}

void Parser::errorAt(const Token& tok, const std::string& msg) {
    std::ostringstream oss;
    oss << "[line " << tok.line << ", col " << tok.col << "] " << msg;
    throw ParseError(oss.str(), tok.line, tok.col);
}

// ═══════════════════════════════════════════════════════════════════
//  program → kernel_decl*
// ═══════════════════════════════════════════════════════════════════

std::unique_ptr<Program> Parser::parseProgram() {
    auto prog = std::make_unique<Program>();
    while (!check(TokenType::END_OF_FILE)) {
        prog->kernels_.push_back(parseKernelDecl());
    }
    return prog;
}

// ═══════════════════════════════════════════════════════════════════
//  kernel_decl → 'kernel' 'void' IDENT '(' param_list ')' block
// ═══════════════════════════════════════════════════════════════════

std::unique_ptr<KernelDecl> Parser::parseKernelDecl() {
    int line = peek().line;
    expect(TokenType::KERNEL, "at start of kernel declaration");
    expect(TokenType::VOID,   "after 'kernel' (only void return supported)");
    Token name = expect(TokenType::IDENTIFIER, "for kernel name");
    expect(TokenType::LPAREN, "after kernel name '" + name.value + "'");

    // param_list → param (',' param)*
    std::vector<std::unique_ptr<ParamDecl>> params;
    if (!check(TokenType::RPAREN)) {
        params.push_back(parseParam());
        while (match(TokenType::COMMA)) {
            params.push_back(parseParam());
        }
    }
    expect(TokenType::RPAREN, "after parameter list");

    auto body = parseBlock();
    return std::make_unique<KernelDecl>(name.value, std::move(params),
                                        std::move(body), line);
}

// ═══════════════════════════════════════════════════════════════════
//  param → type_spec IDENT
//  type_spec → ('int' | 'float') '*'?
// ═══════════════════════════════════════════════════════════════════

std::unique_ptr<ParamDecl> Parser::parseParam() {
    int line = peek().line;
    std::string baseType = parseTypeSpec();
    bool isPtr = (baseType.back() == '*');
    if (isPtr) baseType.pop_back(); // strip '*' for ParamDecl
    Token name = expect(TokenType::IDENTIFIER, "for parameter name");
    return std::make_unique<ParamDecl>(baseType, isPtr, name.value, line);
}

std::string Parser::parseTypeSpec() {
    std::string type;
    if      (match(TokenType::INT))   type = "int";
    else if (match(TokenType::FLOAT)) type = "float";
    else if (match(TokenType::VOID))  type = "void";
    else error("expected type specifier ('int', 'float', or 'void')");

    if (match(TokenType::STAR)) type += "*";
    return type;
}

// ═══════════════════════════════════════════════════════════════════
//  block → '{' stmt* '}'
// ═══════════════════════════════════════════════════════════════════

std::unique_ptr<BlockStmt> Parser::parseBlock() {
    int line = peek().line;
    expect(TokenType::LBRACE, "at start of block");
    std::vector<NodePtr> stmts;
    while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
        stmts.push_back(parseStmt());
    }
    expect(TokenType::RBRACE, "at end of block");
    return std::make_unique<BlockStmt>(std::move(stmts), line);
}

// ═══════════════════════════════════════════════════════════════════
//  stmt → var_decl | if_stmt | for_stmt | return_stmt | expr_stmt
// ═══════════════════════════════════════════════════════════════════

NodePtr Parser::parseStmt() {
    if (check(TokenType::IF))     return parseIfStmt();
    if (check(TokenType::FOR))    return parseForStmt();
    if (check(TokenType::RETURN)) return parseReturnStmt();

    // var_decl starts with a type keyword
    if (check(TokenType::INT) || check(TokenType::FLOAT)) {
        // Disambiguate: type followed by IDENT means var_decl
        // (could also be just 'int' as expression, but that's nonsensical)
        TokenType nextType = peekAt(1).type;
        if (nextType == TokenType::IDENTIFIER || nextType == TokenType::STAR) {
            return parseVarDecl();
        }
    }

    return parseExprStmt();
}

// ═══════════════════════════════════════════════════════════════════
//  var_decl → type_spec IDENT ('=' expr)? ';'
// ═══════════════════════════════════════════════════════════════════

NodePtr Parser::parseVarDecl() {
    int line = peek().line;
    std::string type = parseTypeSpec();
    Token name = expect(TokenType::IDENTIFIER, "in variable declaration");
    NodePtr init = nullptr;
    if (match(TokenType::ASSIGN)) {
        init = parseExpr();
    }
    expect(TokenType::SEMICOLON, "after variable declaration");
    return std::make_unique<VarDeclStmt>(type, name.value, std::move(init), line);
}

// ═══════════════════════════════════════════════════════════════════
//  if_stmt → 'if' '(' expr ')' block ('else' block)?
// ═══════════════════════════════════════════════════════════════════

NodePtr Parser::parseIfStmt() {
    int line = peek().line;
    expect(TokenType::IF, "");
    expect(TokenType::LPAREN, "after 'if'");
    auto cond = parseExpr();
    expect(TokenType::RPAREN, "after if-condition");
    auto thenBlk = parseBlock();
    std::unique_ptr<BlockStmt> elseBlk = nullptr;
    if (match(TokenType::ELSE)) {
        elseBlk = parseBlock();
    }
    return std::make_unique<IfStmt>(std::move(cond), std::move(thenBlk),
                                     std::move(elseBlk), line);
}

// ═══════════════════════════════════════════════════════════════════
//  for_stmt → 'for' '(' stmt expr ';' expr ')' block
// ═══════════════════════════════════════════════════════════════════

NodePtr Parser::parseForStmt() {
    int line = peek().line;
    expect(TokenType::FOR, "");
    expect(TokenType::LPAREN, "after 'for'");

    // init: a full statement (var_decl or expr_stmt, includes its ';')
    NodePtr init = parseStmt();

    // condition expression
    auto cond = parseExpr();
    expect(TokenType::SEMICOLON, "after for-condition");

    // update expression
    auto update = parseExpr();
    expect(TokenType::RPAREN, "after for-update");

    auto body = parseBlock();
    return std::make_unique<ForStmt>(std::move(init), std::move(cond),
                                      std::move(update), std::move(body), line);
}

// ═══════════════════════════════════════════════════════════════════
//  return_stmt → 'return' expr? ';'
// ═══════════════════════════════════════════════════════════════════

NodePtr Parser::parseReturnStmt() {
    int line = peek().line;
    expect(TokenType::RETURN, "");
    NodePtr val = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        val = parseExpr();
    }
    expect(TokenType::SEMICOLON, "after return statement");
    return std::make_unique<ReturnStmt>(std::move(val), line);
}

// ═══════════════════════════════════════════════════════════════════
//  expr_stmt → expr ';'
// ═══════════════════════════════════════════════════════════════════

NodePtr Parser::parseExprStmt() {
    int line = peek().line;
    auto expr = parseExpr();
    expect(TokenType::SEMICOLON, "after expression");
    return std::make_unique<ExprStmt>(std::move(expr), line);
}

// ═══════════════════════════════════════════════════════════════════
//  expr       → assignment
//  assignment → logical_or ('=' assignment)?    [right-associative]
// ═══════════════════════════════════════════════════════════════════

NodePtr Parser::parseExpr() {
    return parseAssignment();
}

NodePtr Parser::parseAssignment() {
    auto lhs = parseLogicalOr();

    if (match(TokenType::ASSIGN)) {
        int line = previous().line;
        auto rhs = parseAssignment(); // right-associative
        return std::make_unique<AssignExpr>(std::move(lhs), std::move(rhs), line);
    }

    return lhs;
}

// ═══════════════════════════════════════════════════════════════════
//  logical_or → logical_and ('||' logical_and)*
// ═══════════════════════════════════════════════════════════════════

NodePtr Parser::parseLogicalOr() {
    auto left = parseLogicalAnd();
    while (match(TokenType::OR)) {
        int line = previous().line;
        auto right = parseLogicalAnd();
        left = std::make_unique<BinaryExpr>("||", std::move(left),
                                             std::move(right), line);
    }
    return left;
}

// ═══════════════════════════════════════════════════════════════════
//  logical_and → equality ('&&' equality)*
// ═══════════════════════════════════════════════════════════════════

NodePtr Parser::parseLogicalAnd() {
    auto left = parseEquality();
    while (match(TokenType::AND)) {
        int line = previous().line;
        auto right = parseEquality();
        left = std::make_unique<BinaryExpr>("&&", std::move(left),
                                             std::move(right), line);
    }
    return left;
}

// ═══════════════════════════════════════════════════════════════════
//  equality → relational (('=='|'!=') relational)*
// ═══════════════════════════════════════════════════════════════════

NodePtr Parser::parseEquality() {
    auto left = parseRelational();
    while (true) {
        std::string op;
        if      (match(TokenType::EQUAL))     op = "==";
        else if (match(TokenType::NOT_EQUAL)) op = "!=";
        else break;
        int line = previous().line;
        auto right = parseRelational();
        left = std::make_unique<BinaryExpr>(op, std::move(left),
                                             std::move(right), line);
    }
    return left;
}

// ═══════════════════════════════════════════════════════════════════
//  relational → additive (('<'|'<='|'>'|'>=') additive)*
// ═══════════════════════════════════════════════════════════════════

NodePtr Parser::parseRelational() {
    auto left = parseAdditive();
    while (true) {
        std::string op;
        if      (match(TokenType::LESS))          op = "<";
        else if (match(TokenType::LESS_EQUAL))    op = "<=";
        else if (match(TokenType::GREATER))       op = ">";
        else if (match(TokenType::GREATER_EQUAL)) op = ">=";
        else break;
        int line = previous().line;
        auto right = parseAdditive();
        left = std::make_unique<BinaryExpr>(op, std::move(left),
                                             std::move(right), line);
    }
    return left;
}

// ═══════════════════════════════════════════════════════════════════
//  additive → multiplicative (('+'|'-') multiplicative)*
// ═══════════════════════════════════════════════════════════════════

NodePtr Parser::parseAdditive() {
    auto left = parseMultiplicative();
    while (true) {
        std::string op;
        if      (match(TokenType::PLUS))  op = "+";
        else if (match(TokenType::MINUS)) op = "-";
        else break;
        int line = previous().line;
        auto right = parseMultiplicative();
        left = std::make_unique<BinaryExpr>(op, std::move(left),
                                             std::move(right), line);
    }
    return left;
}

// ═══════════════════════════════════════════════════════════════════
//  multiplicative → unary (('*'|'/') unary)*
// ═══════════════════════════════════════════════════════════════════

NodePtr Parser::parseMultiplicative() {
    auto left = parseUnary();
    while (true) {
        std::string op;
        if      (match(TokenType::STAR))  op = "*";
        else if (match(TokenType::SLASH)) op = "/";
        else break;
        int line = previous().line;
        auto right = parseUnary();
        left = std::make_unique<BinaryExpr>(op, std::move(left),
                                             std::move(right), line);
    }
    return left;
}

// ═══════════════════════════════════════════════════════════════════
//  unary → '!' unary | primary
// ═══════════════════════════════════════════════════════════════════

NodePtr Parser::parseUnary() {
    if (match(TokenType::NOT)) {
        int line = previous().line;
        return std::make_unique<UnaryExpr>("!", parseUnary(), line);
    }
    return parsePrimary();
}

// ═══════════════════════════════════════════════════════════════════
//  primary → INT_LIT | FLOAT_LIT | BUILTIN
//          | IDENT '[' expr ']' | IDENT | '(' expr ')'
// ═══════════════════════════════════════════════════════════════════

NodePtr Parser::parsePrimary() {
    int line = peek().line;

    // INT_LIT
    if (match(TokenType::INT_LITERAL)) {
        return std::make_unique<IntLiteral>(std::stoi(previous().value), line);
    }

    // FLOAT_LIT
    if (match(TokenType::FLOAT_LITERAL)) {
        return std::make_unique<FloatLiteral>(std::stof(previous().value), line);
    }

    // BUILTIN tokens (threadIdx.x, blockIdx.y, blockDim.z, etc.)
    {
        TokenType bt = peek().type;
        if (bt == TokenType::THREADIDX_X || bt == TokenType::THREADIDX_Y ||
            bt == TokenType::THREADIDX_Z ||
            bt == TokenType::BLOCKIDX_X  || bt == TokenType::BLOCKIDX_Y  ||
            bt == TokenType::BLOCKIDX_Z  ||
            bt == TokenType::BLOCKDIM_X  || bt == TokenType::BLOCKDIM_Y  ||
            bt == TokenType::BLOCKDIM_Z) {
            Token tok = advance();
            return std::make_unique<BuiltinExpr>(tok.value, line);
        }
    }

    // IDENT '[' expr ']'   or   IDENT
    if (match(TokenType::IDENTIFIER)) {
        std::string name = previous().value;
        if (match(TokenType::LBRACKET)) {
            auto idx = parseExpr();
            expect(TokenType::RBRACKET, "in array subscript");
            return std::make_unique<ArrayAccessExpr>(
                std::make_unique<VarExpr>(name, line), std::move(idx), line);
        }
        return std::make_unique<VarExpr>(name, line);
    }

    // '(' expr ')'
    if (match(TokenType::LPAREN)) {
        auto expr = parseExpr();
        expect(TokenType::RPAREN, "to close parenthesized expression");
        return expr;
    }

    // Error
    std::ostringstream oss;
    oss << "unexpected token '" << peek().value << "' ("
        << tokenTypeName(peek().type) << ") — expected an expression";
    error(oss.str());
}

} // namespace miniptx
