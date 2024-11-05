#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <memory>

namespace AST {

// Forward declarations
class ExprAST;
using ExprPtr = std::unique_ptr<ExprAST>;
using ExprList = std::vector<ExprPtr>;

// Base class for all expressions
class ExprAST {
public:
    virtual ~ExprAST() = default;
};

// Number literal expression
class NumberExprAST : public ExprAST {
    union {
        double doubleVal;
        int intVal;
    };
    bool isFloat;
public:
    // Single argument constructors
    explicit NumberExprAST(double val) : doubleVal(val), isFloat(true) {}
    explicit NumberExprAST(int val) : intVal(val), isFloat(false) {}
    
    // Two argument constructors for explicit type control
    NumberExprAST(double val, bool isFloat) : doubleVal(val), isFloat(isFloat) {}
    NumberExprAST(int val, bool isFloat) : intVal(val), isFloat(isFloat) {}
    
    double getDoubleValue() const { return isFloat ? doubleVal : static_cast<double>(intVal); }
    int getIntValue() const { return isFloat ? static_cast<int>(doubleVal) : intVal; }
    bool isFloatingPoint() const { return isFloat; }
};

// String literal expression
class StringExprAST : public ExprAST {
    std::string value;
public:
    StringExprAST(const std::string& val) : value(val) {}
    const std::string& getValue() const { return value; }
};

// Variable reference expression
class VariableExprAST : public ExprAST {
    std::string name;
public:
    VariableExprAST(const std::string& n) : name(n) {}
    const std::string& getName() const { return name; }
};

// Binary operation expression
class BinaryExprAST : public ExprAST {
    char op;
    ExprPtr lhs, rhs;
public:
    BinaryExprAST(char op, ExprPtr lhs, ExprPtr rhs)
        : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
    
    char getOp() const { return op; }
    const ExprPtr& getLHS() const { return lhs; }
    const ExprPtr& getRHS() const { return rhs; }
};

// Unary operation expression
class UnaryExprAST : public ExprAST {
    char op;
    ExprPtr operand;
public:
    UnaryExprAST(char op, ExprPtr operand)
        : op(op), operand(std::move(operand)) {}
    
    char getOp() const { return op; }
    const ExprPtr& getOperand() const { return operand; }
};

// Function call expression
class CallExprAST : public ExprAST {
    std::string callee;
    ExprList args;
public:
    CallExprAST(const std::string& callee, ExprList args)
        : callee(callee), args(std::move(args)) {}
    
    const std::string& getCallee() const { return callee; }
    const ExprList& getArgs() const { return args; }
};

// Grouping expression
class GroupingExprAST : public ExprAST {
    ExprPtr expression;
public:
    GroupingExprAST(ExprPtr expr)
        : expression(std::move(expr)) {}
    
    const ExprPtr& getExpression() const { return expression; }
};

// Base class for all statements
class StmtAST {
public:
    virtual ~StmtAST() = default;
};

using StmtPtr = std::unique_ptr<StmtAST>;
using StmtList = std::vector<StmtPtr>;

// Define statement classes
class YapStmtAST : public StmtAST {
    std::vector<ExprPtr> args;
public:
    YapStmtAST(std::vector<ExprPtr> args) : args(std::move(args)) {}
    const std::vector<ExprPtr>& getArgs() const { return args; }
};

class SusStmtAST : public StmtAST {
    ExprPtr condition;
    StmtList thenBlock;
    StmtList elseBlock;
public:
    SusStmtAST(ExprPtr cond, StmtList thenB, StmtList elseB)
        : condition(std::move(cond)),
          thenBlock(std::move(thenB)),
          elseBlock(std::move(elseB)) {}
          
    const ExprPtr& getCondition() const { return condition; }
    const StmtList& getThenBlock() const { return thenBlock; }
    const StmtList& getElseBlock() const { return elseBlock; }
};

class BetStmtAST : public StmtAST {
    StmtPtr init;
    ExprPtr condition;
    StmtPtr increment;
    StmtList body;
public:
    BetStmtAST(StmtPtr init, ExprPtr cond, StmtPtr inc, StmtList body)
        : init(std::move(init)),
          condition(std::move(cond)),
          increment(std::move(inc)),
          body(std::move(body)) {}
          
    const StmtPtr& getInit() const { return init; }
    const ExprPtr& getCondition() const { return condition; }
    const StmtPtr& getIncrement() const { return increment; }
    const StmtList& getBody() const { return body; }
};

class BruhAST : public StmtAST {
    std::string name;
    std::vector<std::string> args;
    StmtList body;
public:
    BruhAST(const std::string& name, 
            std::vector<std::string> args,
            StmtList body)
        : name(name), args(std::move(args)), body(std::move(body)) {}
};

class CookAST : public StmtAST {
    StmtList body;
public:
    CookAST(StmtList body) : body(std::move(body)) {}
    const StmtList& getBody() const { return body; }
};

// Expression statement
class ExprStmtAST : public StmtAST {
    ExprPtr expr;
public:
    ExprStmtAST(ExprPtr e) : expr(std::move(e)) {}
    const ExprPtr& getExpr() const { return expr; }
};

// Variable declaration statement
class VarDeclStmtAST : public StmtAST {
    std::string name;
    ExprPtr initializer;
public:
    VarDeclStmtAST(const std::string& n, ExprPtr init)
        : name(n), initializer(std::move(init)) {}
    const std::string& getName() const { return name; }
    const ExprPtr& getInitializer() const { return initializer; }
};

// Assignment expression
class AssignExprAST : public ExprAST {
    std::string name;
    ExprPtr value;
public:
    AssignExprAST(const std::string& n, ExprPtr val)
        : name(n), value(std::move(val)) {}
    const std::string& getName() const { return name; }
    const ExprPtr& getValue() const { return value; }
};

} 

#endif