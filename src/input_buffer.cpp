#include "input_buffer.hpp"

#include "errors.hpp"
#include "lexer.hpp"

/**
 * Check if the input needs more lines to complete block structures.
 *
 * This function tokenizes the input and counts block depth by tracking
 * block-opening keywords vs END tokens. A positive depth indicates
 * unclosed blocks that need more input.
 *
 * Also: Special handling for compound terminators. In this language,
 * blocks are closed with "END KEYWORD" (e.g., "END IF", "END WHILE").
 * When we see END followed by IF/WHILE/FOR, the second token is part
 * of the terminator, not a new block opener.
 *
 * @param input The accumulated input buffer
 * @return true if blocks are unclosed, false if input is complete
 */
bool InputBuffer::needsContinuation(const std::string &input) {
    // Use a silent error reporter - ignore errors until we have a full program
    ErrorReporter reporter("", input);
    reporter.setSilent(true);

    Lexer lexer(input, reporter);
    std::vector<Token> tokens = lexer.scanTokens();

    // Track block depth: opening keywords increment, END decrements
    int depth           = 0;
    bool previousWasEnd = false;

    for (const Token &token : tokens) {
        switch (token.type) {
        // Block-opening keywords
        case TOK_FUNCTION:
        case TOK_CLASS:
        case TOK_IF:
        case TOK_WHILE:
        case TOK_CASE:
        case TOK_FOR:
        case TOK_REPEAT:
            // If previous token was END, this is part of "END IF/WHILE/FOR"
            // compound terminator, not a new block opener
            if (!previousWasEnd) {
                depth++;
            }
            previousWasEnd = false;
            break;

        // Block-closing keywords
        case TOK_END:
        case TOK_UNTIL:
            depth--;
            // 'END' can sometimes form compound terminators
            // E.g. "END IF", "END FOR"
            previousWasEnd = (token.type == TOK_END);
            break;

        default:
            previousWasEnd = false;
            break;
        }
    }

    // If depth > 0, we have unclosed blocks and need more input
    return depth > 0;
}
