#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include "token.hpp"

// Forward declaration to break circular dependency
enum InterpreterStage { Lexing, Parsing, Runtime };

// Every type of error we'll need
enum class ErrorType {
    Syntax,   // Syntax errors (invalid token sequences)
    Type,     // Type errors (type mismatches)
    Name,     // Name resolution errors (undefined variables, properties, classes)
    Argument, // Argument count errors
    Index,    // Index errors (out of bounds, etc.)
    VM,       // Internal VM errors (stack overflow, unknown opcode, etc.)
};

/**
 * Get the error code string for a given error type.
 */
inline std::string getErrorCode(ErrorType type) {
    switch (type) {
    case ErrorType::Syntax:
        return "E001";
    case ErrorType::Type:
        return "E002";
    case ErrorType::Name:
        return "E003";
    case ErrorType::Argument:
        return "E004";
    case ErrorType::Index:
        return "E005";
    case ErrorType::VM:
        return "E006";
    default:
        return "E000";
    }
}

// ==============================================================
// Error Exception Classes
// ==============================================================

/**
 * TypeError
 * Thrown when an operation receives operands of incompatible types.
 */
class TypeError : public std::runtime_error {
public:
    const Span span;
    TypeError(Span span, const std::string &message) : std::runtime_error(message), span(span) {
    }
};

/**
 * NameError
 * Thrown when a variable, property, or class name cannot be resolved.
 */
class NameError : public std::runtime_error {
public:
    const Span span;
    NameError(Span span, const std::string &message) : std::runtime_error(message), span(span) {
    }
};

/**
 * ArgumentError
 * Thrown when a function or constructor receives the wrong number of arguments.
 */
class ArgumentError : public std::runtime_error {
public:
    const Span span;
    ArgumentError(Span span, const std::string &message) : std::runtime_error(message), span(span) {
    }
};

/**
 * IndexError
 * Thrown when an index or key is out of bounds, or for related
 * out-of-range access issues.
 */
class IndexError : public std::runtime_error {
public:
    const Span span;
    IndexError(Span span, const std::string &message) : std::runtime_error(message), span(span) {
    }
};

/**
 * VMError
 * Thrown for internal VM errors such as stack overflows or unknown opcodes.
 */
class VMError : public std::runtime_error {
public:
    const Span span;
    VMError(Span span, const std::string &message) : std::runtime_error(message), span(span) {
    }
};

// ANSI color codes
extern const char *C_RED;
extern const char *C_RESET;
extern const char *C_CYAN;
extern const char *C_BLUE;
extern const char *C_GRAY;
extern const char *C_GREEN;

/**
 * Initialize colors based on NO_COLOR environment variable
 */
void initColors();

/**
 * Print an error message from our benevolent overlord SCSA
 */
void printAtarMessage();

/**
 * ErrorReporter class handles all error reporting and formatting
 * Provides context about where the error occurred in the source code
 */
class ErrorReporter {
private:
    // Reference to current interpreter stage
    InterpreterStage &stage;
    // Current source file being processed
    std::string filename;
    // Source code split by lines
    std::vector<std::string> lines;

    /**
     * Get human-readable error type label
     */
    std::string getErrorLabel(ErrorType type);

    /**
     * Get a specific line from the source code
     * @param lineNum The 1-based line number to extract
     * @return The source code line, or empty string if out of range
     */
    std::string getSourceLine(size_t lineNum);

    // Whether we're in a REPL or not
    bool isRepl = false;

    // When true, errors are recorded but not printed (for continuation checking)
    bool isSilent = false;

public:
    /**
     * Construct error reporter with reference to current stage, filename, and source code
     * @param stageRef Reference to the interpreter stage
     * @param file The source filename for error context
     * @param source The full source code for context generation
     */
    ErrorReporter(InterpreterStage &stageRef, const std::string &file = "",
                  const std::string &source = "");

    /**
     * Appends new source code lines to the existing source.
     * Useful for REPL.
     */
    void replAddLine(const std::string &sourceSegment);

    /**
     * Gets the current source line count
     */
    size_t getLineCount() {
        return lines.size();
    }

    /**
     * Report an error with full context including surrounding lines
     * @param type The error type (Syntax, Type, etc.)
     * @param span The source span where the error occurred
     * @param message The error message to display
     */
    void report(ErrorType type, Span span, const std::string &message);

    /**
     * Sets the reporter into REPL mode so it only reports last line.
     */
    void setReplMode(bool active) {
        isRepl = active;
    }

    /**
     * Sets silent mode - errors are recorded but not printed.
     * Useful for checking input continuation without displaying errors.
     */
    void setSilent(bool active) {
        isSilent = active;
    }

    /**
     * Tracks if we've had an error in this session or not.
     */
    bool hadError = false;
};
