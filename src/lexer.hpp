#pragma once

#include "token.hpp"
#include <string>
#include <vector>

namespace miniptx {

/// Lexer: converts source text into a stream of tokens.
///
/// Usage:
///   Lexer lex(sourceCode);
///   Token t;
///   do {
///       t = lex.nextToken();
///       // process t ...
///   } while (t.type != TokenType::END_OF_FILE);
///
class Lexer {
public:
    explicit Lexer(std::string source);

    /// Return the next token from the source. Returns END_OF_FILE when done.
    Token nextToken();

    /// Convenience: tokenize the entire source at once.
    std::vector<Token> tokenizeAll();

private:
    std::string source_;
    size_t      pos_;
    int         line_;
    int         col_;

    char peek() const;
    char peekNext() const;
    char advance();
    bool isAtEnd() const;

    void skipWhitespaceAndComments();

    Token readNumber();
    Token readIdentifierOrKeyword();
    Token makeToken(TokenType type, const std::string& value);
    Token makeToken(TokenType type, const std::string& value, int startLine, int startCol);
};

} // namespace miniptx
