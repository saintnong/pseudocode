#include "main.hpp"
#include "ast_printer.hpp"

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
 * Run the interpreter on a file
 * Reads the file, tokenizes it, and displays the token table
 * @param path Path to the pseudocode file to execute
 * @return 0 on success, 1 on error
 */
int Pseudocode::runFile(const std::string &path) {
    try {
        std::string source = readFile(path);

        // Initialize error reporting at the lexing stage
        InterpreterStage stage = InterpreterStage::Lexing;
        ErrorReporter reporter(stage, path, source);
        Lexer lexer(source, reporter);

        // Tokenize the source code
        std::vector<Token> tokens = lexer.scanTokens();
        if (debugTokens)
            printTokenTable(tokens);

        if (reporter.hadError)
            return 1;

        // Parse tokens
        stage = InterpreterStage::Parsing;
        Parser parser(tokens, source, reporter);
        std::vector<StmtPtr> statements = parser.parse();
        if (debugParse) {
            ASTPrinter printer;
            printer.print(statements);
        }

        if (reporter.hadError)
            return 1;

        // Interpret the parsed statements
        stage = InterpreterStage::Runtime;
        Interpreter interpreter(reporter);
        interpreter.interpret(statements);
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}

/**
 * Run an interactive REPL (Read-Eval-Print-Loop)
 * Allows users to input pseudocode lines interactively
 * Each line is tokenized, parsed, and executed
 * @return Always returns 0
 */
int Pseudocode::runRepl() {
    InterpreterStage stage = InterpreterStage::Lexing;

    std::cout << "Interactive SCSA Pseudocode Interpreter" << std::endl;
    std::cout << "For help type run this program with '--help' or '-h'" << std::endl;
    std::cout << "Type 'exit' to quit" << std::endl;

    // Make an interpreter and error reporter to keep state across this session
    ErrorReporter reporter(stage, "", "");
    reporter.setReplMode(true);
    Interpreter interpreter(reporter);
    std::vector<StmtPtr> statements;

    std::string line;
    while (true) {
        // Display prompt and read a line of input
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
            // --- Lexing ---
            stage = InterpreterStage::Lexing;
            Lexer lexer(line, reporter);
            std::vector<Token> tokens = lexer.scanTokens();
            if (debugTokens)
                printTokenTable(tokens);

            // Check error before parsing
            if (reporter.hadError)
                continue;

            // --- Parsing ---
            stage = InterpreterStage::Parsing;
            Parser parser(tokens, line, reporter);
            std::vector<StmtPtr> parsed = parser.parse();

            if (debugParse) {
                ASTPrinter printer;
                printer.print(parsed);
            }

            // Check error before executing
            if (reporter.hadError)
                continue;

            // --- Execution ---
            stage = InterpreterStage::Runtime;
            interpreter.interpret(parsed);
        } catch (const std::runtime_error &e) {
            // Do nothing
        } catch (const std::exception &e) {
            // Shouldn't happen
            std::cerr << "[This error isn't supposed to occur...] " << e.what() << std::endl;
        }
    }

    return 0;
}

/**
 * Entry point for the Pseudocode interpreter
 *
 */
int main(int argc, char *argv[]) {

#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD consoleMode;
    GetConsoleMode(hConsole, &consoleMode);
    consoleMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hConsole, consoleMode); // Enable ANSI colours

    // Set input/output code page to UTF-8
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif

    Pseudocode pseudocode;

    if (argc > 1 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")) {
        help();
        return 0;
    }

    // Parse optional arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--debug-tokens") {
            pseudocode.debugTokens = true;
        } else if (arg == "--debug-parse") {
            pseudocode.debugParse = true;
        } else {
            // If file ends in .scsa then treat as script
            if (arg.size() < 5 || arg.substr(arg.size() - 5) != ".scsa") {
                help();
                return 1;
            } else {
                return pseudocode.runFile(arg);
            }
        }
    }

    if (argc == 1) {
        return pseudocode.runRepl();
    }
}
