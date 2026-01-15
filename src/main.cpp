#include "main.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "ast_printer.hpp"
#include "errors.hpp"
#include "interpreter.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "runtime.hpp"

/**
 * Read the entire contents of a file into a string
 * @param path The path to the file to read
 * @return The complete file contents as a string
 * @throws std::runtime_error if the file cannot be opened
 */
std::string Pseudocode::readFile(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + path);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

/**
 * Print a formatted table of tokens to stdout
 * Displays token type, lexeme, and line number for debugging
 * @param tokens The vector of tokens to display
 */
void Pseudocode::printTokenTable(const std::vector<Token> &tokens) {
    // Print table header with column alignment
    std::cout << std::left << std::setw(20) << "TOKEN TYPE" << std::setw(25) << "LEXEME" << "LINE"
              << std::endl;
    std::cout << std::string(60, '-') << std::endl;

    // Print each token as a table row
    for (const Token &token : tokens) {
        if (token.type == TOK_EOF)
            break; // Don't display EOF token
        std::cout << std::left << std::setw(20) << token.typeToString() << std::setw(25)
                  << (token.lexeme.empty() ? "N/A" : token.lexeme) << token.line << std::endl;
    }
}

/**
 * Run Interpreter from File
 * Coordinates the full pipeline: Lexing -> Parsing -> Execution.
 * @param path System path to the .scsa script
 * @return 0 on success, 1 on error
 */
int Pseudocode::runFile(const std::string &path) {
    try {
        std::string source = readFile(path);

        // Stage 1: Lexical Analysis
        InterpreterStage stage = InterpreterStage::Lexing;
        ErrorReporter reporter(stage, path, source);
        Lexer lexer(source, reporter);

        std::vector<Token> tokens = lexer.scanTokens();
        if (debugTokens)
            printTokenTable(tokens);

        if (reporter.hadError)
            return 1;

        // Stage 2: Parsing
        stage = InterpreterStage::Parsing;
        Parser parser(tokens, source, reporter);
        std::vector<StmtPtr> statements = parser.parse();

        if (debugParse) {
            ASTPrinter printer;
            printer.print(statements);
        }

        if (reporter.hadError)
            return 1;

        // Stage 3: Execution
        stage = InterpreterStage::Runtime;
        Interpreter interpreter(reporter);
        interpreter.interpret(statements);
    } catch (const std::exception &e) {
        // Catch fatal system or logic errors
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

/**
 * Interactive REPL
 * Provides a persistent environment for executing code line-by-line.
 */
int Pseudocode::runRepl() {
    InterpreterStage stage = InterpreterStage::Lexing;

    std::cout << "Interactive SCSA Pseudocode Interpreter" << std::endl;
    std::cout << "For help, run this program with '--help' or '-h'" << std::endl;
    std::cout << "Type 'exit' or use Ctrl+D to quit" << std::endl;

    // Persist interpreter and reporter to maintain state between lines
    ErrorReporter reporter(stage, "", "");
    reporter.setReplMode(true);
    Interpreter interpreter(reporter);

    // Keep AST nodes alive for the duration of the session
    std::vector<StmtPtr> sessionHistory;

    std::string line;
    while (true) {
        std::cout << "[SCSA] >> " << std::flush;
        if (!std::getline(std::cin, line))
            break;
        if (line.empty())
            continue;
        if (line == "exit")
            break;

        reporter.replAddLine(line);
        reporter.hadError = false;

        try {
            // Lexing
            stage = InterpreterStage::Lexing;
            Lexer lexer(line, reporter);
            std::vector<Token> tokens = lexer.scanTokens();
            if (debugTokens)
                printTokenTable(tokens);

            if (reporter.hadError)
                continue;

            // Parsing
            stage = InterpreterStage::Parsing;
            Parser parser(tokens, line, reporter);
            std::vector<StmtPtr> parsed = parser.parse();

            if (debugParse) {
                ASTPrinter printer;
                printer.print(parsed);
            }

            if (reporter.hadError)
                continue;

            // Execution
            stage = InterpreterStage::Runtime;

            // Special Case: Standalone Expression Evaluation
            if (parsed.size() == 1) {
                if (auto *exprStmt = dynamic_cast<ExpressionStmt *>(parsed[0].get())) {
                    try {
                        RuntimeValue value = interpreter.evaluate(exprStmt->expression.get());
                        std::cout << C_GREEN << "=> " << C_RESET << stringify(value) << std::endl;
                        sessionHistory.push_back(std::move(parsed[0]));
                    } catch (const RuntimeError &error) {
                        Token token     = error.token;
                        std::string msg = error.what();
                        reporter.report(ErrorType::Runtime, token.line, token.column, msg,
                                        token.lexeme.length());
                    }
                    continue;
                }
            }

            // Standard Statement Execution
            interpreter.interpret(parsed);

            // Persist AST nodes (for multi-line block/function definitions)
            sessionHistory.insert(sessionHistory.end(), std::make_move_iterator(parsed.begin()),
                                  std::make_move_iterator(parsed.end()));
        } catch (const std::exception &e) {
            std::cerr << "Runtime Exception: " << e.what() << std::endl;
        }
    }

    return 0;
}

void help() {
    std::cout << "Usage: scsa [--debug-tokens] [--debug-parse] [script.scsa]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --debug-tokens   Print token table after lexing" << std::endl;
    std::cout << "  --debug-parse    Print AST after parsing" << std::endl;
    std::cout << "If no script is provided, an interactive REPL is started." << std::endl;
}

/**
 * Main Entry Point
 * Orchestrates terminal setup and command-line argument parsing.
 */
int main(int argc, char *argv[]) {

#ifdef _WIN32
    // Windows Terminal Setup: Enable ANSI color support and UTF-8
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD consoleMode;
    GetConsoleMode(hConsole, &consoleMode);
    consoleMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hConsole, consoleMode);

    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif

    Pseudocode pseudocode;

    // Handle --help or -h
    if (argc > 1 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")) {
        help();
        return 0;
    }

    // Argument Parser
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--debug-tokens") {
            pseudocode.debugTokens = true;
        } else if (arg == "--debug-parse") {
            pseudocode.debugParse = true;
        } else {
            // Treat anything else as a potential script path
            if (arg.size() < 5 || arg.substr(arg.size() - 5) != ".scsa") {
                help();
                return 1;
            } else {
                return pseudocode.runFile(arg);
            }
        }
    }

    // Default: Start REPL if no file provided
    if (argc == 1) {
        return pseudocode.runRepl();
    }
}
