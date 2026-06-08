#include "interpreter.hpp"
#include "compiler.hpp"
#include <chrono>
#include <functional>
#include <iostream>
#include <random>

Interpreter::Interpreter(ErrorReporter &reporterRef) : reporter(reporterRef) {
    globals     = std::make_shared<Environment>();
    environment = globals;

    // =========================================================================
    // SCSA Pseudocode Standard Library
    // ==========================================================================

    auto inputNative = std::make_shared<NativeFunction>(
        VARIADIC_ARITY, [](Interpreter &, std::vector<RuntimeValue> args) -> RuntimeValue {
            if (args.size() > 0) {
                std::cout << stringify(args[0]);
            }
            std::string inputLine;
            if (!std::getline(std::cin, inputLine)) {
                inputLine = "";
            }
            RuntimeValue result;
            result.value = inputLine;
            return result;
        });
    globals->define("INPUT", {std::static_pointer_cast<Callable>(inputNative)});

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
    globals->define("PRINT", {std::static_pointer_cast<Callable>(printNative)});

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
    globals->define("OUTPUT", {std::static_pointer_cast<Callable>(outputNative)});

    auto intNative = std::make_shared<NativeFunction>(
        1, [](Interpreter &, std::vector<RuntimeValue> args) -> RuntimeValue {
            if (args.empty())
                return {Null{}};
            const auto &val = args[0];
            int result      = 0;
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
                    result = -1;
                }
            }
            RuntimeValue retVal;
            retVal.value = result;
            return retVal;
        });
    globals->define("INT", {std::static_pointer_cast<Callable>(intNative)});

    auto floatNative = std::make_shared<NativeFunction>(
        1, [](Interpreter &, std::vector<RuntimeValue> args) -> RuntimeValue {
            if (args.empty())
                return {Null{}};
            const auto &val = args[0];
            double result   = 0.0;
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

    auto boolNative = std::make_shared<NativeFunction>(
        1, [](Interpreter &, std::vector<RuntimeValue> args) -> RuntimeValue {
            if (args.empty())
                return {Null{}};
            const auto &val = args[0];
            bool result     = false;
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

    auto randomNative = std::make_shared<NativeFunction>(
        2, [](Interpreter &, std::vector<RuntimeValue> args) -> RuntimeValue {
            if (!args[0].is<int>() || !args[1].is<int>()) {
                return {0};
            }
            int min = args[0].as<int>();
            int max = args[1].as<int>();
            if (min > max)
                std::swap(min, max);
            static std::mt19937 gen(std::random_device{}());
            std::uniform_int_distribution<> dis(min, max);
            RuntimeValue retVal;
            retVal.value = dis(gen);
            return retVal;
        });
    globals->define("RANDOM", {std::static_pointer_cast<Callable>(randomNative)});

    auto timeNative = std::make_shared<NativeFunction>(
        0, [](Interpreter &, std::vector<RuntimeValue> args) -> RuntimeValue {
            (void) args;
            auto now = std::chrono::system_clock::now();
            auto duration =
                std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
            RuntimeValue retVal;
            retVal.value = static_cast<double>(duration.count()) / 1000.0;
            return retVal;
        });
    globals->define("TIME", {std::static_pointer_cast<Callable>(timeNative)});

    auto typeNative = std::make_shared<NativeFunction>(
        1, [](Interpreter &, std::vector<RuntimeValue> args) -> RuntimeValue {
            if (args.empty())
                return {"NULL"};
            const auto &val     = args[0];
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
                typeStr = "ARRAY";
            else if (val.is<std::shared_ptr<Dictionary>>())
                typeStr = "DICTIONARY";
            else if (val.is<std::shared_ptr<Callable>>())
                typeStr = "CALLABLE";
            else if (val.is<std::shared_ptr<Instance>>())
                typeStr = "INSTANCE";
            RuntimeValue retVal;
            retVal.value = typeStr;
            return retVal;
        });
    globals->define("TYPE", {std::static_pointer_cast<Callable>(typeNative)});

    vm = std::make_unique<VM>(*this, globals);
}

void Interpreter::interpret(const std::vector<StmtPtr> &statements) {
    try {
        Compiler compiler(reporter);
        auto compiled = compiler.compile(statements);
        vm->run(compiled, {});
    } catch (const RuntimeError &error) {
        Token token     = error.token;
        std::string msg = error.what();
        reporter.report(ErrorType::Runtime, token.line, token.column, msg, token.lexeme.length());
    }
}

RuntimeValue Interpreter::evaluate(Expr *expr) {
    Compiler compiler(reporter);
    auto compiled = compiler.compileExpressionOnly(expr);
    return vm->run(compiled, {});
}

RuntimeValue Interpreter::runFunction(std::shared_ptr<CompiledFunction> compiledFn,
                                      const std::vector<RuntimeValue> &args,
                                      std::shared_ptr<Environment> closure) {
    return vm->run(compiledFn, args, closure);
}
