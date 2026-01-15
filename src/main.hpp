#pragma once

#include <cctype>
#include <string>
#include <vector>

#include "token.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

/**
 * Pseudocode Interpreter Entry Point
 * This class coordinates the lexing, parsing, and execution of Pseudocode programs.
 * It provides high-level APIs for running code from files or in an interactive REPL.
 */
class Pseudocode {
public:
    /**
     * Run Code from File
     * Reads the specified file, tokenizes, parses, and executes the contents.
     * @param path The absolute or relative system path to the source file
     * @return 0 if the program executed successfully, 1 if errors occurred
     */
    int runFile(const std::string &path);

    /**
     * Start Interactive REPL
     * Launches a Read-Eval-Print-Loop where the user can enter code line-by-line.
     * Useful for testing expressions and small code snippets.
     * @return 0 upon termination of the REPL (e.g., EOF or EXIT command)
     */
    int runRepl();

    /**
     * Debugging: Tokenization
     * If true, the interpreter will print a formatted table of tokens after scanning.
     */
    bool debugTokens = false;

    /**
     * Debugging: Parsing
     * If true, the interpreter will print the Abstract Syntax Tree (AST) after parsing.
     */
    bool debugParse = false;

private:
    /**
     * Internal Utility: File Loader
     * @param path Path to the file to be read
     * @return The complete source code as a single string
     * @throws std::runtime_error if the file cannot be opened
     */
    static std::string readFile(const std::string &path);

    /**
     * Internal Utility: Token Visualization
     * Prints a human-readable table to stdout showing token types, lexemes, and locations.
     * @param tokens The list of tokens to display
     */
    static void printTokenTable(const std::vector<Token> &tokens);
};
