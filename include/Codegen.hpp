#pragma once

#include <memory>
#include <string>
#include <vector>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

#include "AST.hpp"

struct LocalVar {
  llvm::AllocaInst *alloca;
  llvm::Type *type;
};

class Codegen {
public:
  Codegen(const std::string &moduleName);
  ~Codegen();

  void generate(const std::vector<Statement *> &statements);

  std::unique_ptr<llvm::Module> takeModule();

private:
  llvm::LLVMContext context;
  std::unique_ptr<llvm::Module> module;
  std::unique_ptr<llvm::IRBuilder<>> builder;
  std::unordered_map<std::string, LocalVar> locals;

  static llvm::Type *getLLVMType(const std::string &type,
                                 llvm::LLVMContext &ctx);

  llvm::Value *genExpr(Expr *expr);
  llvm::Function *genFunction(FunctionDecl *funcDecl);
  void genVarDecl(VarDecl *varDecl);
  void genReturnStatement(ReturnStmt* stmt);
  void genStatement(Statement *stmt);
};
