#pragma once

#include "ast.hpp"
#include <string>
#include <vector>

/**
 * ASTPrinter - Debugging Tool
 * Outputs an Abstract Syntax Tree in a human-readable, indented tree format.
 * Implements the Visitor pattern to traverse AST nodes without dynamic_cast.
 */
class ASTPrinter : public ExprVisitor, public StmtVisitor {
public:
    /**
     * Entry point for printing the AST.
     * @param statements The vector of top-level statements to print
     */
    void print(const std::vector<StmtPtr> &statements);

    // --- ExprVisitor Implementation ---
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
    void visitExpressionStmt(ExpressionStmt *stmt) override;
    void visitReturnStmt(ReturnStmt *stmt) override;
    void visitBlockStmt(BlockStmt *stmt) override;
    void visitIfStmt(IfStmt *stmt) override;
    void visitWhileStmt(WhileStmt *stmt) override;
    void visitFunctionStmt(FunctionStmt *stmt) override;
    void visitClassStmt(ClassStmt *stmt) override;
    void visitForInStmt(ForInStmt *stmt) override;
    void visitForStmt(ForStmt *stmt) override;

private:
    std::string indent = ""; // Holds the current indentation string

    /**
     * Helper to safely trigger the visitor mechanism for expressions
     * @param expr The expression to visit
     */
    void accept(Expr *expr);

    /**
     * Helper to safely trigger the visitor mechanism for statements
     * @param stmt The statement to visit
     */
    void accept(Stmt *stmt);

    /**
     * RAII Helper Scope
     * Automatically increases visual indentation on construction
     * and decreases it on destruction to maintain tree hierarchy.
     */
    struct IndentScope {
        ASTPrinter &printer;
        std::string oldIndent;

        IndentScope(ASTPrinter &p) : printer(p), oldIndent(p.indent) {
            p.indent += "  | ";
        }

        ~IndentScope() {
            printer.indent = oldIndent;
        }
    };
};
