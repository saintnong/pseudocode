#include "chunk.hpp"

void Chunk::write(uint8_t byte, size_t line) {
    code.push_back(byte);
    lines.push_back(line);
}

size_t Chunk::addConstant(RuntimeValue value) {
    constants.push_back(value);
    return constants.size() - 1;
}

size_t Chunk::getLine(size_t offset) const {
    if (offset < lines.size()) {
        return lines[offset];
    }
    return 0;
}
