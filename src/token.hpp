#pragma once

#include <string>

// Every type of token the program can encounter
enum TokenKind {
    // End of file
    TOK_EOF,

    // === Identifier ===
    TOK_IDENTIFIER,

    // === Literals ===
    TOK_STRING,
    TOK_FLOAT,
    TOK_INTEGER,
    TOK_TRUE,
    TOK_FALSE,

    // === Keywords ===
    TOK_CLASS,
    TOK_INHERITS,
    TOK_ATTRIBUTES,
    TOK_METHODS,
    TOK_FUNCTION,
    TOK_RETURN,
    TOK_NEW,
    TOK_END,
    TOK_IF,
    TOK_THEN,
    TOK_ELSE,
    TOK_WHILE,
    TOK_FOR,
    TOK_IN,
    TOK_TO,

    // === Operators ===
    // Normal operators
    TOK_ASSIGN,   // =
    TOK_PLUS,     // +
    TOK_MINUS,    // -
    TOK_MULTIPLY, // *
    TOK_DIVIDE,   // /

    // Comparison operators
    TOK_EQUAL,        // ==
    TOK_NOT_EQUAL,    // !=
    TOK_GREATER_THAN, // >
    TOK_GT_OR_EQ,     // >=
    TOK_LESS_THAN,    // <
    TOK_LT_OR_EQ,     // <=
    TOK_AND,          // AND
    TOK_OR,           // OR
    TOK_NOT,          // NOT

    // Special operators
    TOK_DOT,   // .
    TOK_COLON, // :
    TOK_COMMA, // ,

    // Parentheses
    TOK_LPAREN,   // (
    TOK_RPAREN,   // )
    TOK_LBRACKET, // [
    TOK_RBRACKET  // ]
};

/**
 * Tokens Definition
 * The smallest meaningful unit in a langauge.
 */
struct Token {
    TokenKind type;
    std::string lexeme;

    /**
     * Token Locators
     * These help our program print more helpful error messages by
     * telling it where the token was that caused it to crash.
     */
    size_t line;   // Line number
    size_t column; // Column position (0-indexed)
    size_t length; // Length of the token in characters

    /**
     * Converts a token type to human-readable string
     * @return String representation of the token type
     */
    std::string typeToString() const {
        switch (type) {
        case TOK_EOF:
            return "EOF";
        case TOK_IDENTIFIER:
            return "IDENTIFIER";
        case TOK_STRING:
            return "STRING";
        case TOK_INTEGER:
            return "INTEGER";
        case TOK_FLOAT:
            return "FLOAT";
        case TOK_TRUE:
            return "BOOLEAN(true)";
        case TOK_FALSE:
            return "BOOLEAN(false)";

        case TOK_CLASS:
            return "KEYWORD(CLASS)";
        case TOK_ATTRIBUTES:
            return "KEYWORD(ATTRIBUTES)";
        case TOK_METHODS:
            return "KEYWORD(METHODS)";

        case TOK_FUNCTION:
            return "KEYWORD(FUNCTION)";
        case TOK_RETURN:
            return "KEYWORD(RETURN)";
        case TOK_NEW:
            return "KEYWORD(NEW)";
        case TOK_IF:
            return "KEYWORD(IF)";
        case TOK_END:
            return "KEYWORD(END)";
        case TOK_THEN:
            return "KEYWORD(THEN)";
        case TOK_IN:
            return "KEYWORD(IN)";
        case TOK_ELSE:
            return "KEYWORD(ELSE)";
        case TOK_WHILE:
            return "KEYWORD(WHILE)";
        case TOK_FOR:
            return "KEYWORD(FOR)";
        case TOK_TO:
            return "KEYWORD(TO)";

        case TOK_AND:
            return "OPERATOR(AND)";
        case TOK_OR:
            return "OPERATOR(OR)";
        case TOK_NOT:
            return "OPERATOR(NOT)";

        case TOK_ASSIGN:
            return "OPERATOR(=)";

        case TOK_PLUS:
            return "OPERATOR(+)";
        case TOK_MINUS:
            return "OPERATOR(-)";
        case TOK_MULTIPLY:
            return "OPERATOR(*)";
        case TOK_DIVIDE:
            return "OPERATOR(/)";

        case TOK_EQUAL:
            return "OPERATOR(==)";
        case TOK_NOT_EQUAL:
            return "OPERATOR(!=)";
        case TOK_GREATER_THAN:
            return "OPERATOR(>)";
        case TOK_GT_OR_EQ:
            return "OPERATOR(>=)";
        case TOK_LESS_THAN:
            return "OPERATOR(<)";
        case TOK_LT_OR_EQ:
            return "OPERATOR(<=)";

        case TOK_DOT:
            return "OPERATOR(.)";
        case TOK_COLON:
            return "OPERATOR(:)";
        case TOK_COMMA:
            return "OPERATOR(,)";
        case TOK_LPAREN:
            return "LPAREN";
        case TOK_RPAREN:
            return "RPAREN";
        case TOK_LBRACKET:
            return "LBRACKET";
        case TOK_RBRACKET:
            return "RBRACKET";
        default:
            return "UNKNOWN";
        }
    }
};
