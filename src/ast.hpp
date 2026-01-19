#pragma once

#include <memory>
#include <vector>

#include "runtime.hpp"
#include "token.hpp"

// --- Forward Declarations of Concrete Nodes ---

// Expressions
struct LiteralExpr;
struct VariableExpr;
struct AssignExpr;
struct BinaryExpr;
struct UnaryExpr;
struct CallExpr;
struct GetExpr;
struct ArrayAccessExpr;
struct ArrayLitExpr;
struct NewExpr;

// Statements
struct ExpressionStmt;
struct ReturnStmt;
struct BlockStmt;
struct IfStmt;
struct FunctionStmt;
struct ClassStmt;
struct WhileStmt;
struct ForInStmt;
struct ForStmt;
struct CaseStmt;
struct RepeatUntilStmt;

// --- Visitor Interfaces ---

/**
 * Expression Visitor Interface
 * Defines the contract for operations that traverse Expression nodes.
 * Expression nodes can return values.
 */
struct ExprVisitor {
    virtual ~ExprVisitor()                                           = default;
    virtual RuntimeValue visitLiteralExpr(LiteralExpr *expr)         = 0;
    virtual RuntimeValue visitVariableExpr(VariableExpr *expr)       = 0;
    virtual RuntimeValue visitAssignExpr(AssignExpr *expr)           = 0;
    virtual RuntimeValue visitBinaryExpr(BinaryExpr *expr)           = 0;
    virtual RuntimeValue visitUnaryExpr(UnaryExpr *expr)             = 0;
    virtual RuntimeValue visitCallExpr(CallExpr *expr)               = 0;
    virtual RuntimeValue visitGetExpr(GetExpr *expr)                 = 0;
    virtual RuntimeValue visitArrayAccessExpr(ArrayAccessExpr *expr) = 0;
    virtual RuntimeValue visitArrayLitExpr(ArrayLitExpr *expr)       = 0;
    virtual RuntimeValue visitNewExpr(NewExpr *expr)                 = 0;
};

/**
 * Statement Visitor Interface
 * Defines the contract for operations that traverse Statement nodes.
 */
struct StmtVisitor {
    virtual ~StmtVisitor()                                   = default;
    virtual void visitExpressionStmt(ExpressionStmt *stmt)   = 0;
    virtual void visitReturnStmt(ReturnStmt *stmt)           = 0;
    virtual void visitBlockStmt(BlockStmt *stmt)             = 0;
    virtual void visitIfStmt(IfStmt *stmt)                   = 0;
    virtual void visitFunctionStmt(FunctionStmt *stmt)       = 0;
    virtual void visitClassStmt(ClassStmt *stmt)             = 0;
    virtual void visitWhileStmt(WhileStmt *stmt)             = 0;
    virtual void visitForInStmt(ForInStmt *stmt)             = 0;
    virtual void visitForStmt(ForStmt *stmt)                 = 0;
    virtual void visitCaseStmt(CaseStmt *stmt)               = 0;
    virtual void visitRepeatUntilStmt(RepeatUntilStmt *stmt) = 0;
};

// --- Base Classes ---

/**
 * Base Expression Class
 * Abstract base for all expression nodes in the AST.
 */
struct Expr {
    virtual ~Expr() = default;

    /**
     * Dispatches the specific visit method on the visitor
     * @param visitor The visitor executing an operation
     */
    virtual void accept(ExprVisitor &visitor) = 0;
};

/**
 * Base Statement Class
 * Abstract base for all statement nodes in the AST.
 */
struct Stmt {
    virtual ~Stmt() = default;

    /**
     * Dispatches the specific visit method on the visitor
     * @param visitor The visitor executing an operation
     */
    virtual void accept(StmtVisitor &visitor) = 0;
};

using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

// --- Expressions Implementations ---

/**
 * Literal Expression
 * Represents constant values like numbers, strings, and booleans.
 */
struct LiteralExpr : Expr {
    Token token;
    LiteralExpr(Token t) : token(t) {
    }
    void accept(ExprVisitor &visitor) override {
        visitor.visitLiteralExpr(this);
    }
};

/**
 * Variable Expression
 * Represents a reference to a variable name.
 */
struct VariableExpr : Expr {
    Token name;
    VariableExpr(Token n) : name(n) {
    }
    void accept(ExprVisitor &visitor) override {
        visitor.visitVariableExpr(this);
    }
};

/**
 * Assignment Expression
 * Represents assigning a value to a target variable or property.
 */
struct AssignExpr : Expr {
    Token anchor;
    ExprPtr target;
    ExprPtr value;
    AssignExpr(Token an, ExprPtr t, ExprPtr v)
        : anchor(an), target(std::move(t)), value(std::move(v)) {
    }
    void accept(ExprVisitor &visitor) override {
        visitor.visitAssignExpr(this);
    }
};

/**
 * Binary Expression
 * Represents operations with two operands (e.g., a + b, x > y).
 */
struct BinaryExpr : Expr {
    ExprPtr left;
    Token op;
    ExprPtr right;
    BinaryExpr(ExprPtr l, Token o, ExprPtr r) : left(std::move(l)), op(o), right(std::move(r)) {
    }
    void accept(ExprVisitor &visitor) override {
        visitor.visitBinaryExpr(this);
    }
};

/**
 * Unary Expression
 * Represents operations with a single operand (e.g., -x, NOT y).
 */
struct UnaryExpr : Expr {
    Token op;
    ExprPtr right;
    UnaryExpr(Token o, ExprPtr r) : op(o), right(std::move(r)) {
    }
    void accept(ExprVisitor &visitor) override {
        visitor.visitUnaryExpr(this);
    }
};

/**
 * Call Expression
 * Represents a function or method call with arguments.
 */
struct CallExpr : Expr {
    Token anchor;
    ExprPtr callee;
    std::vector<ExprPtr> args;
    CallExpr(Token an, ExprPtr c, std::vector<ExprPtr> a)
        : anchor(an), callee(std::move(c)), args(std::move(a)) {
    }
    void accept(ExprVisitor &visitor) override {
        visitor.visitCallExpr(this);
    }
};

/**
 * Get Property Expression
 * Represents accessing a property on an object (e.g., obj.prop).
 */
struct GetExpr : Expr {
    ExprPtr object;
    Token name;
    GetExpr(ExprPtr o, Token n) : object(std::move(o)), name(n) {
    }
    void accept(ExprVisitor &visitor) override {
        visitor.visitGetExpr(this);
    }
};

/**
 * Array Access Expression
 * Represents accessing an array element by index (e.g., arr[i]).
 */
struct ArrayAccessExpr : Expr {
    Token anchor;
    ExprPtr array;
    ExprPtr index;
    ArrayAccessExpr(Token an, ExprPtr a, ExprPtr i)
        : anchor(an), array(std::move(a)), index(std::move(i)) {
    }
    void accept(ExprVisitor &visitor) override {
        visitor.visitArrayAccessExpr(this);
    }
};

/**
 * Array Literal Expression
 * Represents the creation of a new array with inline elements (e.g., [1, 2, 3]).
 */
struct ArrayLitExpr : Expr {
    Token anchor;
    std::vector<ExprPtr> elements;
    ArrayLitExpr(Token a, std::vector<ExprPtr> e) : anchor(a), elements(std::move(e)) {
    }
    void accept(ExprVisitor &visitor) override {
        visitor.visitArrayLitExpr(this);
    }
};

/**
 * New Instance Expression
 * Represents the instantiation of a class.
 */
struct NewExpr : Expr {
    Token className;
    std::vector<ExprPtr> args;
    NewExpr(Token c, std::vector<ExprPtr> a) : className(c), args(std::move(a)) {
    }
    void accept(ExprVisitor &visitor) override {
        visitor.visitNewExpr(this);
    }
};

// --- Statements Implementations ---

/**
 * Expression Statement
 * A statement that evaluates an expression (e.g., function call) effectively discarding the result.
 */
struct ExpressionStmt : Stmt {
    ExprPtr expression;
    ExpressionStmt(ExprPtr e) : expression(std::move(e)) {
    }
    void accept(StmtVisitor &visitor) override {
        visitor.visitExpressionStmt(this);
    }
};

/**
 * Return Statement
 * Exits the current function, optionally returning a value.
 */
struct ReturnStmt : Stmt {
    ExprPtr value;
    ReturnStmt(ExprPtr v) : value(std::move(v)) {
    }
    void accept(StmtVisitor &visitor) override {
        visitor.visitReturnStmt(this);
    }
};

/**
 * Block Statement
 * Represents a scope containing a sequence of statements enclosed in braces.
 */
struct BlockStmt : Stmt {
    std::vector<StmtPtr> statements;
    BlockStmt(std::vector<StmtPtr> s) : statements(std::move(s)) {
    }
    void accept(StmtVisitor &visitor) override {
        visitor.visitBlockStmt(this);
    }
};

/**
 * If Statement
 * Conditionally executes a branch of code based on a boolean expression.
 */
struct IfStmt : Stmt {
    Token keyword;
    ExprPtr condition;
    std::vector<StmtPtr> thenBranch;
    std::vector<StmtPtr> elseBranch;
    IfStmt(Token kw, ExprPtr c, std::vector<StmtPtr> t, std::vector<StmtPtr> e)
        : keyword(kw), condition(std::move(c)), thenBranch(std::move(t)), elseBranch(std::move(e)) {
    }
    void accept(StmtVisitor &visitor) override {
        visitor.visitIfStmt(this);
    }
};

/**
 * While Statement
 * Repeats a body of code while a condition remains true.
 */
struct WhileStmt : Stmt {
    Token keyword;
    ExprPtr condition;
    std::vector<StmtPtr> body;
    WhileStmt(Token kw, ExprPtr c, std::vector<StmtPtr> b)
        : keyword(kw), condition(std::move(c)), body(std::move(b)) {
    }
    void accept(StmtVisitor &visitor) override {
        visitor.visitWhileStmt(this);
    }
};

/**
 * Function Declaration
 * Defines a new reusable function with a name, parameters, and a body.
 */
struct FunctionStmt : Stmt {
    Token name;
    std::vector<Token> params;
    std::vector<StmtPtr> body;
    FunctionStmt(Token n, std::vector<Token> p, std::vector<StmtPtr> b)
        : name(n), params(p), body(std::move(b)) {
    }
    void accept(StmtVisitor &visitor) override {
        visitor.visitFunctionStmt(this);
    }
};

/**
 * Class Declaration
 * Defines a new class with a name, an optional superclass, and methods.
 */
struct ClassStmt : Stmt {
    Token name;
    Token superclass;
    std::vector<StmtPtr> methods;
    std::vector<ExprPtr> attributes;
    ClassStmt(Token n, Token s, std::vector<StmtPtr> m, std::vector<ExprPtr> a)
        : name(n), superclass(s), methods(std::move(m)), attributes(std::move(a)) {
    }
    void accept(StmtVisitor &visitor) override {
        visitor.visitClassStmt(this);
    }
};

/**
 * For-In Statement
 * Represents a collection loop:
 * FOR item IN collection
 *     body
 * END FOR
 */
struct ForInStmt : Stmt {
    Token keyword;
    Token variable;
    ExprPtr iterable;
    std::vector<StmtPtr> body;

    ForInStmt(Token kw, Token var, ExprPtr iter, std::vector<StmtPtr> b)
        : keyword(kw), variable(var), iterable(std::move(iter)), body(std::move(b)) {
    }

    void accept(StmtVisitor &visitor) override {
        visitor.visitForInStmt(this);
    }
};

/**
 * For-To Statement
 * Represents a range loop:
 * FOR item = start TO end
 *     body
 * END FOR
 */
struct ForStmt : Stmt {
    Token keyword;
    Token variable;
    ExprPtr start;
    ExprPtr end;
    std::vector<StmtPtr> body;

    ForStmt(Token kw, Token var, ExprPtr start, ExprPtr end, std::vector<StmtPtr> b)
        : keyword(kw), variable(var), start(std::move(start)), end(std::move(end)),
          body(std::move(b)) {
    }

    void accept(StmtVisitor &visitor) override {
        visitor.visitForStmt(this);
    }
};

/**
 * Case Arm
 * Represents a single branch in a CASE statement.
 */
struct CaseArm {
    Token colon;
    std::vector<ExprPtr> values;
    std::vector<StmtPtr> body;
};

/**
 * Case Statement
 * CASE condition OF
 *     value1: body1
 *     value2: body2
 *     OTHERWISE: defaultBody
 * END CASE
 */
struct CaseStmt : Stmt {
    Token keyword;
    ExprPtr condition;
    std::vector<CaseArm> arms;
    std::vector<StmtPtr> defaultBranch;

    CaseStmt(Token kw, ExprPtr cond, std::vector<CaseArm> cases, std::vector<StmtPtr> def)
        : keyword(kw), condition(std::move(cond)), arms(std::move(cases)),
          defaultBranch(std::move(def)) {
    }

    void accept(StmtVisitor &visitor) override {
        visitor.visitCaseStmt(this);
    }
};

/**
 * Repeat-Until Statement
 * REPEAT
 *     body
 * UNTIL condition
 */
struct RepeatUntilStmt : Stmt {
    Token keyword;
    std::vector<StmtPtr> body;
    ExprPtr condition;

    RepeatUntilStmt(Token kw, std::vector<StmtPtr> b, ExprPtr cond)
        : keyword(kw), body(std::move(b)), condition(std::move(cond)) {
    }

    void accept(StmtVisitor &visitor) override {
        visitor.visitRepeatUntilStmt(this);
    }
};
