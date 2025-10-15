#include "Codegen.hpp"
#include "AST.hpp"

using namespace llvm;

llvm::Type *Codegen::getLLVMType(const std::string &type, LLVMContext &ctx) {
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
  for (auto *stmt : statements) {
    genStatement(stmt);
  }
}

std::unique_ptr<Module> Codegen::takeModule() { return std::move(module); }

llvm::Value *Codegen::genExpr(Expr *expr) {
  if (auto *intLit = dynamic_cast<IntLiteral *>(expr)) {
    return llvm::ConstantInt::get(getLLVMType("i32", context), intLit->value);
  }
  if (auto *var = dynamic_cast<Variable *>(expr)) {
    auto it = locals.find(var->name);
    if (it != locals.end()) {
      return builder->CreateLoad(it->second.type, it->second.alloca, var->name);
    }
    return nullptr;
  }
  // Other expressions (BinaryOp, Call, etc.)
  return nullptr;
}

void Codegen::genVarDecl(VarDecl *varDecl) {
  llvm::Type *llvmTy = getLLVMType(varDecl->type, context);

  if (builder->GetInsertBlock() == nullptr) {
    // No active block → global variable
    llvm::Constant *init = llvm::ConstantInt::get(llvmTy, 0);

    if (varDecl->initializer) {
      llvm::Value *initVal = genExpr(varDecl->initializer);
      if (!initVal) {
        fprintf(stderr, "Error: Global variable '%s' initializer is invalid.\n",
                varDecl->name.c_str());
        std::abort();
      }
      if (auto *constInit = llvm::dyn_cast<llvm::Constant>(initVal)) {
        init = constInit;
      } else {
        fprintf(stderr,
                "Error: Global variable '%s' initializer must be constant.\n",
                varDecl->name.c_str());
        std::abort();
      }
    }

    new llvm::GlobalVariable(*module, llvmTy, false,
                             llvm::GlobalValue::ExternalLinkage, init,
                             varDecl->name);
  } else {
    // Local variable: ensure inside a function
    llvm::Function *func = builder->GetInsertBlock()->getParent();
    if (!func) {
      fprintf(stderr,
              "Error: Cannot create local variable '%s' outside a function.\n",
              varDecl->name.c_str());
      std::abort();
    }

    llvm::IRBuilder<> tmpBuilder(&func->getEntryBlock(),
                                 func->getEntryBlock().begin());
    llvm::AllocaInst *alloca =
        tmpBuilder.CreateAlloca(llvmTy, nullptr, varDecl->name);

    locals[varDecl->name] = {alloca, llvmTy};

    if (varDecl->initializer) {
      llvm::Value *initVal = genExpr(varDecl->initializer);
      if (!initVal) {
        fprintf(stderr, "Error: Local variable '%s' initializer is invalid.\n",
                varDecl->name.c_str());
        std::abort();
      }
      builder->CreateStore(initVal, alloca);
    }
  }
}

void Codegen::genStatement(Statement *stmt) {
  if (auto *varDecl = dynamic_cast<VarDecl *>(stmt)) {
    this->genVarDecl(varDecl);
    return;
  } else if (auto *funDecl = dynamic_cast<FunctionDecl *>(stmt)) {
    this->genFunction(funDecl);
    return;
  }
  // ...other statements...
}

llvm::Function *Codegen::genFunction(FunctionDecl *funcDecl) {
  // Determine return type
  llvm::Type *retTy = this->getLLVMType(funcDecl->returnType, this->context);

  std::vector<llvm::Type *> argTypes;
  for (auto &arg : funcDecl->params) {
    argTypes.push_back(this->getLLVMType(arg.second, this->context));
  }

  llvm::FunctionType *funcType =
      llvm::FunctionType::get(retTy, argTypes, false);
  llvm::Function *function =
      llvm::Function::Create(funcType, llvm::Function::ExternalLinkage,
                             funcDecl->name, this->module.get());

  // Create entry block
  llvm::BasicBlock *entry =
      llvm::BasicBlock::Create(this->context, "entry", function);
  builder->SetInsertPoint(entry);

  // Map function args to locals
  locals.clear();
  unsigned idx = 0;
  for (auto &arg : function->args()) {
    arg.setName(funcDecl->params[idx].first);
    llvm::IRBuilder<> tmpBuilder(&function->getEntryBlock(),
                                 function->getEntryBlock().begin());
    llvm::AllocaInst *alloca =
        tmpBuilder.CreateAlloca(arg.getType(), nullptr, arg.getName());
    builder->CreateStore(&arg, alloca);
    locals[arg.getName().str()] = {alloca, arg.getType()};
    idx++;
  }

  // Generate function body
  for (auto *stmt : funcDecl->body) {
    this->genStatement(stmt);
  }

  // If function returns void and no explitit return, insert return
  if (retTy->isVoidTy()) {
    builder->CreateRetVoid();
  }

  return function;
}
