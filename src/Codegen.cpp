#include "Codegen.hpp"

using namespace llvm;

// Simple symbol table for local variables (name -> AllocaInst)
static std::unordered_map<std::string, llvm::AllocaInst *> locals;
static llvm::Type *getLLVMType(const std::string &type,
                               llvm::LLVMContext &ctx) {
  // Integer types (signed and unsigned share the same LLVM integer types;
  // signedness is handled by which IR instructions are used).
  if (type == "i8") {
    return llvm::Type::getInt8Ty(ctx);
  }
  if (type == "i16") {
    return llvm::Type::getInt16Ty(ctx);
  }
  if (type == "i32") {
    return llvm::Type::getInt32Ty(ctx);
  }
  if (type == "i64") {
    return llvm::Type::getInt64Ty(ctx);
  }
  if (type == "i128") {
    return llvm::Type::getInt128Ty(ctx);
  }

  if (type == "u8") {
    return llvm::Type::getInt8Ty(ctx);
  }
  if (type == "u16") {
    return llvm::Type::getInt16Ty(ctx);
  }
  if (type == "u32") {
    return llvm::Type::getInt32Ty(ctx);
  }
  if (type == "u64") {
    return llvm::Type::getInt64Ty(ctx);
  }
  if (type == "u128") {
    return llvm::Type::getInt128Ty(ctx);
  }

  if (type == "void") {
    return llvm::Type::getVoidTy(ctx);
  }
  return llvm::Type::getInt32Ty(ctx);
}

Codegen::Codegen(const std::string &moduleName)
    : module(std::make_unique<Module>(moduleName, context)),
      builder(std::make_unique<IRBuilder<>>(context)) {}

Codegen::~Codegen() { locals.clear(); }

void Codegen::generate(const std::vector<Statement *> &statements) {
  locals.clear();
  // For now, just stub: walk statements and call genStatement
  for (auto *stmt : statements) {
    genStatement(stmt);
  }
}

std::unique_ptr<Module> Codegen::takeModule() { return std::move(module); }

llvm::Value *Codegen::genExpr(Expr *expr) {
  // Stub: implement per Expr type
  if (auto *intLit = dynamic_cast<IntLiteral *>(expr)) {
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                  intLit->value);
  }
  if (auto *var = dynamic_cast<Variable *>(expr)) {
    auto it = locals.find(var->name);
    if (it != locals.end()) {
      return builder->CreateLoad(llvm::Type::getInt32Ty(context), it->second,
                                 var->name);
    }
    // Variable not found
    return nullptr;
  }
  // ...other exprs...
  return nullptr;
}

void Codegen::genVarDecl(VarDecl *varDecl) {
  llvm::Type *llvmTy = getLLVMType(varDecl->type, this->context);

  if (this->builder->GetInsertBlock() == nullptr) {
    // Global variable
    llvm::Constant *init = llvm::ConstantInt::get(llvmTy, 0);
    if (varDecl->initializer) {
      if (auto *constInit = llvm::dyn_cast<llvm::Constant>(
              this->genExpr(varDecl->initializer))) {
        init = constInit;
      }
    }

    new llvm::GlobalVariable(*this->module, llvmTy, false,
                             llvm::GlobalValue::ExternalLinkage, init,
                             varDecl->name);
  } else {
    // Local variable
    llvm::Function *func = this->builder->GetInsertBlock()->getParent();
    llvm::IRBuilder<> tmpBuilder(&func->getEntryBlock(),
                                 func->getEntryBlock().begin());

    auto *alloca = tmpBuilder.CreateAlloca(llvmTy, nullptr, varDecl->name);
    locals[varDecl->name] = alloca;

    // Initialize if initializer exists
    if (varDecl->initializer) {
      llvm::Value *initVal = this->genExpr(varDecl->initializer);
      if (initVal) {
        this->builder->CreateStore(initVal, alloca);
      }
    }
  }
}

void Codegen::genStatement(Statement *stmt) {
  if (auto *varDecl = dynamic_cast<VarDecl *>(stmt)) {
    this->genVarDecl(varDecl);
    return;
  }
  // ...other statements...
}
