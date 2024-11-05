#include "lexer.h"

// Constructor initializes the lexer with source code
Lexer::Lexer(const std::string& source) : source(source) {}

// Main tokenization function that processes the entire source code
std::vector<Token> Lexer::scanTokens() {
    while (!isAtEnd()) {
        start = current;
        scanToken();
    }
    tokens.push_back({TOK_EOF, "", false});
    return tokens;
}

// Checks if we've reached the end of the source code
bool Lexer::isAtEnd() const {
    return current >= source.length();
}

// Checks if the next character matches the expected one and consumes it if true
bool Lexer::match(char expected) {
    if (isAtEnd()) return false;
    if (source[current] != expected) return false;
    
    current++;
    return true;
}

// Consumes and returns the current character, advancing the position
char Lexer::advance() {
    return source[current++];
}

// Adds a token with just its type (for operators and punctuation)
void Lexer::addToken(TokenType type) {
    addToken(type, source.substr(start, current - start), false);
}

// Adds a token with its type, lexeme, and float flag
void Lexer::addToken(TokenType type, const std::string& lexeme, bool isFloat) {
    tokens.push_back({type, lexeme, isFloat});
}

// Core tokenization function that processes a single token
void Lexer::scanToken() {
    char c = advance();
    switch (c) {
        // Single-character tokens (operators and punctuation)
        case '(': addToken(TOK_LEFT_PAREN); break;
        case ')': addToken(TOK_RIGHT_PAREN); break;
        case '{': addToken(TOK_LEFT_BRACE); break;
        case '}': addToken(TOK_RIGHT_BRACE); break;
        case '[': addToken(TOK_LEFT_BRACKET); break;
        case ']': addToken(TOK_RIGHT_BRACKET); break;
        case ',': addToken(TOK_COMMA); break;
        case '.': addToken(TOK_DOT); break;
        case '-': addToken(TOK_MINUS); break;
        case '+': addToken(TOK_PLUS); break;
        case ';': addToken(TOK_SEMICOLON); break;
        case '*': addToken(TOK_STAR); break;
        case '/': addToken(TOK_SLASH); break;

        // Two-character tokens (comparison operators)
        case '!': addToken(match('=') ? TOK_BANG_EQUAL : TOK_BANG); break;
        case '=': addToken(match('=') ? TOK_EQUAL_EQUAL : TOK_EQUAL); break;
        case '<': addToken(match('=') ? TOK_LESS_EQUAL : TOK_LESS); break;
        case '>': addToken(match('=') ? TOK_GREATER_EQUAL : TOK_GREATER); break;

        // Whitespace handling
        case ' ':
        case '\r':
        case '\t':
            break;
        case '\n':
            line++;
            break;

        // String literal handling
        case '"': string(); break;

        default:
            if (isDigit(c)) {
                number();
            } else if (isAlpha(c)) {
                identifier();
            } else {
                std::cerr << "Unexpected character at line " << line << ": " << c << '\n';
            }
            break;
    }
}

// Processes string literals between double quotes
void Lexer::string() {
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') line++;
        advance();
    }
    
    if (isAtEnd()) {
        std::cerr << "Unterminated string at line " << line << std::endl;
        return;
    }
    
    // Consume the closing "
    advance();
    
    // Get the string value (without the quotes)
    std::string value = source.substr(start + 1, current - start - 2);
    addToken(TOK_STRING_LITERAL, value);
}

// Processes numeric literals (both integers and floating-point)
void Lexer::number() {
    bool isFloat = false;
    std::string numStr;
    
    // Add the first digit we already consumed
    numStr = source[current - 1];
    
    // Collect digits before decimal point
    while (isDigit(peek())) {
        numStr += advance();
    }
    
    // Look for decimal point
    if (peek() == '.' && isDigit(peekNext())) {
        isFloat = true;
        numStr += advance(); // Consume the '.'
        while (isDigit(peek())) {
            numStr += advance();
        }
        
        // Convert to double and check if it's actually a whole number
        double value = std::stod(numStr);
        if (value == std::floor(value)) {
            // It's a whole number, convert back to integer
            numStr = std::to_string((int)value);
            isFloat = false;
        }
    }
    
    addToken(TOK_NUMBER_LITERAL, numStr, isFloat);
}

// Looks ahead two characters without consuming them
char Lexer::peekNext() const {
    if (current + 1 >= source.length()) return '\0';
    return source[current + 1];
}

// Helper function to check if a character is a digit
bool Lexer::isDigit(char c) const {
    return c >= '0' && c <= '9';
}

// Helper function to check if a character is a letter or underscore
bool Lexer::isAlpha(char c) const {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           c == '_';
}

// Helper function to check if a character is alphanumeric
bool Lexer::isAlphaNumeric(char c) const {
    return isAlpha(c) || isDigit(c);
}

// Processes identifiers and keywords
void Lexer::identifier() {
    while (isAlphaNumeric(peek())) advance();

    std::string text = source.substr(start, current - start);
    
    static const std::unordered_map<std::string, TokenType> keywords = {
        {"yap", TOK_YAP},
        {"fr", TOK_FR},
        {"no_cap", TOK_NO_CAP},
        {"cap", TOK_CAP},
        {"bet", TOK_BET},
        {"goon", TOK_GOON},
        {"bruh", TOK_BRUH},
        {"solulu", TOK_SOLULU},
        {"delulu", TOK_DELULU},
        {"oof", TOK_OOF},
        {"pookie", TOK_POOKIE},
        {"goated", TOK_GOATED},
        {"ohio", TOK_OHIO},
        {"yeet", TOK_YEET},
        {"yoink", TOK_YOINK},
        {"squad", TOK_SQUAD},
        {"sigma", TOK_SIGMA},
        {"ghost", TOK_GHOST},
        {"cook", TOK_COOK}
    };

    auto it = keywords.find(text);
    TokenType type = (it != keywords.end()) ? it->second : TOK_IDENTIFIER;
    addToken(type);
}

// Looks at the current character without consuming it
char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source[current];
}