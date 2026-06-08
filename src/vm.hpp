#pragma once

#include <vector>
#include <array>
#include <memory>
#include "runtime.hpp"
#include "chunk.hpp"
#include "errors.hpp"

struct CallFrame {
    std::shared_ptr<CompiledFunction> function;
    const uint8_t *ip;
    size_t slotsBase;
    EnvironmentPtr closure;
};

class Interpreter;

class VM {
private:
    Interpreter &interpreter;
    ErrorReporter &reporter;
    
    static constexpr size_t STACK_MAX = 256 * 1024;
    std::array<RuntimeValue, STACK_MAX> stack;
    size_t stackTop = 0;

    static constexpr size_t FRAMES_MAX = 1024;
    std::array<CallFrame, FRAMES_MAX> frames;
    size_t frameCount = 0;

    EnvironmentPtr globals;
    size_t runInitialFrameCount = 0;

    void push(RuntimeValue value);
    RuntimeValue pop();
    RuntimeValue peek(int distance = 0);

    bool isTruthy(const RuntimeValue &value) const;
    bool isEqual(const RuntimeValue &a, const RuntimeValue &b) const;
    void runtimeError(const std::string &message);

    void execute();

public:
    VM(Interpreter &interpreter, ErrorReporter &reporter, EnvironmentPtr globals);

    RuntimeValue run(std::shared_ptr<CompiledFunction> function, const std::vector<RuntimeValue> &args, EnvironmentPtr closure = nullptr);
};
