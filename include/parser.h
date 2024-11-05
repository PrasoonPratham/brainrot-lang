#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"
#include <memory>
#include <vector>
#include <string>
#include <initializer_list>

class Parser {
public:
    Parser(std::vector<Token> tokens);
    std::unique_ptr<AST::CookAST> parseCook();

private:
    std::vector<Token> tokens;
    size_t current = 0;

    // Utility methods
    bool isAtEnd() const;
    Token peek() const;
    Token previous() const;
    Token advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    bool match(std::initializer_list<TokenType> types);
    Token consume(TokenType type, const std::string& message);
    
    // Expression parsing methods
    AST::ExprPtr expression();
    AST::ExprPtr equality();
    AST::ExprPtr comparison();
    AST::ExprPtr term();
    AST::ExprPtr factor();
    AST::ExprPtr unary();
    AST::ExprPtr primary();
    
    // Statement parsing methods
    AST::StmtPtr statement();
    AST::StmtPtr yapStatement();
    AST::StmtPtr frStatement();
    AST::StmtPtr betStatement();
    AST::StmtPtr bruhStatement();
    AST::StmtList block();
};

#endif