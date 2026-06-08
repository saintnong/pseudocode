#pragma once

#include "runtime.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

struct Chunk {
    std::vector<uint8_t> code;
    std::vector<RuntimeValue> constants;
    std::vector<Span> spans; // maps each byte in code to its source span

    void write(uint8_t byte, Span span);
    size_t addConstant(RuntimeValue value);
    Span getSpan(size_t offset) const;
};
