#include "chunk.hpp"

void Chunk::write(uint8_t byte, Span span) {
    code.push_back(byte);
    spans.push_back(span);
}

size_t Chunk::addConstant(RuntimeValue value) {
    constants.push_back(value);
    return constants.size() - 1;
}

Span Chunk::getSpan(size_t offset) const {
    if (offset < spans.size()) {
        return spans[offset];
    }
    return {0, 0, 0};
}
