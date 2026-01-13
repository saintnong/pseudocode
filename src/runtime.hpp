#pragma once

#include "token.hpp"
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <variant>
#include <vector>

// Forward declarations
struct Callable;
struct Instance;
class Interpreter;

using Null = std::monostate;

// --- Value Type Definition ---

// A variant to hold any supported runtime value
using Value = std::variant<Null,                                              // null
                           int,                                               // integer
                           double,                                            // decimal
                           bool,                                              // boolean
                           std::string,                                       // string
                           std::shared_ptr<std::vector<struct RuntimeValue>>, // Array (shared for
                                                                              // ref semantics)
                           std::shared_ptr<Callable>,                         // Function/Class
                           std::shared_ptr<Instance>                          // Object Instance
                           >;

// Wrapper struct to allow recursive definition in variant (for Arrays)
struct RuntimeValue {
    Value value;

    template <typename T> bool is() const {
        return std::holds_alternative<T>(value);
    }
    template <typename T> const T &as() const {
        return std::get<T>(value);
    }
    template <typename T> T &as() {
        return std::get<T>(value);
    }
};

// --- Exceptions ---

// Thrown for runtime errors (e.g., divide by zero)
class RuntimeError : public std::runtime_error {
public:
    const Token &token;
    RuntimeError(const Token &token, const std::string &message)
        : std::runtime_error(message), token(token) {
    }
};

// --- Environment (Scope) ---

class Environment : public std::enable_shared_from_this<Environment> {
    std::map<std::string, RuntimeValue> values;
    std::shared_ptr<Environment> enclosing;

public:
    Environment() : enclosing(nullptr) {
    }
    Environment(std::shared_ptr<Environment> enclosing) : enclosing(enclosing) {
    }

    void define(const std::string &name, RuntimeValue value) {
        values[name] = value;
    }

    RuntimeValue get(const Token &name) {
        if (values.count(name.lexeme)) {
            return values.at(name.lexeme);
        }
        if (enclosing)
            return enclosing->get(name);

        throw RuntimeError(name, "Undefined variable '" + name.lexeme + "'.");
    }
};

// --- Interfaces for Callables and Instances ---

struct Callable {
    virtual ~Callable() = default;
    virtual int arity() = 0;
    virtual RuntimeValue call(Interpreter &interpreter, std::vector<RuntimeValue> arguments) = 0;
    virtual std::string toString()                                                           = 0;
};

struct Instance {
    std::shared_ptr<Callable> klass; // Reference to class (which is a callable)
    std::map<std::string, RuntimeValue> fields;

    Instance(std::shared_ptr<Callable> k) : klass(k) {
    }

    /**
     * Class getter
     * Returns the class's property
     */
    RuntimeValue get(const Token &name) {
        if (fields.count(name.lexeme)) {
            return fields.at(name.lexeme);
        }
        throw RuntimeError(name, "Undefined property '" + name.lexeme + "'.");
    }

    void set(const Token &name, RuntimeValue value) {
        fields[name.lexeme] = value;
    }
};

/**
 * Stringify a Runtime value.
 * Used when the values are printed.
 */
inline std::string stringify(const RuntimeValue &v) {
    /**
     * Null just goes to 'Null'
     */
    if (v.is<Null>())
        return "Null";

    /**
     * Ints are converted using the C++ standard libary
     */
    if (v.is<int>()) {
        std::string text = std::to_string(v.as<int>());
        return text;
    }

    /**
     * Doubles are converted to strings like you would expect,
     * except they have trailing zeroes chopped off to look nicer.
     * E.g. 4.5900000 -> 4.59
     */
    if (v.is<double>()) {
        std::string text = std::to_string(v.as<double>());
        // Trim trailing zeros for integer-like doubles
        text.erase(text.find_last_not_of('0') + 1, std::string::npos);
        if (text.back() == '.')
            text.pop_back();
        return text;
    }

    /**
     * Bools go to true/false
     */
    if (v.is<bool>())
        return v.as<bool>() ? "true" : "false";

    /**
     * Strings are just cast without any modification
     */
    if (v.is<std::string>())
        return v.as<std::string>();

    /**
     * Vectors stringified recursively.
     * It ends up looking like python:
     * E.g. [1, 2, 3]
     */
    if (v.is<std::shared_ptr<std::vector<RuntimeValue>>>()) {
        // Convert this vector to a string representation
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
    /**
     * Functions are stringified using their class method.
     * Ends up looking like:
     * <FUNCTION myFunc>
     */
    if (v.is<std::shared_ptr<Callable>>())
        return v.as<std::shared_ptr<Callable>>()->toString();

    /**
     * Classes are stringified using the Instance's parent class's attached method.
     * Ends up looking like:
     * <CLASS myFunc>
     */
    if (v.is<std::shared_ptr<Instance>>())
        return v.as<std::shared_ptr<Instance>>()->klass->toString();
    return "unknown";
}
