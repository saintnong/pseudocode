#pragma once

#include <string>
#include <vector>

// Forward declaration to break circular dependency
enum InterpreterStage { Lexing, Parsing, Runtime };

// Every type of error we'll need
enum class ErrorType {
    Syntax,  // Syntax errors (invalid token sequences)
    Type,    // Type errors (type mismatches)
    Runtime, // Runtime errors (errors during execution)
};

// ANSI color codes for terminal output
extern const char *C_RED;
extern const char *C_RESET;
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
     * Get human-readable interpreter stage label
     */
    std::string getStageLabel();

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
     * @param line The line number where error occurred
     * @param column The column position where error occurred
     * @param message The error message to display
     * @param length The length of the erroneous token (for underlining)
     */
    void report(ErrorType type, size_t line, size_t column, const std::string &message,
                size_t length = 1);

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
