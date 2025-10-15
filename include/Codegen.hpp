#pragma once

#include <memory>
#include <string>
#include <vector>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

#include "AST.hpp"

class Codegen {
public:
  Codegen(const std::string &moduleName);
  ~Codegen();

  // Entry point: generate LLVM IR for a list of statements (the program)
  void generate(const std::vector<Statement *> &statements);

  // Access generated module
  std::unique_ptr<llvm::Module> takeModule();

private:
  llvm::LLVMContext context;
  std::unique_ptr<llvm::Module> module;
  std::unique_ptr<llvm::IRBuilder<>> builder;

  llvm::Value *genExpr(Expr *expr);
  void genStatement(Statement *stmt);

  void genVarDecl(VarDecl *varDecl);
};
