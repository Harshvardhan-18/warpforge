#include "lexer.hpp"
#include <cctype>
#include <unordered_map>

namespace miniptx {

// ─── Keyword table ─────────────────────────────────────────────────

static const std::unordered_map<std::string, TokenType> keywords = {
    {"kernel", TokenType::KERNEL},
    {"void",   TokenType::VOID},
    {"int",    TokenType::INT},
    {"float",  TokenType::FLOAT},
    {"if",     TokenType::IF},
    {"else",   TokenType::ELSE},
    {"for",    TokenType::FOR},
    {"return", TokenType::RETURN},
};

// ─── Built-in table (threadIdx.x → THREADIDX_X, etc.) ─────────────

struct BuiltinEntry {
    const char* suffix;   // ".x", ".y", ".z"
    TokenType   tokenX;
    TokenType   tokenY;
    TokenType   tokenZ;
};

static const std::unordered_map<std::string, BuiltinEntry> builtinPrefixes = {
    {"threadIdx", {".x", TokenType::THREADIDX_X, TokenType::THREADIDX_Y, TokenType::THREADIDX_Z}},
    {"blockIdx",  {".x", TokenType::BLOCKIDX_X,  TokenType::BLOCKIDX_Y,  TokenType::BLOCKIDX_Z}},
    {"blockDim",  {".x", TokenType::BLOCKDIM_X,  TokenType::BLOCKDIM_Y,  TokenType::BLOCKDIM_Z}},
};

// ─── Lexer implementation ──────────────────────────────────────────

Lexer::Lexer(std::string source)
    : source_(std::move(source)), pos_(0), line_(1), col_(1) {}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source_[pos_];
}

char Lexer::peekNext() const {
    if (pos_ + 1 >= source_.size()) return '\0';
    return source_[pos_ + 1];
}

char Lexer::advance() {
    char c = source_[pos_++];
    if (c == '\n') { line_++; col_ = 1; }
    else           { col_++; }
    return c;
}

bool Lexer::isAtEnd() const {
    return pos_ >= source_.size();
}

// ─── Skip whitespace & line comments ───────────────────────────────

void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = peek();

        // Whitespace
        if (std::isspace(static_cast<unsigned char>(c))) {
            advance();
            continue;
        }

        // Line comment: //
        if (c == '/' && peekNext() == '/') {
            while (!isAtEnd() && peek() != '\n') advance();
            continue;
        }

        // Block comment: /* ... */
        if (c == '/' && peekNext() == '*') {
            advance(); advance(); // consume /*
            while (!isAtEnd()) {
                if (peek() == '*' && peekNext() == '/') {
                    advance(); advance(); // consume */
                    break;
                }
                advance();
            }
            continue;
        }

        break;
    }
}

// ─── Number literals ───────────────────────────────────────────────

Token Lexer::readNumber() {
    int startLine = line_;
    int startCol  = col_;
    std::string num;
    bool isFloat = false;

    // Integer part
    while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek()))) {
        num += advance();
    }

    // Decimal point
    if (!isAtEnd() && peek() == '.' &&
        std::isdigit(static_cast<unsigned char>(peekNext()))) {
        isFloat = true;
        num += advance(); // '.'
        while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek()))) {
            num += advance();
        }
    }

    // Float suffix 'f' or 'F'
    if (!isAtEnd() && (peek() == 'f' || peek() == 'F')) {
        isFloat = true;
        advance(); // consume but don't include in value
    }

    return Token(isFloat ? TokenType::FLOAT_LITERAL : TokenType::INT_LITERAL,
                 num, startLine, startCol);
}

// ─── Identifiers, keywords, and GPU built-ins ──────────────────────

Token Lexer::readIdentifierOrKeyword() {
    int startLine = line_;
    int startCol  = col_;
    std::string id;

    while (!isAtEnd() &&
           (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) {
        id += advance();
    }

    // Check for GPU built-in: threadIdx.x, blockIdx.y, blockDim.z, etc.
    auto bit = builtinPrefixes.find(id);
    if (bit != builtinPrefixes.end()) {
        // Must see '.' followed by x, y, or z
        if (!isAtEnd() && peek() == '.') {
            char comp = peekNext();
            if (comp == 'x' || comp == 'y' || comp == 'z') {
                advance(); // consume '.'
                advance(); // consume component letter
                std::string full = id + "." + std::string(1, comp);

                TokenType tt;
                if      (comp == 'x') tt = bit->second.tokenX;
                else if (comp == 'y') tt = bit->second.tokenY;
                else                  tt = bit->second.tokenZ;

                return Token(tt, full, startLine, startCol);
            }
        }
    }

    // Check for keyword
    auto kwit = keywords.find(id);
    if (kwit != keywords.end()) {
        return Token(kwit->second, id, startLine, startCol);
    }

    // Regular identifier
    return Token(TokenType::IDENTIFIER, id, startLine, startCol);
}

// ─── Token helpers ─────────────────────────────────────────────────

Token Lexer::makeToken(TokenType type, const std::string& value) {
    return Token(type, value, line_, col_);
}

Token Lexer::makeToken(TokenType type, const std::string& value,
                       int startLine, int startCol) {
    return Token(type, value, startLine, startCol);
}

// ─── Main tokenisation entry ───────────────────────────────────────

Token Lexer::nextToken() {
    skipWhitespaceAndComments();

    if (isAtEnd()) return makeToken(TokenType::END_OF_FILE, "");

    int startLine = line_;
    int startCol  = col_;

    char c = peek();

    // Number literals
    if (std::isdigit(static_cast<unsigned char>(c))) {
        return readNumber();
    }

    // Identifiers, keywords, and built-ins
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
        return readIdentifierOrKeyword();
    }

    // Consume the character for operator / symbol matching
    advance();

    switch (c) {
        // ── Single-character symbols ──
        case '+': return Token(TokenType::PLUS,      "+", startLine, startCol);
        case '-': return Token(TokenType::MINUS,     "-", startLine, startCol);
        case '/': return Token(TokenType::SLASH,     "/", startLine, startCol);
        case '(': return Token(TokenType::LPAREN,    "(", startLine, startCol);
        case ')': return Token(TokenType::RPAREN,    ")", startLine, startCol);
        case '{': return Token(TokenType::LBRACE,    "{", startLine, startCol);
        case '}': return Token(TokenType::RBRACE,    "}", startLine, startCol);
        case '[': return Token(TokenType::LBRACKET,  "[", startLine, startCol);
        case ']': return Token(TokenType::RBRACKET,  "]", startLine, startCol);
        case ';': return Token(TokenType::SEMICOLON, ";", startLine, startCol);
        case ',': return Token(TokenType::COMMA,     ",", startLine, startCol);
        case '*': return Token(TokenType::STAR,      "*", startLine, startCol);

        // ── Two-character operators ──
        case '=':
            if (!isAtEnd() && peek() == '=') {
                advance();
                return Token(TokenType::EQUAL, "==", startLine, startCol);
            }
            return Token(TokenType::ASSIGN, "=", startLine, startCol);

        case '!':
            if (!isAtEnd() && peek() == '=') {
                advance();
                return Token(TokenType::NOT_EQUAL, "!=", startLine, startCol);
            }
            return Token(TokenType::NOT, "!", startLine, startCol);

        case '<':
            if (!isAtEnd() && peek() == '=') {
                advance();
                return Token(TokenType::LESS_EQUAL, "<=", startLine, startCol);
            }
            return Token(TokenType::LESS, "<", startLine, startCol);

        case '>':
            if (!isAtEnd() && peek() == '=') {
                advance();
                return Token(TokenType::GREATER_EQUAL, ">=", startLine, startCol);
            }
            return Token(TokenType::GREATER, ">", startLine, startCol);

        case '&':
            if (!isAtEnd() && peek() == '&') {
                advance();
                return Token(TokenType::AND, "&&", startLine, startCol);
            }
            // Single '&' is not in the spec — treat as unknown
            return Token(TokenType::UNKNOWN, "&", startLine, startCol);

        case '|':
            if (!isAtEnd() && peek() == '|') {
                advance();
                return Token(TokenType::OR, "||", startLine, startCol);
            }
            return Token(TokenType::UNKNOWN, "|", startLine, startCol);

        default:
            return Token(TokenType::UNKNOWN, std::string(1, c), startLine, startCol);
    }
}

// ─── Convenience: tokenize entire source ───────────────────────────

std::vector<Token> Lexer::tokenizeAll() {
    std::vector<Token> tokens;
    while (true) {
        Token t = nextToken();
        tokens.push_back(t);
        if (t.type == TokenType::END_OF_FILE) break;
    }
    return tokens;
}

} // namespace miniptx
