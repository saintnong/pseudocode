#include "interpreter.hpp"
#include <functional>
#include <iostream>

/**
 * Return Signal
 * A special wrapper class used to propagate return values up through statement execution back to
 * the function call site. This is a little cursed but what can we do about it?
 */
struct ReturnSignal {
    RuntimeValue value;
    explicit ReturnSignal(RuntimeValue v) : value(v) {
    }
};

// =================================================================================================
//           Definition and Implementation of Pseudocode Functions and Classes
// =================================================================================================

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

    /**
     * Creates a new environment where "this" is bound to the specific instance.
     * This allows methods to access instance fields and other methods.
     */
    std::shared_ptr<UserFunction> bind(std::shared_ptr<Instance> instance) {
        auto environment = std::make_shared<Environment>(closure);

        RuntimeValue instanceValue;
        instanceValue.value = instance;

        environment->define("this", instanceValue);

        return std::make_shared<UserFunction>(declaration, environment);
    }

    int arity() override {
        return static_cast<int>(declaration->params.size());
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
 * Native Function
 * Lets us add standard library functions to Pseudocode easily.
 */
typedef std::function<RuntimeValue(Interpreter &, std::vector<RuntimeValue>)> NativeFn;

class NativeFunction : public Callable {
    NativeFn function;
    int _arity;

public:
    NativeFunction(int arity, NativeFn function) : function(std::move(function)), _arity(arity) {
    }

    /**
     * Returns the number of arguments the native function expects.
     */
    int arity() override {
        return _arity;
    }

    /**
     * Invokes the underlying function.
     * Arguments are passed from the interpreter directly to the native implementation.
     */
    RuntimeValue call(Interpreter &interpreter, std::vector<RuntimeValue> arguments) override {
        return function(interpreter, arguments);
    }

    std::string toString() override {
        return "<NATIVE FUNCTION>";
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
    std::map<std::string, RuntimeValue> defaultFields;

public:
    // instantiation
    UserClass(const std::string &n) : name(n) {
    }

    // Adds a method
    void addMethod(const std::string &methodName, std::shared_ptr<Callable> method) {
        methods[methodName] = method;
    }

    // Adds a field
    void addField(const std::string &fieldName, RuntimeValue value) {
        defaultFields[fieldName] = value;
    }

    // Finds a method which this class has
    std::shared_ptr<UserFunction> findMethod(const std::string &methodName) {
        if (methods.count(methodName)) {
            return std::dynamic_pointer_cast<UserFunction>(methods.at(methodName));
        }

        // TODO:
        // Check superclass here

        return nullptr;
    }

    int arity() override {
        std::shared_ptr<UserFunction> constructor = findMethod(name);
        if (constructor != nullptr) {
            return constructor->arity();
        }
        return 0;
    }

    RuntimeValue call(Interpreter &interpreter, std::vector<RuntimeValue> arguments) override {
        (void) interpreter;
        (void) arguments;
        auto instance =
            std::make_shared<Instance>(std::static_pointer_cast<Callable>(shared_from_this()));

        // Populate new instance with default field values
        for (const auto &[key, val] : defaultFields) {
            instance->fields[key] = val;
        }

        // Find class constructor method
        std::shared_ptr<UserFunction> constructor = findMethod(name);
        if (constructor != nullptr) {
            constructor->bind(instance)->call(interpreter, arguments);
        }

        // Return results
        RuntimeValue result;
        result.value = instance;
        return result;
    }

    std::string toString() override {
        return "<CLASS INSTANCE " + name + ">";
    }
};

// =================================================================================================
//                             Interpreter Class Implementation
// =================================================================================================

/**
 * Creates an interpreter, and its environment.
 * @returns An interpreter
 */
Interpreter::Interpreter(ErrorReporter &reporterRef) : reporter(reporterRef) {
    // Initiate our global environment, and begin interpreting in global scope.
    globals     = std::make_shared<Environment>();
    environment = globals;

    // =========================================================================
    // SCSA Pseudocode Standard Library
    // ==========================================================================

    /**
     * INPUT native function
     * >> INPUT(prompt: String)
     * => String
     * Returns input from stdin. Optionally takes a prompt which is printed before taking input.
     */
    auto inputNative = std::make_shared<NativeFunction>(
        VARIADIC_ARITY, [](Interpreter &, std::vector<RuntimeValue> args) -> RuntimeValue {
            // Optional prompt argument
            if (args.size() > 0) {
                std::cout << stringify(args[0]);
            }

            // Read from standard input
            std::string inputLine;
            if (!std::getline(std::cin, inputLine)) {
                inputLine = "";
            }

            // Return te result as a RuntimeValue
            RuntimeValue result;
            result.value = inputLine;
            return result;
        });

    // Global scope registration
    RuntimeValue inputVal;
    inputVal.value = std::static_pointer_cast<Callable>(inputNative);
    globals->define("INPUT", inputVal);

    /**
     * PRINT native function
     * >> PRINT(... vars: String)
     * => Null
     * Print to stdout with a trailing newline. Given multiple parameters they will be printed
     * sequentially with a space delimiter. Parameters will converted to type String when suitable.
     */
    auto printNative = std::make_shared<NativeFunction>(
        VARIADIC_ARITY, [](Interpreter &, std::vector<RuntimeValue> args) -> RuntimeValue {
            for (size_t i = 0; i < args.size(); ++i) {
                // Space before every element except the first
                if (i > 0)
                    std::cout << " ";
                std::cout << stringify(args[i]);
            }
            // Print a newline at the end of the statement
            std::cout << std::endl;

            RuntimeValue nullValue;
            nullValue.value = Null{};
            return nullValue;
        });

    // Register PRINT in the global scope
    RuntimeValue printVal;
    printVal.value = std::static_pointer_cast<Callable>(printNative);
    globals->define("PRINT", printVal);

    /**
     * OUTPUT native function
     * >> OUTPUT(... vars: String)
     * => Null
     * Print to stdout with no trailing newline. Given multiple parameters they will be printed
     * sequentially with a space delimiter. Parameters will converted to type String when suitable.
     */
    auto outputNative = std::make_shared<NativeFunction>(
        VARIADIC_ARITY, [](Interpreter &, std::vector<RuntimeValue> args) -> RuntimeValue {
            for (size_t i = 0; i < args.size(); ++i) {
                if (i > 0)
                    std::cout << " ";
                std::cout << stringify(args[i]);
            }

            RuntimeValue nullValue;
            nullValue.value = Null{};
            return nullValue;
        });

    // Register OUTPUT in the global scope
    RuntimeValue outputVal;
    outputVal.value = std::static_pointer_cast<Callable>(outputNative);
    globals->define("OUTPUT", outputVal);

    /**
     * INT native function
     * >> INT(value: Any)
     * => Int
     * Converts a value to an Integer.
     * - Floats are truncated.
     * - Bools convert to 1 (true) or 0 (false).
     * - Strings are parsed (returns -1 if parsing fails).
     */
    auto intNative = std::make_shared<NativeFunction>(
        1, [](Interpreter &, std::vector<RuntimeValue> args) -> RuntimeValue {
            if (args.empty()) return { Null{} };
            const auto &val = args[0];

            int result = 0;

            if (val.is<int>()) {
                result = val.as<int>();
            } else if (val.is<double>()) {
                result = static_cast<int>(val.as<double>());
            } else if (val.is<bool>()) {
                result = val.as<bool>() ? 1 : 0;
            } else if (val.is<std::string>()) {
                try {
                    result = std::stoi(val.as<std::string>());
                } catch (...) {
                    // Failed conversion
                    result = -1;
                }
            }

            RuntimeValue retVal;
            retVal.value = result;
            return retVal;
        });

    globals->define("INT", { std::static_pointer_cast<Callable>(intNative) });

    /**
     * FLOAT native function
     * >> FLOAT(value: Any)
     * => Double
     * Converts a value to a floating point number.
     * - Ints are converted directly.
     * - Strings are parsed (returns 0.0 if parsing fails).
     */
    auto floatNative = std::make_shared<NativeFunction>(
        1, [](Interpreter &, std::vector<RuntimeValue> args) -> RuntimeValue {
            if (args.empty()) return { Null{} };
            const auto &val = args[0];

            double result = 0.0;

            if (val.is<double>()) {
                result = val.as<double>();
            } else if (val.is<int>()) {
                result = static_cast<double>(val.as<int>());
            } else if (val.is<bool>()) {
                result = val.as<bool>() ? 1.0 : 0.0;
            } else if (val.is<std::string>()) {
                try {
                    result = std::stod(val.as<std::string>());
                } catch (...) {
                    result = 0.0;
                }
            }

            RuntimeValue retVal;
            retVal.value = result;
            return retVal;
        });

    globals->define("FLOAT", { std::static_pointer_cast<Callable>(floatNative) });

    /**
     * STRING native function
     * >> STRING(value: Any)
     * => String
     * Converts any value to its string representation.
     */
    auto stringNative = std::make_shared<NativeFunction>(
        1, [](Interpreter &, std::vector<RuntimeValue> args) -> RuntimeValue {
            if (args.empty()) return { Null{} };
            std::string result = stringify(args[0]);

            RuntimeValue retVal;
            retVal.value = result;
            return retVal;
        });

    globals->define("STRING", { std::static_pointer_cast<Callable>(stringNative) });

    /**
     * BOOL native function
     * >> BOOL(value: Any)
     * => Boolean
     * Converts a value to a boolean.
     * - Numbers: 0 is false, anything else is true.
     * - Strings: "true" (case-sensitive) is true, everything else is false.
     * - Null: false.
     */
    auto boolNative = std::make_shared<NativeFunction>(
        1, [](Interpreter &, std::vector<RuntimeValue> args) -> RuntimeValue {
            if (args.empty()) return { Null{} };
            const auto &val = args[0];

            bool result = false;

            if (val.is<bool>()) {
                result = val.as<bool>();
            } else if (val.is<int>()) {
                result = val.as<int>() != 0;
            } else if (val.is<double>()) {
                result = val.as<double>() != 0.0;
            } else if (val.is<std::string>()) {
                std::string s = val.as<std::string>();
                result = (s == "true" || s == "TRUE");
            } else if (!val.is<Null>()) {
                result = true;
            }

            RuntimeValue retVal;
            retVal.value = result;
            return retVal;
        });

    globals->define("BOOL", { std::static_pointer_cast<Callable>(boolNative) });
}

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
 * Null and false are falsy, zeroes are also falsy, everything else is truthy
 */
bool Interpreter::isTruthy(const RuntimeValue &object) {
    if (object.is<Null>())
        return false;
    if (object.is<bool>())
        return object.as<bool>();
    if (object.is<int>())
        return object.as<int>() != 0;
    if (object.is<double>())
        return object.as<double>() != 0.0;
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
        environment->define(varExpr->name.lexeme, value);
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
    case TOK_AND: {
        if (!isTruthy(left))
            return left;
        return evaluate(expr->right.get());
        break;
    }
    case TOK_OR: {
        if (isTruthy(left))
            return left;
        return evaluate(expr->right.get());
        break;
    }
    case TOK_IN: {
        // Ensure we can 'IN'
        if (!right.is<std::shared_ptr<std::vector<RuntimeValue>>>()) {
            throw RuntimeError(expr->op, "'IN' operator requires right hand side to be a list.");
        }

        auto arr   = right.as<std::shared_ptr<std::vector<RuntimeValue>>>();
        bool found = false;

        // Iterate through vector and use standard isEqual check
        for (const auto &element : *arr) {
            if (isEqual(left, element)) {
                found = true;
                break;
            }
        }
        result.value = found;
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

    // Only call things that we can call
    if (!callee.is<std::shared_ptr<Callable>>()) {
        throw RuntimeError(expr->anchor, "You can only call functions and classes.");
    }

    auto function = callee.as<std::shared_ptr<Callable>>();

    // Check that the function's arity matches our call
    // We skip this check if the arity is variadic (any number of arguments)
    int arity = function->arity();
    if (arity != VARIADIC_ARITY) {
        if (static_cast<int>(arguments.size()) != arity) {
            throw RuntimeError(expr->anchor, "Expected " + std::to_string(arity) +
                                                 " arguments but got " +
                                                 std::to_string(arguments.size()) + ".");
        }
    }

    return function->call(*this, arguments);
}

/**
 * Visit a property get expression
 * Retrieves a property value from an instance
 */
RuntimeValue Interpreter::visitGetExpr(GetExpr *expr) {
    RuntimeValue object = evaluate(expr->object.get());

    // Make sure we're getting from an instance
    if (!object.is<std::shared_ptr<Instance>>()) {
        throw RuntimeError(expr->name, "Only instances have properties.");
    }

    // Extract the instance and string
    auto instance    = object.as<std::shared_ptr<Instance>>();
    std::string name = expr->name.lexeme;

    // Check for a field (variable)
    if (instance->fields.count(name)) {
        return instance->fields.at(name);
    }

    // Look for a method in the class
    auto klass = std::dynamic_pointer_cast<UserClass>(instance->klass);

    if (klass) {
        auto method = klass->findMethod(name);
        if (method) {
            // If the method is found, we MUST bind it to the instance.
            // This ensures that when the function is eventually called,
            // 'this' refers to 'instance'.
            RuntimeValue result;
            result.value = method->bind(instance);
            return result;
        }
    }

    throw RuntimeError(expr->name, "Undefined property '" + name + "'.");
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

    // Add the class's default attributes
    for (const auto &attr : stmt->attributes) {
        if (auto *assignment = dynamic_cast<AssignExpr *>(attr.get())) {
            if (auto *varExpr = dynamic_cast<VariableExpr *>(assignment->target.get())) {
                // Assign attribute to class template
                std::string fieldName = varExpr->name.lexeme;
                RuntimeValue val      = evaluate(assignment->value.get());
                klass->addField(fieldName, val);
            } else {
                throw RuntimeError(assignment->anchor, "Expected a valid field name.");
            }
        }
    }

    // Add the class methods to class
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
