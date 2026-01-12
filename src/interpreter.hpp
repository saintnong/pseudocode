#pragma once

#include "ast.hpp"
#include "errors.hpp"
#include "runtime.hpp"
#include <memory>
#include <vector>

/**
 * Interpreter Class
 */
class Interpreter : public ExprVisitor, public StmtVisitor {
public:
    /**
     * Global scope
     * Holds native functions and top-level variables.
     */
    std::shared_ptr<Environment> globals;

    /**
     * Current scope
     * The current scope that the local environment the interpreter is operating in.
     */
    std::shared_ptr<Environment> environment;

    /**
     * Creates an interpreter, and its environment.
     * @returns An interpreter
     */
    Interpreter(ErrorReporter &reporterRef) : reporter(reporterRef) {
        // Initiate our global environment, and begin interpreting in global scope.
        globals     = std::make_shared<Environment>();
        environment = globals;
    }

    /**
     * Execute each statement, until an error occurs.
     */
    void interpret(const std::vector<StmtPtr> &statements) {
        try {
            for (const auto &stmt : statements) {
                execute(stmt.get());
            }
        } catch (const RuntimeError &error) {
            // Runtime error caught.
            // Report it nicely.
            Token token     = error.token;
            std::string msg = error.what();
            reporter.report(ErrorType::Runtime, token.line, token.column, msg,
                            token.lexeme.length());
        }
    }

    // Public API for executing blocks
    // Used by Callables to execute function body.
    void executeBlock(const std::vector<StmtPtr> &statements, std::shared_ptr<Environment> env);

    // --- ExprVisitor ---
    RuntimeValue visitLiteralExpr(LiteralExpr *expr) override;
    RuntimeValue visitVariableExpr(VariableExpr *expr) override;
    RuntimeValue visitAssignExpr(AssignExpr *expr) override;
    RuntimeValue visitBinaryExpr(BinaryExpr *expr) override;
    RuntimeValue visitCallExpr(CallExpr *expr) override;
    RuntimeValue visitGetExpr(GetExpr *expr) override;
    RuntimeValue visitArrayAccessExpr(ArrayAccessExpr *expr) override;
    RuntimeValue visitArrayLitExpr(ArrayLitExpr *expr) override;
    RuntimeValue visitNewExpr(NewExpr *expr) override;

    // --- StmtVisitor ---
    void visitExpressionStmt(ExpressionStmt *stmt) override;
    void visitPrintStmt(PrintStmt *stmt) override;
    void visitReturnStmt(ReturnStmt *stmt) override;
    void visitBlockStmt(BlockStmt *stmt) override;
    void visitIfStmt(IfStmt *stmt) override;
    void visitWhileStmt(WhileStmt *stmt) override;
    void visitFunctionStmt(FunctionStmt *stmt) override;
    void visitClassStmt(ClassStmt *stmt) override;
    void visitForInStmt(ForInStmt *stmt) override;

    /**
     * Error Reporter
     * Reporter which reports errors at a specific token.
     */
    ErrorReporter &reporter;
private:
    // Execute a statement
    void execute(Stmt *stmt);

    // Evaluate an expression
    RuntimeValue evaluate(Expr *expr);

    // === Truthiness Calculators ===

    bool isTruthy(const RuntimeValue &object);
    bool isEqual(const RuntimeValue &a, const RuntimeValue &b);
    void checkNumberOperand(const Token &operatorToken, const RuntimeValue &operand);
    void checkNumberOperands(const Token &operatorToken, const RuntimeValue &left,
                             const RuntimeValue &right);
};
