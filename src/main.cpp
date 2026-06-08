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
#include "input_buffer.hpp"
#include "interpreter.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "runtime.hpp"
#include "version.hpp"

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
                  << (token.lexeme.empty() ? "N/A" : token.lexeme) << token.span.line << std::endl;
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

        // === Lexing ===
        ErrorReporter reporter(path, source);
        Lexer lexer(source, reporter);

        std::vector<Token> tokens = lexer.scanTokens();
        if (debugTokens)
            printTokenTable(tokens);

        if (reporter.hadError)
            return 1;

        // === Parsing ===
        Parser parser(tokens, reporter);
        std::vector<StmtPtr> statements = parser.parse();

        if (debugParse) {
            ASTPrinter printer;
            printer.print(statements);
        }

        if (reporter.hadError)
            return 1;

        // === Execution ===
        Interpreter interpreter(reporter);
        interpreter.interpret(statements);
    } catch (const std::runtime_error &) {
        // Runtime errors which were raised by ourselves can be caught
        return 1;
    }

    return 0;
}

/**
 * Interactive REPL
 * Provides a persistent environment for executing code line-by-line.
 * Supports multi-line input for functions, classes, and control structures.
 */
int Pseudocode::runRepl() {
    std::cout << C_CYAN << "  +-----------------------------------------------+" << C_RESET
              << std::endl;
    std::cout << C_CYAN << "  | " << C_RESET << "SCSA Pseudocode Interpreter [" << C_BLUE
              << VERSION_NUMBER << C_RESET << "]" << C_CYAN << " |" << C_RESET << std::endl;
    std::cout << C_CYAN << "  +-----------------------------------------------+" << C_RESET
              << std::endl
              << std::endl;
    std::cout << "  ~ For help, run this program with '--help' or '-h'" << std::endl;
    std::cout << "  ~ Type 'exit' or use Ctrl+D to quit" << std::endl << std::endl;

    // Persist interpreter and reporter to maintain state between lines
    ErrorReporter reporter("", "");
    reporter.setReplMode(true);
    Interpreter interpreter(reporter);

    // Keep AST nodes alive for the duration of the session
    std::vector<StmtPtr> sessionHistory;

    // Multi-line input buffer
    std::string buffer;
    bool inMultiline = false;

    std::string line;
    while (true) {
        // Show appropriate prompt based on multi-line state
        if (inMultiline) {
            std::cout << " ..  " << std::flush;
        } else {
            std::cout << " ->  " << std::flush;
        }

        if (!std::getline(std::cin, line))
            break;

        // Only check exit command when not in multi-line mode
        if (!inMultiline && line == "exit")
            break;

        // Accumulate input
        if (!buffer.empty()) {
            buffer += "\n";
        }
        buffer += line;

        // Check if we need more input (unclosed blocks)
        if (InputBuffer::needsContinuation(buffer)) {
            inMultiline = true;
            continue;
        }

        // Complete input received - process it
        inMultiline = false;

        // Skip empty buffers
        if (buffer.empty()) {
            continue;
        }

        size_t startingLine = reporter.getLineCount() + 1;
        reporter.replAddLine(buffer);
        reporter.hadError = false;

        try {
            // === Lexing ===
            Lexer lexer(buffer, reporter, startingLine);
            std::vector<Token> tokens = lexer.scanTokens();
            if (debugTokens)
                printTokenTable(tokens);

            if (reporter.hadError) {
                buffer.clear();
                continue;
            }

            // === Parsing ===
            Parser parser(tokens, reporter);
            std::vector<StmtPtr> parsed = parser.parse();

            if (debugParse) {
                ASTPrinter printer;
                printer.print(parsed);
            }

            if (reporter.hadError) {
                buffer.clear();
                continue;
            }

            // === Execution ===

            // Special Case: Standalone Expression Evaluation
            if (parsed.size() == 1) {
                if (auto *exprStmt = dynamic_cast<ExpressionStmt *>(parsed[0].get())) {
                    try {
                        // Print the evaluated value of the expression
                        RuntimeValue value = interpreter.evaluate(exprStmt->expression.get());
                        std::cout << C_GREEN << " ==  " << C_CYAN << stringify(value) << C_RESET
                                  << std::endl;
                        sessionHistory.push_back(std::move(parsed[0]));
                    } catch (const TypeError &error) {
                        reporter.report(ErrorType::Type, error.span, error.what());
                    } catch (const NameError &error) {
                        reporter.report(ErrorType::Name, error.span, error.what());
                    } catch (const ArgumentError &error) {
                        reporter.report(ErrorType::Argument, error.span, error.what());
                    } catch (const IndexError &error) {
                        reporter.report(ErrorType::Index, error.span, error.what());
                    } catch (const VMError &error) {
                        reporter.report(ErrorType::VM, error.span, error.what());
                    }
                    buffer.clear();
                    continue;
                }
            }

            // Standard Statement Execution
            interpreter.interpret(parsed);

            // Persist AST nodes
            sessionHistory.insert(sessionHistory.end(), std::make_move_iterator(parsed.begin()),
                                  std::make_move_iterator(parsed.end()));
        } catch (const std::runtime_error &) {
            // This error is reported elsewhere
        } catch (const std::exception &e) {
            std::cerr << "Unexpected Error: " << e.what() << std::endl;
        }

        // Clear buffer after processing
        buffer.clear();
    }

    return 0;
}

void help() {
    std::cout << "Usage: scsa [--debug-tokens] [--debug-parse] [script.scsa]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --debug-tokens   Print token table after lexing" << std::endl;
    std::cout << "  --debug-parse    Print AST after parsing" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "Environment Variables:" << std::endl;
    std::cout << "  NO_COLOR         Disable ANSI color output (https://no-color.org)" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "If no script is provided, an interactive REPL is started." << std::endl;
}

/**
 * Windows-specific terminal initialization
 * Enables ANSI escape sequences and UTF-8 output
 */
void setupTerminal() {
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD consoleMode;
    if (GetConsoleMode(hConsole, &consoleMode)) {
        consoleMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hConsole, consoleMode);
    }

    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif
}

/**
 * Main Entry Point
 */
int main(int argc, char *argv[]) {
    setupTerminal();
    initColors();

    Pseudocode pseudocode;
    std::string scriptPath;

    // Argument Parser
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            help();
            return 0;
        } else if (arg == "--debug-tokens") {
            pseudocode.debugTokens = true;
        } else if (arg == "--debug-parse") {
            pseudocode.debugParse = true;
        } else if (!arg.empty() && arg[0] == '-') {
            std::cerr << "Unknown option: " << arg << std::endl;
            help();
            return 1;
        } else {
            // Treat as a script path
            if (!scriptPath.empty()) {
                std::cerr << "Error: Only one script file can be executed at a time." << std::endl;
                help();
                return 1;
            }

            // Validate extension
            if (arg.size() < 5 || arg.substr(arg.size() - 5) != ".scsa") {
                std::cerr << "Error: Script file must have .scsa extension: " << arg << std::endl;
                help();
                return 1;
            }

            scriptPath = arg;
        }
    }

    // Decide whether to run file or REPL
    if (!scriptPath.empty()) {
        return pseudocode.runFile(scriptPath);
    } else {
        return pseudocode.runRepl();
    }
}
