#include "codegen.h"
#include <iostream>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <functional>
#include <map>

CodeGen::CodeGen() {
    // Initialize LLVM's native target, assembly printer, and parser
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    // Create LLVM context, module, and IR builder
    context = std::make_unique<llvm::LLVMContext>();
    module = std::make_unique<llvm::Module>("brainrotlang", *context);
    builder = std::make_unique<llvm::IRBuilder<>>(*context);

    // Configure target-specific settings
    auto targetTriple = llvm::sys::getProcessTriple();
    module->setTargetTriple(targetTriple);

    // Initialize Just-In-Time compiler
    auto JITBuilder = llvm::orc::LLJITBuilder();
    auto jitOrError = JITBuilder.create();
    if (auto err = jitOrError.takeError()) {
        std::cerr << "Failed to create JIT: " 
                  << llvm::toString(std::move(err)) << std::endl;
        return;
    }
    jit = std::move(*jitOrError);

    // Configure data layout based on JIT settings
    module->setDataLayout(jit->getDataLayout());
}

void CodeGen::generateCode(AST::CookAST* ast) {
    // Set up printf function for output operations
    auto printfType = llvm::FunctionType::get(
        builder->getInt32Ty(),
        {builder->getInt8Ty()->getPointerTo()},
        true  // varargs
    );
    module->getOrInsertFunction("printf", printfType);

    // Create main function with return type int and no parameters
    auto mainType = llvm::FunctionType::get(builder->getInt32Ty(), {}, false);
    auto mainFunc = llvm::Function::Create(
        mainType,
        llvm::Function::ExternalLinkage,
        "main",
        module.get()
    );

    // Set up the entry point of the program
    auto entry = llvm::BasicBlock::Create(*context, "entry", mainFunc);
    builder->SetInsertPoint(entry);

    // Generate IR for each statement in the AST
    for (const auto& stmt : ast->getBody()) {
        generateStmt(stmt.get());
    }

    // Add return 0 at the end of main
    builder->CreateRet(builder->getInt32(0));
}

llvm::Value* CodeGen::generateExpr(AST::ExprAST* expr) {
    // Handle variable references
    if (auto varExpr = dynamic_cast<AST::VariableExprAST*>(expr)) {
        if (auto it = namedValues.find(varExpr->getName()); it != namedValues.end()) {
            return builder->CreateLoad(builder->getDoubleTy(), it->second, varExpr->getName());
        }
        throw std::runtime_error("Unknown variable name: " + varExpr->getName());
    }
    
    // Handle numeric literals (both integer and floating-point)
    if (auto numberExpr = dynamic_cast<AST::NumberExprAST*>(expr)) {
        if (numberExpr->isFloatingPoint()) {
            return llvm::ConstantFP::get(*context, 
                llvm::APFloat(numberExpr->getDoubleValue()));
        } else {
            return llvm::ConstantInt::get(*context, 
                llvm::APInt(32, numberExpr->getIntValue()));
        }
    }
    
    // Handle string literals
    if (auto stringExpr = dynamic_cast<AST::StringExprAST*>(expr)) {
        return builder->CreateGlobalStringPtr(stringExpr->getValue());
    }
    
    // Handle binary operations (+, -, *, /, <, >, =)
    if (auto binaryExpr = dynamic_cast<AST::BinaryExprAST*>(expr)) {
        llvm::Value* L = generateExpr(binaryExpr->getLHS().get());
        llvm::Value* R = generateExpr(binaryExpr->getRHS().get());
        
        bool lhsIsFloat = L->getType()->isDoubleTy();
        bool rhsIsFloat = R->getType()->isDoubleTy();
        
        bool needsFloat = lhsIsFloat || rhsIsFloat;
        
        if (needsFloat) {
            if (!lhsIsFloat) L = builder->CreateSIToFP(L, builder->getDoubleTy());
            if (!rhsIsFloat) R = builder->CreateSIToFP(R, builder->getDoubleTy());
            
            switch (binaryExpr->getOp()) {
                case '+': return builder->CreateFAdd(L, R);
                case '-': return builder->CreateFSub(L, R);
                case '*': return builder->CreateFMul(L, R);
                case '/': return builder->CreateFDiv(L, R);
                case '<': return builder->CreateFCmpOLT(L, R, "cmptmp");
                case '>': return builder->CreateFCmpOGT(L, R, "cmptmp");
                case '=': return builder->CreateFCmpOEQ(L, R, "cmptmp");
            }
        } else {
            switch (binaryExpr->getOp()) {
                case '+': return builder->CreateAdd(L, R);
                case '-': return builder->CreateSub(L, R);
                case '*': return builder->CreateMul(L, R);
                case '/': {
                    L = builder->CreateSIToFP(L, builder->getDoubleTy());
                    R = builder->CreateSIToFP(R, builder->getDoubleTy());
                    return builder->CreateFDiv(L, R);
                }
                case '<': return builder->CreateICmpSLT(L, R, "cmptmp");
                case '>': return builder->CreateICmpSGT(L, R, "cmptmp");
                case '=': return builder->CreateICmpEQ(L, R, "cmptmp");
            }
        }
        throw std::runtime_error("Invalid binary operator");
    }
    
    // Handle unary operations (currently only negation)
    if (auto unaryExpr = dynamic_cast<AST::UnaryExprAST*>(expr)) {
        llvm::Value* operandVal = generateExpr(unaryExpr->getOperand().get());
        
        if (!operandVal) return nullptr;
        
        switch (unaryExpr->getOp()) {
            case '-':
                return builder->CreateFNeg(operandVal, "negtmp");
            default:
                throw std::runtime_error("Invalid unary operator");
        }
    }
    
    // Handle parenthesized expressions
    if (auto groupExpr = dynamic_cast<AST::GroupingExprAST*>(expr)) {
        return generateExpr(groupExpr->getExpression().get());
    }
    
    // Handle variable assignment
    if (auto assignExpr = dynamic_cast<AST::AssignExprAST*>(expr)) {
        llvm::Value* value = generateExpr(assignExpr->getValue().get());
        if (auto it = namedValues.find(assignExpr->getName()); it != namedValues.end()) {
            builder->CreateStore(value, it->second);
            return value;
        }
        throw std::runtime_error("Undefined variable: " + assignExpr->getName());
    }
    
    // Handle function calls (currently only 'yap' for printing)
    if (auto callExpr = dynamic_cast<AST::CallExprAST*>(expr)) {
        if (callExpr->getCallee() == "yap") {
            auto printfFunc = module->getFunction("printf");
            std::vector<llvm::Value*> argsV;
            
            // Handle multiple arguments
            std::string formatStr;
            for (const auto& arg : callExpr->getArgs()) {
                llvm::Value* argVal = generateExpr(arg.get());
                if (argVal->getType()->isDoubleTy()) {
                    formatStr += "%f";
                } else if (argVal->getType()->isIntegerTy()) {
                    formatStr += "%d";
                } else if (argVal->getType()->isPointerTy()) {
                    formatStr += "%s";
                }
                argsV.push_back(argVal);
            }
            formatStr += "\n";
            
            // Create format string and add it as first argument
            argsV.insert(argsV.begin(), builder->CreateGlobalStringPtr(formatStr));
            
            return builder->CreateCall(printfFunc, argsV);
        }
        throw std::runtime_error("Unknown function: " + callExpr->getCallee());
    }
    
    throw std::runtime_error("Unknown expression type");
}

void CodeGen::generateStmt(AST::StmtAST* stmt) {
    // Handle variable declarations
    if (auto varDecl = dynamic_cast<AST::VarDeclStmtAST*>(stmt)) {
        // Allocate space for variable and store initial value
        llvm::Function* function = builder->GetInsertBlock()->getParent();
        llvm::IRBuilder<> tempBuilder(&function->getEntryBlock(),
                                    function->getEntryBlock().begin());
        llvm::AllocaInst* alloca = tempBuilder.CreateAlloca(
            builder->getDoubleTy(),
            nullptr,
            varDecl->getName()
        );
        
        // Store initial value
        llvm::Value* initVal = generateExpr(varDecl->getInitializer().get());
        builder->CreateStore(initVal, alloca);
        
        // Add to symbol table
        namedValues[varDecl->getName()] = alloca;
        return;
    }
    
    // Handle print statements ('yap')
    else if (auto yapStmt = dynamic_cast<AST::YapStmtAST*>(stmt)) {
        auto printfFunc = module->getFunction("printf");
        std::vector<llvm::Value*> printArgs;
        std::string formatStr;
        
        // Process each argument
        for (const auto& arg : yapStmt->getArgs()) {
            auto value = generateExpr(arg.get());
            
            // Add to format string based on type
            if (auto strExpr = dynamic_cast<AST::StringExprAST*>(arg.get())) {
                formatStr += "%s";
            } else if (value->getType()->isDoubleTy()) {
                formatStr += "%.6f";
            } else if (value->getType()->isIntegerTy()) {
                formatStr += "%d";
            }
            
            printArgs.push_back(value);
        }
        formatStr += "\n"; // Add newline at the end
        
        // Insert format string as first argument
        printArgs.insert(printArgs.begin(), 
            builder->CreateGlobalStringPtr(formatStr));
        
        // Create the call
        builder->CreateCall(printfFunc, printArgs);
    }
    
    // Handle if statements ('sus')
    else if (auto susStmt = dynamic_cast<AST::SusStmtAST*>(stmt)) {
        // Generate condition
        llvm::Value* condValue = generateExpr(susStmt->getCondition().get());
        
        if (condValue->getType()->isDoubleTy()) {
            condValue = builder->CreateFCmpONE(
                condValue,
                llvm::ConstantFP::get(*context, llvm::APFloat(0.0)),
                "ifcond"
            );
        }
        
        llvm::Function* theFunction = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(*context, "then", theFunction);
        llvm::BasicBlock* elseBB = llvm::BasicBlock::Create(*context, "else");
        llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(*context, "ifcont");
        
        builder->CreateCondBr(condValue, thenBB, elseBB);
        
        // Emit then block
        builder->SetInsertPoint(thenBB);
        for (const auto& thenStmt : susStmt->getThenBlock()) {
            generateStmt(thenStmt.get());
        }
        builder->CreateBr(mergeBB);
        
        // Emit else block
        theFunction->insert(theFunction->end(), elseBB);
        builder->SetInsertPoint(elseBB);
        for (const auto& elseStmt : susStmt->getElseBlock()) {
            generateStmt(elseStmt.get());
        }
        builder->CreateBr(mergeBB);
        
        theFunction->insert(theFunction->end(), mergeBB);
        builder->SetInsertPoint(mergeBB);
    }
    
    // Handle loop statements ('bet')
    else if (auto betStmt = dynamic_cast<AST::BetStmtAST*>(stmt)) {
        llvm::Function* theFunction = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock* condBB = llvm::BasicBlock::Create(*context, "loopcond", theFunction);
        llvm::BasicBlock* loopBB = llvm::BasicBlock::Create(*context, "loop");
        llvm::BasicBlock* afterBB = llvm::BasicBlock::Create(*context, "afterloop");
        
        // Generate initialization
        if (betStmt->getInit()) {
            generateStmt(betStmt->getInit().get());
        }
        
        builder->CreateBr(condBB);
        builder->SetInsertPoint(condBB);
        
        // Generate condition
        llvm::Value* condValue = nullptr;
        if (betStmt->getCondition()) {
            condValue = generateExpr(betStmt->getCondition().get());
            if (condValue->getType()->isDoubleTy()) {
                condValue = builder->CreateFCmpONE(
                    condValue,
                    llvm::ConstantFP::get(*context, llvm::APFloat(0.0)),
                    "loopcond"
                );
            }
        } else {
            condValue = builder->getInt1(true);
        }
        
        builder->CreateCondBr(condValue, loopBB, afterBB);
        
        theFunction->insert(theFunction->end(), loopBB);
        builder->SetInsertPoint(loopBB);
        
        for (const auto& bodyStmt : betStmt->getBody()) {
            generateStmt(bodyStmt.get());
        }
        
        if (betStmt->getIncrement()) {
            generateStmt(betStmt->getIncrement().get());
        }
        
        builder->CreateBr(condBB);
        
        theFunction->insert(theFunction->end(), afterBB);
        builder->SetInsertPoint(afterBB);
    }
    
    // Handle expression statements
    else if (auto exprStmt = dynamic_cast<AST::ExprStmtAST*>(stmt)) {
        generateExpr(exprStmt->getExpr().get());
    }
}

void CodeGen::executeCode() {
    // Ensure JIT is properly initialized
    if (!jit) {
        llvm::errs() << "JIT not initialized\n";
        return;
    }

    // Create thread-safe module for JIT compilation
    auto threadSafeModule = llvm::orc::ThreadSafeModule(
        std::move(module),
        std::move(context)
    );

    // Add module to JIT compiler
    if (auto err = jit->addIRModule(std::move(threadSafeModule))) {
        llvm::errs() << "Error adding module to JIT: " 
                     << toString(std::move(err)) << "\n";
        return;
    }

    // Look up main function
    auto mainSymbol = jit->lookup("main");
    if (!mainSymbol) {
        llvm::errs() << "Could not find main function: "
                     << toString(mainSymbol.takeError()) << "\n";
        return;
    }

    // Execute main function
    auto mainFn = llvm::jitTargetAddressToFunction<int(*)()>(
        mainSymbol->getValue()
    );

    llvm::outs() << "Executing main function...\n";
    int result = mainFn();
    llvm::outs() << "Program finished with code: " << result << "\n";
}

// Helper function to generate appropriate printf format string based on value type
llvm::Value* CodeGen::getFormatString(llvm::Value* exprValue) {
    // For floating point types (double), use %.6f format with 6 decimal places
    if (exprValue->getType()->isDoubleTy()) {
        return builder->CreateGlobalStringPtr("%.6f\n");
    } 
    // For integer types, use %d format
    else if (exprValue->getType()->isIntegerTy()) {
        return builder->CreateGlobalStringPtr("%d\n");
    } 
    // For pointer types (strings), use %s format
    else if (exprValue->getType()->isPointerTy()) {
        return builder->CreateGlobalStringPtr("%s\n");
    }
    
    // If type doesn't match any of the above, throw an error
    throw std::runtime_error("Unsupported type for yap statement");
}