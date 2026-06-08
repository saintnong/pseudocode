#pragma once

#include "ast.hpp"
#include "chunk.hpp"
#include "errors.hpp"
#include <memory>
#include <string>
#include <vector>

struct Local {
    std::string name;
    int depth;
    int slot;
};

class Compiler {
private:
    ErrorReporter &reporter;
    std::vector<Local> locals;
    int scopeDepth             = 0;
    int nextLocalSlot          = 0;
    bool lastAssignWasNewLocal = false;

    Compiler *enclosing = nullptr;
    std::shared_ptr<CompiledFunction> currentFn;

    Chunk &currentChunk() {
        return *currentFn->chunk;
    }

    void emitByte(uint8_t byte, Span span);
    void emitShort(uint16_t value, Span span);
    void emitConstant(RuntimeValue value, Span span);
    size_t emitJump(uint8_t instruction, Span span);
    void patchJump(size_t offset, Span span);
    void emitLoop(size_t loopStart, Span span);

    void beginScope();
    void endScope(Span span);
    int addLocal(const std::string &name, Span span, bool reserveSlot = false);
    int resolveLocal(const std::string &name);

    // Expression compilation (manual dispatch)
    void compileExpression(Expr *expr);
    void compileLiteralExpr(LiteralExpr *expr);
    void compileVariableExpr(VariableExpr *expr);
    void compileAssignExpr(AssignExpr *expr);
    void compileBinaryExpr(BinaryExpr *expr);
    void compileUnaryExpr(UnaryExpr *expr);
    void compileCallExpr(CallExpr *expr);
    void compileGetExpr(GetExpr *expr);
    void compileArrayAccessExpr(ArrayAccessExpr *expr);
    void compileArrayLitExpr(ArrayLitExpr *expr);
    void compileDictLitExpr(DictLitExpr *expr);
    void compileNewExpr(NewExpr *expr);

    // Statement compilation (manual dispatch)
    void compileStatement(Stmt *stmt);
    void compileExpressionStmt(ExpressionStmt *stmt);
    void compileReturnStmt(ReturnStmt *stmt);
    void compileBlockStmt(BlockStmt *stmt);
    void compileIfStmt(IfStmt *stmt);
    void compileFunctionStmt(FunctionStmt *stmt);
    void compileClassStmt(ClassStmt *stmt);
    void compileWhileStmt(WhileStmt *stmt);
    void compileForInStmt(ForInStmt *stmt);
    void compileForStmt(ForStmt *stmt);
    void compileCaseStmt(CaseStmt *stmt);
    void compileRepeatUntilStmt(RepeatUntilStmt *stmt);

    void errorAt(Span span, const std::string &message);

public:
    Compiler(ErrorReporter &reporter, Compiler *enclosing = nullptr, std::string fnName = "");

    std::shared_ptr<CompiledFunction> compile(const std::vector<StmtPtr> &statements);
    std::shared_ptr<CompiledFunction> compileExpressionOnly(Expr *expr);
};
