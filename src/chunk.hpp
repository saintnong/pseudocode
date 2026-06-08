#pragma once

#include "runtime.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

struct Chunk {
    std::vector<uint8_t> code;
    std::vector<RuntimeValue> constants;
    std::vector<size_t> lines; // maps each byte in code to its source line number

    void write(uint8_t byte, size_t line);
    size_t addConstant(RuntimeValue value);
    size_t getLine(size_t offset) const;
};
