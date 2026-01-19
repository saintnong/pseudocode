#pragma once

#include "ast.hpp"
#include "errors.hpp"

#include <functional>

/**
 * Parser - Converts a stream of tokens into an Abstract Syntax Tree (AST)
 *
 * Uses a Pratt parsing algorithm for expressions combined with recursive descent
 * for statements. This hybrid approach allows efficient handling of operator precedence
 * while keeping statement parsing straightforward.
 */
class Parser {
public:
    /**
     * Parser Constructor
     * @param tokens Vector of tokens from the lexer
     * @param source The original source code (for error context)
     * @param reporter Error reporter for handling parse failures
     */
    Parser(const std::vector<Token> &tokens, const std::string &source, ErrorReporter &reporter);

    /**
     * Entry point for parsing
     * Parses the entire token stream into top-level statements
     * @return Vector of AST statement nodes
     */
    std::vector<StmtPtr> parse();

private:
    const std::vector<Token> &tokens;
    const std::string &source;
    ErrorReporter &reporter;
    size_t current              = 0;
    static constexpr bool TRACE = false; // Set to false to disable tracing

    /**
     * Operator Precedence Levels
     * Used by Pratt parser to handle operator associativity and binding strength.
     * Higher numbers bind tighter (higher precedence).
     */
    enum Precedence {
        PREC_NONE       = 0,
        PREC_ASSIGNMENT = 10, // =
        PREC_OR         = 12, // OR
        PREC_AND        = 14, // AND
        PREC_EQUALITY   = 20, // == !=
        PREC_COMPARISON = 30, // < > <= >= IN
        PREC_TERM       = 40, // + -
        PREC_FACTOR     = 50, // * /
        PREC_UNARY      = 60, // NOT -
        PREC_CALL       = 70, // . () []
        PREC_PRIMARY    = 80  // (highest)
    };

    // --- Token Navigation ---
    bool match(TokenKind type);
    bool check(TokenKind type);
    Token advance();
    Token consume(TokenKind type, std::string message);
    Token peek();
    Token previous();
    bool isAtEnd();
    bool nextTokenIsCaseArm();
    Precedence getPrecedence(TokenKind type);

    // --- Debug Tracing ---
    /**
     * Log entry into a parsing function for debugging
     */
    void traceEnter(const std::string &name);

    /**
     * Log exit from a parsing function for debugging
     */
    void traceExit(const std::string &name);

    // --- Error Handling ---
    /**
     * Report an error at a specific token with source context
     * Extracts the source line and provides visual feedback
     */
    void errorAt(Token token, const std::string &message);

    /**
     * Synchronize parser state after error
     * Discards tokens until a statement boundary is found to prevent cascading errors
     */
    void synchronize();

    // --- Statement Parsing ---
    /**
     * Parse a top-level declaration (class or function) or a statement
     * Entry point for the recursive descent portion of the parser
     */
    StmtPtr declaration();

    /**
     * Parse a class declaration with optional inheritance, attributes, and methods
     */
    StmtPtr classDeclaration();

    /**
     * Parse a function declaration with parameters and body
     */
    StmtPtr functionDeclaration();

    /**
     * Parse a single statement (if, while, print, return, or expression statement)
     */
    StmtPtr statement();

    /**
     * Parse an if-then-else statement
     */
    StmtPtr ifStatement();

    /**
     * Parse a while loop with condition and body
     */
    StmtPtr whileStatement();

    /**
     * Parse a for-in loop with variable, iterable, and body
     */
    StmtPtr forInStatement(Token variable);

    /**
     * Parse a for-to loop with a variable, start, end, and body
     */
    StmtPtr forStatement(Token variable);

    /**
     * Parse a case statement
     */
    StmtPtr caseStatement();

    /**
     * Parse a return statement (with optional value)
     */
    StmtPtr returnStatement();
    StmtPtr repeatUntilStatement();

    /**
     * Parse a block of statements until a block-terminating keyword (END, ELSE)
     */
    std::vector<StmtPtr> block();

    // --- Pratt Expression Parsing ---

    /**
     * Signature for prefix parselets (handle start of expression: literals, variables, unary)
     */
    using PrefixFn = std::function<ExprPtr()>;

    /**
     * Signature for infix parselets (handle operators with left operand: binary ops, calls)
     */
    using InfixFn = std::function<ExprPtr(ExprPtr)>;

    /**
     * Parse an expression with given minimum precedence
     * Core of Pratt parsing: handles operator precedence and associativity
     */
    ExprPtr parseExpression(Precedence precedence = PREC_ASSIGNMENT);

    // --- Prefix Handlers (beginning of expression) ---
    /**
     * Parse an identifier or variable reference
     */
    ExprPtr variable();

    /**
     * Parse a literal value (number, string, boolean)
     */
    ExprPtr literal();

    /**
     * Parse a parenthesized expression (grouping)
     */
    ExprPtr grouping();

    /**
     * Parse an array literal [elem1, elem2, ...]
     */
    ExprPtr arrayLiteral();

    /**
     * Parse object instantiation (new ClassName(...))
     */
    ExprPtr newObject();

    // --- Infix Handlers (operator with left operand) ---
    /**
     * Parse a binary operation (left op right)
     * Recursively parses right side with appropriate precedence
     */
    ExprPtr binary(ExprPtr left);

    /**
     * Parse a function call (callee(...))
     */
    ExprPtr call(ExprPtr left);

    /**
     * Parse member/property access (object.property)
     */
    ExprPtr dot(ExprPtr left);

    /**
     * Parse array subscript access (array[index])
     */
    ExprPtr subscript(ExprPtr left);

    /**
     * Parse assignment (target = value)
     * Right-associative: a = b = c parses as a = (b = c)
     */
    ExprPtr assignment(ExprPtr left);
};
