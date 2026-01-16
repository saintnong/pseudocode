#include "interpreter.hpp"
#include <chrono>
#include <functional>
#include <iostream>
#include <random>

/**
 * Return Signal
 * A special exception-based mechanism used to propagate return values from deep within
 * nested statement execution back to the function call site.
 */
struct ReturnSignal {
    // The value being returned from the function
    RuntimeValue value;

    /**
     * Create a new return signal
     * @param v The value to return
     */
    explicit ReturnSignal(RuntimeValue v) : value(v) {
    }
};

// =================================================================================================
//           Definition and Implementation of Pseudocode Functions and Classes
// =================================================================================================

/**
 * User-Defined Function
 * Represents a function defined in Pseudocode source.
 * Implements the Callable interface for runtime invocation.
 */
class UserFunction : public Callable {
    // The AST node where the function was defined
    FunctionStmt *declaration;
    // The lexicographical environment where the function was created (for closures)
    std::shared_ptr<Environment> closure;

public:
    /**
     * Create a user function
     * @param decl The function declaration statement
     * @param closure The surrounding environment scope
     */
    UserFunction(FunctionStmt *decl, std::shared_ptr<Environment> closure)
        : declaration(decl), closure(closure) {
    }

    /**
     * Bind function to a class instance
     * Creates a new environment where "this" refers to the provided instance.
     * @param instance The object instance to bind to
     * @return A new UserFunction with the bound environment
     */
    std::shared_ptr<UserFunction> bind(std::shared_ptr<Instance> instance) {
        auto environment = std::make_shared<Environment>(closure);

        RuntimeValue instanceValue;
        instanceValue.value = instance;

        environment->define("this", instanceValue);

        return std::make_shared<UserFunction>(declaration, environment);
    }

    /**
     * @return Number of expected parameters
     */
    int arity() override {
        return static_cast<int>(declaration->params.size());
    }

    /**
     * Execute the function body
     * @param interpreter The active interpreter
     * @param arguments Evaluated argument values
     * @return The function's return value (or Null if no return)
     */
    RuntimeValue call(Interpreter &interpreter, std::vector<RuntimeValue> arguments) override {
        auto functionEnv = std::make_shared<Environment>(closure);

        // Bind arguments to parameter names in the local scope
        for (size_t i = 0; i < declaration->params.size(); i++) {
            functionEnv->define(declaration->params[i].lexeme, arguments[i]);
        }

        try {
            // Execute the body statements
            interpreter.executeBlock(declaration->body, functionEnv);
        } catch (const ReturnSignal &signal) {
            // Intercept return signals to get the result
            return signal.value;
        }

        // Default return value is Null
        RuntimeValue nullValue;
        nullValue.value = Null{};
        return nullValue;
    }

    /**
     * @return Debug string for the function
     */
    std::string toString() override {
        return "<FUNCTION " + declaration->name.lexeme + ">";
    }
};

/**
 * Native Function
 * Bridge for C++ functions to be called within Pseudocode.
 */
typedef std::function<RuntimeValue(Interpreter &, std::vector<RuntimeValue>)> NativeFn;

class NativeFunction : public Callable {
    // The C++ implementation of the function
    NativeFn function;
    // Cached arity (-1 for variadic)
    int _arity;

public:
    /**
     * Create a native function
     * @param arity Expected number of arguments
     * @param function The C++ implementation
     */
    NativeFunction(int arity, NativeFn function) : function(std::move(function)), _arity(arity) {
    }

    /**
     * @return Function arity
     */
    int arity() override {
        return _arity;
    }

    /**
     * Invoke the native implementation
     * @param interpreter The current interpreter
     * @param arguments Evaluated parameters
     * @return The result from the C++ implementation
     */
    RuntimeValue call(Interpreter &interpreter, std::vector<RuntimeValue> arguments) override {
        return function(interpreter, arguments);
    }

    /**
     * @return Debug string for native functions
     */
    std::string toString() override {
        return "<NATIVE FUNCTION>";
    }
};

/**
 * User-Defined Class
 * Represents a class blueprint in Pseudocode.
 * Manages its own methods and field defaults.
 */
class UserClass : public Callable, public std::enable_shared_from_this<UserClass> {
    // Name of the class
    std::string name;
    // Map of method names to their implementations
    std::map<std::string, std::shared_ptr<Callable>> methods;
    // Initial field values for new instances
    std::map<std::string, RuntimeValue> defaultFields;

public:
    /**
     * Create a new class definition
     * @param n The class name
     */
    UserClass(const std::string &n) : name(n) {
    }

    /**
     * Add a method to the class
     * @param methodName The identifier for the method
     * @param method The implementation callable
     */
    void addMethod(const std::string &methodName, std::shared_ptr<Callable> method) {
        methods[methodName] = method;
    }

    /**
     * Add a default field value
     * @param fieldName The identifier for the attribute
     * @param value Its initial state
     */
    void addField(const std::string &fieldName, RuntimeValue value) {
        defaultFields[fieldName] = value;
    }

    /**
     * Lookup a method by name
     * @param methodName The name to search for
     * @return Shared pointer to the method, or nullptr if not found
     */
    std::shared_ptr<UserFunction> findMethod(const std::string &methodName) {
        if (methods.count(methodName)) {
            return std::dynamic_pointer_cast<UserFunction>(methods.at(methodName));
        }

        // TODO: Support inheritance method lookups

        return nullptr;
    }

    /**
     * @return Arity of the constructor (same as the class-named method)
     */
    int arity() override {
        std::shared_ptr<UserFunction> constructor = findMethod(name);
        if (constructor != nullptr) {
            return constructor->arity();
        }
        return 0;
    }

    /**
     * Class Instantiation (e.g. new MyClass())
     * Creates a new instance, initializes fields, and runs the constructor.
     * @param interpreter The active interpreter
     * @param arguments Constructor arguments
     * @return The newly created Instance
     */
    RuntimeValue call(Interpreter &interpreter, std::vector<RuntimeValue> arguments) override {
        (void) interpreter;
        (void) arguments;
        auto instance =
            std::make_shared<Instance>(std::static_pointer_cast<Callable>(shared_from_this()));

        // Populate new instance with default field values
        for (const auto &[key, val] : defaultFields) {
            instance->fields[key] = val;
        }

        // Find and execute the class constructor method if it exists
        std::shared_ptr<UserFunction> constructor = findMethod(name);
        if (constructor != nullptr) {
            constructor->bind(instance)->call(interpreter, arguments);
        }

        // Return the instance wrapped as a runtime value
        RuntimeValue result;
        result.value = instance;
        return result;
    }

    /**
     * @return Debug string for the class
     */
    std::string toString() override {
        return "<CLASS " + name + ">";
    }
};

// =================================================================================================
//                             Interpreter Class Implementation
// =================================================================================================

/**
 * Interpreter Constructor
 * Initializes the execution environment and registers the standard library.
 * @param reporterRef Reference to error reporter for communicating runtime failure
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
     * Reads a line of text from standard input.
     * @param prompt Optional string to display before reading
     * @return The input string (trimmed of newline)
     */
    auto inputNative = std::make_shared<NativeFunction>(
        VARIADIC_ARITY, [](Interpreter &, std::vector<RuntimeValue> args) -> RuntimeValue {
            // Optional prompt argument handling
            if (args.size() > 0) {
                std::cout << stringify(args[0]);
            }

            // Read from standard input
            std::string inputLine;
            if (!std::getline(std::cin, inputLine)) {
                inputLine = "";
            }

            // Return the result as a RuntimeValue variant
            RuntimeValue result;
            result.value = inputLine;
            return result;
        });

    // Register INPUT in the global scope
    RuntimeValue inputVal;
    inputVal.value = std::static_pointer_cast<Callable>(inputNative);
    globals->define("INPUT", inputVal);

    /**
     * PRINT native function
     * >> PRINT(... vars: Any)
     * => Null
     * Outputs values to the console followed by a newline.
     * Multiple arguments are separated by spaces.
     */
    auto printNative = std::make_shared<NativeFunction>(
        VARIADIC_ARITY, [](Interpreter &, std::vector<RuntimeValue> args) -> RuntimeValue {
            for (size_t i = 0; i < args.size(); ++i) {
                if (i > 0)
                    std::cout << " ";
                std::cout << stringify(args[i]);
            }
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
     * >> OUTPUT(... vars: Any)
     * => Null
     * Outputs values to the console WITHOUT a trailing newline.
     * Multiple arguments are separated by spaces.
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
     * => Integer
     * Explicitly casts a value to an integer.
     */
    auto intNative = std::make_shared<NativeFunction>(
        1, [](Interpreter &, std::vector<RuntimeValue> args) -> RuntimeValue {
            if (args.empty())
                return {Null{}};
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
                    // Fallback for failed parsing
                    result = -1;
                }
            }

            RuntimeValue retVal;
            retVal.value = result;
            return retVal;
        });

    globals->define("INT", {std::static_pointer_cast<Callable>(intNative)});

    /**
     * FLOAT native function
     * >> FLOAT(value: Any)
     * => Double
     * Explicitly casts a value to a floating point number.
     */
    auto floatNative = std::make_shared<NativeFunction>(
        1, [](Interpreter &, std::vector<RuntimeValue> args) -> RuntimeValue {
            if (args.empty())
                return {Null{}};
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

    globals->define("FLOAT", {std::static_pointer_cast<Callable>(floatNative)});

    /**
     * STRING native function
     * >> STRING(value: Any)
     * => String
     * Converts any value to its string representation.
     */
    auto stringNative = std::make_shared<NativeFunction>(
        1, [](Interpreter &, std::vector<RuntimeValue> args) -> RuntimeValue {
            if (args.empty())
                return {Null{}};
            std::string result = stringify(args[0]);

            RuntimeValue retVal;
            retVal.value = result;
            return retVal;
        });

    globals->define("STRING", {std::static_pointer_cast<Callable>(stringNative)});

    /**
     * BOOL native function
     * >> BOOL(value: Any)
     * => Boolean
     * Explicitly casts a value to a boolean.
     */
    auto boolNative = std::make_shared<NativeFunction>(
        1, [](Interpreter &, std::vector<RuntimeValue> args) -> RuntimeValue {
            if (args.empty())
                return {Null{}};
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
                result        = (s == "true" || s == "TRUE");
            } else if (!val.is<Null>()) {
                result = true;
            }

            RuntimeValue retVal;
            retVal.value = result;
            return retVal;
        });

    globals->define("BOOL", {std::static_pointer_cast<Callable>(boolNative)});

    /**
     * RANDOM native function
     * >> RANDOM(min: Integer, max: Integer)
     * => Integer
     * Returns a pseudo-random integer within the specified range [min, max].
     */
    auto randomNative = std::make_shared<NativeFunction>(
        2, [](Interpreter &, std::vector<RuntimeValue> args) -> RuntimeValue {
            if (!args[0].is<int>() || !args[1].is<int>()) {
                // Returns 0 if parameters are wrong type
                return {0};
            }

            int min = args[0].as<int>();
            int max = args[1].as<int>();

            // Inverted ranges when the user is a dumbass
            if (min > max)
                std::swap(min, max);

            static std::mt19937 gen(std::random_device{}());
            std::uniform_int_distribution<> dis(min, max);

            RuntimeValue retVal;
            retVal.value = dis(gen);
            return retVal;
        });

    globals->define("RANDOM", {std::static_pointer_cast<Callable>(randomNative)});

    /**
     * TIME native function
     * >> TIME()
     * => Double
     * Returns the number of seconds since the Unix epoch.
     */
    auto timeNative = std::make_shared<NativeFunction>(
        0, [](Interpreter &, std::vector<RuntimeValue> args) -> RuntimeValue {
            (void) args;
            auto now = std::chrono::system_clock::now();
            auto duration =
                std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());

            // Convert milliseconds to seconds
            RuntimeValue retVal;
            retVal.value = static_cast<double>(duration.count()) / 1000.0;
            return retVal;
        });

    globals->define("TIME", {std::static_pointer_cast<Callable>(timeNative)});

    /**
     * TYPE native function
     * >> TYPE(value: Any)
     * => String
     * Returns the name of the value's internal type as a string.
     */
    auto typeNative = std::make_shared<NativeFunction>(
        1, [](Interpreter &, std::vector<RuntimeValue> args) -> RuntimeValue {
            if (args.empty())
                return {"NULL"};
            const auto &val = args[0];

            std::string typeStr = "UNKNOWN";
            if (val.is<int>())
                typeStr = "INT";
            else if (val.is<double>())
                typeStr = "FLOAT";
            else if (val.is<bool>())
                typeStr = "BOOL";
            else if (val.is<std::string>())
                typeStr = "STRING";
            else if (val.is<Null>())
                typeStr = "NULL";
            else if (val.is<std::shared_ptr<std::vector<RuntimeValue>>>())
                typeStr = "LIST";
            else if (val.is<std::shared_ptr<Callable>>())
                typeStr = "CALLABLE";
            else if (val.is<std::shared_ptr<Instance>>())
                typeStr = "INSTANCE";

            RuntimeValue retVal;
            retVal.value = typeStr;
            return retVal;
        });

    globals->define("TYPE", {std::static_pointer_cast<Callable>(typeNative)});
}

/**
 * Execute Statement
 * Dispatches the statement to the appropriate StmtVisitor implementation.
 * @param stmt Pointer to the statement to execute
 */
void Interpreter::execute(Stmt *stmt) {
    stmt->accept(*this);
}

/**
 * Evaluate Expression
 * Evaluates an expression AST node into a concrete RuntimeValue.
 * Uses manual dispatch (downcasting) as an alternative to the visitor pattern
 * for better integration with return values.
 * @param expr The expression to evaluate
 * @return The resulting value
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
    if (auto *e = dynamic_cast<UnaryExpr *>(expr))
        return visitUnaryExpr(e);
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

    // Fallback for null expressions
    RuntimeValue null;
    null.value = Null{};
    return null;
}

/**
 * Execute Scoped Block
 * Evaluates a list of statements within a provided environment.
 * Restores the previous environment after the block finishes, even if an error occurs.
 * @param statements Statement nodes in the block
 * @param env The environment scope for this block
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
        // Ensure environment is restored on exceptions (like ReturnSignal)
        environment = previous;
        throw;
    }
}

/**
 * Logic: Truthiness
 * Determines the boolean interpretation of any runtime value.
 * - Null is false
 * - False is false
 * - 0 and 0.0 are false
 * - Everything else is true
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
 * Visit: Literal Expression
 * Converts a primitive AST literal into its runtime equivalent.
 * @param expr Literal node (TOK_INTEGER, TOK_FLOAT, TOK_STRING, TOK_TRUE, TOK_FALSE)
 * @return The corresponding RuntimeValue
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
 * Visit: Variable Reference
 * Retrieves the value of an identifier from the current environment scope.
 * @param expr Variable node containing the name token
 * @return The value currently bound to the name
 */
RuntimeValue Interpreter::visitVariableExpr(VariableExpr *expr) {
    return environment->get(expr->name);
}

/**
 * Visit: Assignment
 * Evaluates the right-hand side and binds it to the target (variable, field, or array element).
 * Supports complex targets like object properties and array indices.
 * @param expr Assignment node (target = value)
 * @return The assigned value
 */
RuntimeValue Interpreter::visitAssignExpr(AssignExpr *expr) {
    RuntimeValue value = evaluate(expr->value.get());

    if (auto *varExpr = dynamic_cast<VariableExpr *>(expr->target.get())) {
        // Simple variable assignment
        environment->define(varExpr->name.lexeme, value);
    } else if (auto *getExpr = dynamic_cast<GetExpr *>(expr->target.get())) {
        // Object property assignment (object.field = value)
        RuntimeValue object = evaluate(getExpr->object.get());
        if (!object.is<std::shared_ptr<Instance>>()) {
            throw RuntimeError(getExpr->name, "Only instances have fields.");
        }
        object.as<std::shared_ptr<Instance>>()->set(getExpr->name, value);
    } else if (auto *arrayAccess = dynamic_cast<ArrayAccessExpr *>(expr->target.get())) {
        // Array element assignment (array[index] = value)
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
 * Visit: Unary Expression
 * Handles logical negation (NOT) and numeric negation (-).
 * @param expr Unary node containing operator and operand
 * @return Result of the operation
 */
RuntimeValue Interpreter::visitUnaryExpr(UnaryExpr *expr) {
    RuntimeValue right = evaluate(expr->right.get());

    // Switch for both NOT and -
    switch (expr->op.type) {
    case TOK_MINUS:
        checkNumberOperand(expr->op, right);
        if (right.is<double>()) {
            return {-right.as<double>()};
        }
        return {-right.as<int>()};
    case TOK_NOT:
        return {!isTruthy(right)};
    default:
        throw RuntimeError(expr->op, "Invalid unary operator.");
    }
}

/**
 * Visit: Binary Expression
 * Handles arithmetic (+, -, *, /), comparison (>, >=, <, <=),
 * equality (==), logical (AND, OR), and collection membership (IN) operators.
 * @param expr Binary node containing operator and operands
 * @return Result of the operation
 */
RuntimeValue Interpreter::visitBinaryExpr(BinaryExpr *expr) {
    RuntimeValue left  = evaluate(expr->left.get());
    RuntimeValue right = evaluate(expr->right.get());
    RuntimeValue result;

    switch (expr->op.type) {
    case TOK_PLUS: {
        // String Concatenation or Numeric Addition
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
        // Numeric Subtraction
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
        // Numeric Multiplication
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
        // Numeric Division (always returns double)
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
        // Generic Equality Check
        result.value = isEqual(left, right);
        break;
    }
    case TOK_NOT_EQUAL: {
        // Generic Inequality Check
        result.value = !isEqual(left, right);
        break;
    }
    case TOK_AND: {
        // Logical AND (short-circuiting)
        if (!isTruthy(left))
            return left;
        return evaluate(expr->right.get());
    }
    case TOK_OR: {
        // Logical OR (short-circuiting)
        if (isTruthy(left))
            return left;
        return evaluate(expr->right.get());
    }
    case TOK_IN: {
        // Collection Membership
        if (!right.is<std::shared_ptr<std::vector<RuntimeValue>>>()) {
            throw RuntimeError(expr->op, "'IN' operator requires right hand side to be an array.");
        }

        auto arr     = right.as<std::shared_ptr<std::vector<RuntimeValue>>>();
        result.value = false;

        for (const auto &item : *arr) {
            if (isEqual(left, item)) {
                result.value = true;
                break;
            }
        }
        break;
    }
    default:
        result.value = Null{};
    }

    return result;
}

/**
 * Visit: Function or Constructor Call
 * Evaluates the callee and arguments, then invokes the call method.
 * @param expr Call node (callee(args))
 * @return Return value from the invocation
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

    std::shared_ptr<Callable> function = callee.as<std::shared_ptr<Callable>>();

    // Validate argument count (arity)
    if (function->arity() != VARIADIC_ARITY &&
        (size_t) arguments.size() != (size_t) function->arity()) {
        throw RuntimeError(expr->anchor, "Expected " + std::to_string(function->arity()) +
                                             " arguments but got " +
                                             std::to_string(arguments.size()) + ".");
    }

    return function->call(*this, arguments);
}

/**
 * Visit: Property Access
 * Retrieves a field or method from a class instance.
 * @param expr Get node (object.name)
 * @return Property value or bound method
 */
RuntimeValue Interpreter::visitGetExpr(GetExpr *expr) {
    RuntimeValue object = evaluate(expr->object.get());

    /**
     * Special case: Arrays as objects
     */
    if (object.is<std::shared_ptr<std::vector<RuntimeValue>>>()) {
        auto arr         = object.as<std::shared_ptr<std::vector<RuntimeValue>>>();
        std::string name = expr->name.lexeme;

        // ===============================
        // Array Methods
        // ===============================
        // arr.append('item')
        if (name == "append") {
            // Return a native which simply appends the list
            auto appendMethod = std::make_shared<NativeFunction>(
                1, [arr](Interpreter&, std::vector<RuntimeValue> args) -> RuntimeValue {
                    arr->push_back(args[0]);
                    return {arr};
                }
            );

            RuntimeValue method;
            method.value = std::static_pointer_cast<Callable>(appendMethod);
            return method;
        }

        // ===============================
        // Array Properties
        // ===============================
        // arr.length
        if (name == "length") {
            RuntimeValue len;
            len.value = static_cast<int>(arr->size());
            return len;
        }
    }

    // Ensure that we are only accessing instances
    if (!object.is<std::shared_ptr<Instance>>()) {
        throw RuntimeError(expr->name, "Only instances have properties.");
    }

    auto instance    = object.as<std::shared_ptr<Instance>>();
    std::string name = expr->name.lexeme;

    // First check for fields on the instance
    if (instance->fields.count(name)) {
        return instance->fields.at(name);
    }

    // Then look for methods in the instance's class
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
 * Visit: Array Access
 * Retrieves an element from an array using a numeric index.
 * @param expr Index node (array[index])
 * @return The value at the specified position
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
 * Visit: Array Literal
 * Creates a new heap-allocated vector holding the evaluated elements.
 * @param expr List of expressions [e1, e2, ...]
 * @return A shared pointer to the new array
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
 * Visit: Class Instantiation
 * Creates a new object instance. Syntactic sugar for calling a class.
 * @param expr New node (NEW ClassName(args))
 * @return The created instance
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

    if (klass->arity() != VARIADIC_ARITY && arguments.size() != (size_t) klass->arity()) {
        throw RuntimeError(expr->className, "Expected " + std::to_string(klass->arity()) +
                                                " arguments but got " +
                                                std::to_string(arguments.size()) + ".");
    }

    return klass->call(*this, arguments);
}

/**
 * Visit: Expression Statement
 * Evaluates an expression and ignores its result.
 * Used for calls or assignments that stand alone as statements.
 */
void Interpreter::visitExpressionStmt(ExpressionStmt *stmt) {
    evaluate(stmt->expression.get());
}

/**
 * Visit: Return Statement
 * Triggers a stack unwind by throwing a ReturnSignal.
 * This is the only way to exit a function body early with a value.
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
 * Visit: Block Statement
 * Executes a group of statements within a new local environment scope.
 */
void Interpreter::visitBlockStmt(BlockStmt *stmt) {
    executeBlock(stmt->statements, std::make_shared<Environment>(environment));
}

/**
 * Visit: If Statement
 * Evaluates the condition and conditionally executes the then or else branch.
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
 * Visit: While Statement
 * Executes the body repeatedly as long as the condition remains truthy.
 */
void Interpreter::visitWhileStmt(WhileStmt *stmt) {
    while (isTruthy(evaluate(stmt->condition.get()))) {
        for (const auto &s : stmt->body) {
            execute(s.get());
        }
    }
}

/**
 * Visit: Function Declaration
 * Creates a UserFunction object and defines it in the current scope.
 */
void Interpreter::visitFunctionStmt(FunctionStmt *stmt) {
    auto function = std::make_shared<UserFunction>(stmt, environment);
    RuntimeValue funcValue;
    funcValue.value = std::static_pointer_cast<Callable>(function);
    environment->define(stmt->name.lexeme, funcValue);
}

/**
 * Visit: Class Declaration
 * Creates a UserClass object, populates it with methods and field defaults,
 * and defines it in the current scope.
 */
void Interpreter::visitClassStmt(ClassStmt *stmt) {
    auto klass = std::make_shared<UserClass>(stmt->name.lexeme);

    // Add the class's default attributes (fields)
    for (const auto &attr : stmt->attributes) {
        if (auto *assignment = dynamic_cast<AssignExpr *>(attr.get())) {
            if (auto *varExpr = dynamic_cast<VariableExpr *>(assignment->target.get())) {
                std::string fieldName = varExpr->name.lexeme;
                RuntimeValue val      = evaluate(assignment->value.get());
                klass->addField(fieldName, val);
            } else {
                throw RuntimeError(assignment->anchor, "Expected a valid field name.");
            }
        }
    }

    // Add methods to the class definition
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
 * Visit: For-In Loop
 * Iterates over the elements of an array, executing the body for each one.
 * Binds the current element to a local loop variable.
 */
void Interpreter::visitForInStmt(ForInStmt *stmt) {
    RuntimeValue iterable = evaluate(stmt->iterable.get());

    if (!iterable.is<std::shared_ptr<std::vector<RuntimeValue>>>()) {
        throw RuntimeError(stmt->variable, "Can only iterate over arrays.");
    }

    auto arr = iterable.as<std::shared_ptr<std::vector<RuntimeValue>>>();

    for (const auto &element : *arr) {
        // Create a fresh scope for each iteration to avoid leaks between iterations
        auto loopEnv = std::make_shared<Environment>(environment);
        loopEnv->define(stmt->variable.lexeme, element);
        executeBlock(stmt->body, loopEnv);
    }
}
