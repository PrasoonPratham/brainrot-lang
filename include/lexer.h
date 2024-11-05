#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>

enum TokenType {
    // Keywords
    TOK_YAP,        // print
    TOK_FR,         // if
    TOK_NO_CAP,     // if
    TOK_CAP,        // else
    TOK_BET,        // for
    TOK_GOON,       // while
    TOK_BRUH,       // function
    TOK_SOLULU,     // return
    TOK_DELULU,     // try
    TOK_OOF,        // throw
    TOK_POOKIE,     // var
    TOK_GOATED,     // priority
    TOK_OHIO,       // null/void
    TOK_YEET,       // array delete
    TOK_YOINK,      // array add
    TOK_SQUAD,      // array
    TOK_SIGMA,      // class
    TOK_GHOST,      // exit
    TOK_COOK,       // main

    // Single-character tokens
    TOK_LEFT_PAREN,    // (
    TOK_RIGHT_PAREN,   // )
    TOK_LEFT_BRACE,    // {
    TOK_RIGHT_BRACE,   // }
    TOK_LEFT_BRACKET,  // [
    TOK_RIGHT_BRACKET, // ]
    TOK_COMMA,         // ,
    TOK_DOT,          // .
    TOK_MINUS,         // -
    TOK_PLUS,          // +
    TOK_SEMICOLON,     // ;
    TOK_SLASH,         // /
    TOK_STAR,          // *
    TOK_BANG,          // !
    TOK_EQUAL,         // =
    TOK_LESS,          // <
    TOK_GREATER,       // >

    // Two-character tokens
    TOK_BANG_EQUAL,     // !=
    TOK_EQUAL_EQUAL,    // ==
    TOK_LESS_EQUAL,     // <=
    TOK_GREATER_EQUAL,  // >=

    // Literals
    TOK_IDENTIFIER,
    TOK_STRING_LITERAL,
    TOK_NUMBER_LITERAL,

    TOK_EOF
};

struct Token {
    TokenType type;
    std::string lexeme;
    bool isFloat = false;
    
    Token(TokenType type, std::string lexeme, bool isFloat = false)
        : type(type), lexeme(std::move(lexeme)), isFloat(isFloat) {}
};

class Lexer {
public:
    explicit Lexer(const std::string& source);
    std::vector<Token> scanTokens();

private:
    std::string source;
    std::vector<Token> tokens;
    size_t start = 0;
    size_t current = 0;
    size_t line = 1;

    void scanToken();
    void string();
    void number();
    void identifier();
    void addToken(TokenType type);
    void addToken(TokenType type, const std::string& lexeme, bool isFloat = false);
    
    bool isAtEnd() const;
    bool match(char expected);
    char advance();
    char peek() const;
    char peekNext() const;
    bool isDigit(char c) const;
    bool isAlpha(char c) const;
    bool isAlphaNumeric(char c) const;
};