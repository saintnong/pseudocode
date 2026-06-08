#pragma once

#include "token.hpp"
#include <map>
#include <memory>
#include <stdexcept>
#include <variant>
#include <vector>

// --- Forward Declarations ---
struct Callable;
struct Instance;
struct Dictionary;
class Interpreter;

/**
 * Null Type
 * Represents the absence of a value in Pseudocode.
 */
using Null = std::monostate;

// --- Runtime Value System ---

/**
 * Value Variant
 * A variant type that can hold any valid runtime value in the Pseudocode language.
 * This includes primitive types (int, double, bool, string), collections (arrays),
 * and object-oriented constructs (callables, instances).
 */
using Value = std::variant<Null,                                              // null
                           int,                                               // integer
                           double,                                            // decimal
                           bool,                                              // boolean
                           std::string,                                       // string
                           std::shared_ptr<std::vector<struct RuntimeValue>>, // Array (shared)
                           std::shared_ptr<Dictionary>,                       // Dictionary (shared)
                           std::shared_ptr<Callable>,                         // Function/Class
                           std::shared_ptr<Instance>                          // Object Instance
                           >;

/**
 * RuntimeValue Wrapper
 * A thin wrapper around the Value variant.
 * Provides convenience methods for type checking and safe casting.
 */
struct RuntimeValue {
    Value value;

    /**
     * Check if the value is of a specific type
     * @tparam T The type to check for
     * @return true if the variant holds type T
     */
    template <typename T> bool is() const {
        return std::holds_alternative<T>(value);
    }

    /**
     * Cast the value to a specific type (const)
     * @tparam T The type to cast to
     * @return Const reference to the value as type T
     */
    template <typename T> const T &as() const {
        return std::get<T>(value);
    }

    /**
     * Cast the value to a specific type (mutable)
     * @tparam T The type to cast to
     * @return Reference to the value as type T
     */
    template <typename T> T &as() {
        return std::get<T>(value);
    }
};

/**
 * Dictionary Type
 * An ordered collection of key-value pairs. Keys must be strings, integers, or booleans.
 */
struct Dictionary {
    std::vector<std::pair<RuntimeValue, RuntimeValue>> entries;
};

using DictPtr = std::shared_ptr<Dictionary>;

// --- Execution Exceptions ---

/**
 * RuntimeError
 * Exception thrown when a terminal error occurs during script execution.
 * Captures the problematic token for high-quality error reporting.
 */
class RuntimeError : public std::runtime_error {
public:
    // The token where the error occurred
    const Token &token;

    /**
     * Create a new runtime error
     * @param token The token associated with the error
     * @param message Descriptive error message
     */
    RuntimeError(const Token &token, const std::string &message)
        : std::runtime_error(message), token(token) {
    }
};

// --- Scope Management ---

/**
 * Environment
 * Manages variable bindings and scope nesting.
 * Each environment holds a map of identifiers to values and points to its parent scope.
 */
class Environment : public std::enable_shared_from_this<Environment> {
    // Variable storage for the current scope
    std::map<std::string, RuntimeValue> values;
    // Pointer to the surrounding (outer) scope
    std::shared_ptr<Environment> enclosing;

public:
    /**
     * Create a top-level (global) environment
     */
    Environment() : enclosing(nullptr) {
    }

    /**
     * Create a local environment nested within another
     * @param enclosing The parent environment
     */
    Environment(std::shared_ptr<Environment> enclosing) : enclosing(enclosing) {
    }

    /**
     * Define or update a variable in the current scope
     * @param name The identifier name
     * @param value The value to bind
     */
    void define(const std::string &name, RuntimeValue value) {
        values[name] = value;
    }

    /**
     * Retrieve a variable's value, searching up the scope chain
     * @param name The token representing the variable name
     * @return The found RuntimeValue
     * @throws RuntimeError if the variable is not defined in any accessible scope
     */
    RuntimeValue get(const Token &name) {
        if (values.count(name.lexeme)) {
            return values.at(name.lexeme);
        }
        if (enclosing)
            return enclosing->get(name);

        throw RuntimeError(name, "Undefined variable '" + name.lexeme + "'.");
    }
};

// --- Core Runtime Interfaces ---

/**
 * Callable Interface
 * Represents anything that can be "called" like a function or constructor.
 */
struct Callable {
    virtual ~Callable() = default;

    /**
     * @return The number of arguments the callable expects
     */
    virtual int arity() = 0;

    /**
     * Invoke the callable
     * @param interpreter The active interpreter instance
     * @param arguments List of evaluated runtime arguments
     * @return The result of the invocation
     */
    virtual RuntimeValue call(Interpreter &interpreter, std::vector<RuntimeValue> arguments) = 0;

    /**
     * @return A string representation of the callable (e.g. <FUNCTION name>)
     */
    virtual std::string toString() = 0;
};

/**
 * Instance Structure
 * Represents a concrete instance of a user-defined class.
 * Holds its own state (fields) and maintains a link to its class definition.
 */
struct Instance {
    // Reference to the class that created this instance
    std::shared_ptr<Callable> klass;
    // Instance-specific field storage (shared between views)
    std::shared_ptr<std::map<std::string, RuntimeValue>> fields;
    // Optional superclass override for 'super' lookups
    std::shared_ptr<Callable> superclassContext;

    /**
     * Create a new instance of a class
     * @param k Shared pointer to the class definition
     */
    Instance(std::shared_ptr<Callable> k)
        : klass(k), fields(std::make_shared<std::map<std::string, RuntimeValue>>()),
          superclassContext(nullptr) {
    }

    /**
     * Create a new view of an instance with a different class context (for super)
     * @param other The instance to copy from
     * @param context The superclass context to use
     */
    Instance(const Instance &other, std::shared_ptr<Callable> context)
        : klass(other.klass), fields(other.fields), superclassContext(context) {
    }

    /**
     * Access a property on this instance
     * @param name The token representing the property name
     * @return The value of the property
     * @throws RuntimeError if the property does not exist
     */
    RuntimeValue get(const Token &name) {
        if (fields->count(name.lexeme)) {
            return fields->at(name.lexeme);
        }
        throw RuntimeError(name, "Undefined property '" + name.lexeme + "'.");
    }

    /**
     * Set the value of a property on this instance
     * @param name The token representing the property name
     * @param value The new value to assign
     */
    void set(const Token &name, RuntimeValue value) {
        (*fields)[name.lexeme] = value;
    }
};

/**
 * Stringify Utility
 * Converts any RuntimeValue into its string representation for display.
 * Handles recursion for collections like arrays.
 * @param v The runtime value to stringify
 * @return String representation of the value
 */
inline std::string stringify(const RuntimeValue &v) {
    // Handle Null
    if (v.is<Null>())
        return "Null";

    // Handle Integers
    if (v.is<int>()) {
        return std::to_string(v.as<int>());
    }

    // Handle Doubles (with trailing zero trimming)
    if (v.is<double>()) {
        std::string text = std::to_string(v.as<double>());
        text.erase(text.find_last_not_of('0') + 1, std::string::npos);
        if (text.back() == '.')
            text.pop_back();
        return text;
    }

    // Handle Booleans
    if (v.is<bool>())
        return v.as<bool>() ? "true" : "false";

    // Handle Strings
    if (v.is<std::string>())
        return v.as<std::string>();

    // Handle Arrays (Recursive)
    if (v.is<std::shared_ptr<std::vector<RuntimeValue>>>()) {
        auto arr           = v.as<std::shared_ptr<std::vector<RuntimeValue>>>();
        std::string result = "[";
        for (size_t i = 0; i < arr->size(); ++i) {
            result += stringify((*arr)[i]);
            if (i < arr->size() - 1)
                result += ", ";
        }
        result += "]";
        return result;
    }

    // Handle Dictionaries (Recursive)
    if (v.is<std::shared_ptr<Dictionary>>()) {
        auto dict          = v.as<std::shared_ptr<Dictionary>>();
        std::string result = "{";
        for (size_t i = 0; i < dict->entries.size(); ++i) {
            result += stringify(dict->entries[i].first);
            result += ": ";
            result += stringify(dict->entries[i].second);
            if (i < dict->entries.size() - 1)
                result += ", ";
        }
        result += "}";
        return result;
    }

    // Handle Callables (Functions/Classes)
    if (v.is<std::shared_ptr<Callable>>())
        return v.as<std::shared_ptr<Callable>>()->toString();

    // Handle Class Instances
    if (v.is<std::shared_ptr<Instance>>())
        return v.as<std::shared_ptr<Instance>>()->klass->toString();

    return "unknown";
}
