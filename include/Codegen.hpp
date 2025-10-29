#pragma once

#include <memory>
#include <string>
#include <vector>

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

#include "AST.hpp"
#include "ModuleMetadata.hpp"

struct LocalVar {
  llvm::AllocaInst *alloca;
  llvm::Type *type;
  std::string typeStr;
  bool isConst;
};

class Codegen {
public:
  Codegen(const std::string &moduleName);
  ~Codegen();

  void generate(const std::vector<Statement *> &statements);

  std::unique_ptr<llvm::Module> takeModule();

  void setModuleName(const std::string &name);
  ModuleMetadata getExportedSymbols() const;

private:
  llvm::LLVMContext context;
  std::unique_ptr<llvm::Module> module;
  std::unique_ptr<llvm::IRBuilder<>> builder;
  std::vector<std::unordered_map<std::string, LocalVar>> scopeStack;
  std::unordered_map<std::string, llvm::StructType *> structTypes;
  std::unordered_map<std::string,
                     std::vector<std::pair<std::string, std::string>>>
      structFieldMetadata;
  std::string currentModuleName;
  ModuleMetadata currentModuleExports;

  static std::string getPointedToType(const std::string &ptrType) {
    if (ptrType.empty() || ptrType.back() != '*') {
      return "";
    }
    return ptrType.substr(0, ptrType.size() - 1);
  }

  llvm::Type *getLLVMType(const std::string &type, llvm::LLVMContext &ctx);

  /// helper: cast integer values between widths (signed extend / trunc)
  static llvm::Value *castIntegerIfNeeded(llvm::IRBuilder<> *builder,
                                          llvm::Value *val, llvm::Type *fromTy,
                                          llvm::Type *toTy);

  /// helper: check if a type string represents an unsigned type
  static bool isUnsignedType(const std::string &typeStr) {
    return !typeStr.empty() && typeStr[0] == 'u';
  }

  std::string getExprTypeStr(Expr *expr) {
    if (auto *var = dynamic_cast<Variable *>(expr)) {
      LocalVar *localVar = this->findVariable(var->name);
      return localVar->typeStr;
    }

    if (dynamic_cast<IntLiteral *>(expr)) {
      return "i32";
    }
    if (dynamic_cast<FloatLiteral *>(expr)) {
      return "f32";
    }
    if (dynamic_cast<BoolLiteral *>(expr)) {
      return "bool";
    }
    if (dynamic_cast<CharLiteral *>(expr)) {
      return "char";
    }
    // For more complex expressions, we can't easily determine type without full
    // type inference Default to signed for now...
    return "i32";
  }

  /// find l-value storage for a variable name (local alloca or global variable)
  [[deprecated("Use Codegen::findVariable(name) instead.")]]
  static llvm::Value *
  findLValueStorage(llvm::Module *module,
                    std::unordered_map<std::string, LocalVar> &locals,
                    const std::string &name);

  /// helper: Get field index by name for a struct type
  int getFieldIndex(const std::string &structName,
                    const std::string &fieldName);

  // Scope management
  void pushScope();
  void popScope();
  LocalVar *findVariable(const std::string &name);
  void addVariable(const std::string &name, const LocalVar &var);

  llvm::Value *genExpr(Expr *expr);
  llvm::Value *genBinaryExpr(BinaryExpr *expr);
  llvm::Value *genCallExpr(CallExpr *expr);
  llvm::Value *genStringLiteral(const std::string &str);
  llvm::Value *genCharLiteral(char c);
  llvm::Value *genUnaryExpr(UnaryExpr *expr);
  llvm::Value *genLValue(Expr *expr);
  llvm::Value *genExprLValue(Expr *expr);
  llvm::Function *genFunction(FunctionDecl *funcDecl);
  void genVarDecl(VarDecl *varDecl);
  void genReturnStatement(ReturnStmt *stmt);
  void genStatement(Statement *stmt);

  // Control flow
  void genIfStatement(IfStmt *stmt);
  void genWhileStatement(WhileStmt *stmt);
  void genForStatement(ForStmt *stmt);
  void genBlockStatement(BlockStmt *stmt);

  // Structs
  void genStructDecl(StructDecl *structDecl);
  llvm::Value *genStructLiteral(StructLiteral *expr);
  llvm::Value *genMemberAccessExpr(MemberAccessExpr *expr);
};
