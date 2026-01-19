/**
 * Suppress warnings for using std::getenv on MSVC (C4996)
 * This is only unsafe due to thread safety, which we don't need for this project
 */
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "errors.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>

// Define global color pointers
const char *C_RED   = "\033[31m";
const char *C_RESET = "\033[0m";
const char *C_BLUE  = "\033[34m";
const char *C_GRAY  = "\033[2m";
const char *C_GREEN = "\033[32m";

void initColors() {
    if (const char *noColor = std::getenv("NO_COLOR")) {
        // If NO_COLOR is set (regardless of value), disable colors
        if (noColor[0] != '\0') {
            C_RED   = "";
            C_RESET = "";
            C_BLUE  = "";
            C_GRAY  = "";
            C_GREEN = "";
        }
    }
}

/**
 * Berate the user because they had an error
 */
void printAtarMessage() {
    std::cout << C_RED << "[SCSA] Your ATAR is cooked, -99999 marks." << std::endl
              << "[SCSA] Congratulations, you are the first student"
              << " to ever get a negative study score! 😭" << std::endl
              << "[SCSA] Say goodbye to your future. L + ratio 😂 😂" << C_RESET << std::endl;
}

/**
 * ErrorReporter Constructor
 * Stores a reference to the current interpreter stage, filename, and source code for error
 * reporting
 * @param stageRef Reference to the current InterpreterStage
 * @param file The source filename being processed
 * @param source The full source code for context generation
 */
ErrorReporter::ErrorReporter(InterpreterStage &stageRef, const std::string &file,
                             const std::string &source)
    : stage(stageRef), filename(file) {
    if (source.empty())
        return;

    // Split the source code into lines
    std::string currentLine;
    for (char c : source) {
        if (c == '\n') {
            lines.push_back(currentLine);
            currentLine.clear();
        } else {
            currentLine += c;
        }
    }
    // Add the last line if it's not empty or if source ends with newline
    lines.push_back(currentLine);
}

/**
 * Adds a line to the source code of the error reporter.
 */
void ErrorReporter::replAddLine(const std::string &sourceSegment) {
    if (sourceSegment.empty())
        return;

    std::stringstream ss(sourceSegment);
    std::string line;

    // Parse the new segment and add each line to our history
    while (std::getline(ss, line)) {
        lines.push_back(line);
    }

    if (lines.empty() && !sourceSegment.empty()) {
        lines.push_back(sourceSegment);
    }
}

/**
 * Map error types to human-readable labels
 * @param type The error type to convert
 * @return String representation of the error type
 */
std::string ErrorReporter::getErrorLabel(ErrorType type) {
    switch (type) {
    case ErrorType::Syntax:
        return "Syntax Error";
    case ErrorType::Type:
        return "Type Error";
    case ErrorType::Runtime:
        return "Runtime Error";
    default:
        return "Unknown Error";
    }
}

/**
 * Map interpreter stages to human-readable labels
 * @return String representation of the current stage
 */
std::string ErrorReporter::getStageLabel() {
    switch (stage) {
    case InterpreterStage::Lexing:
        return "Lexing";
    case InterpreterStage::Parsing:
        return "Parsing";
    case InterpreterStage::Runtime:
        return "Runtime";
    default:
        return "Unknown";
    }
}

/**
 * Extract a specific line from the source code
 * @param lineNum The 1-based line number to extract
 * @return The source code line, or empty string if out of range
 */
std::string ErrorReporter::getSourceLine(size_t lineNum) {
    // Returns the specified source line

    if (lines.empty() || lineNum < 1 || lineNum > lines.size()) {
        return "";
    }
    return lines[lineNum - 1];
}

/**
 * Format and display a complete error message with context
 * Shows the error stage, location, line content, and error pointer
 * @param type The type of error (Syntax, Type, etc.)
 * @param line The line number where the error occurred
 * @param column The column position where the error occurred
 * @param message The error message to display
 * @param lineSource The actual source code line containing the error
 * @param length The length of the erroneous token (for underlining)
 */
void ErrorReporter::report(ErrorType type, size_t line, size_t column, const std::string &message,
                           size_t length) {
    hadError = true;

    // In silent mode, record the error but don't output anything
    if (isSilent) {
        return;
    }

    // Print which stage the interpreter is in
    std::string stageLabel = getStageLabel();
    std::cerr << C_RED << "[An error has occurred during the stage: '" << stageLabel << "']"
              << std::endl;

    // Get the error line
    std::string errorLine = getSourceLine(line);

    // Create coordinate string
    std::string lineStr   = std::to_string(line);
    std::string separator = " │ ";
    size_t gutterWidth    = lineStr.length() + 3;

    // Print filename (Aligned)
    if (!filename.empty()) {
        std::string indent(gutterWidth - 2, ' ');
        std::cerr << C_BLUE << indent << "┌──[" << filename << ":" << line << ":" << column + 1
                  << "]" << C_RESET << std::endl;
    }

    // Print previous two lines (if they exist)
    if (line > 1 && !isRepl) {
        // Determine start line (max 2 lines back, but not less than 1)
        size_t startContext = (line > 2) ? line - 2 : 1;

        for (size_t i = startContext; i < line; i++) {
            std::string prevLine = getSourceLine(i);

            if (!prevLine.empty()) {
                std::string prevLineString = std::to_string(i);
                std::cerr << C_GRAY << prevLineString << C_RESET;

                // Pad to match the width of the main error line number
                for (size_t j = prevLineString.length(); j < lineStr.length(); j++) {
                    std::cerr << " ";
                }

                std::cerr << C_BLUE << separator << C_RESET << C_GRAY << prevLine << C_RESET
                          << std::endl;
            }
        }
    }

    // Print the error line with highlighting
    std::cerr << C_BLUE << lineStr << separator << C_RESET;

    // Print the line up to the error
    column = std::min(column, errorLine.length()); // Crash prevention
    std::cerr << errorLine.substr(0, column);

    // Print the erroneous token in red
    std::cerr << C_RED << errorLine.substr(column, length) << C_RESET;

    // Print the rest of the line
    if (column + length < errorLine.length()) {
        std::cerr << errorLine.substr(column + length);
    }
    std::cerr << std::endl;

    // Generate the caret alignment on error line
    // Print spaces equal to the width of the gutter
    // Print empty line to seperate
    for (size_t i = 0; i < lineStr.length(); i++) {
        std::cerr << " ";
    }
    std::cerr << C_BLUE << separator << C_RESET;

    // Account for tab characters inside the source code itself
    for (size_t i = 0; i < column; i++) {
        if (i < errorLine.length() && errorLine[i] == '\t') {
            std::cerr << '\t';
        } else {
            std::cerr << ' ';
        }
    }

    // Draw the underline carets
    std::cerr << C_RED;
    for (size_t i = 0; i < length; i++) {
        std::cerr << '^';
    }

    // Print error label and message next to the carets
    std::string label = getErrorLabel(type);
    std::cerr << " " << label << ": " << C_RESET << message << std::endl;

    // Print line after
    if (!isRepl) {
        std::string nextLine       = getSourceLine(line + 1);
        std::string nextLineString = std::to_string(line + 1);
        if (!nextLine.empty()) {
            std::cerr << C_GRAY << nextLineString << C_RESET;

            // pad
            for (size_t i = nextLineString.length(); i < lineStr.length(); i++) {
                std::cerr << " ";
            }

            std::cerr << C_BLUE << separator << C_RESET << C_GRAY << nextLine << C_RESET
                      << std::endl;
        }
    }

    // // Lovely message from our overlords SCSA
    // printAtarMessage();

    throw std::runtime_error("");
}
