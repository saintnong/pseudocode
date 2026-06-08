#include "ast_printer.hpp"
#include <iostream>

// --- Entry Point & Helpers ---

/**
 * Entry point for printing the AST
 * Iterates through a list of statements and triggers the visitor pattern for each.
 * @param statements The vector of statement pointers to process
 */
void ASTPrinter::print(const std::vector<StmtPtr> &statements) {
    std::cout << "AST Root" << std::endl;
    for (const auto &stmt : statements) {
        accept(stmt.get());
    }
}

/**
 * Helper to accept an Expression visitor
 * Checks for null pointers before delegating to the node's accept method.
 * @param expr Raw pointer to the expression node
 */
void ASTPrinter::accept(Expr *expr) {
    if (expr)
        expr->accept(*this);
}

/**
 * Helper to accept a Statement visitor
 * Checks for null pointers before delegating to the node's accept method.
 * @param stmt Raw pointer to the statement node
 */
void ASTPrinter::accept(Stmt *stmt) {
    if (stmt)
        stmt->accept(*this);
}

// --- Statement Visitors ---

/**
 * Visit a Class Declaration
 * Prints the class name, optional superclass, and recursively prints methods.
 * @param stmt Pointer to the class statement node
 */
void ASTPrinter::visitClassStmt(ClassStmt *stmt) {
    std::cout << indent << "[Class] " << stmt->name.lexeme;
    if (stmt->superclass.type != TOK_EOF) {
        std::cout << " < " << stmt->superclass.lexeme;
    }
    std::cout << std::endl;

    IndentScope scope(*this);

    // Attributes
    for (const auto &attr : stmt->attributes) {
        accept(attr.get());
    }

    // Methods
    for (const auto &method : stmt->methods) {
        accept(method.get());
    }
}

/**
 * Visit a Function Declaration
 * Prints the function signature and recursively prints the body statements.
 * @param stmt Pointer to the function statement node
 */
void ASTPrinter::visitFunctionStmt(FunctionStmt *stmt) {
    std::cout << indent << "[Function] " << stmt->name.lexeme << "(";
    for (size_t i = 0; i < stmt->params.size(); ++i) {
        std::cout << stmt->params[i].lexeme << (i < stmt->params.size() - 1 ? ", " : "");
    }
    std::cout << ")" << std::endl;

    IndentScope scope(*this);
    for (const auto &bodyStmt : stmt->body) {
        accept(bodyStmt.get());
    }
}

/**
 * Visit an If-Else Statement
 * Prints the structure of the conditional logic, including the condition,
 * the 'then' branch, and the optional 'else' branch.
 * @param stmt Pointer to the if-statement node
 */
void ASTPrinter::visitIfStmt(IfStmt *stmt) {
    std::cout << indent << "[If]" << std::endl;

    IndentScope scope(*this);

    std::cout << indent << "Condition:" << std::endl;
    {
        IndentScope condScope(*this);
        accept(stmt->condition.get());
    }

    std::cout << indent << "Then:" << std::endl;
    {
        IndentScope thenScope(*this);
        for (const auto &st : stmt->thenBranch)
            accept(st.get());
    }

    for (const auto &elseIf : stmt->elseIfBranches) {
        std::cout << indent << "Else If Condition:" << std::endl;
        {
            IndentScope condScope(*this);
            accept(elseIf.condition.get());
        }
        std::cout << indent << "Else If Body:" << std::endl;
        {
            IndentScope bodyScope(*this);
            for (const auto &st : elseIf.body)
                accept(st.get());
        }
    }

    if (!stmt->elseBranch.empty()) {
        std::cout << indent << "Else:" << std::endl;
        IndentScope elseScope(*this);
        for (const auto &st : stmt->elseBranch)
            accept(st.get());
    }
}

/**
 * Visit a While Loop
 * Prints the loop structure, including the condition and the loop body.
 * @param stmt Pointer to the while-statement node
 */
void ASTPrinter::visitWhileStmt(WhileStmt *stmt) {
    std::cout << indent << "[While]" << std::endl;

    IndentScope scope(*this);
    std::cout << indent << "Condition:" << std::endl;
    {
        IndentScope condScope(*this);
        accept(stmt->condition.get());
    }

    std::cout << indent << "Body:" << std::endl;
    {
        IndentScope bodyScope(*this);
        for (const auto &st : stmt->body)
            accept(st.get());
    }
}

/**
 * Visit a For-In Statement
 * Prints the for-in loop structure: variable, iterable, and loop body.
 * @param stmt Pointer to the for-in statement node
 */
void ASTPrinter::visitForInStmt(ForInStmt *stmt) {
    std::cout << indent << "[ForIn] " << stmt->variable.lexeme << std::endl;

    IndentScope scope(*this);
    std::cout << indent << "Iterable:" << std::endl;
    {
        IndentScope iterScope(*this);
        accept(stmt->iterable.get());
    }

    std::cout << indent << "Body:" << std::endl;
    {
        IndentScope bodyScope(*this);
        for (const auto &st : stmt->body)
            accept(st.get());
    }
}

/**
 * Visit a For-To Statement
 * Prints the for-to loop structure: variable, start, end, and loop body.
 * @param stmt Pointer to the for-to statement node
 */
void ASTPrinter::visitForStmt(ForStmt *stmt) {
    std::cout << indent << "[For] " << stmt->variable.lexeme << std::endl;

    IndentScope scope(*this);
    std::cout << indent << "Start:" << std::endl;
    {
        IndentScope iterScope(*this);
        accept(stmt->start.get());
    }

    std::cout << indent << "End:" << std::endl;
    {
        IndentScope iterScope(*this);
        accept(stmt->end.get());
    }

    std::cout << indent << "Body:" << std::endl;
    {
        IndentScope bodyScope(*this);
        for (const auto &st : stmt->body)
            accept(st.get());
    }
}

/**
 * Visit a Return Statement
 * Prints the return marker and recursively prints the return value if present.
 * @param stmt Pointer to the return statement node
 */
void ASTPrinter::visitReturnStmt(ReturnStmt *stmt) {
    std::cout << indent << "[Return]" << std::endl;
    if (stmt->value) {
        IndentScope scope(*this);
        accept(stmt->value.get());
    }
}

/**
 * Visit an Expression Statement
 * Prints a generic expression marker and recursively prints the expression.
 * Used for expressions that stand alone as statements (e.g., function calls).
 * @param stmt Pointer to the expression statement node
 */
void ASTPrinter::visitExpressionStmt(ExpressionStmt *stmt) {
    std::cout << indent << "[ExprStmt]" << std::endl;
    IndentScope scope(*this);
    accept(stmt->expression.get());
}

/**
 * Visit a Block Statement
 * Prints the block marker and iterates through the list of statements within the block.
 * @param stmt Pointer to the block statement node
 */
void ASTPrinter::visitBlockStmt(BlockStmt *stmt) {
    std::cout << indent << "[Block]" << std::endl;
    IndentScope scope(*this);
    for (const auto &s : stmt->statements) {
        accept(s.get());
    }
}

// --- Expression Visitors ---

/**
 * Visit a Binary Expression
 * Prints the operator and recursively prints the left and right operands.
 * @param expr Pointer to the binary expression node
 */
RuntimeValue ASTPrinter::visitBinaryExpr(BinaryExpr *expr) {
    std::cout << indent << "Binary (" << expr->op.lexeme << ")" << std::endl;
    IndentScope scope(*this);
    accept(expr->left.get());
    accept(expr->right.get());

    return {Null{}};
}

/**
 * Visit a Unary Expression
 * Prints the operator and recursively prints the operand.
 * @param expr Pointer to the unary expression node
 */
RuntimeValue ASTPrinter::visitUnaryExpr(UnaryExpr *expr) {
    std::cout << indent << "Unary (" << expr->op.lexeme << ")" << std::endl;
    IndentScope scope(*this);
    accept(expr->right.get());

    return {Null{}};
}

/**
 * Visit an Assignment Expression
 * Prints the assignment marker, the target (variable), and the value being assigned.
 * @param expr Pointer to the assignment expression node
 */
RuntimeValue ASTPrinter::visitAssignExpr(AssignExpr *expr) {
    std::cout << indent << "Assign (=)" << std::endl;
    IndentScope scope(*this);

    std::cout << indent << "Target:" << std::endl;
    {
        IndentScope targetScope(*this);
        accept(expr->target.get());
    }

    std::cout << indent << "Value:" << std::endl;
    {
        IndentScope valScope(*this);
        accept(expr->value.get());
    }

    return {Null{}};
}

/**
 * Visit a Literal Value
 * Prints the raw value of the literal token (e.g., number, string, boolean).
 * @param expr Pointer to the literal expression node
 */
RuntimeValue ASTPrinter::visitLiteralExpr(LiteralExpr *expr) {
    std::cout << indent << "Literal: " << expr->token.lexeme << std::endl;

    return {Null{}};
}

/**
 * Visit a Variable Reference
 * Prints the name of the variable being accessed.
 * @param expr Pointer to the variable expression node
 */
RuntimeValue ASTPrinter::visitVariableExpr(VariableExpr *expr) {
    std::cout << indent << "Var: " << expr->name.lexeme << std::endl;

    return {Null{}};
}

/**
 * Visit a Function/Method Call
 * Prints the call marker, the callee (function name/object), and the arguments.
 * @param expr Pointer to the call expression node
 */
RuntimeValue ASTPrinter::visitCallExpr(CallExpr *expr) {
    std::cout << indent << "Call" << std::endl;
    IndentScope scope(*this);

    std::cout << indent << "Callee:" << std::endl;
    {
        IndentScope calleeScope(*this);
        accept(expr->callee.get());
    }

    std::cout << indent << "Args:" << std::endl;
    {
        IndentScope argScope(*this);
        for (const auto &arg : expr->args) {
            accept(arg.get());
        }
    }

    return {Null{}};
}

/**
 * Visit a Property Get Expression
 * Prints the property name and the object being accessed (e.g., object.property).
 * @param expr Pointer to the get expression node
 */
RuntimeValue ASTPrinter::visitGetExpr(GetExpr *expr) {
    std::cout << indent << "Get Property: ." << expr->name.lexeme << std::endl;
    IndentScope scope(*this);
    accept(expr->object.get());

    return {Null{}};
}

/**
 * Visit an Array Access Expression
 * Prints the array access structure, the array expression, and the index expression.
 * @param expr Pointer to the array access expression node
 */
RuntimeValue ASTPrinter::visitArrayAccessExpr(ArrayAccessExpr *expr) {
    std::cout << indent << "Array Index []" << std::endl;
    IndentScope scope(*this);

    std::cout << indent << "Array:" << std::endl;
    {
        IndentScope arrScope(*this);
        accept(expr->array.get());
    }

    std::cout << indent << "Index:" << std::endl;
    {
        IndentScope idxScope(*this);
        accept(expr->index.get());
    }

    return {Null{}};
}

/**
 * Visit an Array Literal
 * Prints the array definition and recursively prints all elements inside.
 * @param expr Pointer to the array literal expression node
 */
RuntimeValue ASTPrinter::visitArrayLitExpr(ArrayLitExpr *expr) {
    std::cout << indent << "Array Literal []" << std::endl;
    IndentScope scope(*this);
    for (const auto &elem : expr->elements) {
        accept(elem.get());
    }

    return {Null{}};
}

/**
 * Visit a Dictionary Literal
 * Prints the dictionary definition and recursively prints all key-value pairs.
 * @param expr Pointer to the dictionary literal expression node
 */
RuntimeValue ASTPrinter::visitDictLitExpr(DictLitExpr *expr) {
    std::cout << indent << "Dictionary Literal {}" << std::endl;
    IndentScope scope(*this);
    for (const auto &entry : expr->entries) {
        std::cout << indent << "Key:" << std::endl;
        {
            IndentScope keyScope(*this);
            accept(entry.key.get());
        }
        std::cout << indent << "Value:" << std::endl;
        {
            IndentScope valScope(*this);
            accept(entry.value.get());
        }
    }

    return {Null{}};
}

/**
 * Visit a New Instance Expression
 * Prints the class instantiation and the constructor arguments.
 * @param expr Pointer to the new expression node
 */
RuntimeValue ASTPrinter::visitNewExpr(NewExpr *expr) {
    std::cout << indent << "New " << expr->className.lexeme << std::endl;
    IndentScope scope(*this);
    for (const auto &arg : expr->args) {
        accept(arg.get());
    }

    return {Null{}};
}

/**
 * Visit a Case Statement
 * Prints the case condition, arms, and optional default branch.
 * @param stmt Pointer to the case statement node
 */
void ASTPrinter::visitCaseStmt(CaseStmt *stmt) {
    std::cout << indent << "[Case]" << std::endl;

    IndentScope scope(*this);
    std::cout << indent << "Condition:" << std::endl;
    {
        IndentScope condScope(*this);
        accept(stmt->condition.get());
    }

    for (size_t i = 0; i < stmt->arms.size(); ++i) {
        std::cout << indent << "Arm " << i << " (colon at " << stmt->arms[i].colon.span.line << ":"
                  << stmt->arms[i].colon.span.start << "):" << std::endl;
        IndentScope armScope(*this);
        std::cout << indent << "Values:" << std::endl;
        {
            IndentScope valScope(*this);
            for (const auto &val : stmt->arms[i].values)
                accept(val.get());
        }
        std::cout << indent << "Body:" << std::endl;
        {
            IndentScope bodyScope(*this);
            for (const auto &st : stmt->arms[i].body)
                accept(st.get());
        }
    }

    if (!stmt->defaultBranch.empty()) {
        std::cout << indent << "Otherwise:" << std::endl;
        IndentScope elseScope(*this);
        for (const auto &st : stmt->defaultBranch)
            accept(st.get());
    }
}

/**
 * Visit a Repeat-Until Loop
 * Prints the loop structure: body and the terminating condition.
 * @param stmt Pointer to the repeat-until statement node
 */
void ASTPrinter::visitRepeatUntilStmt(RepeatUntilStmt *stmt) {
    std::cout << indent << "[RepeatUntil]" << std::endl;

    IndentScope scope(*this);
    std::cout << indent << "Body:" << std::endl;
    {
        IndentScope bodyScope(*this);
        for (const auto &st : stmt->body)
            accept(st.get());
    }

    std::cout << indent << "Until Condition:" << std::endl;
    {
        IndentScope condScope(*this);
        accept(stmt->condition.get());
    }
}
