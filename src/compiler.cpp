#include "compiler.hpp"
#include "opcodes.hpp"
#include <iostream>
#include <stdexcept>

Compiler::Compiler(ErrorReporter &reporter, Compiler *enclosing, std::string fnName)
    : reporter(reporter), enclosing(enclosing) {
    currentFn        = std::make_shared<CompiledFunction>();
    currentFn->chunk = std::make_shared<Chunk>();
    currentFn->arity = 0;
    currentFn->name  = fnName;
}

static Span defaultSpan() {
    return {1, 0, 1};
}

static Span getExprSpan(Expr *expr) {
    if (auto *lit = dynamic_cast<LiteralExpr *>(expr)) {
        return lit->token.span;
    }
    if (auto *var = dynamic_cast<VariableExpr *>(expr)) {
        return var->name.span;
    }
    if (auto *assign = dynamic_cast<AssignExpr *>(expr)) {
        Span lhs = getExprSpan(assign->target.get());
        Span rhs = getExprSpan(assign->value.get());
        return {lhs.line, lhs.start, rhs.end};
    }
    if (auto *bin = dynamic_cast<BinaryExpr *>(expr)) {
        Span lhs = getExprSpan(bin->left.get());
        Span rhs = getExprSpan(bin->right.get());
        return {lhs.line, lhs.start, rhs.end};
    }
    if (auto *un = dynamic_cast<UnaryExpr *>(expr)) {
        Span operand = getExprSpan(un->right.get());
        return {un->op.span.line, un->op.span.start, operand.end};
    }
    if (auto *call = dynamic_cast<CallExpr *>(expr)) {
        Span span = getExprSpan(call->callee.get());
        if (!call->args.empty()) {
            span.end = getExprSpan(call->args.back().get()).end;
        }
        return span;
    }
    if (auto *get = dynamic_cast<GetExpr *>(expr)) {
        Span obj = getExprSpan(get->object.get());
        return {obj.line, obj.start, get->name.span.end};
    }
    if (auto *arr = dynamic_cast<ArrayAccessExpr *>(expr)) {
        Span obj = getExprSpan(arr->array.get());
        return {obj.line, obj.start, arr->anchor.span.end};
    }
    if (auto *arrLit = dynamic_cast<ArrayLitExpr *>(expr)) {
        return arrLit->anchor.span;
    }
    if (auto *dictLit = dynamic_cast<DictLitExpr *>(expr)) {
        return dictLit->anchor.span;
    }
    if (auto *newExpr = dynamic_cast<NewExpr *>(expr)) {
        Span span = newExpr->className.span;
        if (!newExpr->args.empty()) {
            span.end = getExprSpan(newExpr->args.back().get()).end;
        }
        return span;
    }
    return {1, 0, 1};
}

std::shared_ptr<CompiledFunction> Compiler::compile(const std::vector<StmtPtr> &statements) {
    for (const auto &stmt : statements) {
        if (stmt) {
            compileStatement(stmt.get());
        }
    }
    Span last = currentChunk().spans.empty() ? defaultSpan() : currentChunk().spans.back();
    emitByte(OP_NULL, last);
    emitByte(OP_RETURN, last);
    return currentFn;
}

std::shared_ptr<CompiledFunction> Compiler::compileExpressionOnly(Expr *expr) {
    compileExpression(expr);
    emitByte(OP_RETURN, defaultSpan());
    return currentFn;
}

void Compiler::emitByte(uint8_t byte, Span span) {
    currentChunk().write(byte, span);
}

void Compiler::emitShort(uint16_t value, Span span) {
    emitByte((value >> 8) & 0xff, span);
    emitByte(value & 0xff, span);
}

void Compiler::emitConstant(RuntimeValue value, Span span) {
    size_t index = currentChunk().addConstant(value);
    if (index > 65535) {
        errorAt(span, "Too many constants in one chunk.");
    }
    emitByte(OP_CONSTANT, span);
    emitShort(static_cast<uint16_t>(index), span);
}

size_t Compiler::emitJump(uint8_t instruction, Span span) {
    emitByte(instruction, span);
    emitByte(0xff, span);
    emitByte(0xff, span);
    return currentChunk().code.size() - 2;
}

void Compiler::patchJump(size_t offset, Span span) {
    size_t jump = currentChunk().code.size() - offset - 2;
    if (jump > 65535) {
        errorAt(span, "Too much code to jump over.");
    }
    currentChunk().code[offset]     = (jump >> 8) & 0xff;
    currentChunk().code[offset + 1] = jump & 0xff;
}

void Compiler::emitLoop(size_t loopStart, Span span) {
    emitByte(OP_LOOP, span);
    size_t offset = currentChunk().code.size() - loopStart + 2;
    if (offset > 65535) {
        errorAt(span, "Loop body too large.");
    }
    emitShort(static_cast<uint16_t>(offset), span);
}

void Compiler::beginScope() {
    scopeDepth++;
}

void Compiler::endScope(Span span) {
    scopeDepth--;
    while (!locals.empty() && locals.back().depth > scopeDepth) {
        emitByte(OP_POP, span);
        locals.pop_back();
        nextLocalSlot--;
    }
}

int Compiler::addLocal(const std::string &name, Span span, bool reserveSlot) {
    for (int i = static_cast<int>(locals.size()) - 1; i >= 0; i--) {
        if (locals[i].name == name) {
            return locals[i].slot;
        }
    }

    Local local;
    local.name  = name;
    local.depth = scopeDepth;
    local.slot  = nextLocalSlot++;
    locals.push_back(local);
    if (reserveSlot) {
        emitByte(OP_NULL, span);
    }
    return local.slot;
}

int Compiler::resolveLocal(const std::string &name) {
    for (int i = static_cast<int>(locals.size()) - 1; i >= 0; i--) {
        if (locals[i].name == name) {
            return locals[i].slot;
        }
    }
    return -1;
}

void Compiler::errorAt(Span span, const std::string &message) {
    reporter.report(ErrorType::Syntax, span, message);
}

// =================================================================================================
//           Expression Compilation
// =================================================================================================

void Compiler::compileExpression(Expr *expr) {
    if (!expr)
        return;
    if (auto *litExpr = dynamic_cast<LiteralExpr *>(expr)) {
        compileLiteralExpr(litExpr);
    } else if (auto *varExpr = dynamic_cast<VariableExpr *>(expr)) {
        compileVariableExpr(varExpr);
    } else if (auto *assignExpr = dynamic_cast<AssignExpr *>(expr)) {
        compileAssignExpr(assignExpr);
    } else if (auto *binExpr = dynamic_cast<BinaryExpr *>(expr)) {
        compileBinaryExpr(binExpr);
    } else if (auto *unExpr = dynamic_cast<UnaryExpr *>(expr)) {
        compileUnaryExpr(unExpr);
    } else if (auto *callExpr = dynamic_cast<CallExpr *>(expr)) {
        compileCallExpr(callExpr);
    } else if (auto *getExpr = dynamic_cast<GetExpr *>(expr)) {
        compileGetExpr(getExpr);
    } else if (auto *arrAccExpr = dynamic_cast<ArrayAccessExpr *>(expr)) {
        compileArrayAccessExpr(arrAccExpr);
    } else if (auto *arrLitExpr = dynamic_cast<ArrayLitExpr *>(expr)) {
        compileArrayLitExpr(arrLitExpr);
    } else if (auto *dictLitExpr = dynamic_cast<DictLitExpr *>(expr)) {
        compileDictLitExpr(dictLitExpr);
    } else if (auto *newExpr = dynamic_cast<NewExpr *>(expr)) {
        compileNewExpr(newExpr);
    }
}

void Compiler::compileLiteralExpr(LiteralExpr *expr) {
    Token token = expr->token;
    Span span   = token.span;

    if (token.type == TOK_INTEGER) {
        RuntimeValue val;
        val.value = std::stoi(token.lexeme);
        emitConstant(val, span);
    } else if (token.type == TOK_FLOAT) {
        RuntimeValue val;
        val.value = std::stod(token.lexeme);
        emitConstant(val, span);
    } else if (token.type == TOK_STRING) {
        RuntimeValue val;
        val.value = token.lexeme;
        emitConstant(val, span);
    } else if (token.type == TOK_TRUE) {
        emitByte(OP_TRUE, span);
    } else if (token.type == TOK_FALSE) {
        emitByte(OP_FALSE, span);
    } else {
        emitByte(OP_NULL, span);
    }
}

void Compiler::compileVariableExpr(VariableExpr *expr) {
    std::string name = expr->name.lexeme;
    Span span        = expr->name.span;

    int slot = resolveLocal(name);
    if (slot != -1) {
        emitByte(OP_GET_LOCAL, span);
        emitShort(static_cast<uint16_t>(slot), span);
    } else {
        RuntimeValue val;
        val.value      = name;
        size_t nameIdx = currentChunk().addConstant(val);
        emitByte(OP_GET_GLOBAL, span);
        emitShort(static_cast<uint16_t>(nameIdx), span);
    }
}

void Compiler::compileAssignExpr(AssignExpr *expr) {
    Span span = expr->anchor.span;

    if (auto *varExpr = dynamic_cast<VariableExpr *>(expr->target.get())) {
        std::string name = varExpr->name.lexeme;

        // Check if variable already exists as a local
        int slot = resolveLocal(name);

        if (slot == -1 && enclosing != nullptr) {
            // New local variable inside a function — the compiled value
            // becomes the local's stack slot directly (no OP_SET_LOCAL needed)
            compileExpression(expr->value.get());
            addLocal(name, span);
            lastAssignWasNewLocal = true;
            return;
        }

        compileExpression(expr->value.get());

        if (slot != -1) {
            emitByte(OP_SET_LOCAL, span);
            emitShort(static_cast<uint16_t>(slot), span);
        } else {
            RuntimeValue val;
            val.value      = name;
            size_t nameIdx = currentChunk().addConstant(val);
            emitByte(OP_SET_GLOBAL, span);
            emitShort(static_cast<uint16_t>(nameIdx), span);
        }
    } else if (auto *getExpr = dynamic_cast<GetExpr *>(expr->target.get())) {
        compileExpression(getExpr->object.get());
        compileExpression(expr->value.get());

        RuntimeValue val;
        val.value      = getExpr->name.lexeme;
        size_t nameIdx = currentChunk().addConstant(val);
        emitByte(OP_SET_PROPERTY, span);
        emitShort(static_cast<uint16_t>(nameIdx), span);
    } else if (auto *arrayAccess = dynamic_cast<ArrayAccessExpr *>(expr->target.get())) {
        compileExpression(arrayAccess->array.get());
        compileExpression(arrayAccess->index.get());
        compileExpression(expr->value.get());
        emitByte(OP_INDEX_SET, span);
    }
}

void Compiler::compileBinaryExpr(BinaryExpr *expr) {
    Span span = getExprSpan(expr);

    if (expr->op.type == TOK_AND) {
        compileExpression(expr->left.get());
        size_t endJump = emitJump(OP_AND_JUMP, span);
        emitByte(OP_POP, span);
        compileExpression(expr->right.get());
        patchJump(endJump, span);
        return;
    }
    if (expr->op.type == TOK_OR) {
        compileExpression(expr->left.get());
        size_t endJump = emitJump(OP_OR_JUMP, span);
        emitByte(OP_POP, span);
        compileExpression(expr->right.get());
        patchJump(endJump, span);
        return;
    }

    compileExpression(expr->left.get());
    compileExpression(expr->right.get());

    switch (expr->op.type) {
    case TOK_PLUS:
        emitByte(OP_ADD, span);
        break;
    case TOK_MINUS:
        emitByte(OP_SUBTRACT, span);
        break;
    case TOK_MULTIPLY:
        emitByte(OP_MULTIPLY, span);
        break;
    case TOK_DIVIDE:
        emitByte(OP_DIVIDE, span);
        break;
    case TOK_MOD:
        emitByte(OP_MOD, span);
        break;
    case TOK_EQUAL:
        emitByte(OP_EQUAL, span);
        break;
    case TOK_NOT_EQUAL:
        emitByte(OP_NOT_EQUAL, span);
        break;
    case TOK_GREATER_THAN:
        emitByte(OP_GREATER, span);
        break;
    case TOK_GT_OR_EQ:
        emitByte(OP_GREATER_EQUAL, span);
        break;
    case TOK_LESS_THAN:
        emitByte(OP_LESS, span);
        break;
    case TOK_LT_OR_EQ:
        emitByte(OP_LESS_EQUAL, span);
        break;
    case TOK_IN:
        emitByte(OP_IN, span);
        break;
    default:
        errorAt(expr->op.span, "Unknown binary operator.");
    }
}

void Compiler::compileUnaryExpr(UnaryExpr *expr) {
    Span span = getExprSpan(expr);
    compileExpression(expr->right.get());

    switch (expr->op.type) {
    case TOK_MINUS:
        emitByte(OP_NEGATE, span);
        break;
    case TOK_NOT:
        emitByte(OP_NOT, span);
        break;
    default:
        errorAt(expr->op.span, "Unknown unary operator.");
    }
}

void Compiler::compileCallExpr(CallExpr *expr) {
    Span span = getExprSpan(expr);
    compileExpression(expr->callee.get());

    for (const auto &arg : expr->args) {
        compileExpression(arg.get());
    }

    emitByte(OP_CALL, span);
    emitByte(static_cast<uint8_t>(expr->args.size()), span);
}

void Compiler::compileGetExpr(GetExpr *expr) {
    Span span = getExprSpan(expr);
    compileExpression(expr->object.get());

    RuntimeValue val;
    val.value      = expr->name.lexeme;
    size_t nameIdx = currentChunk().addConstant(val);
    emitByte(OP_GET_PROPERTY, span);
    emitShort(static_cast<uint16_t>(nameIdx), span);
}

void Compiler::compileArrayAccessExpr(ArrayAccessExpr *expr) {
    Span span = getExprSpan(expr);
    compileExpression(expr->array.get());
    compileExpression(expr->index.get());
    emitByte(OP_INDEX_GET, span);
}

void Compiler::compileArrayLitExpr(ArrayLitExpr *expr) {
    Span span = getExprSpan(expr);
    for (const auto &elem : expr->elements) {
        compileExpression(elem.get());
    }
    emitByte(OP_ARRAY, span);
    emitShort(static_cast<uint16_t>(expr->elements.size()), span);
}

void Compiler::compileDictLitExpr(DictLitExpr *expr) {
    Span span = getExprSpan(expr);
    for (const auto &entry : expr->entries) {
        compileExpression(entry.key.get());
        compileExpression(entry.value.get());
    }
    emitByte(OP_DICT, span);
    emitShort(static_cast<uint16_t>(expr->entries.size()), span);
}

void Compiler::compileNewExpr(NewExpr *expr) {
    Span span = getExprSpan(expr);
    for (const auto &arg : expr->args) {
        compileExpression(arg.get());
    }
    RuntimeValue val;
    val.value      = expr->className.lexeme;
    size_t nameIdx = currentChunk().addConstant(val);
    emitByte(OP_NEW_INSTANCE, span);
    emitShort(static_cast<uint16_t>(nameIdx), span);
    emitByte(static_cast<uint8_t>(expr->args.size()), span);
}

// =================================================================================================
//           Statement Compilation
// =================================================================================================

void Compiler::compileStatement(Stmt *stmt) {
    if (!stmt)
        return;
    if (auto *exprStmt = dynamic_cast<ExpressionStmt *>(stmt)) {
        compileExpressionStmt(exprStmt);
    } else if (auto *retStmt = dynamic_cast<ReturnStmt *>(stmt)) {
        compileReturnStmt(retStmt);
    } else if (auto *blockStmt = dynamic_cast<BlockStmt *>(stmt)) {
        compileBlockStmt(blockStmt);
    } else if (auto *ifStmt = dynamic_cast<IfStmt *>(stmt)) {
        compileIfStmt(ifStmt);
    } else if (auto *funcStmt = dynamic_cast<FunctionStmt *>(stmt)) {
        compileFunctionStmt(funcStmt);
    } else if (auto *classStmt = dynamic_cast<ClassStmt *>(stmt)) {
        compileClassStmt(classStmt);
    } else if (auto *whileStmt = dynamic_cast<WhileStmt *>(stmt)) {
        compileWhileStmt(whileStmt);
    } else if (auto *forInStmt = dynamic_cast<ForInStmt *>(stmt)) {
        compileForInStmt(forInStmt);
    } else if (auto *forStmt = dynamic_cast<ForStmt *>(stmt)) {
        compileForStmt(forStmt);
    } else if (auto *caseStmt = dynamic_cast<CaseStmt *>(stmt)) {
        compileCaseStmt(caseStmt);
    } else if (auto *repeatStmt = dynamic_cast<RepeatUntilStmt *>(stmt)) {
        compileRepeatUntilStmt(repeatStmt);
    }
}

void Compiler::compileExpressionStmt(ExpressionStmt *stmt) {
    lastAssignWasNewLocal = false;
    compileExpression(stmt->expression.get());
    if (lastAssignWasNewLocal) {
        // The expression created a new local variable — the pushed value
        // IS the local's stack slot, so don't pop it
        lastAssignWasNewLocal = false;
    } else {
        Span last = currentChunk().spans.empty() ? defaultSpan() : currentChunk().spans.back();
        emitByte(OP_POP, last);
    }
}

void Compiler::compileReturnStmt(ReturnStmt *stmt) {
    Span last = currentChunk().spans.empty() ? defaultSpan() : currentChunk().spans.back();
    if (stmt->value) {
        compileExpression(stmt->value.get());
    } else {
        emitByte(OP_NULL, last);
    }
    emitByte(OP_RETURN, last);
}

void Compiler::compileBlockStmt(BlockStmt *stmt) {
    for (const auto &s : stmt->statements) {
        compileStatement(s.get());
    }
}

void Compiler::compileIfStmt(IfStmt *stmt) {
    Span span = stmt->keyword.span;

    compileExpression(stmt->condition.get());
    size_t thenJump = emitJump(OP_JUMP_IF_FALSE, span);

    beginScope();
    for (const auto &s : stmt->thenBranch) {
        compileStatement(s.get());
    }
    endScope(span);

    std::vector<size_t> endJumps;
    size_t endJump = emitJump(OP_JUMP, span);
    endJumps.push_back(endJump);

    patchJump(thenJump, span);

    for (const auto &elseIf : stmt->elseIfBranches) {
        compileExpression(elseIf.condition.get());
        size_t nextJump = emitJump(OP_JUMP_IF_FALSE, span);

        beginScope();
        for (const auto &s : elseIf.body) {
            compileStatement(s.get());
        }
        endScope(span);

        size_t skipJump = emitJump(OP_JUMP, span);
        endJumps.push_back(skipJump);

        patchJump(nextJump, span);
    }

    beginScope();
    for (const auto &s : stmt->elseBranch) {
        compileStatement(s.get());
    }
    endScope(span);

    for (size_t jump : endJumps) {
        patchJump(jump, span);
    }
}

void Compiler::compileFunctionStmt(FunctionStmt *stmt) {
    Span span         = stmt->name.span;
    std::string name  = stmt->name.lexeme;

    Compiler subCompiler(reporter, this, name);
    subCompiler.currentFn->arity = static_cast<int>(stmt->params.size());

    for (const auto &param : stmt->params) {
        subCompiler.addLocal(param.lexeme, span, false);
    }

    auto compiled = subCompiler.compile(stmt->body);

    RuntimeValue funcVal;
    funcVal.value =
        std::static_pointer_cast<Callable>(std::make_shared<UserFunction>(compiled, nullptr));
    size_t constIdx = currentChunk().addConstant(funcVal);

    if (enclosing != nullptr) {
        emitByte(OP_CLOSURE, span);
        emitShort(static_cast<uint16_t>(constIdx), span);
        addLocal(name, span);
    } else {
        emitByte(OP_CLOSURE, span);
        emitShort(static_cast<uint16_t>(constIdx), span);

        RuntimeValue nameVal;
        nameVal.value  = name;
        size_t nameIdx = currentChunk().addConstant(nameVal);
        emitByte(OP_DEFINE_GLOBAL, span);
        emitShort(static_cast<uint16_t>(nameIdx), span);
    }
}

void Compiler::compileClassStmt(ClassStmt *stmt) {
    Span span           = stmt->name.span;
    std::string className = stmt->name.lexeme;

    RuntimeValue classNameVal;
    classNameVal.value = className;
    size_t nameIdx     = currentChunk().addConstant(classNameVal);

    emitByte(OP_CLASS, span);
    emitShort(static_cast<uint16_t>(nameIdx), span);

    if (stmt->superclass.type != TOK_EOF) {
        VariableExpr superVar(stmt->superclass);
        compileVariableExpr(&superVar);
        emitByte(OP_INHERIT, span);
    }

    for (const auto &attr : stmt->attributes) {
        if (auto *assignment = dynamic_cast<AssignExpr *>(attr.get())) {
            if (auto *varExpr = dynamic_cast<VariableExpr *>(assignment->target.get())) {
                std::string fieldName = varExpr->name.lexeme;

                Compiler subCompiler(reporter, this, className + "::" + fieldName + "_init");
                auto compiledInit = subCompiler.compileExpressionOnly(assignment->value.get());

                RuntimeValue initVal;
                initVal.value = std::static_pointer_cast<Callable>(
                    std::make_shared<UserFunction>(compiledInit, nullptr));
                size_t initConstIdx = currentChunk().addConstant(initVal);

                emitByte(OP_CLOSURE, span);
                emitShort(static_cast<uint16_t>(initConstIdx), span);

                RuntimeValue attrNameVal;
                attrNameVal.value  = fieldName;
                size_t attrNameIdx = currentChunk().addConstant(attrNameVal);

                emitByte(OP_ATTRIBUTE, span);
                emitShort(static_cast<uint16_t>(attrNameIdx), span);
            }
        }
    }

    for (const auto &method : stmt->methods) {
        if (auto *funcStmt = dynamic_cast<FunctionStmt *>(method.get())) {
            std::string methodName = funcStmt->name.lexeme;

            Compiler subCompiler(reporter, this, className + "::" + methodName);
            subCompiler.currentFn->arity = static_cast<int>(funcStmt->params.size());

            for (const auto &param : funcStmt->params) {
                subCompiler.addLocal(param.lexeme, span, false);
            }

            auto compiledMethod = subCompiler.compile(funcStmt->body);

            RuntimeValue methodVal;
            methodVal.value = std::static_pointer_cast<Callable>(
                std::make_shared<UserFunction>(compiledMethod, nullptr));
            size_t methodConstIdx = currentChunk().addConstant(methodVal);

            emitByte(OP_CLOSURE, span);
            emitShort(static_cast<uint16_t>(methodConstIdx), span);

            RuntimeValue methodNameVal;
            methodNameVal.value  = methodName;
            size_t methodNameIdx = currentChunk().addConstant(methodNameVal);

            emitByte(OP_METHOD, span);
            emitShort(static_cast<uint16_t>(methodNameIdx), span);
        }
    }

    if (enclosing != nullptr) {
        addLocal(className, span);
    } else {
        emitByte(OP_DEFINE_GLOBAL, span);
        emitShort(static_cast<uint16_t>(nameIdx), span);
    }
}

void Compiler::compileWhileStmt(WhileStmt *stmt) {
    Span span       = stmt->keyword.span;
    size_t loopStart = currentChunk().code.size();

    compileExpression(stmt->condition.get());
    size_t exitJump = emitJump(OP_JUMP_IF_FALSE, span);

    beginScope();
    for (const auto &s : stmt->body) {
        compileStatement(s.get());
    }
    endScope(span);

    emitLoop(loopStart, span);
    patchJump(exitJump, span);
}

void Compiler::compileForInStmt(ForInStmt *stmt) {
    Span span = stmt->keyword.span;

    // Reserve 4 stack slots for loop variables (var, arr copy, index, size)
    int varSlot = addLocal(stmt->variable.lexeme, span, true);
    addLocal("__arr_" + stmt->variable.lexeme, span, true);
    addLocal("__idx_" + stmt->variable.lexeme, span, true);
    addLocal("__size_" + stmt->variable.lexeme, span, true);

    // Push iterable value on top of reserved slots
    compileExpression(stmt->iterable.get());

    // OP_FOR_IN_PREPARE pops the iterable and fills the 4 reserved slots
    emitByte(OP_FOR_IN_PREPARE, span);
    emitShort(static_cast<uint16_t>(varSlot), span);

    size_t loopStart = currentChunk().code.size();

    beginScope();
    for (const auto &s : stmt->body) {
        compileStatement(s.get());
    }
    endScope(span);

    emitByte(OP_FOR_IN_LOOP, span);
    emitShort(static_cast<uint16_t>(varSlot), span);
    emitShort(static_cast<uint16_t>(loopStart), span);

    // Clean up the 4 local slots
    emitByte(OP_POP, span);
    emitByte(OP_POP, span);
    emitByte(OP_POP, span);
    emitByte(OP_POP, span);
    for (int i = 0; i < 4; i++) {
        if (!locals.empty()) {
            locals.pop_back();
            nextLocalSlot--;
        }
    }
}

void Compiler::compileForStmt(ForStmt *stmt) {
    Span span = stmt->keyword.span;

    // Push start value onto stack → becomes the loop variable's slot
    compileExpression(stmt->start.get());
    int varSlot = addLocal(stmt->variable.lexeme, span);

    // Push end value onto stack → becomes the limit slot
    compileExpression(stmt->end.get());
    int limitSlot = addLocal("__limit_" + stmt->variable.lexeme, span);

    // Push placeholder for step → becomes the step slot
    emitByte(OP_NULL, span);
    addLocal("__step_" + stmt->variable.lexeme, span);

    emitByte(OP_FOR_PREPARE, span);
    emitShort(static_cast<uint16_t>(varSlot), span);
    size_t exitJump = emitJump(OP_JUMP_IF_FALSE, span);

    size_t loopStart = currentChunk().code.size();

    beginScope();
    for (const auto &s : stmt->body) {
        compileStatement(s.get());
    }
    endScope(span);

    emitByte(OP_FOR_LOOP, span);
    emitShort(static_cast<uint16_t>(varSlot), span);
    emitShort(static_cast<uint16_t>(loopStart), span);

    patchJump(exitJump, span);

    // Clean up the 3 local slots (var, limit, step)
    emitByte(OP_POP, span);
    emitByte(OP_POP, span);
    emitByte(OP_POP, span);
    // Remove locals from compiler tracking
    for (int i = 0; i < 3; i++) {
        if (!locals.empty()) {
            locals.pop_back();
            nextLocalSlot--;
        }
    }

    (void) limitSlot;
}

void Compiler::compileCaseStmt(CaseStmt *stmt) {
    Span span = stmt->keyword.span;

    compileExpression(stmt->condition.get());

    std::vector<size_t> endJumps;

    for (const auto &arm : stmt->arms) {
        std::vector<size_t> bodyJumps;
        size_t nextArmJump = 0;

        for (size_t i = 0; i < arm.values.size(); ++i) {
            emitByte(OP_DUP, span);
            compileExpression(arm.values[i].get());
            emitByte(OP_EQUAL, span);

            if (i < arm.values.size() - 1) {
                size_t nextValJump = emitJump(OP_JUMP_IF_FALSE, span);
                bodyJumps.push_back(emitJump(OP_JUMP, span));
                patchJump(nextValJump, span);
            } else {
                nextArmJump = emitJump(OP_JUMP_IF_FALSE, span);
            }
        }

        for (size_t jump : bodyJumps) {
            patchJump(jump, span);
        }

        emitByte(OP_POP, span);
        for (const auto &s : arm.body) {
            compileStatement(s.get());
        }

        endJumps.push_back(emitJump(OP_JUMP, span));
        patchJump(nextArmJump, span);
    }

    emitByte(OP_POP, span);
    for (const auto &s : stmt->defaultBranch) {
        compileStatement(s.get());
    }

    for (size_t jump : endJumps) {
        patchJump(jump, span);
    }
}

void Compiler::compileRepeatUntilStmt(RepeatUntilStmt *stmt) {
    Span span       = stmt->keyword.span;
    size_t loopStart = currentChunk().code.size();

    beginScope();
    for (const auto &s : stmt->body) {
        compileStatement(s.get());
    }
    endScope(span);

    compileExpression(stmt->condition.get());
    emitByte(OP_NOT, span);
    size_t exitJump = emitJump(OP_JUMP_IF_FALSE, span);
    emitLoop(loopStart, span);
    patchJump(exitJump, span);
}
