#pragma once

#include <string>

/**
 * InputBuffer - Determines if REPL input needs more lines
 *
 * Analyzes partial input to detect unclosed block structures
 * (FUNCTION, CLASS, IF, WHILE, FOR), allowing the REPL to accumulate
 * multi-line input before parsing and executing.
 */
class InputBuffer {
public:
    /**
     * Check if the input is incomplete and needs more lines
     *
     * Tokenizes the input and tracks block depth:
     * - Block-opening tokens (FUNCTION, CLASS, IF, WHILE, FOR) increment depth
     * - END tokens decrement depth
     *
     * @param input The accumulated REPL input buffer
     * @return true if there are unclosed blocks (depth > 0), false otherwise
     */
    static bool needsContinuation(const std::string &input);
};
