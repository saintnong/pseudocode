#include "parser.hpp"

/**
 * Parser Constructor
 * Initializes the parser with a token stream and source code for error reporting
 * @param tokens Vector of tokens from lexer
 * @param source Original source code for error context
 * @param reporter Error reporting utility
 */
Parser::Parser(const std::vector<Token> &tokens, const std::string &source, ErrorReporter &reporter)
    : tokens(tokens), source(source), reporter(reporter) {
}

// ============================================================
// Debug Tracing
//
// Traces entry and exit of parsing functions.
// ============================================================

/**
 * Log parser function entry for debugging
 * Only output if TRACE is enabled
 */
void Parser::traceEnter(const std::string &name) {
    if (TRACE) {
        std::cerr << "[Parse] > " << name << " @ token: " << peek().lexeme << std::endl;
    }
}

/**
 * Log parser function exit for debugging
 */
void Parser::traceExit(const std::string &name) {
    if (TRACE) {
        std::cerr << "[Parse] < " << name << std::endl;
    }
}

std::vector<StmtPtr> Parser::parse() {
    traceEnter("parse");
    std::vector<StmtPtr> statements;
    while (!isAtEnd()) {
        statements.push_back(declaration());
    }
    traceExit("parse");
    return statements;
}

// ============================================================
// Pratt Parsing Engine
//
// Implements operator precedence parsing to correctly handle expressions
// with different precedence levels (e.g., * binds tighter than +).
// Processes both prefix operators (literals, identifiers) and infix
// operators (binary ops, function calls, member access).
// ============================================================

ExprPtr Parser::parseExpression(Precedence precedence) {
    advance(); // Move to the prefix token
    Token prefixToken = previous();

    // Dispatch to appropriate prefix handler based on token type
    ExprPtr left;
    switch (prefixToken.type) {
    case TOK_IDENTIFIER:
        left = variable();
        break;
    case TOK_INTEGER:
    case TOK_FLOAT:
    case TOK_STRING:
    case TOK_TRUE:
    case TOK_FALSE:
        left = literal();
        break;
    case TOK_LPAREN:
        left = grouping();
        break;
    case TOK_LBRACKET:
        left = arrayLiteral();
        break;
    case TOK_NEW:
        left = newObject();
        break;
    // Handle unary minus (-5)
    case TOK_MINUS:
        // Recursively parse high precedence (unary binds tight)
        left = std::make_unique<BinaryExpr>(
            std::make_unique<LiteralExpr>(Token{TOK_INTEGER, "0", prefixToken.line, 0, 1}),
            prefixToken,
            parseExpression(PREC_CALL) // unary hack for -x
        );
        break;
    default:
        errorAt(prefixToken, "Expected expression.");
        return nullptr;
    }

    // Process infix operators while they have higher precedence than context
    // This implements left-associativity for operators with same precedence
    while (precedence < getPrecedence(peek().type)) {
        Token infixToken = advance();

        switch (infixToken.type) {
        case TOK_PLUS:
        case TOK_MINUS:
        case TOK_MULTIPLY:
        case TOK_DIVIDE:
        case TOK_EQUAL:
        case TOK_GREATER_THAN:
        case TOK_GT_OR_EQ:
        case TOK_LESS_THAN:
        case TOK_LT_OR_EQ:
        case TOK_IN:
            left = binary(std::move(left));
            break;
        case TOK_LPAREN:
            left = call(std::move(left));
            break;
        case TOK_DOT:
            left = dot(std::move(left));
            break;
        case TOK_LBRACKET:
            left = subscript(std::move(left));
            break;
        case TOK_ASSIGN:
            left = assignment(std::move(left));
            break;
        default:
            return left;
        }
    }

    return left;
}

Parser::Precedence Parser::getPrecedence(TokenType type) {
    /**
     * Return the precedence level of a token operator
     * Used by Pratt parser to determine parsing order
     */
    switch (type) {
    case TOK_ASSIGN:
        return PREC_ASSIGNMENT;
    case TOK_EQUAL:
        return PREC_EQUALITY;
    case TOK_LESS_THAN:
    case TOK_GREATER_THAN:
    case TOK_GT_OR_EQ:
    case TOK_LT_OR_EQ:
    case TOK_IN:
        return PREC_COMPARISON;
    case TOK_PLUS:
    case TOK_MINUS:
        return PREC_TERM;
    case TOK_MULTIPLY:
    case TOK_DIVIDE:
        return PREC_FACTOR;
    case TOK_DOT:
    case TOK_LPAREN:
    case TOK_LBRACKET:
        return PREC_CALL;
    default:
        return PREC_NONE;
    }
}

// ============================================================
// Expression Parselets
//
// Each parselet handles a specific type of expression syntax.
// Prefix parselets handle the start of an expression.
// Infix parselets handle operations with a left operand.
// ============================================================

/**
 * Prefix: variable
 * Consumes an identifier token and wraps it as a variable reference
 */
ExprPtr Parser::variable() {
    return std::make_unique<VariableExpr>(previous());
}

/**
 * Prefix: literal
 * Consumes a literal token (number, string, boolean) without modification
 */
ExprPtr Parser::literal() {
    return std::make_unique<LiteralExpr>(previous());
}

/**
 * Prefix: grouping
 * Handles parenthesized expressions (expr)
 */
ExprPtr Parser::grouping() {
    ExprPtr expr = parseExpression(PREC_NONE);
    consume(TOK_RPAREN, "Expected ')' after expression.");
    return expr;
}

/**
 * Prefix: arrayLiteral
 * Parses array literal syntax: [elem1, elem2, ...]
 */
ExprPtr Parser::arrayLiteral() {
    Token anchor = previous();
    // We already consumed '['
    std::vector<ExprPtr> elements;
    if (!check(TOK_RBRACKET)) {
        do {
            elements.push_back(parseExpression(PREC_NONE));
        } while (match(TOK_COMMA));
    }
    consume(TOK_RBRACKET, "Expected ']' after array elements.");
    return std::make_unique<ArrayLitExpr>(anchor, std::move(elements));
}

/**
 * Prefix: newObject
 * Parses object instantiation: new ClassName(arg1, arg2, ...)
 */
ExprPtr Parser::newObject() {
    Token className = consume(TOK_IDENTIFIER, "Expected class name after 'new'.");
    consume(TOK_LPAREN, "Expected '(' after class name.");
    std::vector<ExprPtr> args;
    if (!check(TOK_RPAREN)) {
        do {
            args.push_back(parseExpression(PREC_NONE));
        } while (match(TOK_COMMA));
    }
    consume(TOK_RPAREN, "Expected ')' after arguments.");
    return std::make_unique<NewExpr>(className, std::move(args));
}

/**
 * Infix: binary operator
 * Parses binary operations (left op right) with proper left-associativity
 * Uses precedence+1 to ensure left-associativity for same-precedence operators
 */
ExprPtr Parser::binary(ExprPtr left) {
    Token op = previous();
    // Parse the right side with higher precedence for right-associative,
    // or same+1 for left-associative. Standard math is left-associative.
    Precedence prec = getPrecedence(op.type);
    ExprPtr right   = parseExpression((Precedence) (prec + 1));
    return std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
}

/**
 * Infix: function call
 * Parses function calls: callee(arg1, arg2, ...)
 */
ExprPtr Parser::call(ExprPtr left) {
    // left is the callee
    std::vector<ExprPtr> args;
    if (!check(TOK_RPAREN)) {
        do {
            args.push_back(parseExpression(PREC_NONE));
        } while (match(TOK_COMMA));
    }
    Token anchor = consume(TOK_RPAREN, "Expected ')' after arguments.");
    return std::make_unique<CallExpr>(anchor, std::move(left), std::move(args));
}

/**
 * Infix: member access
 * Parses property/member access: object.property
 */
ExprPtr Parser::dot(ExprPtr left) {
    Token name = consume(TOK_IDENTIFIER, "Expected property name after '.'.");
    return std::make_unique<GetExpr>(std::move(left), name);
}

/**
 * Infix: array subscript
 * Parses array element access: array[index]
 */
ExprPtr Parser::subscript(ExprPtr left) {
    Token anchor  = previous();
    ExprPtr index = parseExpression(PREC_NONE);
    consume(TOK_RBRACKET, "Expected ']' after index.");
    return std::make_unique<ArrayAccessExpr>(anchor, std::move(left), std::move(index));
}

/**
 * Infix: assignment
 * Parses assignment: target = value
 * Right-associative, so a = b = c parses as a = (b = c)
 */
ExprPtr Parser::assignment(ExprPtr left) {
    // Left side must be a valid assignment target
    Token assigned = tokens[current - 2];
    if (!dynamic_cast<VariableExpr *>(left.get()) && !dynamic_cast<GetExpr *>(left.get()) &&
        !dynamic_cast<ArrayAccessExpr *>(left.get())) {
        errorAt(assigned, "Invalid assignment target.");
        return nullptr;
    }

    // The = token has already been consumed in parseExpression
    // Parse the right-hand side expression
    // Right associative, so we parse everything to the right
    ExprPtr value = parseExpression(PREC_NONE);

    return std::make_unique<AssignExpr>(assigned, std::move(left), std::move(value));
}

// ============================================================
// Statement Parsing (Recursive Descent)
//
// Handles top-level and block-level statements.
// Uses recursive descent for clarity and straightforward error recovery.
// ============================================================

/**
 * Top-level declaration
 * Dispatches to class or function declarations, or falls back to statement parsing
 */
StmtPtr Parser::declaration() {
    traceEnter("declaration");
    try {
        if (match(TOK_CLASS)) {
            traceExit("declaration");
            return classDeclaration();
        }
        if (match(TOK_FUNCTION)) {
            traceExit("declaration");
            return functionDeclaration();
        }
        traceExit("declaration");
        return statement();
    } catch (const std::exception &e) {
        // Synchronize to next valid statement to prevent cascading errors
        synchronize();
        return nullptr;
    }
}

/**
 * Class declaration
 * Parses: CLASS name [INHERITS superclass]
 *         [ATTRIBUTES ...]
 *         [METHODS methodList]
 *         END name
 */
StmtPtr Parser::classDeclaration() {
    // Start parsing class
    traceEnter("classDeclaration");
    Token name = consume(TOK_IDENTIFIER, "Expected class name.");
    if (TRACE)
        std::cerr << "[Parse]   class: " << name.lexeme << std::endl;
    Token superclass{TOK_EOF, "", 0, 0, 0};

    // Inheritance
    if (match(TOK_INHERITS)) {
        superclass = consume(TOK_IDENTIFIER, "Expected superclass name.");
    }

    // Construct attributes list
    std::vector<ExprPtr> attributes;
    if (match(TOK_ATTRIBUTES)) {
        // Optional colon
        if (check(TOK_COLON))
            advance();

        // Loop until we hit next section or end of class
        while (!check(TOK_METHODS) && !check(TOK_END) && !isAtEnd()) {
            // Consume the assignment left part
            Token attrName = consume(TOK_IDENTIFIER, "Expected attribute name.");
            consume(TOK_ASSIGN, "Expected '=' after attribute name. Fields must be initialised.");

            // Parse the left and right expressions
            ExprPtr value  = parseExpression(PREC_NONE);
            ExprPtr target = std::make_unique<VariableExpr>(attrName);

            // Add to attributes list
            attributes.push_back(
                std::make_unique<AssignExpr>(attrName, std::move(target), std::move(value)));
        }
    }

    // Construct method list
    std::vector<StmtPtr> methods;
    if (match(TOK_METHODS)) {
        if (check(TOK_COLON))
            advance();
        while (!check(TOK_END) && !isAtEnd()) {
            methods.push_back(functionDeclaration());
        }
    }

    consume(TOK_END, "Expected 'END' after class body.");
    Token endName = consume(TOK_IDENTIFIER, "Expected class name after 'END'.");

    // Validate name.lexeme == endName.lexeme
    if (name.lexeme != endName.lexeme) {
        errorAt(endName, "Class name after 'END' does not match class declaration.");
    }
    traceExit("classDeclaration");

    return std::make_unique<ClassStmt>(name, superclass, std::move(methods), std::move(attributes));
}

/**
 * Function declaration
 * Parses: FUNCTION name(param1, param2, ...)
 *             body
 *         END name
 */
StmtPtr Parser::functionDeclaration() {
    traceEnter("functionDeclaration");

    // Handle whether 'FUNCTION' was consumed by caller or not
    // since class declaration also calls this method
    if (previous().type != TOK_FUNCTION && !match(TOK_FUNCTION)) {
        errorAt(peek(), "Expected 'FUNCTION' keyword.");
    }

    // Construct function name
    Token name = consume(TOK_IDENTIFIER, "Expected function name.");
    if (TRACE)
        std::cerr << "[Parse]   function: " << name.lexeme << std::endl;
    consume(TOK_LPAREN, "Expected '('.");

    // Construct parameters list
    std::vector<Token> params;
    if (!check(TOK_RPAREN)) {
        do {
            params.push_back(consume(TOK_IDENTIFIER, "Expected parameter name."));
        } while (match(TOK_COMMA));
    }
    consume(TOK_RPAREN, "Expected ')'.");

    std::vector<StmtPtr> body = block();

    consume(TOK_END, "Expected 'END' after function body.");

    // Make sure names match
    Token endName = consume(TOK_IDENTIFIER, "Expected function name after 'END'.");
    if (name.lexeme != endName.lexeme) {
        errorAt(endName, "Function name after 'END' does not match function declaration.");
    }
    traceExit("functionDeclaration");

    return std::make_unique<FunctionStmt>(name, params, std::move(body));
}

/**
 * Statement parsing
 * Dispatches to specific statement types (if, while, print, return), or
 * expression statements
 */
StmtPtr Parser::statement() {
    traceEnter("statement");
    if (match(TOK_RETURN)) {
        traceExit("statement");
        return returnStatement();
    }
    if (match(TOK_PRINT)) {
        traceExit("statement");
        return printStatement();
    }
    if (match(TOK_WHILE)) {
        traceExit("statement");
        return whileStatement();
    }
    if (match(TOK_FOR)) {
        traceExit("statement");
        return forInStatement();
    }
    if (match(TOK_IF)) {
        traceExit("statement");
        return ifStatement();
    }

    ExprPtr expr = parseExpression(PREC_NONE);
    traceExit("statement");
    return std::make_unique<ExpressionStmt>(std::move(expr));
}

/**
 * If-then-else statement
 * Parses: IF condition THEN statements [ELSE statements] END IF
 */
StmtPtr Parser::ifStatement() {
    traceEnter("ifStatement");
    ExprPtr condition = parseExpression(PREC_NONE);
    consume(TOK_THEN, "Expected 'THEN' after if condition.");

    std::vector<StmtPtr> thenBranch;
    std::vector<StmtPtr> elseBranch;

    while (!check(TOK_ELSE) && !check(TOK_END) && !isAtEnd()) {
        thenBranch.push_back(statement());
    }

    if (match(TOK_ELSE)) {
        while (!check(TOK_END) && !isAtEnd()) {
            elseBranch.push_back(statement());
        }
    }

    consume(TOK_END, "Expected 'END' after if.");
    Token keyword = consume(TOK_IF, "Expected 'IF' after 'END'.");
    traceExit("ifStatement");

    return std::make_unique<IfStmt>(keyword, std::move(condition), std::move(thenBranch),
                                    std::move(elseBranch));
}

/**
 * While loop statement
 * Parses: WHILE condition statements END WHILE
 */
StmtPtr Parser::whileStatement() {
    traceEnter("whileStatement");
    ExprPtr condition = parseExpression(PREC_NONE);
    // Pseudocode didn't strictly show "DO", but usually loop starts block
    std::vector<StmtPtr> body = block();
    consume(TOK_END, "Expected 'END' after while loop.");
    Token keyword = consume(TOK_WHILE, "Expected 'WHILE' after 'END'.");
    traceExit("whileStatement");
    return std::make_unique<WhileStmt>(keyword, std::move(condition), std::move(body));
}

/**
 * For-in loop statement
 * Parses: FOR variable IN iterable statements END FOR
 */
StmtPtr Parser::forInStatement() {
    traceEnter("forInStatement");

    // Get the loop variable
    Token variable = consume(TOK_IDENTIFIER, "Expected variable name after FOR.");

    // Expect IN keyword
    consume(TOK_IN, "Expected 'IN' after for loop variable.");

    // Parse the iterable expression
    ExprPtr iterable = parseExpression(PREC_NONE);

    // Parse the loop body
    std::vector<StmtPtr> body = block();

    // Expect END FOR
    consume(TOK_END, "Expected 'END' after for loop.");
    Token keyword = consume(TOK_FOR, "Expected 'FOR' after 'END'.");

    traceExit("forInStatement");
    return std::make_unique<ForInStmt>(keyword, variable, std::move(iterable), std::move(body));
}

/**
 * Print statement
 * Parses: PRINT(expression)
 */
StmtPtr Parser::printStatement() {
    consume(TOK_LPAREN, "Expected '(' after PRINT.");
    ExprPtr expr = parseExpression(PREC_NONE);
    consume(TOK_RPAREN, "Expected ')' after PRINT argument.");
    return std::make_unique<PrintStmt>(std::move(expr));
}

/**
 * Return statement
 * Parses: RETURN [expression]
 * Expression is optional
 */
StmtPtr Parser::returnStatement() {
    ExprPtr value = nullptr;
    // Check if next token is start of an expression
    if (!check(TOK_END) && !check(TOK_ELSE)) {
        value = parseExpression(PREC_NONE);
    }
    return std::make_unique<ReturnStmt>(std::move(value));
}

/**
 * Parse a block of statements
 * Continues until hitting a block-terminating keyword (END, ELSE, etc.)
 * Used for function bodies, if/else branches, and loop bodies
 */
std::vector<StmtPtr> Parser::block() {
    std::vector<StmtPtr> stmts;
    // Keep parsing statements until we hit a keyword that terminates a block
    // (END, ELSE, etc.)
    while (!check(TOK_END) && !check(TOK_ELSE) && !isAtEnd()) {
        stmts.push_back(statement());
    }
    return stmts;
}

// ============================================================
// Helper Functions for Token Navigation
// ============================================================

/**
 * Advance to next token and return the previous one
 */
Token Parser::advance() {
    if (!isAtEnd())
        current++;
    return previous();
}

/**
 * Check if we've reached end of token stream
 */
bool Parser::isAtEnd() {
    return peek().type == TOK_EOF;
}

/**
 * Return current token without advancing
 */
Token Parser::peek() {
    return tokens[current];
}

/**
 * Return previous token
 */
Token Parser::previous() {
    return tokens[current - 1];
}

/**
 * Check if current token matches given type
 */
bool Parser::check(TokenType type) {
    if (isAtEnd())
        return false;
    return peek().type == type;
}

/**
 * If current token matches type, consume it and return true
 */
bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

/**
 * Consume a token of expected type or report an error
 */
Token Parser::consume(TokenType type, std::string message) {
    if (check(type))
        return advance();

    errorAt(peek(), message);
    return peek();
}

/**
 * Report an error at a specific token with source context
 */
void Parser::errorAt(Token token, const std::string &message) {
    if (token.type == TOK_EOF) {
        reporter.report(ErrorType::Syntax, token.line, 0, message + " at end", 1);
        return;
    }

    // Use token's stored column position and length directly
    reporter.report(ErrorType::Syntax, token.line, token.column, message, token.length);
}

void Parser::synchronize() {
    advance();

    while (!isAtEnd()) {
        if (previous().type == TOK_END)
            return; // 'END' often finishes blocks

        switch (peek().type) {
        case TOK_CLASS:
        case TOK_FUNCTION:
        case TOK_IF:
        case TOK_WHILE:
        case TOK_PRINT:
        case TOK_RETURN:
            return; // Statement boundaries
        default:
            break;
        }

        advance();
    }
}
