#pragma once

#include "ast.hpp"
#include "errors.hpp"
#include "runtime.hpp"
#include <memory>
#include <vector>

/**
 * Variadic Arity Constant
 * Used to indicate that a function accepts a variable number of arguments.
 */
#define VARIADIC_ARITY -1

/**
 * Interpreter Class
 * The core execution engine for the Pseudocode language.
 * Implements the Visitor pattern for both Expressions and Statements to traverse and execute the
 * AST.
 */
class Interpreter : public ExprVisitor, public StmtVisitor {
public:
    /**
     * Global Scope
     * Holds native functions (std-lib) and top-level variables.
     * Persistent throughout the lifetime of the interpreter.
     */
    std::shared_ptr<Environment> globals;

    /**
     * Current Scope
     * The environment in which variables are currently being looked up and defined.
     * Changes as the interpreter enters/exits blocks and function calls.
     */
    std::shared_ptr<Environment> environment;

    /**
     * Initialize the interpreter
     * Sets up the global environment and registers native functions.
     * @param reporterRef Reference to error reporter for runtime error handling
     */
    Interpreter(ErrorReporter &reporterRef);

    /**
     * Execute a program
     * The main entry point for running a list of statements.
     * @param statements Vector of AST statement nodes to execute
     */
    void interpret(const std::vector<StmtPtr> &statements) {
        try {
            for (const auto &stmt : statements) {
                execute(stmt.get());
            }
        } catch (const RuntimeError &error) {
            // Catch and format runtime errors for the user
            Token token     = error.token;
            std::string msg = error.what();
            reporter.report(ErrorType::Runtime, token.line, token.column, msg,
                            token.lexeme.length());
        }
    }

    /**
     * Evaluate an expression
     * Dispatches to the appropriate visit method and returns the resulting value.
     * @param expr Pointer to the expression node to evaluate
     * @return The resulting RuntimeValue
     */
    RuntimeValue evaluate(Expr *expr);

    /**
     * Execute a block of statements in a specific environment
     * Used for function bodies and local scopes.
     * @param statements The statements within the block
     * @param env The environment (scope) to use for execution
     */
    void executeBlock(const std::vector<StmtPtr> &statements, std::shared_ptr<Environment> env);

    // --- ExprVisitor Implementation ---
    // These methods return a RuntimeValue resulting from evaluating the expression node.

    RuntimeValue visitLiteralExpr(LiteralExpr *expr) override;
    RuntimeValue visitVariableExpr(VariableExpr *expr) override;
    RuntimeValue visitAssignExpr(AssignExpr *expr) override;
    RuntimeValue visitBinaryExpr(BinaryExpr *expr) override;
    RuntimeValue visitUnaryExpr(UnaryExpr *expr) override;
    RuntimeValue visitCallExpr(CallExpr *expr) override;
    RuntimeValue visitGetExpr(GetExpr *expr) override;
    RuntimeValue visitArrayAccessExpr(ArrayAccessExpr *expr) override;
    RuntimeValue visitArrayLitExpr(ArrayLitExpr *expr) override;
    RuntimeValue visitNewExpr(NewExpr *expr) override;

    // --- StmtVisitor Implementation ---
    // These methods execute the statement node and do not return a value.

    void visitExpressionStmt(ExpressionStmt *stmt) override;
    void visitReturnStmt(ReturnStmt *stmt) override;
    void visitBlockStmt(BlockStmt *stmt) override;
    void visitIfStmt(IfStmt *stmt) override;
    void visitWhileStmt(WhileStmt *stmt) override;
    void visitFunctionStmt(FunctionStmt *stmt) override;
    void visitClassStmt(ClassStmt *stmt) override;
    void visitForInStmt(ForInStmt *stmt) override;
    void visitForStmt(ForStmt *stmt) override;
    void visitCaseStmt(CaseStmt *stmt) override;

    /**
     * Error Reporter
     * Used by the interpreter to communicate runtime issues to the user.
     */
    ErrorReporter &reporter;

private:
    /**
     * Internal statement execution helper
     * @param stmt Raw pointer to the statement to execute
     */
    void execute(Stmt *stmt);

    // --- Internal Helpers ---

    /**
     * Check if a value is considered 'true' in Pseudocode.
     * Rules: null is false, booleans are their literal value, everything else is true.
     */
    bool isTruthy(const RuntimeValue &object);

    /**
     * Check for equality between two runtime values.
     * Handles type mismatches and deep comparison for scalars.
     */
    bool isEqual(const RuntimeValue &a, const RuntimeValue &b);

    /**
     * Validate that an operand is a number (int or double).
     * @throws RuntimeError if validation fails
     */
    void checkNumberOperand(const Token &operatorToken, const RuntimeValue &operand);

    /**
     * Validate that two operands are both numbers.
     * @throws RuntimeError if validation fails
     */
    void checkNumberOperands(const Token &operatorToken, const RuntimeValue &left,
                             const RuntimeValue &right);
};
