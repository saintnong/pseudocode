#pragma once

#include "chunk.hpp"
#include "errors.hpp"
#include "runtime.hpp"
#include <array>
#include <memory>
#include <vector>

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
    void runtimeError(ErrorType type, const std::string &message);

    void execute();

public:
    VM(Interpreter &interpreter, EnvironmentPtr globals);

    RuntimeValue run(std::shared_ptr<CompiledFunction> function,
                     const std::vector<RuntimeValue> &args, EnvironmentPtr closure = nullptr);
};
