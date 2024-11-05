#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <map>

class CodeGen {
public:
    CodeGen();
    void generateCode(AST::CookAST* ast);
    void executeCode();

private:
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;
    std::unique_ptr<llvm::orc::LLJIT> jit;
    std::map<std::string, llvm::AllocaInst*> namedValues;

    llvm::Function* createPrintFunction();
    void generateStmt(AST::StmtAST* stmt);
    llvm::Value* generateExpr(AST::ExprAST* expr);
    llvm::Value* getFormatString(llvm::Value* exprValue);
};

#endif