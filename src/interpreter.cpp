// interpreter.cpp

#include "interpreter.hpp"
#include <iostream>

/**
 * Return Signal
 * A special wrapper class used to propagate return values up through
 * statement execution back to the function call site. This allows us
 * to exit early from nested statement blocks without using exceptions
 * for control flow of the interpreted language.
 */
struct ReturnSignal {
    RuntimeValue value;
    explicit ReturnSignal(RuntimeValue v) : value(v) {
    }
};

/**
 * User-Defined Function
 * Represents a function defined in the Pseudocode language.
 * Implements the Callable interface to be invokable at runtime.
 */
class UserFunction : public Callable {
    FunctionStmt *declaration;
    std::shared_ptr<Environment> closure;

public:
    UserFunction(FunctionStmt *decl, std::shared_ptr<Environment> closure)
        : declaration(decl), closure(closure) {
    }

    int arity() override {
        return declaration->params.size();
    }

    RuntimeValue call(Interpreter &interpreter, std::vector<RuntimeValue> arguments) override {
        auto functionEnv = std::make_shared<Environment>(closure);

        for (size_t i = 0; i < declaration->params.size(); i++) {
            functionEnv->define(declaration->params[i].lexeme, arguments[i]);
        }

        try {
            interpreter.executeBlock(declaration->body, functionEnv);
        } catch (const ReturnSignal &signal) {
            return signal.value;
        }

        RuntimeValue nullValue;
        nullValue.value = Null{};
        return nullValue;
    }

    std::string toString() override {
        return "<FUNCTION " + declaration->name.lexeme + ">";
    }
};

/**
 * User-Defined Class
 * Represents a class defined in the Pseudocode language.
 * Classes are callable (act as constructors) and create Instance objects.
 */
class UserClass : public Callable, public std::enable_shared_from_this<UserClass> {
    std::string name;
    std::map<std::string, std::shared_ptr<Callable>> methods;

public:
    UserClass(const std::string &n) : name(n) {
    }

    void addMethod(const std::string &methodName, std::shared_ptr<Callable> method) {
        methods[methodName] = method;
    }

    int arity() override {
        return 0;
    }

    RuntimeValue call(Interpreter &interpreter, std::vector<RuntimeValue> arguments) override {
        (void) interpreter;
        (void) arguments;
        auto instance =
            std::make_shared<Instance>(std::static_pointer_cast<Callable>(shared_from_this()));
        RuntimeValue result;
        result.value = instance;
        return result;
    }

    std::string toString() override {
        return "<CLASS " + name + ">";
    }
};

/**
 * Execute a single statement by dispatching to its visitor method
 */
void Interpreter::execute(Stmt *stmt) {
    stmt->accept(*this);
}

/**
 * Evaluate an expression by dispatching to its visitor method
 * Uses dynamic dispatch to call the appropriate visit method based on expression type
 */
RuntimeValue Interpreter::evaluate(Expr *expr) {
    if (auto *e = dynamic_cast<LiteralExpr *>(expr))
        return visitLiteralExpr(e);
    if (auto *e = dynamic_cast<VariableExpr *>(expr))
        return visitVariableExpr(e);
    if (auto *e = dynamic_cast<AssignExpr *>(expr))
        return visitAssignExpr(e);
    if (auto *e = dynamic_cast<BinaryExpr *>(expr))
        return visitBinaryExpr(e);
    if (auto *e = dynamic_cast<CallExpr *>(expr))
        return visitCallExpr(e);
    if (auto *e = dynamic_cast<GetExpr *>(expr))
        return visitGetExpr(e);
    if (auto *e = dynamic_cast<ArrayAccessExpr *>(expr))
        return visitArrayAccessExpr(e);
    if (auto *e = dynamic_cast<ArrayLitExpr *>(expr))
        return visitArrayLitExpr(e);
    if (auto *e = dynamic_cast<NewExpr *>(expr))
        return visitNewExpr(e);

    RuntimeValue null;
    null.value = Null{};
    return null;
}

/**
 * Execute a block of statements in a new environment scope
 */
void Interpreter::executeBlock(const std::vector<StmtPtr> &statements,
                               std::shared_ptr<Environment> env) {
    std::shared_ptr<Environment> previous = environment;
    try {
        environment = env;
        for (const auto &stmt : statements) {
            execute(stmt.get());
        }
        environment = previous;
    } catch (...) {
        environment = previous;
        throw;
    }
}

/**
 * Determine if a runtime value is considered "truthy"
 * Null and false are falsy, everything else is truthy
 */
bool Interpreter::isTruthy(const RuntimeValue &object) {
    if (object.is<Null>())
        return false;
    if (object.is<bool>())
        return object.as<bool>();
    return true;
}

/**
 * Check equality between two runtime values
 * Handles all supported types with proper comparison logic
 */
bool Interpreter::isEqual(const RuntimeValue &a, const RuntimeValue &b) {
    if (a.is<Null>() && b.is<Null>())
        return true;
    if (a.is<Null>() || b.is<Null>())
        return false;

    if (a.is<int>() && b.is<int>())
        return a.as<int>() == b.as<int>();
    if (a.is<double>() && b.is<double>())
        return a.as<double>() == b.as<double>();
    if (a.is<int>() && b.is<double>())
        return a.as<int>() == b.as<double>();
    if (a.is<double>() && b.is<int>())
        return a.as<double>() == b.as<int>();
    if (a.is<bool>() && b.is<bool>())
        return a.as<bool>() == b.as<bool>();
    if (a.is<std::string>() && b.is<std::string>())
        return a.as<std::string>() == b.as<std::string>();

    return false;
}

/**
 * Verify that a single operand is numeric
 */
void Interpreter::checkNumberOperand(const Token &operatorToken, const RuntimeValue &operand) {
    if (operand.is<int>() || operand.is<double>())
        return;
    throw RuntimeError(operatorToken, "Operand must be a number.");
}

/**
 * Verify that both operands are numeric
 */
void Interpreter::checkNumberOperands(const Token &operatorToken, const RuntimeValue &left,
                                      const RuntimeValue &right) {
    if ((left.is<int>() || left.is<double>()) && (right.is<int>() || right.is<double>()))
        return;
    throw RuntimeError(operatorToken, "Operands must be numbers.");
}

/**
 * Visit a literal expression node
 * Converts the token to its corresponding runtime value
 */
RuntimeValue Interpreter::visitLiteralExpr(LiteralExpr *expr) {
    RuntimeValue result;

    switch (expr->token.type) {
    case TOK_INTEGER:
        result.value = std::stoi(expr->token.lexeme);
        break;
    case TOK_FLOAT:
        result.value = std::stod(expr->token.lexeme);
        break;
    case TOK_STRING:
        result.value = expr->token.lexeme;
        break;
    case TOK_TRUE:
        result.value = true;
        break;
    case TOK_FALSE:
        result.value = false;
        break;
    default:
        result.value = Null{};
    }

    return result;
}

/**
 * Visit a variable expression node
 * Looks up the variable in the current environment
 */
RuntimeValue Interpreter::visitVariableExpr(VariableExpr *expr) {
    return environment->get(expr->name);
}

/**
 * Visit an assignment expression node
 * Evaluates the value and assigns it to the target
 */
RuntimeValue Interpreter::visitAssignExpr(AssignExpr *expr) {
    RuntimeValue value = evaluate(expr->value.get());

    if (auto *varExpr = dynamic_cast<VariableExpr *>(expr->target.get())) {
        environment->assign(varExpr->name, value);
    } else if (auto *getExpr = dynamic_cast<GetExpr *>(expr->target.get())) {
        RuntimeValue object = evaluate(getExpr->object.get());
        if (!object.is<std::shared_ptr<Instance>>()) {
            throw RuntimeError(getExpr->name, "Only instances have fields.");
        }
        object.as<std::shared_ptr<Instance>>()->set(getExpr->name, value);
    } else if (auto *arrayAccess = dynamic_cast<ArrayAccessExpr *>(expr->target.get())) {
        RuntimeValue arrayVal = evaluate(arrayAccess->array.get());
        RuntimeValue indexVal = evaluate(arrayAccess->index.get());

        if (!arrayVal.is<std::shared_ptr<std::vector<RuntimeValue>>>()) {
            throw RuntimeError(expr->anchor, "Can only index arrays.");
        }
        if (!indexVal.is<int>()) {
            throw RuntimeError(expr->anchor, "Array index must be an integer.");
        }

        auto arr = arrayVal.as<std::shared_ptr<std::vector<RuntimeValue>>>();
        int idx  = indexVal.as<int>();

        if (idx < 0 || idx >= static_cast<int>(arr->size())) {
            throw RuntimeError(expr->anchor, "Array index out of bounds.");
        }

        (*arr)[idx] = value;
    }

    return value;
}

/**
 * Visit a binary expression node
 * Handles arithmetic, comparison, and equality operators
 */
RuntimeValue Interpreter::visitBinaryExpr(BinaryExpr *expr) {
    RuntimeValue left  = evaluate(expr->left.get());
    RuntimeValue right = evaluate(expr->right.get());
    RuntimeValue result;

    switch (expr->op.type) {
    case TOK_PLUS: {
        if (left.is<std::string>() && right.is<std::string>()) {
            result.value = left.as<std::string>() + right.as<std::string>();
        } else {
            checkNumberOperands(expr->op, left, right);
            if (left.is<double>() || right.is<double>()) {
                double l     = left.is<double>() ? left.as<double>() : left.as<int>();
                double r     = right.is<double>() ? right.as<double>() : right.as<int>();
                result.value = l + r;
            } else {
                result.value = left.as<int>() + right.as<int>();
            }
        }
        break;
    }
    case TOK_MINUS: {
        checkNumberOperands(expr->op, left, right);
        if (left.is<double>() || right.is<double>()) {
            double l     = left.is<double>() ? left.as<double>() : left.as<int>();
            double r     = right.is<double>() ? right.as<double>() : right.as<int>();
            result.value = l - r;
        } else {
            result.value = left.as<int>() - right.as<int>();
        }
        break;
    }
    case TOK_MULTIPLY: {
        checkNumberOperands(expr->op, left, right);
        if (left.is<double>() || right.is<double>()) {
            double l     = left.is<double>() ? left.as<double>() : left.as<int>();
            double r     = right.is<double>() ? right.as<double>() : right.as<int>();
            result.value = l * r;
        } else {
            result.value = left.as<int>() * right.as<int>();
        }
        break;
    }
    case TOK_DIVIDE: {
        checkNumberOperands(expr->op, left, right);
        double l = left.is<double>() ? left.as<double>() : left.as<int>();
        double r = right.is<double>() ? right.as<double>() : right.as<int>();
        if (r == 0.0) {
            throw RuntimeError(expr->op, "Division by zero.");
        }
        result.value = l / r;
        break;
    }
    case TOK_GREATER_THAN: {
        checkNumberOperands(expr->op, left, right);
        double l     = left.is<double>() ? left.as<double>() : left.as<int>();
        double r     = right.is<double>() ? right.as<double>() : right.as<int>();
        result.value = l > r;
        break;
    }
    case TOK_GT_OR_EQ: {
        checkNumberOperands(expr->op, left, right);
        double l     = left.is<double>() ? left.as<double>() : left.as<int>();
        double r     = right.is<double>() ? right.as<double>() : right.as<int>();
        result.value = l >= r;
        break;
    }
    case TOK_LESS_THAN: {
        checkNumberOperands(expr->op, left, right);
        double l     = left.is<double>() ? left.as<double>() : left.as<int>();
        double r     = right.is<double>() ? right.as<double>() : right.as<int>();
        result.value = l < r;
        break;
    }
    case TOK_LT_OR_EQ: {
        checkNumberOperands(expr->op, left, right);
        double l     = left.is<double>() ? left.as<double>() : left.as<int>();
        double r     = right.is<double>() ? right.as<double>() : right.as<int>();
        result.value = l <= r;
        break;
    }
    case TOK_EQUAL: {
        result.value = isEqual(left, right);
        break;
    }
    default:
        result.value = Null{};
    }

    return result;
}

/**
 * Visit a function call expression, or class instantiation
 * Evaluates the callee and arguments, then invokes the function
 */
RuntimeValue Interpreter::visitCallExpr(CallExpr *expr) {
    RuntimeValue callee = evaluate(expr->callee.get());

    std::vector<RuntimeValue> arguments;
    for (const auto &arg : expr->args) {
        arguments.push_back(evaluate(arg.get()));
    }

    if (!callee.is<std::shared_ptr<Callable>>()) {
        throw RuntimeError(expr->anchor, "Can only call functions and classes.");
    }

    auto function = callee.as<std::shared_ptr<Callable>>();

    if (arguments.size() != static_cast<size_t>(function->arity())) {
        throw RuntimeError(expr->anchor, "Expected " + std::to_string(function->arity()) +
                                             " arguments but got " +
                                             std::to_string(arguments.size()) + ".");
    }

    return function->call(*this, arguments);
}

/**
 * Visit a property get expression
 * Retrieves a property value from an instance
 */
RuntimeValue Interpreter::visitGetExpr(GetExpr *expr) {
    RuntimeValue object = evaluate(expr->object.get());

    if (!object.is<std::shared_ptr<Instance>>()) {
        throw RuntimeError(expr->name, "Only instances have properties.");
    }

    return object.as<std::shared_ptr<Instance>>()->get(expr->name);
}

/**
 * Visit an array access expression
 * Retrieves an element from an array by index
 */
RuntimeValue Interpreter::visitArrayAccessExpr(ArrayAccessExpr *expr) {
    RuntimeValue arrayVal = evaluate(expr->array.get());
    RuntimeValue indexVal = evaluate(expr->index.get());

    if (!arrayVal.is<std::shared_ptr<std::vector<RuntimeValue>>>()) {
        throw RuntimeError(expr->anchor, "Can only index arrays.");
    }
    if (!indexVal.is<int>()) {
        throw RuntimeError(expr->anchor, "Array index must be an integer.");
    }

    auto arr = arrayVal.as<std::shared_ptr<std::vector<RuntimeValue>>>();
    int idx  = indexVal.as<int>();

    if (idx < 0 || idx >= static_cast<int>(arr->size())) {
        throw RuntimeError(expr->anchor, "Array index out of bounds.");
    }

    return (*arr)[idx];
}

/**
 * Visit an array literal expression
 * Creates a new array with the evaluated elements
 */
RuntimeValue Interpreter::visitArrayLitExpr(ArrayLitExpr *expr) {
    auto arr = std::make_shared<std::vector<RuntimeValue>>();

    for (const auto &elem : expr->elements) {
        arr->push_back(evaluate(elem.get()));
    }

    RuntimeValue result;
    result.value = arr;
    return result;
}

/**
 * Visit a new instance expression
 * Creates a new instance of the specified class
 */
RuntimeValue Interpreter::visitNewExpr(NewExpr *expr) {
    RuntimeValue classVal = environment->get(expr->className);

    if (!classVal.is<std::shared_ptr<Callable>>()) {
        throw RuntimeError(expr->className, "Can only instantiate classes.");
    }

    std::vector<RuntimeValue> arguments;
    for (const auto &arg : expr->args) {
        arguments.push_back(evaluate(arg.get()));
    }

    auto klass = classVal.as<std::shared_ptr<Callable>>();
    return klass->call(*this, arguments);
}

/**
 * Visit an expression statement
 * Evaluates the expression and discards the result
 */
void Interpreter::visitExpressionStmt(ExpressionStmt *stmt) {
    evaluate(stmt->expression.get());
}

/**
 * Visit a print statement
 * Evaluates the expression and outputs it to stdout
 */
void Interpreter::visitPrintStmt(PrintStmt *stmt) {
    RuntimeValue value = evaluate(stmt->expression.get());
    std::cout << stringify(value) << std::endl;
}

/**
 * Visit a return statement
 * Throws a ReturnSignal to unwind the statement execution back to the call site
 * The visitor pattern already returns RuntimeValue for expressions, but we need
 * this signal to break out of the statement execution loop in function bodies
 */
void Interpreter::visitReturnStmt(ReturnStmt *stmt) {
    RuntimeValue value;
    if (stmt->value) {
        value = evaluate(stmt->value.get());
    } else {
        value.value = Null{};
    }
    throw ReturnSignal(value);
}

/**
 * Visit a block statement
 * Creates a new scope and executes the statements within it
 */
void Interpreter::visitBlockStmt(BlockStmt *stmt) {
    executeBlock(stmt->statements, std::make_shared<Environment>(environment));
}

/**
 * Visit an if statement
 * Evaluates the condition and executes the appropriate branch
 */
void Interpreter::visitIfStmt(IfStmt *stmt) {
    RuntimeValue condition = evaluate(stmt->condition.get());

    if (isTruthy(condition)) {
        for (const auto &s : stmt->thenBranch) {
            execute(s.get());
        }
    } else if (!stmt->elseBranch.empty()) {
        for (const auto &s : stmt->elseBranch) {
            execute(s.get());
        }
    }
}

/**
 * Visit a while statement
 * Repeatedly executes the body 'while' the condition is truthy
 */
void Interpreter::visitWhileStmt(WhileStmt *stmt) {
    while (isTruthy(evaluate(stmt->condition.get()))) {
        for (const auto &s : stmt->body) {
            execute(s.get());
        }
    }
}

/**
 * Visit a function declaration statement
 * Creates a function object and binds it to the function name
 */
void Interpreter::visitFunctionStmt(FunctionStmt *stmt) {
    auto function = std::make_shared<UserFunction>(stmt, environment);
    RuntimeValue funcValue;
    funcValue.value = std::static_pointer_cast<Callable>(function);
    environment->define(stmt->name.lexeme, funcValue);
}

/**
 * Visit a class declaration statement
 * Creates a class object and binds methods to it
 */
void Interpreter::visitClassStmt(ClassStmt *stmt) {
    auto klass = std::make_shared<UserClass>(stmt->name.lexeme);

    for (const auto &method : stmt->methods) {
        if (auto *funcStmt = dynamic_cast<FunctionStmt *>(method.get())) {
            auto methodFunc = std::make_shared<UserFunction>(funcStmt, environment);
            klass->addMethod(funcStmt->name.lexeme, methodFunc);
        }
    }

    RuntimeValue klassValue;
    klassValue.value = std::static_pointer_cast<Callable>(klass);
    environment->define(stmt->name.lexeme, klassValue);
}

/**
 * Visit a for-in statement
 * Iterates over an array, binding each element to the loop variable
 */
void Interpreter::visitForInStmt(ForInStmt *stmt) {
    RuntimeValue iterable = evaluate(stmt->iterable.get());

    if (!iterable.is<std::shared_ptr<std::vector<RuntimeValue>>>()) {
        throw RuntimeError(stmt->variable, "Can only iterate over arrays.");
    }

    auto arr = iterable.as<std::shared_ptr<std::vector<RuntimeValue>>>();

    for (const auto &element : *arr) {
        auto loopEnv = std::make_shared<Environment>(environment);
        loopEnv->define(stmt->variable.lexeme, element);
        executeBlock(stmt->body, loopEnv);
    }
}
