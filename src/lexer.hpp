#pragma once

#include <cctype>
#include <string>
#include <unordered_map>
#include <vector>

#include "errors.hpp"
#include "token.hpp"

// --- Lexer Declaration ---
// The Lexer class tokenizes pseudocode source into a stream of tokens.
// It handles keywords, identifiers, literals, operators, and comments.

class Lexer {
private:
    // Source code being tokenized
    std::string source;
    // Output tokens collected during scanning
    std::vector<Token> tokens;
    // Start position of current token in source
    size_t start;
    // Current position in source
    size_t current;
    // Current line number (for error reporting)
    size_t line;
    // Line number at the start of current token
    size_t startLine;
    // Column at the start of current token (0-indexed)
    size_t startColumn;
    // Current column position in the current line (0-indexed)
    size_t column;
    // Map of keywords to their token types
    std::unordered_map<std::string, TokenKind> keywords;

    // Error reporter for communicating issues
    ErrorReporter &reporter;

public:
    /**
     * Construct a lexer for the given source code
     * @param src The source code string to tokenize
     * @param errReporter Reference to error reporter
     * @param startLine Starting line in the current source
     */
    Lexer(const std::string &src, ErrorReporter &errReporter, size_t startLine = 1);

    /**
     * Scan all tokens from the source code
     * @return Vector of all tokens in the source
     */
    std::vector<Token> scanTokens();

private:
    /**
     * Check if we're at the end of source
     */
    bool isAtEnd();

    /**
     * Consume and return the current character
     */
    char advance();

    /**
     * Peek at the current character without consuming
     */
    char peek();

    /**
     * Conditionally consume a character if it matches expected
     */
    bool match(char expected);

    /**
     * Add a token to the token list from current span
     */
    void addToken(TokenKind type);

    /**
     * Add a token with a specific literal value
     */
    void addToken(TokenKind type, std::string literal);

    /**
     * Report a lexical error with context
     */
    void reportError(ErrorType type, const std::string &message);

    /**
     * Scan a string literal
     */
    void string(char quoteType);

    /**
     * Scan a numeric literal (int or float)
     */
    void number();

    /**
     * Scan an identifier or keyword
     */
    void identifier();

    /**
     * Main token scanning dispatch
     */
    void scanToken();
};
