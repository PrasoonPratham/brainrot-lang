#include "parser.h"
#include <stdexcept>
#include <iostream>

// Constructor - Takes a vector of tokens to parse
Parser::Parser(std::vector<Token> tokens) : tokens(std::move(tokens)) {}

// Helper method to move forward in the token stream
Token Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

// Check if we've reached the end of our token stream
bool Parser::isAtEnd() const {
    return peek().type == TOK_EOF;
}

// Peek at the current token without consuming it
Token Parser::peek() const {
    return tokens[current];
}

// Get the most recently consumed token
Token Parser::previous() const {
    return tokens[current - 1];
}

// Try to match the current token with an expected type
// Returns true and advances if matched, false otherwise
bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

// Check if current token matches expected type without consuming it
bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

// Consume a token if it matches expected type, otherwise throw error
Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    throw std::runtime_error(message);
}

// Parse the main cook{} function - Entry point of our program
std::unique_ptr<AST::CookAST> Parser::parseCook() {
    // Every program must start with 'cook'
    if (!match(TOK_COOK)) {
        throw std::runtime_error("Expected 'cook' at start of program");
    }
    
    // Parse the body of our main function
    AST::StmtList body;
    consume(TokenType::TOK_LEFT_BRACE, "Expected '{' after 'cook'");
    
    // Keep parsing statements until we hit the closing brace
    while (!check(TokenType::TOK_RIGHT_BRACE) && !isAtEnd()) {
        body.push_back(statement());
    }
    
    consume(TokenType::TOK_RIGHT_BRACE, "Expected '}' after block");
    return std::make_unique<AST::CookAST>(std::move(body));
}

// Parse any type of statement - This is where the magic happens!
AST::StmtPtr Parser::statement() {
    // Handle different types of statements based on their keywords
    if (match(TOK_YAP)) return yapStatement();     // yap("Hello!")
    if (match(TOK_FR)) return frStatement();       // fr (condition) { ... }
    if (match(TOK_BET)) return betStatement();     // bet loops
    if (match(TOK_POOKIE)) {
        // Handle pookie (variable) declarations
        Token name = consume(TOK_IDENTIFIER, "Expected variable name after 'pookie'");
        consume(TOK_EQUAL, "Expected '=' after variable name");
        AST::ExprPtr initializer = expression();
        consume(TOK_SEMICOLON, "Expected ';' after variable declaration");
        return std::make_unique<AST::VarDeclStmtAST>(name.lexeme, std::move(initializer));
    }
    if (match(TOK_NO_CAP)) {
        // Handle if-else statements with our funky 'no_cap/cap' syntax
        consume(TOK_LEFT_PAREN, "Expected '(' after 'no_cap'");
        AST::ExprPtr condition = expression();
        consume(TOK_RIGHT_PAREN, "Expected ')' after condition");
        
        AST::StmtList thenBlock = block();
        AST::StmtList elseBlock;
        
        if (match(TOK_CAP)) {
            elseBlock = block();
        }
        
        return std::make_unique<AST::SusStmtAST>(
            std::move(condition),
            std::move(thenBlock),
            std::move(elseBlock)
        );
    }
    if (match(TOK_BRUH)) return bruhStatement();
    
    throw std::runtime_error("Expected statement");
}

// Parse expressions - this handles all our mathematical and logical operations
AST::ExprPtr Parser::expression() {
    return equality();  // Start with equality as it has lowest precedence
}

// Handle equality comparisons (== and !=)
AST::ExprPtr Parser::equality() {
    // Start with comparison expression
    AST::ExprPtr expr = comparison();
    
    // Keep chaining equality operators as long as we find them
    while (match(TOK_EQUAL_EQUAL) || match(TOK_BANG_EQUAL)) {
        Token op = previous();
        AST::ExprPtr right = comparison();
        expr = std::make_unique<AST::BinaryExprAST>(
            op.lexeme[0],
            std::move(expr),
            std::move(right)
        );
    }
    
    return expr;
}

// Parse a block of code surrounded by { }
AST::StmtList Parser::block() {
    // Make sure we start with an opening brace
    consume(TOK_LEFT_BRACE, "Expected '{' at start of block");
    AST::StmtList statements;
    
    // Keep parsing statements until we hit the closing brace
    while (!check(TOK_RIGHT_BRACE) && !isAtEnd()) {
        statements.push_back(statement());
    }
    
    // Make sure we end with a closing brace
    consume(TOK_RIGHT_BRACE, "Expected '}' after block");
    return statements;
}

// Handle our print statement 'yap'
AST::StmtPtr Parser::yapStatement() {
    // yap takes arguments in parentheses
    consume(TOK_LEFT_PAREN, "Expected '(' after 'yap'");
    std::vector<AST::ExprPtr> args;
    
    // Parse comma-separated arguments
    if (!check(TOK_RIGHT_PAREN)) {
        do {
            args.push_back(expression());
        } while (match(TOK_COMMA));
    }
    
    // Clean up with closing parenthesis and semicolon
    consume(TOK_RIGHT_PAREN, "Expected ')' after arguments");
    consume(TOK_SEMICOLON, "Expected ';' after yap statement");
    
    return std::make_unique<AST::YapStmtAST>(std::move(args));
}

// Handle our for loop 'bet' statement
AST::StmtPtr Parser::betStatement() {
    // bet loops look like: bet (init, condition, increment) { body }
    consume(TOK_LEFT_PAREN, "Expected '(' after 'bet'");
    
    // Parse initialization part
    AST::StmtPtr init;
    if (match(TOK_POOKIE)) {
        // Handle new variable declaration as initializer
        Token name = consume(TOK_IDENTIFIER, "Expected variable name");
        consume(TOK_EQUAL, "Expected '=' after variable name");
        AST::ExprPtr initializer = expression();
        init = std::make_unique<AST::VarDeclStmtAST>(name.lexeme, std::move(initializer));
    } else {
        // Handle assignment as initializer
        Token name = consume(TOK_IDENTIFIER, "Expected variable name");
        consume(TOK_EQUAL, "Expected '=' after variable name");
        AST::ExprPtr value = expression();
        init = std::make_unique<AST::ExprStmtAST>(
            std::make_unique<AST::AssignExprAST>(name.lexeme, std::move(value))
        );
    }
    consume(TOK_COMMA, "Expected ',' after initialization");
    
    // Parse condition
    AST::ExprPtr condition = expression();
    consume(TOK_COMMA, "Expected ',' after condition");
    
    // Parse increment
    Token name = consume(TOK_IDENTIFIER, "Expected variable name");
    consume(TOK_EQUAL, "Expected '=' after variable name");
    AST::ExprPtr value = expression();
    AST::StmtPtr increment = std::make_unique<AST::ExprStmtAST>(
        std::make_unique<AST::AssignExprAST>(name.lexeme, std::move(value))
    );
    
    consume(TOK_RIGHT_PAREN, "Expected ')' after for clauses");
    
    // Parse body
    AST::StmtList body = block();
    
    return std::make_unique<AST::BetStmtAST>(
        std::move(init),
        std::move(condition),
        std::move(increment),
        std::move(body)
    );
}

// Handle function declarations with 'bruh'
AST::StmtPtr Parser::bruhStatement() {
    // Parse function: bruh name(params) { body }
    Token name = consume(TOK_IDENTIFIER, "Expected function name after 'bruh'");
    consume(TOK_LEFT_PAREN, "Expected '(' after function name");
    
    std::vector<std::string> parameters;
    if (!check(TOK_RIGHT_PAREN)) {
        do {
            Token param = consume(TOK_IDENTIFIER, "Expected parameter name");
            parameters.push_back(param.lexeme);
        } while (match(TOK_COMMA));
    }
    consume(TOK_RIGHT_PAREN, "Expected ')' after parameters");
    
    // Parse function body
    AST::StmtList body = block();
    
    return std::make_unique<AST::BruhAST>(
        name.lexeme,
        std::move(parameters),
        std::move(body)
    );
}

// Handle comparison expressions like greater than and less than
AST::ExprPtr Parser::comparison() {
    AST::ExprPtr expr = term();
    
    while (match({TOK_GREATER, TOK_GREATER_EQUAL, TOK_LESS, TOK_LESS_EQUAL})) {
        Token op = previous();
        AST::ExprPtr right = term();
        expr = std::make_unique<AST::BinaryExprAST>(
            op.lexeme[0],
            std::move(expr),
            std::move(right)
        );
    }
    
    return expr;
}

// Handle term expressions like addition and subtraction
AST::ExprPtr Parser::term() {
    AST::ExprPtr expr = factor();

    while (match(TOK_PLUS) || match(TOK_MINUS)) {
        Token op = previous();
        AST::ExprPtr right = factor();
        expr = std::make_unique<AST::BinaryExprAST>(
            op.lexeme[0],
            std::move(expr),
            std::move(right)
        );
    }

    return expr;
}

// Add the match method that takes an initializer list
bool Parser::match(std::initializer_list<TokenType> types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

// Handle factor expressions like multiplication and division
AST::ExprPtr Parser::factor() {
    AST::ExprPtr expr = unary();

    while (match(TOK_STAR) || match(TOK_SLASH)) {
        Token op = previous();
        AST::ExprPtr right = unary();
        expr = std::make_unique<AST::BinaryExprAST>(
            op.lexeme[0],
            std::move(expr),
            std::move(right)
        );
    }

    return expr;
}

AST::ExprPtr Parser::unary() {
    if (match(TOK_MINUS) || match(TOK_BANG)) {
        Token op = previous();
        AST::ExprPtr right = unary();
        return std::make_unique<AST::UnaryExprAST>(op.lexeme[0], std::move(right));
    }

    return primary();
}

// Handle primary expressions like numbers, identifiers, and strings
AST::ExprPtr Parser::primary() {
    if (match(TOK_NUMBER_LITERAL)) {
        Token numToken = previous();
        try {
            if (numToken.isFloat) {
                double value = std::stod(numToken.lexeme);
                return std::make_unique<AST::NumberExprAST>(value);
            } else {
                int value = std::stoi(numToken.lexeme);
                return std::make_unique<AST::NumberExprAST>(value);
            }
        } catch (const std::exception& e) {
            std::cerr << "Error parsing number '" << numToken.lexeme << "': " << e.what() << std::endl;
            throw;
        }
    }
    
    if (match(TOK_IDENTIFIER)) {
        return std::make_unique<AST::VariableExprAST>(previous().lexeme);
    }

    if (match(TOK_STRING_LITERAL)) {
        return std::make_unique<AST::StringExprAST>(previous().lexeme);
    }

    if (match(TOK_LEFT_PAREN)) {
        AST::ExprPtr expr = expression();
        consume(TOK_RIGHT_PAREN, "Expect ')' after expression.");
        return std::make_unique<AST::GroupingExprAST>(std::move(expr));
    }

    throw std::runtime_error("Expected expression.");
}

// Handle our if-else statement 'fr'
AST::StmtPtr Parser::frStatement() {
    consume(TOK_LEFT_PAREN, "Expected '(' after 'fr'");
    AST::ExprPtr condition = expression();
    consume(TOK_RIGHT_PAREN, "Expected ')' after condition");
    
    // Parse the main if block
    AST::StmtList thenBlock = block();
    AST::StmtList elseBlock;
    
    // Check for else (cap)
    if (match(TOK_CAP)) {
        elseBlock = block();
    }
    
    return std::make_unique<AST::SusStmtAST>(
        std::move(condition),
        std::move(thenBlock),
        std::move(elseBlock)
    );
}