#include "vm.hpp"
#include "opcodes.hpp"
#include "interpreter.hpp"
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <algorithm>

VM::VM(Interpreter &interpreter, ErrorReporter &reporter, EnvironmentPtr globals)
    : interpreter(interpreter), reporter(reporter), globals(globals) {}

void VM::push(RuntimeValue value) {
    if (stackTop >= STACK_MAX) {
        runtimeError("Stack overflow.");
    }
    stack[stackTop++] = value;
}

RuntimeValue VM::pop() {
    if (stackTop == 0) {
        runtimeError("Stack underflow.");
    }
    return stack[--stackTop];
}

RuntimeValue VM::peek(int distance) {
    if (stackTop <= static_cast<size_t>(distance)) {
        runtimeError("Stack underflow during peek.");
    }
    return stack[stackTop - 1 - distance];
}

bool VM::isTruthy(const RuntimeValue &value) const {
    if (value.is<Null>()) return false;
    if (value.is<bool>()) return value.as<bool>();
    if (value.is<int>()) return value.as<int>() != 0;
    if (value.is<double>()) return value.as<double>() != 0.0;
    if (value.is<std::string>()) return !value.as<std::string>().empty();
    return true;
}

bool VM::isEqual(const RuntimeValue &a, const RuntimeValue &b) const {
    if (a.value.index() != b.value.index()) {
        // Allow comparison between int and double
        if (a.is<int>() && b.is<double>()) {
            return static_cast<double>(a.as<int>()) == b.as<double>();
        }
        if (a.is<double>() && b.is<int>()) {
            return a.as<double>() == static_cast<double>(b.as<int>());
        }
        return false;
    }
    if (a.is<Null>()) return true;
    if (a.is<int>()) return a.as<int>() == b.as<int>();
    if (a.is<double>()) return a.as<double>() == b.as<double>();
    if (a.is<bool>()) return a.as<bool>() == b.as<bool>();
    if (a.is<std::string>()) return a.as<std::string>() == b.as<std::string>();
    if (a.is<std::shared_ptr<std::vector<RuntimeValue>>>()) {
        return a.as<std::shared_ptr<std::vector<RuntimeValue>>>() == b.as<std::shared_ptr<std::vector<RuntimeValue>>>();
    }
    if (a.is<std::shared_ptr<Dictionary>>()) {
        return a.as<std::shared_ptr<Dictionary>>() == b.as<std::shared_ptr<Dictionary>>();
    }
    if (a.is<std::shared_ptr<Callable>>()) {
        return a.as<std::shared_ptr<Callable>>() == b.as<std::shared_ptr<Callable>>();
    }
    if (a.is<std::shared_ptr<Instance>>()) {
        return a.as<std::shared_ptr<Instance>>() == b.as<std::shared_ptr<Instance>>();
    }
    return false;
}

void VM::runtimeError(const std::string &message) {
    size_t line = 1;
    if (frameCount > 0) {
        CallFrame *frame = &frames[frameCount - 1];
        size_t offset = frame->ip - frame->function->chunk->code.data() - 1;
        line = frame->function->chunk->getLine(offset);
    }
    // Report syntax error? No, runtime error.
    // SCSA has a RuntimeError exception that main.cpp catches
    throw RuntimeError(Token{TOK_EOF, "", line, 0, 0}, message);
}

RuntimeValue VM::run(std::shared_ptr<CompiledFunction> function, const std::vector<RuntimeValue> &args, EnvironmentPtr closure) {
    size_t savedInitialFrameCount = runInitialFrameCount;
    size_t initialFrameCount = frameCount;
    runInitialFrameCount = frameCount;

    // Push dummy callee for function callframe setup consistency
    push(RuntimeValue{Null{}});
    for (const auto &arg : args) {
        push(arg);
    }

    CallFrame callFrame;
    callFrame.function = function;
    callFrame.ip = function->chunk->code.data();
    callFrame.slotsBase = stackTop - args.size();
    callFrame.closure = closure;
    frames[frameCount++] = callFrame;

    try {
        execute();
    } catch (...) {
        // Reset frame count and stack on exception to prevent VM corruptions
        frameCount = initialFrameCount;
        runInitialFrameCount = savedInitialFrameCount;
        throw;
    }

    frameCount = initialFrameCount;
    runInitialFrameCount = savedInitialFrameCount;
    return pop();
}

void VM::execute() {
    CallFrame *frame = &frames[frameCount - 1];

#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() (frame->function->chunk->constants[READ_SHORT()])

    while (true) {
        uint8_t instruction = READ_BYTE();
        switch (instruction) {
        case OP_CONSTANT: {
            push(READ_CONSTANT());
            break;
        }
        case OP_NULL:  push(RuntimeValue{Null{}}); break;
        case OP_TRUE:  push(RuntimeValue{true}); break;
        case OP_FALSE: push(RuntimeValue{false}); break;
        case OP_POP:   pop(); break;
        case OP_DUP:   push(peek(0)); break;

        case OP_ADD: {
            RuntimeValue b = pop();
            RuntimeValue a = pop();
            if (a.is<std::string>() || b.is<std::string>()) {
                push(RuntimeValue{stringify(a) + stringify(b)});
            } else if (a.is<std::shared_ptr<std::vector<RuntimeValue>>>() && b.is<std::shared_ptr<std::vector<RuntimeValue>>>()) {
                auto arrA = a.as<std::shared_ptr<std::vector<RuntimeValue>>>();
                auto arrB = b.as<std::shared_ptr<std::vector<RuntimeValue>>>();
                auto newArr = std::make_shared<std::vector<RuntimeValue>>();
                newArr->insert(newArr->end(), arrA->begin(), arrA->end());
                newArr->insert(newArr->end(), arrB->begin(), arrB->end());
                push(RuntimeValue{newArr});
            } else if (a.is<double>() || b.is<double>()) {
                double valA = a.is<double>() ? a.as<double>() : a.as<int>();
                double valB = b.is<double>() ? b.as<double>() : b.as<int>();
                push(RuntimeValue{valA + valB});
            } else if (a.is<int>() && b.is<int>()) {
                push(RuntimeValue{a.as<int>() + b.as<int>()});
            } else {
                runtimeError("Operands must be numbers, strings, or arrays.");
            }
            break;
        }

        case OP_SUBTRACT: {
            RuntimeValue b = pop();
            RuntimeValue a = pop();
            if (a.is<double>() || b.is<double>()) {
                double valA = a.is<double>() ? a.as<double>() : a.as<int>();
                double valB = b.is<double>() ? b.as<double>() : b.as<int>();
                push(RuntimeValue{valA - valB});
            } else if (a.is<int>() && b.is<int>()) {
                push(RuntimeValue{a.as<int>() - b.as<int>()});
            } else {
                runtimeError("Operands must be numbers.");
            }
            break;
        }

        case OP_MULTIPLY: {
            RuntimeValue b = pop();
            RuntimeValue a = pop();
            if (a.is<std::string>() || b.is<std::string>()) {
                bool validLeft = a.is<std::string>() && b.is<int>();
                bool validRight = b.is<std::string>() && a.is<int>();
                if (!validLeft && !validRight) {
                    runtimeError("A string can only be multiplied by an integer.");
                }
                std::string base = a.is<std::string>() ? a.as<std::string>() : b.as<std::string>();
                int count = a.is<int>() ? a.as<int>() : b.as<int>();
                std::string res = "";
                if (count > 0) {
                    res.reserve(base.length() * count);
                    for (int i = 0; i < count; ++i) res += base;
                }
                push(RuntimeValue{res});
            } else if (a.is<std::shared_ptr<std::vector<RuntimeValue>>>() || b.is<std::shared_ptr<std::vector<RuntimeValue>>>()) {
                using ArrayPtr = std::shared_ptr<std::vector<RuntimeValue>>;
                bool validLeft = a.is<ArrayPtr>() && b.is<int>();
                bool validRight = b.is<ArrayPtr>() && a.is<int>();
                if (!validLeft && !validRight) {
                    runtimeError("An array can only be multiplied by an integer.");
                }
                auto base = a.is<ArrayPtr>() ? a.as<ArrayPtr>() : b.as<ArrayPtr>();
                int count = a.is<int>() ? a.as<int>() : b.as<int>();
                auto res = std::make_shared<std::vector<RuntimeValue>>();
                if (count > 0) {
                    res->reserve(base->size() * count);
                    for (int i = 0; i < count; ++i) {
                        res->insert(res->end(), base->begin(), base->end());
                    }
                }
                push(RuntimeValue{res});
            } else if (a.is<double>() || b.is<double>()) {
                double valA = a.is<double>() ? a.as<double>() : a.as<int>();
                double valB = b.is<double>() ? b.as<double>() : b.as<int>();
                push(RuntimeValue{valA * valB});
            } else if (a.is<int>() && b.is<int>()) {
                push(RuntimeValue{a.as<int>() * b.as<int>()});
            } else {
                runtimeError("Operands must be numbers.");
            }
            break;
        }

        case OP_DIVIDE: {
            RuntimeValue b = pop();
            RuntimeValue a = pop();
            double valA = a.is<double>() ? a.as<double>() : a.as<int>();
            double valB = b.is<double>() ? b.as<double>() : b.as<int>();
            if (valB == 0.0) {
                runtimeError("Division by zero.");
            }
            push(RuntimeValue{valA / valB});
            break;
        }

        case OP_NEGATE: {
            RuntimeValue a = pop();
            if (a.is<double>()) {
                push(RuntimeValue{-a.as<double>()});
            } else if (a.is<int>()) {
                push(RuntimeValue{-a.as<int>()});
            } else {
                runtimeError("Operand must be a number.");
            }
            break;
        }

        case OP_NOT: {
            push(RuntimeValue{!isTruthy(pop())});
            break;
        }

        case OP_EQUAL: {
            RuntimeValue b = pop();
            RuntimeValue a = pop();
            push(RuntimeValue{isEqual(a, b)});
            break;
        }

        case OP_NOT_EQUAL: {
            RuntimeValue b = pop();
            RuntimeValue a = pop();
            push(RuntimeValue{!isEqual(a, b)});
            break;
        }

        case OP_GREATER: {
            RuntimeValue b = pop();
            RuntimeValue a = pop();
            if ((a.is<double>() || a.is<int>()) && (b.is<double>() || b.is<int>())) {
                double valA = a.is<double>() ? a.as<double>() : a.as<int>();
                double valB = b.is<double>() ? b.as<double>() : b.as<int>();
                push(RuntimeValue{valA > valB});
            } else {
                runtimeError("Operands must be numbers.");
            }
            break;
        }

        case OP_GREATER_EQUAL: {
            RuntimeValue b = pop();
            RuntimeValue a = pop();
            if ((a.is<double>() || a.is<int>()) && (b.is<double>() || b.is<int>())) {
                double valA = a.is<double>() ? a.as<double>() : a.as<int>();
                double valB = b.is<double>() ? b.as<double>() : b.as<int>();
                push(RuntimeValue{valA >= valB});
            } else {
                runtimeError("Operands must be numbers.");
            }
            break;
        }

        case OP_LESS: {
            RuntimeValue b = pop();
            RuntimeValue a = pop();
            if ((a.is<double>() || a.is<int>()) && (b.is<double>() || b.is<int>())) {
                double valA = a.is<double>() ? a.as<double>() : a.as<int>();
                double valB = b.is<double>() ? b.as<double>() : b.as<int>();
                push(RuntimeValue{valA < valB});
            } else {
                runtimeError("Operands must be numbers.");
            }
            break;
        }

        case OP_LESS_EQUAL: {
            RuntimeValue b = pop();
            RuntimeValue a = pop();
            if ((a.is<double>() || a.is<int>()) && (b.is<double>() || b.is<int>())) {
                double valA = a.is<double>() ? a.as<double>() : a.as<int>();
                double valB = b.is<double>() ? b.as<double>() : b.as<int>();
                push(RuntimeValue{valA <= valB});
            } else {
                runtimeError("Operands must be numbers.");
            }
            break;
        }

        case OP_IN: {
            RuntimeValue coll = pop();
            RuntimeValue item = pop();
            if (coll.is<std::shared_ptr<std::vector<RuntimeValue>>>()) {
                auto arr = coll.as<std::shared_ptr<std::vector<RuntimeValue>>>();
                bool found = false;
                for (const auto &elem : *arr) {
                    if (isEqual(item, elem)) { found = true; break; }
                }
                push(RuntimeValue{found});
            } else if (coll.is<std::shared_ptr<Dictionary>>()) {
                auto dict = coll.as<std::shared_ptr<Dictionary>>();
                if (!isValidDictKey(item)) {
                    runtimeError("Dictionary keys must be strings, integers, or booleans.");
                }
                DictKey key = toDictKey(item);
                push(RuntimeValue{dict->entries.count(key) > 0});
            } else if (coll.is<std::string>()) {
                if (!item.is<std::string>()) {
                    runtimeError("Can only search for substrings inside strings.");
                }
                std::string s = coll.as<std::string>();
                std::string sub = item.as<std::string>();
                push(RuntimeValue{s.find(sub) != std::string::npos});
            } else {
                runtimeError("'IN' operator requires right hand side to be an array or dictionary.");
            }
            break;
        }

        case OP_GET_LOCAL: {
            uint16_t slot = READ_SHORT();
            push(stack[frame->slotsBase + slot]);
            break;
        }

        case OP_SET_LOCAL: {
            uint16_t slot = READ_SHORT();
            stack[frame->slotsBase + slot] = peek(0);
            break;
        }

        case OP_GET_GLOBAL: {
            std::string name = READ_CONSTANT().as<std::string>();
            RuntimeValue val;
            bool found = false;
            if (frame->closure) {
                try {
                    val = frame->closure->get(Token{TOK_IDENTIFIER, name, 0, 0, 0});
                    found = true;
                } catch (...) {}
            }
            if (!found) {
                try {
                    val = globals->get(Token{TOK_IDENTIFIER, name, 0, 0, 0});
                    found = true;
                } catch (...) {
                    runtimeError("Undefined variable '" + name + "'.");
                }
            }
            push(val);
            break;
        }

        case OP_SET_GLOBAL: {
            std::string name = READ_CONSTANT().as<std::string>();
            RuntimeValue val = peek(0);
            bool assigned = false;
            if (frame->closure) {
                try {
                    frame->closure->get(Token{TOK_IDENTIFIER, name, 0, 0, 0});
                    frame->closure->define(name, val);
                    assigned = true;
                } catch (...) {}
            }
            if (!assigned) {
                globals->define(name, val);
            }
            break;
        }

        case OP_DEFINE_GLOBAL: {
            std::string name = READ_CONSTANT().as<std::string>();
            globals->define(name, pop());
            break;
        }

        case OP_CLOSURE: {
            // SCSA has no closures, so OP_CLOSURE just loads the user function constant
            push(READ_CONSTANT());
            break;
        }

        case OP_GET_PROPERTY: {
            std::string name = READ_CONSTANT().as<std::string>();
            RuntimeValue object = pop();

            if (object.is<std::shared_ptr<Instance>>()) {
                auto instance = object.as<std::shared_ptr<Instance>>();

                // Check fields first
                if (instance->fields->count(name)) {
                    push(instance->fields->at(name));
                    break;
                }

                // Determine which class to look up methods on
                std::shared_ptr<UserClass> lookupClass;
                if (instance->superclassContext) {
                    lookupClass = std::dynamic_pointer_cast<UserClass>(instance->superclassContext);
                } else {
                    lookupClass = std::dynamic_pointer_cast<UserClass>(instance->klass);
                }

                if (lookupClass) {
                    // Special property: super
                    if (name == "super") {
                        if (!lookupClass->getSuperclass()) {
                            runtimeError("Class '" + lookupClass->toString() + "' has no superclass.");
                        }
                        auto superMethod = std::make_shared<NativeFunction>(
                            0,
                            [instance, lookupClass](Interpreter &, std::vector<RuntimeValue> args) -> RuntimeValue {
                                (void) args;
                                auto superInstance =
                                    std::make_shared<Instance>(*instance, lookupClass->getSuperclass());
                                return {superInstance};
                            });
                        push(RuntimeValue{std::static_pointer_cast<Callable>(superMethod)});
                        break;
                    }

                    auto method = lookupClass->findMethod(name);
                    if (method) {
                        push(RuntimeValue{method->bind(instance)});
                        break;
                    }
                }

                runtimeError("Undefined property '" + name + "'.");
            } else if (object.is<std::shared_ptr<std::vector<RuntimeValue>>>()) {
                auto arr = object.as<std::shared_ptr<std::vector<RuntimeValue>>>();
                if (name == "append") {
                    auto appendMethod = std::make_shared<NativeFunction>(
                        1, [arr](Interpreter &, std::vector<RuntimeValue> args) -> RuntimeValue {
                            arr->push_back(args[0]);
                            return {arr};
                        });
                    push(RuntimeValue{std::static_pointer_cast<Callable>(appendMethod)});
                } else if (name == "slice") {
                    auto sliceMethod = std::make_shared<NativeFunction>(
                        2, [arr](Interpreter &, std::vector<RuntimeValue> args) -> RuntimeValue {
                            if (!args[0].is<int>() || !args[1].is<int>()) {
                                throw RuntimeError(Token{TOK_IDENTIFIER, "slice", 0, 0, 0}, "Slice indices must be integers.");
                            }
                            int start = args[0].as<int>();
                            int end = args[1].as<int>();
                            int size = static_cast<int>(arr->size());
                            if (start < 0) start = 0;
                            if (end >= size) end = size - 1;
                            auto res = std::make_shared<std::vector<RuntimeValue>>();
                            if (start <= end && start < size) {
                                for (int i = start; i <= end; ++i) {
                                    res->push_back((*arr)[i]);
                                }
                            }
                            return {res};
                        });
                    push(RuntimeValue{std::static_pointer_cast<Callable>(sliceMethod)});
                } else if (name == "length") {
                    push(RuntimeValue{static_cast<int>(arr->size())});
                } else {
                    runtimeError("Unknown array property '" + name + "'.");
                }
            } else if (object.is<std::shared_ptr<Dictionary>>()) {
                auto dict = object.as<std::shared_ptr<Dictionary>>();
                if (name == "keys") {
                    auto keysMethod = std::make_shared<NativeFunction>(
                        0, [dict](Interpreter &, std::vector<RuntimeValue> args) -> RuntimeValue {
                            (void) args;
                            auto keys = std::make_shared<std::vector<RuntimeValue>>();
                            for (const auto &k : dict->keys) {
                                keys->push_back(fromDictKey(k));
                            }
                            return {keys};
                        });
                    push(RuntimeValue{std::static_pointer_cast<Callable>(keysMethod)});
                } else if (name == "values") {
                    auto valsMethod = std::make_shared<NativeFunction>(
                        0, [dict](Interpreter &, std::vector<RuntimeValue> args) -> RuntimeValue {
                            (void) args;
                            auto vals = std::make_shared<std::vector<RuntimeValue>>();
                            for (const auto &k : dict->keys) {
                                vals->push_back(dict->entries.at(k));
                            }
                            return {vals};
                        });
                    push(RuntimeValue{std::static_pointer_cast<Callable>(valsMethod)});
                } else if (name == "get") {
                    auto getMethod = std::make_shared<NativeFunction>(
                        VARIADIC_ARITY, [dict](Interpreter &, std::vector<RuntimeValue> args) -> RuntimeValue {
                            if (args.size() < 1 || args.size() > 2) {
                                throw RuntimeError(Token{TOK_IDENTIFIER, "get", 0, 0, 0},
                                    "Expected 1 or 2 arguments but got " + std::to_string(args.size()) + ".");
                            }
                            if (!isValidDictKey(args[0])) {
                                throw RuntimeError(Token{TOK_IDENTIFIER, "get", 0, 0, 0},
                                    "Dictionary keys must be strings, integers, or booleans.");
                            }
                            DictKey key = toDictKey(args[0]);
                            auto it = dict->entries.find(key);
                            if (it != dict->entries.end()) {
                                return it->second;
                            }
                            if (args.size() == 2) {
                                return args[1];
                            }
                            RuntimeValue nullVal; nullVal.value = Null{};
                            return nullVal;
                        });
                    push(RuntimeValue{std::static_pointer_cast<Callable>(getMethod)});
                } else if (name == "remove") {
                    auto removeMethod = std::make_shared<NativeFunction>(
                        1, [dict](Interpreter &, std::vector<RuntimeValue> args) -> RuntimeValue {
                            if (!isValidDictKey(args[0])) {
                                throw RuntimeError(Token{TOK_IDENTIFIER, "remove", 0, 0, 0}, "Dictionary keys must be strings, integers, or booleans.");
                            }
                            DictKey key = toDictKey(args[0]);
                            dict->entries.erase(key);
                            auto it = std::find(dict->keys.begin(), dict->keys.end(), key);
                            if (it != dict->keys.end()) dict->keys.erase(it);
                            RuntimeValue nullVal; nullVal.value = Null{};
                            return nullVal;
                        });
                    push(RuntimeValue{std::static_pointer_cast<Callable>(removeMethod)});
                } else if (name == "length") {
                    push(RuntimeValue{static_cast<int>(dict->keys.size())});
                } else {
                    runtimeError("This is not a valid dictionary property or method.");
                }
            } else if (object.is<std::string>()) {
                std::string s = object.as<std::string>();
                if (name == "length") {
                    push(RuntimeValue{static_cast<int>(s.length())});
                } else if (name == "slice") {
                    auto sliceMethod = std::make_shared<NativeFunction>(
                        2, [s](Interpreter &, std::vector<RuntimeValue> args) -> RuntimeValue {
                            if (!args[0].is<int>() || !args[1].is<int>()) {
                                throw RuntimeError(Token{TOK_IDENTIFIER, "slice", 0, 0, 0}, "Slice indices must be integers.");
                            }
                            int start = args[0].as<int>();
                            int end = args[1].as<int>();
                            int size = static_cast<int>(s.length());
                            if (start < 0) start = 0;
                            if (end >= size) end = size - 1;
                            std::string res = "";
                            if (start <= end && start < size) {
                                res = s.substr(start, end - start + 1);
                            }
                            return {res};
                        });
                    push(RuntimeValue{std::static_pointer_cast<Callable>(sliceMethod)});
                } else {
                    runtimeError("Unknown string property '" + name + "'.");
                }
            } else {
                runtimeError("Only instances have properties.");
            }
            break;
        }

        case OP_SET_PROPERTY: {
            std::string name = READ_CONSTANT().as<std::string>();
            RuntimeValue val = pop();
            RuntimeValue object = pop();
            if (!object.is<std::shared_ptr<Instance>>()) {
                runtimeError("Only instances have fields.");
            }
            object.as<std::shared_ptr<Instance>>()->set(Token{TOK_IDENTIFIER, name, 0, 0, 0}, val);
            push(val);
            break;
        }

        case OP_INDEX_GET: {
            RuntimeValue idxVal = pop();
            RuntimeValue container = pop();

            if (container.is<std::shared_ptr<std::vector<RuntimeValue>>>()) {
                if (!idxVal.is<int>()) {
                    runtimeError("Array index must be an integer.");
                }
                auto arr = container.as<std::shared_ptr<std::vector<RuntimeValue>>>();
                int idx = idxVal.as<int>();
                if (idx < 0 || idx >= static_cast<int>(arr->size())) {
                    runtimeError("Array index out of bounds.");
                }
                push((*arr)[idx]);
            } else if (container.is<std::shared_ptr<Dictionary>>()) {
                auto dict = container.as<std::shared_ptr<Dictionary>>();
                if (!isValidDictKey(idxVal)) {
                    runtimeError("Dictionary keys must be strings, integers, or booleans.");
                }
                DictKey key = toDictKey(idxVal);
                if (dict->entries.count(key) == 0) {
                    runtimeError("Dictionary key not found.");
                }
                push(dict->entries.at(key));
            } else if (container.is<std::string>()) {
                if (!idxVal.is<int>()) {
                    runtimeError("String index must be an integer.");
                }
                std::string s = container.as<std::string>();
                int idx = idxVal.as<int>();
                if (idx < 0 || idx >= static_cast<int>(s.length())) {
                    runtimeError("String index out of bounds.");
                }
                push(RuntimeValue{std::string(1, s[idx])});
            } else {
                runtimeError("Can only index arrays, strings, or dictionaries.");
            }
            break;
        }

        case OP_INDEX_SET: {
            RuntimeValue val = pop();
            RuntimeValue idxVal = pop();
            RuntimeValue container = pop();

            if (container.is<std::shared_ptr<std::vector<RuntimeValue>>>()) {
                if (!idxVal.is<int>()) {
                    runtimeError("Array index must be an integer.");
                }
                auto arr = container.as<std::shared_ptr<std::vector<RuntimeValue>>>();
                int idx = idxVal.as<int>();
                if (idx < 0 || idx >= static_cast<int>(arr->size())) {
                    runtimeError("Array index out of bounds.");
                }
                (*arr)[idx] = val;
            } else if (container.is<std::shared_ptr<Dictionary>>()) {
                auto dict = container.as<std::shared_ptr<Dictionary>>();
                if (!isValidDictKey(idxVal)) {
                    runtimeError("Dictionary keys must be strings, integers, or booleans.");
                }
                setDictEntry(*dict, idxVal, val);
            } else {
                runtimeError("Can only assign index to arrays or dictionaries.");
            }
            push(val);
            break;
        }

        case OP_ARRAY: {
            uint16_t count = READ_SHORT();
            auto arr = std::make_shared<std::vector<RuntimeValue>>();
            arr->resize(count);
            for (int i = count - 1; i >= 0; --i) {
                (*arr)[i] = pop();
            }
            push(RuntimeValue{arr});
            break;
        }

        case OP_DICT: {
            uint16_t count = READ_SHORT();
            auto dict = std::make_shared<Dictionary>();
            // Entries on stack are [key1, val1, key2, val2, ...]
            // So we pop in reverse: pop val, then pop key
            std::vector<std::pair<RuntimeValue, RuntimeValue>> temp(count);
            for (int i = count - 1; i >= 0; --i) {
                temp[i].second = pop();
                temp[i].first = pop();
            }
            for (const auto &p : temp) {
                if (!isValidDictKey(p.first)) {
                    runtimeError("Dictionary keys must be strings, integers, or booleans.");
                }
                setDictEntry(*dict, p.first, p.second);
            }
            push(RuntimeValue{dict});
            break;
        }

        case OP_NEW_INSTANCE: {
            std::string className = READ_CONSTANT().as<std::string>();
            uint8_t argCount = READ_BYTE();

            RuntimeValue classVal;
            try {
                classVal = globals->get(Token{TOK_IDENTIFIER, className, 0, 0, 0});
            } catch (...) {
                runtimeError("Undefined class '" + className + "'.");
            }

            if (!classVal.is<std::shared_ptr<Callable>>()) {
                runtimeError("Can only instantiate classes.");
            }

            auto klass = classVal.as<std::shared_ptr<Callable>>();

            // Validate constructor arity
            if (klass->arity() != VARIADIC_ARITY && klass->arity() != argCount) {
                runtimeError("Expected " + std::to_string(klass->arity()) +
                             " arguments but got " + std::to_string(argCount) + ".");
            }

            std::vector<RuntimeValue> args(argCount);
            for (int i = argCount - 1; i >= 0; --i) {
                args[i] = pop();
            }

            push(klass->call(interpreter, args));
            break;
        }

        case OP_CALL: {
            uint8_t argCount = READ_BYTE();
            RuntimeValue callee = peek(argCount);

            if (!callee.is<std::shared_ptr<Callable>>()) {
                runtimeError("Can only call functions and classes.");
            }

            std::shared_ptr<Callable> callable = callee.as<std::shared_ptr<Callable>>();

            // Validate arity
            if (callable->arity() != VARIADIC_ARITY && callable->arity() != argCount) {
                runtimeError("Expected " + std::to_string(callable->arity()) +
                             " arguments but got " + std::to_string(argCount) + ".");
            }

            // Check if it's a UserFunction (bytecode function)
            auto userFunc = std::dynamic_pointer_cast<UserFunction>(callable);
            if (userFunc) {
                auto compiled = userFunc->getCompiledFunction();
                if (frameCount >= FRAMES_MAX) {
                    runtimeError("Stack overflow (too many call frames).");
                }
                
                CallFrame nextFrame;
                nextFrame.function = compiled;
                nextFrame.ip = compiled->chunk->code.data();
                nextFrame.slotsBase = stackTop - argCount;
                nextFrame.closure = userFunc->getClosure();
                frames[frameCount++] = nextFrame;
                frame = &frames[frameCount - 1];
            } else {
                // Native or Class call
                std::vector<RuntimeValue> args(argCount);
                for (int i = argCount - 1; i >= 0; --i) {
                    args[i] = pop();
                }
                pop(); // pop callee
                push(callable->call(interpreter, args));
            }
            break;
        }

        case OP_RETURN: {
            RuntimeValue result = pop();
            frameCount--;
            if (frameCount <= runInitialFrameCount) {
                // Returned from the frame that started this run() call
                stackTop = frame->slotsBase - 1;
                push(result);
                return;
            }
            // Discard all stack slots for the callframe (including callee and arguments)
            stackTop = frame->slotsBase - 1;
            push(result);
            frame = &frames[frameCount - 1];
            break;
        }

        case OP_JUMP: {
            uint16_t offset = READ_SHORT();
            frame->ip += offset;
            break;
        }

        case OP_JUMP_IF_FALSE: {
            uint16_t offset = READ_SHORT();
            if (!isTruthy(pop())) {
                frame->ip += offset;
            }
            break;
        }

        case OP_LOOP: {
            uint16_t offset = READ_SHORT();
            frame->ip -= offset;
            break;
        }

        case OP_AND_JUMP: {
            uint16_t offset = READ_SHORT();
            if (!isTruthy(peek(0))) {
                frame->ip += offset;
            }
            break;
        }

        case OP_OR_JUMP: {
            uint16_t offset = READ_SHORT();
            if (isTruthy(peek(0))) {
                frame->ip += offset;
            }
            break;
        }

        case OP_FOR_PREPARE: {
            uint16_t slot = READ_SHORT();
            RuntimeValue limitVal = stack[frame->slotsBase + slot + 1];
            RuntimeValue loopVarVal = stack[frame->slotsBase + slot];

            if (!loopVarVal.is<int>() || !limitVal.is<int>()) {
                runtimeError("Start and end values in FOR loop must be integers.");
            }

            int loopVar = loopVarVal.as<int>();
            int limit = limitVal.as<int>();
            int step = (loopVar <= limit) ? 1 : -1;

            stack[frame->slotsBase + slot + 2] = RuntimeValue{step};

            bool keepGoing = (step > 0 && loopVar <= limit) || (step < 0 && loopVar >= limit);
            push(RuntimeValue{keepGoing});
            break;
        }

        case OP_FOR_LOOP: {
            uint16_t slot = READ_SHORT();
            uint16_t jumpOffset = READ_SHORT();

            int loopVar = stack[frame->slotsBase + slot].as<int>();
            int limit = stack[frame->slotsBase + slot + 1].as<int>();
            int step = stack[frame->slotsBase + slot + 2].as<int>();

            int nextVal = loopVar + step;
            bool keepGoing = (step > 0 && nextVal <= limit) || (step < 0 && nextVal >= limit);

            if (keepGoing) {
                stack[frame->slotsBase + slot] = RuntimeValue{nextVal};
                frame->ip = frame->function->chunk->code.data() + jumpOffset;
            }
            break;
        }

        case OP_FOR_IN_PREPARE: {
            uint16_t slot = READ_SHORT();
            RuntimeValue iterable = pop();

            auto flatArray = std::make_shared<std::vector<RuntimeValue>>();

            if (iterable.is<std::shared_ptr<std::vector<RuntimeValue>>>()) {
                flatArray = iterable.as<std::shared_ptr<std::vector<RuntimeValue>>>();
            } else if (iterable.is<std::shared_ptr<Dictionary>>()) {
                auto dict = iterable.as<std::shared_ptr<Dictionary>>();
                for (const auto &k : dict->keys) {
                    flatArray->push_back(fromDictKey(k));
                }
            } else if (iterable.is<std::string>()) {
                std::string s = iterable.as<std::string>();
                for (char c : s) {
                    flatArray->push_back(RuntimeValue{std::string(1, c)});
                }
            } else {
                runtimeError("Can only iterate over arrays, strings, or dictionaries.");
            }

            stack[frame->slotsBase + slot + 1] = RuntimeValue{flatArray};
            stack[frame->slotsBase + slot + 2] = RuntimeValue{0}; // index
            stack[frame->slotsBase + slot + 3] = RuntimeValue{static_cast<int>(flatArray->size())}; // size

            int size = static_cast<int>(flatArray->size());
            if (0 < size) {
                stack[frame->slotsBase + slot] = (*flatArray)[0];
            } else {
                stack[frame->slotsBase + slot] = RuntimeValue{Null{}};
            }
            break;
        }

        case OP_FOR_IN_LOOP: {
            uint16_t slot = READ_SHORT();
            uint16_t jumpOffset = READ_SHORT();

            auto flatArray = stack[frame->slotsBase + slot + 1].as<std::shared_ptr<std::vector<RuntimeValue>>>();
            int index = stack[frame->slotsBase + slot + 2].as<int>();
            int size = stack[frame->slotsBase + slot + 3].as<int>();

            int nextIndex = index + 1;
            if (nextIndex < size) {
                stack[frame->slotsBase + slot + 2] = RuntimeValue{nextIndex};
                stack[frame->slotsBase + slot] = (*flatArray)[nextIndex];
                frame->ip = frame->function->chunk->code.data() + jumpOffset;
            }
            break;
        }

        case OP_CLASS: {
            std::string className = READ_CONSTANT().as<std::string>();
            auto klass = std::make_shared<UserClass>(className, nullptr);
            push(RuntimeValue{std::static_pointer_cast<Callable>(klass)});
            break;
        }

        case OP_INHERIT: {
            RuntimeValue superclassVal = pop();
            RuntimeValue subclassVal = peek(0);

            if (!superclassVal.is<std::shared_ptr<Callable>>()) {
                runtimeError("Superclass must be a class.");
            }
            auto superclass = std::dynamic_pointer_cast<UserClass>(superclassVal.as<std::shared_ptr<Callable>>());
            if (!superclass) {
                runtimeError("Superclass must be a user-defined class.");
            }
            auto subclass = std::dynamic_pointer_cast<UserClass>(subclassVal.as<std::shared_ptr<Callable>>());

            // Cyclic inheritance check
            std::shared_ptr<UserClass> tracer = superclass;
            while (tracer != nullptr) {
                if (tracer->getName() == subclass->getName()) {
                    runtimeError("Cycle detected in inheritance chain.");
                }
                tracer = tracer->getSuperclass();
            }

            // Set the superclass on the subclass
            subclass->setSuperclass(superclass);
            break;
        }

        case OP_METHOD: {
            std::string methodName = READ_CONSTANT().as<std::string>();
            RuntimeValue methodVal = pop();
            RuntimeValue classVal = peek(0);

            auto klass = std::dynamic_pointer_cast<UserClass>(classVal.as<std::shared_ptr<Callable>>());
            klass->addMethod(methodName, methodVal.as<std::shared_ptr<Callable>>());
            break;
        }

        case OP_ATTRIBUTE: {
            std::string attrName = READ_CONSTANT().as<std::string>();
            RuntimeValue initVal = pop();
            RuntimeValue classVal = peek(0);

            auto klass = std::dynamic_pointer_cast<UserClass>(classVal.as<std::shared_ptr<Callable>>());
            // Wait! initVal contains the compiled function wrapping the initializer expression.
            // Under UserClass:
            // `std::map<std::string, std::shared_ptr<CompiledFunction>> defaultFields;`
            // We need to cast initVal to a UserFunction, get its CompiledFunction, and add it!
            auto userFunc = std::dynamic_pointer_cast<UserFunction>(initVal.as<std::shared_ptr<Callable>>());
            klass->addField(attrName, userFunc->getCompiledFunction());
            break;
        }

        default:
            runtimeError("Unknown instruction opcode: " + std::to_string(instruction));
        }
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
}
