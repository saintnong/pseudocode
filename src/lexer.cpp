#include "lexer.hpp"

/**
 * Lexer Constructor
 * Initializes the lexer with source code and sets up the keyword mapping.
 * @param src The source code to tokenize
 * @param errReporter Reference to error reporter for error handling
 */
Lexer::Lexer(const std::string &src, ErrorReporter &errReporter)
    : source(src), start(0), current(0), line(1), startLine(1), startColumn(0), column(0),
      reporter(errReporter) {

    /**
     * Initialize the keyword map
     */
    keywords["CLASS"]      = TOK_CLASS;
    keywords["ATTRIBUTES"] = TOK_ATTRIBUTES;
    keywords["METHODS"]    = TOK_METHODS;
    keywords["FUNCTION"]   = TOK_FUNCTION;
    keywords["RETURN"]     = TOK_RETURN;
    keywords["END"]        = TOK_END;
    keywords["NEW"]        = TOK_NEW;

    keywords["WHILE"] = TOK_WHILE;
    keywords["IF"]    = TOK_IF;
    keywords["THEN"]  = TOK_THEN;
    keywords["ELSE"]  = TOK_ELSE;
    keywords["IN"]    = TOK_IN;
    keywords["FOR"]   = TOK_FOR;
    keywords["AND"]   = TOK_AND;
    keywords["OR"]    = TOK_OR;
    keywords["NOT"]   = TOK_NOT;

    keywords["TRUE"]  = TOK_TRUE;
    keywords["FALSE"] = TOK_FALSE;

    keywords["Attributes"] = TOK_ATTRIBUTES;
    keywords["Methods"]    = TOK_METHODS;
    keywords["True"]       = TOK_TRUE;
    keywords["False"]      = TOK_FALSE;
    keywords["new"]        = TOK_NEW;
}

/**
 * Main tokenization loop
 * Scans through the entire source code and generates a list of tokens.
 * @return Vector of tokens representing the source code
 */
std::vector<Token> Lexer::scanTokens() {
    while (!isAtEnd()) {
        start       = current;
        startLine   = line;
        startColumn = column;
        scanToken();
    }
    tokens.push_back({TOK_EOF, "", line, column, 0});
    return tokens;
}

/**
 * Check if we've reached the end of the source code
 * @return true if current position is at or past the end of source
 */
bool Lexer::isAtEnd() {
    return current >= source.length();
}

/**
 * Consume and return the current character, then move to the next one
 * @return The current character before advancing
 */
char Lexer::advance() {
    char c = source[current++];
    if (c == '\n') {
        column = 0;
    } else {
        column++;
    }
    return c;
}

/**
 * Look at the current character without consuming it
 * @return The current character, or null terminator if at end
 */
char Lexer::peek() {
    if (isAtEnd())
        return '\0';
    return source[current];
}

/**
 * Conditionally consume a character if it matches the expected one
 * @param expected The character to match
 * @return true if the character was matched and consumed, false otherwise
 */
bool Lexer::match(char expected) {
    if (isAtEnd())
        return false;
    if (source[current] != expected)
        return false;
    current++;
    return true;
}

/**
 * Add a token to the token list, extracting lexeme from source
 * @param type The type of token to add
 */
void Lexer::addToken(TokenKind type) {
    std::string text = source.substr(start, current - start);
    size_t length    = current - start;
    tokens.push_back({type, text, line, startColumn, length});
}

/**
 * Add a token to the token list with a provided literal value
 * @param type The type of token to add
 * @param literal The literal value for the token
 */
void Lexer::addToken(TokenKind type, std::string literal) {
    size_t length = current - start;
    tokens.push_back({type, literal, line, startColumn, length});
}

/**
 * Report an error with contextual information about the problematic line
 * @param type The type of error (Syntax, Type, etc.)
 * @param message The error message to display
 */
void Lexer::reportError(ErrorType type, const std::string &message) {
    // Calculate the column position relative to the start of the line
    size_t lineStart = start;
    while (lineStart > 0 && source[lineStart - 1] != '\n') {
        lineStart--;
    }

    size_t errorColumn = start - lineStart;

    // Calculate the length of the erroneous token
    size_t tokenLength = current - start;
    if (tokenLength == 0)
        tokenLength = 1;

    // Truncate token length if we meet a newline within it
    for (size_t i = start; i < current; i++) {
        if (source[i] == '\n') {
            tokenLength = i - start;
            break;
        }
    }

    // Report error at the line where the token started, not where we are now
    // This is important for multi-line tokens like unterminated strings
    reporter.report(type, startLine, errorColumn, message, tokenLength);
}

/**
 * Scan a string literal delimited by the specified quote character
 * Handles line counting for multi-line strings
 * @param quoteType The quote character that delimits the string ('\"' or '\'')
 */
void Lexer::string(char quoteType) {
    // Consume characters until we find the closing quote
    while (peek() != quoteType && !isAtEnd()) {
        if (peek() == '\n')
            line++; // Track line numbers for multi-line strings
        advance();
    }

    // Report error if string is never terminated
    if (isAtEnd()) {
        reportError(ErrorType::Syntax, "Unterminated string.");
        return;
    }

    advance(); // Consume closing quote
    // Extract string value without the surrounding quotes
    std::string value = source.substr(start + 1, current - start - 2);
    addToken(TOK_STRING, value);
}

/**
 * Scan a numeric literal (integer or float)
 * Detects floats by looking for a decimal point followed by digits
 */
void Lexer::number() {
    // Consume all leading digits
    while (isdigit(peek())) {
        advance();
    }

    // Check for decimal point to distinguish float from integer
    // Look ahead to ensure there's a digit after the decimal
    if (peek() == '.' && isdigit(source[current + 1])) {
        advance(); // Consume the decimal point
        while (isdigit(peek()))
            advance();
        addToken(TOK_FLOAT);
    } else {
        // Ensure number doesn't run into a variable name (e.g., "123abc")
        if (!isalpha(peek())) {
            addToken(TOK_INTEGER);
        } else {
            reportError(ErrorType::Syntax, "Identifier starts with number.");
        }
    }
}

/**
 * Scan an identifier or keyword
 * Identifiers can contain alphanumeric characters and underscores
 * Keywords are recognized by lookup in the keyword map
 */
void Lexer::identifier() {
    // Consume all alphanumeric characters and underscores
    while (isalnum(peek()) || peek() == '_')
        advance();

    std::string text = source.substr(start, current - start);
    TokenKind type   = TOK_IDENTIFIER;

    // Check if the identifier is actually a reserved keyword
    if (keywords.find(text) != keywords.end()) {
        type = keywords[text];
    }
    addToken(type);
}

/**
 * Scan a single token from the source code
 * Dispatches to appropriate handler based on the current character
 */
void Lexer::scanToken() {
    char c = advance();
    switch (c) {
    /**
     * Parentheses
     */
    case '(':
        addToken(TOK_LPAREN);
        break;
    case ')':
        addToken(TOK_RPAREN);
        break;
    case '[':
        addToken(TOK_LBRACKET);
        break;
    case ']':
        addToken(TOK_RBRACKET);
        break;

    /**
     * Special tokens
     */
    case ',':
        addToken(TOK_COMMA);
        break;
    case '.':
        addToken(TOK_DOT);
        break;
    case ':':
        addToken(TOK_COLON);
        break;
    case '!':
        if (match('=')) {
            addToken(TOK_NOT_EQUAL);
        } else {
            reportError(ErrorType::Syntax, "Unexpected character '!'. Did you mean '!=' ?");
        }
        break;

    /**
     * Arithmetic tokens
     */
    case '+':
        addToken(TOK_PLUS);
        break;
    case '-':
        addToken(TOK_MINUS);
        break;
    case '/':
        // Division and comments are handled together
        if (match('/')) {
            // A comment goes until the end of the line.
            // We keep advancing until we hit a newline or EOF.
            // Also don't consume \n since that's consumed elsewhere.
            while (peek() != '\n' && !isAtEnd()) {
                advance();
            }
        } else {
            addToken(TOK_DIVIDE);
        }
        break;
    case '*':
        addToken(TOK_MULTIPLY);
        break;

    case '#':
        // Other method for comments
        while (peek() != '\n' && !isAtEnd()) {
            advance();
        }
        break;

    /**
     * Assignment and comparisons
     */
    case '=':
        // Equality and assignments handled together
        addToken(match('=') ? TOK_EQUAL : TOK_ASSIGN);
        break;
    case '<':
        // < and <= handled together
        addToken(match('=') ? TOK_LT_OR_EQ : TOK_LESS_THAN);
        break;
    case '>':
        // > and >= handled together
        addToken(match('=') ? TOK_GT_OR_EQ : TOK_GREATER_THAN);
        break;

    /**
     * Skipped bytes
     */
    case ' ':
    case '\r':
    case '\t':
        break;
    case '\n':
        line++;
        break;

    /**
     * String handling
     */
    case '"':
        string('"');
        break;
    case '\'':
        string('\'');
        break;

    default:
        if (isdigit(c)) {
            // Ints e.g. 123
            number();
        } else if (isalpha(c) || c == '_') {
            // Identifiers
            identifier();
        } else {
            // Bad character
            std::string msg = "Unexpected character '";
            msg += c;
            msg += "'.";
            reportError(ErrorType::Syntax, msg);
        }
        break;
    }
}
