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
 * Main Pseudocode interpreter class
 * Provides both file execution and interactive REPL modes
 * Exposes a public API for running pseudocode programs
 */
class Pseudocode {
public:
    /**
     * Execute pseudocode from a file
     * @param path Path to the pseudocode file to run
     * @return 0 on success, 1 on error
     */
    int runFile(const std::string &path);

    /**
     * Run an interactive REPL (Read-Eval-Print-Loop)
     * Allows users to type pseudocode lines and see tokenization
     * @return Always returns 0
     */
    int runRepl();

    /**
     * Optional debug modes for debugging tokenization and parsing
     */
    bool debugTokens = false; // Print token table after Lexing
    bool debugParse  = false; // Print AST after Parsing
private:
    /**
     * Read entire file contents into a string
     * @param path Path to the file to read
     * @return File contents as string
     */
    static std::string readFile(const std::string &path);

    /**
     * Print tokens in a formatted table
     * @param tokens Vector of tokens to display
     */
    static void printTokenTable(const std::vector<Token> &tokens);
};
