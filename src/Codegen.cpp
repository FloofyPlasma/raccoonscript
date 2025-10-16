#include "Codegen.hpp"
#include "AST.hpp"

using namespace llvm;

llvm::Value *Codegen::castIntegerIfNeeded(llvm::IRBuilder<> *builder,
                                          llvm::Value *val, llvm::Type *fromTy,
                                          llvm::Type *toTy) {
  if (fromTy == toTy) {
    return val;
  }
  auto *fromInt = llvm::dyn_cast<llvm::IntegerType>(fromTy);
  auto *toInt = llvm::dyn_cast<llvm::IntegerType>(toTy);

  if (!fromInt || !toInt) {
    // Non-integer conversions are not handled here.
    return nullptr;
  }

  unsigned fromW = fromInt->getBitWidth();
  unsigned toW = toInt->getBitWidth();

  if (fromW == toW) {
    return val;
  }

  if (fromW < toW) {
    return builder->CreateSExt(val, toTy, "sexttmp");
  } else {
    return builder->CreateTrunc(val, toTy, "trunctmp");
  }
}

llvm::Value *
Codegen::findLValueStorage(llvm::Module *module,
                           std::unordered_map<std::string, LocalVar> &locals,
                           const std::string &name) {
  auto it = locals.find(name);
  if (it != locals.end()) {
    return it->second.alloca;
  }

  if (auto *g = module->getGlobalVariable(name)) {
    return g;
  }

  return nullptr;
}

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
  } else if (auto *var = dynamic_cast<Variable *>(expr)) {
    // Check local first
    auto it = locals.find(var->name);
    if (it != locals.end()) {
      return builder->CreateLoad(it->second.type, it->second.alloca, var->name);
    }

    // Check global
    if (auto *global = module->getGlobalVariable(var->name)) {
      return builder->CreateLoad(global->getValueType(), global, var->name);
    }

    return nullptr;
  } else if (auto *binExp = dynamic_cast<BinaryExpr *>(expr)) {
    return this->genBinaryExpr(binExp);
  } else if (auto *callExp = dynamic_cast<CallExpr *>(expr)) {
    return this->genCallExpr(callExp);
  }
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

    // Save current insertion point
    llvm::IRBuilder<>::InsertPoint oldIP = builder->saveIP();

    // Insert alloca at the start of the entry block
    llvm::BasicBlock *entry = &func->getEntryBlock();
    builder->SetInsertPoint(entry, entry->begin());
    llvm::AllocaInst *alloca =
        builder->CreateAlloca(llvmTy, nullptr, varDecl->name);

    // Restore insertion point
    builder->restoreIP(oldIP);

    // Initialize if initializer exists
    if (varDecl->initializer) {
      llvm::Value *initVal = genExpr(varDecl->initializer);
      if (!initVal) {
        fprintf(stderr, "Error: Local variable '%s' initializer is invalid.\n",
                varDecl->name.c_str());
        std::abort();
      }
      builder->CreateStore(initVal, alloca);
    }

    locals[varDecl->name] = {alloca, llvmTy};
  }
}

void Codegen::genStatement(Statement *stmt) {
  if (auto *varDecl = dynamic_cast<VarDecl *>(stmt)) {
    this->genVarDecl(varDecl);
    return;
  } else if (auto *funDecl = dynamic_cast<FunctionDecl *>(stmt)) {
    this->genFunction(funDecl);
    return;
  } else if (auto *retStmt = dynamic_cast<ReturnStmt *>(stmt)) {
    this->genReturnStatement(retStmt);
    return;
  } else if (auto *exprStmt = dynamic_cast<ExprStmt *>(stmt)) {
    this->genExpr(exprStmt->expr);
    return;
  } else if (auto *ifStmt = dynamic_cast<IfStmt *>(stmt)) {
    this->genIfStatement(ifStmt);
    return;
  } else if (auto *whileStmt = dynamic_cast<WhileStmt *>(stmt)) {
    this->genWhileStatement(whileStmt);
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

void Codegen::genReturnStatement(ReturnStmt *stmt) {
  if (!stmt->value) {
    // void return
    builder->CreateRetVoid();
    return;
  }
  llvm::Value *retVal = nullptr;
  retVal = this->genExpr(stmt->value);
  if (!retVal) {
    fprintf(stderr, "Error: Return value expression is invalid.\n");
    std::abort();
  }
  builder->CreateRet(retVal);
}

llvm::Value *Codegen::genBinaryExpr(BinaryExpr *expr) {
  // assignment: lhs must be variable
  if (expr->op == TokenType::Equal) {
    // ensure LHS is a variable ast node
    auto *lhsVar = dynamic_cast<Variable *>(expr->left);
    if (!lhsVar) {
      fprintf(stderr,
              "Erorr: Left-hand side of assignment must be a variable.\n");
      std::abort();
    }

    llvm::Value *rhsVal = this->genExpr(expr->right);
    if (!rhsVal) {
      fprintf(stderr, "Error: Invalid RHS in assignment.\n");
      std::abort();
    }

    // find storage
    llvm::Value *lval =
        this->findLValueStorage(this->module.get(), this->locals, lhsVar->name);
    if (!lval) {
      fprintf(stderr, "Error: Unknown variable '%s' in assignment.\n",
              lhsVar->name.c_str());
      std::abort();
    }

    llvm::Type *dstTy = nullptr;
    if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(lval)) {
      dstTy = alloca->getAllocatedType();
    } else if (auto *g = llvm::dyn_cast<llvm::GlobalVariable>(lval)) {
      dstTy = g->getValueType();
    } else {
      fprintf(stderr, "Error: Unsupported lvalue storage for '%s'.\n",
              lhsVar->name.c_str());
      std::abort();
    }

    // cast RHS to destination
    llvm::Type *srcTy = rhsVal->getType();
    llvm::Value *rhsCasted = rhsVal;
    if (srcTy != dstTy) {
      rhsCasted =
          this->castIntegerIfNeeded(this->builder.get(), rhsVal, srcTy, dstTy);
      if (!rhsCasted) {
        fprintf(stderr,
                "Error: Failed to convert assignment for RHS for '%s'.\n",
                lhsVar->name.c_str());
        std::abort();
      }
    }

    // store :3
    if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(lval)) {
      this->builder->CreateStore(rhsCasted, alloca);
    } else if (auto *g = llvm::dyn_cast<llvm::GlobalVariable>(lval)) {
      this->builder->CreateStore(rhsCasted, g);
    }

    return rhsCasted;
  }

  llvm::Value *lhs = this->genExpr(expr->left);
  llvm::Value *rhs = this->genExpr(expr->right);

  if (!lhs || !rhs) {
    fprintf(stderr, "Error: Invalid operands in binary expression.\n");
    std::abort();
  }

  // TODO: Break assumption that we are working with integer types.
  switch (expr->op) {
  case TokenType::Plus: {
    return this->builder->CreateAdd(lhs, rhs, "addtmp");
  }
  case TokenType::Minus: {
    return this->builder->CreateSub(lhs, rhs, "subtmp");
  }
  case TokenType::Star: {
    return this->builder->CreateMul(lhs, rhs, "multmp");
  }
  case TokenType::Slash: {
    return this->builder->CreateSDiv(lhs, rhs, "divtmp");
  }
  case TokenType::Percent: {
    return this->builder->CreateSRem(lhs, rhs, "modtmp");
  }
  case TokenType::DoubleEqual: {
    return this->builder->CreateICmpEQ(lhs, rhs, "eqtmp");
  }
  case TokenType::BangEqual: {
    return this->builder->CreateICmpNE(lhs, rhs, "netmp");
  }
  case TokenType::LessThan: {
    return this->builder->CreateICmpSLT(lhs, rhs, "lttmp");
  }
  case TokenType::LessEqual: {
    return this->builder->CreateICmpSLE(lhs, rhs, "letmp");
  }
  case TokenType::GreaterThan: {
    return this->builder->CreateICmpSGT(lhs, rhs, "gttmp");
  }
  case TokenType::GreaterEqual: {
    return this->builder->CreateICmpSGE(lhs, rhs, "getmp");
  }
  default: {
    fprintf(stderr, "Error: Unknown binary operator '%s'.\n", expr->op);
    std::abort();
  }
  }
}

void Codegen::genIfStatement(IfStmt *stmt) {
  llvm::Value *condVal = this->genExpr(stmt->condition);
  if (!condVal) {
    fprintf(stderr, "Error: Invalid if condition.\n");
    std::abort();
  }

  llvm::Function *func = this->builder->GetInsertBlock()->getParent();

  llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(context, "then", func);
  llvm::BasicBlock *elseBB =
      !stmt->elseBranch.empty()
          ? llvm::BasicBlock::Create(context, "else", func)
          : nullptr;
  llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "ifcont", func);

  if (elseBB) {
    this->builder->CreateCondBr(condVal, thenBB, elseBB);
  } else {
    this->builder->CreateCondBr(condVal, thenBB, mergeBB);
  }

  // then block
  this->builder->SetInsertPoint(thenBB);
  for (auto *s : stmt->thenBranch) {
    this->genStatement(s);
  }
  this->builder->CreateBr(mergeBB);

  // Else block
  if (elseBB) {
    this->builder->SetInsertPoint(elseBB);
    for (auto *s : stmt->elseBranch) {
      this->genStatement(s);
    }
    this->builder->CreateBr(mergeBB);
  }

  // continue after if
  this->builder->SetInsertPoint(mergeBB);
}

void Codegen::genWhileStatement(WhileStmt *stmt) {
  llvm::Function *func = this->builder->GetInsertBlock()->getParent();

  llvm::BasicBlock *condBB =
      llvm::BasicBlock::Create(context, "whilecond", func);
  llvm::BasicBlock *loopBB = llvm::BasicBlock::Create(context, "loop", func);
  llvm::BasicBlock *afterBB =
      llvm::BasicBlock::Create(context, "afterloop", func);

  this->builder->CreateBr(condBB);

  // Condition block
  this->builder->SetInsertPoint(condBB);
  llvm::Value *condVal = this->genExpr(stmt->condition);
  if (!condVal) {
    fprintf(stderr, "Error: Invalid while condition.\n");
    std::abort();
  }
  this->builder->CreateCondBr(condVal, loopBB, afterBB);

  // Loop body
  this->builder->SetInsertPoint(loopBB);
  for (auto *s : stmt->body) {
    this->genStatement(s);
  }
  this->builder->CreateBr(condBB);

  // continue after loop
  this->builder->SetInsertPoint(afterBB);
}

llvm::Value *Codegen::genCallExpr(CallExpr *expr) {
  llvm::Function *callee = this->module->getFunction(expr->name);
  if (!callee) {
    fprintf(stderr, "Error: Unknown function '%s',\n", expr->name.c_str());
    std::abort();
  }

  std::vector<llvm::Value *> args;
  for (auto *argExpr : expr->args) {
    llvm::Value *argVal = this->genExpr(argExpr);
    if (!argVal) {
      fprintf(stderr, "Error: Invalid argument in call to '%s'.\n",
              expr->name.c_str());
      std::abort();
    }
    args.push_back(argVal);
  }

  return this->builder->CreateCall(
      callee, args, callee->getReturnType()->isVoidTy() ? "" : "calltmp");
}
