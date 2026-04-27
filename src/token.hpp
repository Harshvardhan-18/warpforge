#pragma once

#include <string>
#include <ostream>

namespace miniptx {

enum class TokenType {
    // ── Keywords ──
    KERNEL,
    VOID,
    INT,
    FLOAT,
    IF,
    ELSE,
    FOR,
    RETURN,

    // ── Literals ──
    INT_LITERAL,
    FLOAT_LITERAL,

    // ── Identifier ──
    IDENTIFIER,

    // ── Operators ──
    PLUS,           // +
    MINUS,          // -
    STAR,           // *
    SLASH,          // /
    ASSIGN,         // =
    EQUAL,          // ==
    NOT_EQUAL,      // !=
    LESS,           // <
    LESS_EQUAL,     // <=
    GREATER,        // >
    GREATER_EQUAL,  // >=
    AND,            // &&
    OR,             // ||
    NOT,            // !

    // ── Symbols / Delimiters ──
    LPAREN,         // (
    RPAREN,         // )
    LBRACE,         // {
    RBRACE,         // }
    LBRACKET,       // [
    RBRACKET,       // ]
    COMMA,          // ,
    SEMICOLON,      // ;

    // ── GPU Built-in tokens ──
    THREADIDX_X,
    THREADIDX_Y,
    THREADIDX_Z,
    BLOCKIDX_X,
    BLOCKIDX_Y,
    BLOCKIDX_Z,
    BLOCKDIM_X,
    BLOCKDIM_Y,
    BLOCKDIM_Z,

    // ── Special ──
    END_OF_FILE,
    UNKNOWN,
};

struct Token {
    TokenType   type;
    std::string value;
    int         line;
    int         col;

    Token() : type(TokenType::END_OF_FILE), line(0), col(0) {}
    Token(TokenType t, std::string val, int ln, int c)
        : type(t), value(std::move(val)), line(ln), col(c) {}
};

/// Return a human-readable name for a token type.
inline const char* tokenTypeName(TokenType t) {
    switch (t) {
        case TokenType::KERNEL:         return "KERNEL";
        case TokenType::VOID:           return "VOID";
        case TokenType::INT:            return "INT";
        case TokenType::FLOAT:          return "FLOAT";
        case TokenType::IF:             return "IF";
        case TokenType::ELSE:           return "ELSE";
        case TokenType::FOR:            return "FOR";
        case TokenType::RETURN:         return "RETURN";
        case TokenType::INT_LITERAL:    return "INT_LITERAL";
        case TokenType::FLOAT_LITERAL:  return "FLOAT_LITERAL";
        case TokenType::IDENTIFIER:     return "IDENTIFIER";
        case TokenType::PLUS:           return "PLUS";
        case TokenType::MINUS:          return "MINUS";
        case TokenType::STAR:           return "STAR";
        case TokenType::SLASH:          return "SLASH";
        case TokenType::ASSIGN:         return "ASSIGN";
        case TokenType::EQUAL:          return "EQUAL";
        case TokenType::NOT_EQUAL:      return "NOT_EQUAL";
        case TokenType::LESS:           return "LESS";
        case TokenType::LESS_EQUAL:     return "LESS_EQUAL";
        case TokenType::GREATER:        return "GREATER";
        case TokenType::GREATER_EQUAL:  return "GREATER_EQUAL";
        case TokenType::AND:            return "AND";
        case TokenType::OR:             return "OR";
        case TokenType::NOT:            return "NOT";
        case TokenType::LPAREN:         return "LPAREN";
        case TokenType::RPAREN:         return "RPAREN";
        case TokenType::LBRACE:         return "LBRACE";
        case TokenType::RBRACE:         return "RBRACE";
        case TokenType::LBRACKET:       return "LBRACKET";
        case TokenType::RBRACKET:       return "RBRACKET";
        case TokenType::COMMA:          return "COMMA";
        case TokenType::SEMICOLON:      return "SEMICOLON";
        case TokenType::THREADIDX_X:    return "THREADIDX_X";
        case TokenType::THREADIDX_Y:    return "THREADIDX_Y";
        case TokenType::THREADIDX_Z:    return "THREADIDX_Z";
        case TokenType::BLOCKIDX_X:     return "BLOCKIDX_X";
        case TokenType::BLOCKIDX_Y:     return "BLOCKIDX_Y";
        case TokenType::BLOCKIDX_Z:     return "BLOCKIDX_Z";
        case TokenType::BLOCKDIM_X:     return "BLOCKDIM_X";
        case TokenType::BLOCKDIM_Y:     return "BLOCKDIM_Y";
        case TokenType::BLOCKDIM_Z:     return "BLOCKDIM_Z";
        case TokenType::END_OF_FILE:    return "END_OF_FILE";
        case TokenType::UNKNOWN:        return "UNKNOWN";
    }
    return "??";
}

inline std::ostream& operator<<(std::ostream& os, const Token& t) {
    os << tokenTypeName(t.type) << "  \"" << t.value << "\"  "
       << t.line << ":" << t.col;
    return os;
}

} // namespace miniptx
