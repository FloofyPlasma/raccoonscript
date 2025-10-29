#include "Codegen.hpp"
#include "AST.hpp"
#include "Token.hpp"

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
  if (type.back() == '*') {
    return llvm::PointerType::getUnqual(
        getLLVMType(type.substr(0, type.size() - 1), ctx));
  }

  auto it = this->structTypes.find(type);
  if (it != this->structTypes.end()) {
    return it->second;
  }

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

  if (type == "usize") {
    return llvm::Type::getInt64Ty(ctx);
    // TODO: Use data layout for actual pointer size
  }

  if (type == "f32") {
    return llvm::Type::getFloatTy(ctx);
  }
  if (type == "f64") {
    return llvm::Type::getDoubleTy(ctx);
  }

  if (type == "bool") {
    return llvm::Type::getInt8Ty(ctx);
  }

  if (type == "void") {
    return llvm::Type::getVoidTy(ctx);
  }

  if (type == "char") {
    return llvm::Type::getInt8Ty(ctx);
  }

  return llvm::Type::getInt32Ty(ctx);
}

Codegen::Codegen(const std::string &moduleName)
    : module(std::make_unique<Module>(moduleName, context)),
      builder(std::make_unique<IRBuilder<>>(context)),
      currentModuleName(moduleName) {
  this->pushScope();
  this->currentModuleExports.moduleName = moduleName;
}

Codegen::~Codegen() { this->scopeStack.clear(); }

void Codegen::generate(const std::vector<Statement *> &statements) {
  for (auto *stmt : statements) {
    this->genStatement(stmt);
  }
}

std::unique_ptr<Module> Codegen::takeModule() { return std::move(module); }

llvm::Value *Codegen::genExpr(Expr *expr) {
  if (auto *intLit = dynamic_cast<IntLiteral *>(expr)) {
    return llvm::ConstantInt::get(getLLVMType("i32", context), intLit->value);
  } else if (auto *floatLit = dynamic_cast<FloatLiteral *>(expr)) {
    return llvm::ConstantFP::get(this->getLLVMType("f32", this->context),
                                 floatLit->value);
  } else if (auto *boolLit = dynamic_cast<BoolLiteral *>(expr)) {
    return llvm::ConstantInt::get(this->getLLVMType("bool", this->context),
                                  boolLit->value);
  } else if (auto *var = dynamic_cast<Variable *>(expr)) {
    LocalVar *localVar = this->findVariable(var->name);

    if (localVar->alloca == nullptr) {
      if (auto *global = this->module->getGlobalVariable(var->name)) {
        return this->builder->CreateLoad(global->getValueType(), global,
                                         var->name);
      }
      fprintf(stderr, "Error: Global variable '%s' not found in module.\n",
              var->name.c_str());
      std::abort();
    }

    return this->builder->CreateLoad(localVar->alloca->getAllocatedType(),
                                     localVar->alloca, var->name);
  } else if (auto *binExp = dynamic_cast<BinaryExpr *>(expr)) {
    return this->genBinaryExpr(binExp);
  } else if (auto *callExp = dynamic_cast<CallExpr *>(expr)) {
    return this->genCallExpr(callExp);
  } else if (auto *charLiteral = dynamic_cast<CharLiteral *>(expr)) {
    return this->genCharLiteral(charLiteral->value);
  } else if (auto *stringLiteral = dynamic_cast<StrLiteral *>(expr)) {
    return this->genStringLiteral(stringLiteral->value);
  } else if (auto *unary = dynamic_cast<UnaryExpr *>(expr)) {
    return this->genUnaryExpr(unary);
  } else if (auto *structLit = dynamic_cast<StructLiteral *>(expr)) {
    return this->genStructLiteral(structLit);
  } else if (auto *memberAccess = dynamic_cast<MemberAccessExpr *>(expr)) {
    return this->genMemberAccessExpr(memberAccess);
  }
  return nullptr;
}

void Codegen::genVarDecl(VarDecl *varDecl) {
  llvm::Type *llvmTy = getLLVMType(varDecl->type, context);

  if (builder->GetInsertBlock() == nullptr) {
    // No active block → global variable
    llvm::Constant *init = llvm::ConstantInt::get(llvmTy, 0);

    if (varDecl->initializer) {
      llvm::Value *initVal = this->genExpr(varDecl->initializer);
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

    llvm::GlobalVariable *globalVar = new llvm::GlobalVariable(
        *module, llvmTy, false, llvm::GlobalValue::ExternalLinkage, init,
        varDecl->name);

    if (this->scopeStack.empty()) {
      fprintf(stderr, "Error: No global scope available.\nThis error should "
                      "never happen.\nSomething went terribly wrong.\n");
      std::abort();
    }

    // Global varaibles don't have an alloca pointer, findVariable will handle
    // this case.
    this->scopeStack[0][varDecl->name] = {nullptr, llvmTy, varDecl->type,
                                          varDecl->isConst};
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

    this->addVariable(varDecl->name,
                      {alloca, llvmTy, varDecl->type, varDecl->isConst});

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
  }
}

void Codegen::genStatement(Statement *stmt) {
  if (auto *varDecl = dynamic_cast<VarDecl *>(stmt)) {
    this->genVarDecl(varDecl);
    return;
  } else if (auto *funDecl = dynamic_cast<FunctionDecl *>(stmt)) {
    this->genFunction(funDecl);
    return;
  } else if (auto *structDecl = dynamic_cast<StructDecl *>(stmt)) {
    this->genStructDecl(structDecl);
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
  } else if (auto *forStmt = dynamic_cast<ForStmt *>(stmt)) {
    this->genForStatement(forStmt);
    return;
  } else if (auto *blockStmt = dynamic_cast<BlockStmt *>(stmt)) {
    this->genBlockStatement(blockStmt);
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

  // Name mangling (module_functionName)
  std::string functionName = funcDecl->name;
  if (funcDecl->isExported && !this->currentModuleName.empty()) {
    functionName = this->currentModuleName + "_" + funcDecl->name;

    ExportedFunction exportedFunc;
    exportedFunc.name = funcDecl->name;
    exportedFunc.params = funcDecl->params;
    exportedFunc.returnType = funcDecl->returnType;
    this->currentModuleExports.functions.push_back(exportedFunc);
  }

  llvm::Function *function =
      llvm::Function::Create(funcType, llvm::Function::ExternalLinkage,
                             functionName, this->module.get());

  // Create entry block
  llvm::BasicBlock *entry =
      llvm::BasicBlock::Create(this->context, "entry", function);
  builder->SetInsertPoint(entry);

  // Map function args to locals
  this->pushScope();

  unsigned idx = 0;
  for (auto &arg : function->args()) {
    arg.setName(funcDecl->params[idx].first);
    llvm::IRBuilder<> tmpBuilder(&function->getEntryBlock(),
                                 function->getEntryBlock().begin());
    llvm::AllocaInst *alloca =
        tmpBuilder.CreateAlloca(arg.getType(), nullptr, arg.getName());
    builder->CreateStore(&arg, alloca);
    this->addVariable(arg.getName().str(),
                      {alloca, arg.getType(), funcDecl->params[idx].second});
    idx++;
  }

  // Generate function body
  for (auto *stmt : funcDecl->body) {
    this->genStatement(stmt);
  }

  this->popScope();

  // If function returns void and no explicit return, insert return
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
  if (expr->op == TokenType::Equal) {
    // left must be lvalue
    llvm::Value *lhsPtr = genExprLValue(expr->left);
    llvm::Value *rhsVal = genExpr(expr->right);
    return builder->CreateStore(rhsVal, lhsPtr);
  }

  llvm::Value *lhs = this->genExpr(expr->left);
  llvm::Value *rhs = this->genExpr(expr->right);

  if (!lhs || !rhs) {
    fprintf(stderr, "Error: Invalid operands in binary expression.\n");
    std::abort();
  }

  llvm::Type *lhsType = lhs->getType();
  llvm::Type *rhsType = rhs->getType();

  if (lhsType != rhsType) {
    // If one is float and other is int, convert int to float
    if (lhsType->isFloatingPointTy() && rhsType->isIntegerTy()) {
      rhs = this->builder->CreateSIToFP(rhs, lhsType, "inttofp");
      rhsType = lhsType;
    } else if (lhsType->isIntegerTy() && rhsType->isFloatingPointTy()) {
      lhs = this->builder->CreateSIToFP(lhs, rhsType, "inttofp");
      lhsType = rhsType;
    }
    // If both are integers but different widths, extend to wider
    else if (lhsType->isIntegerTy() && rhsType->isIntegerTy()) {
      auto *lhsInt = llvm::cast<llvm::IntegerType>(lhsType);
      auto *rhsInt = llvm::cast<llvm::IntegerType>(rhsType);
      if (lhsInt->getBitWidth() < rhsInt->getBitWidth()) {
        lhs = this->builder->CreateSExt(lhs, rhsType, "sext");
        lhsType = rhsType;
      } else if (lhsInt->getBitWidth() > rhsInt->getBitWidth()) {
        rhs = this->builder->CreateSExt(rhs, lhsType, "sext");
        rhsType = lhsType;
      }
    }
  }

  bool isFloat = lhs->getType()->isFloatingPointTy();

  std::string lhsTypeStr = this->getExprTypeStr(expr->left);
  bool isUnsigned = this->isUnsignedType(lhsTypeStr);

  switch (expr->op) {
  case TokenType::Plus: {
    if (isFloat) {
      return this->builder->CreateFAdd(lhs, rhs, "faddtmp");
    }
    return this->builder->CreateAdd(lhs, rhs, "addtmp");
  }
  case TokenType::Minus: {
    if (isFloat) {
      return this->builder->CreateFSub(lhs, rhs, "fsubtmp");
    }
    return this->builder->CreateSub(lhs, rhs, "subtmp");
  }
  case TokenType::Star: {
    if (isFloat) {
      return this->builder->CreateFMul(lhs, rhs, "fmultmp");
    }
    return this->builder->CreateMul(lhs, rhs, "multmp");
  }
  case TokenType::Slash: {
    if (isFloat) {
      return this->builder->CreateFDiv(lhs, rhs, "fdivtmp");
    }
    if (isUnsigned) {
      return builder->CreateUDiv(lhs, rhs, "udivtmp");
    }
    return builder->CreateSDiv(lhs, rhs, "divtmp");
  }
  case TokenType::Percent: {
    if (isFloat) {
      return this->builder->CreateFRem(lhs, rhs, "fremtmp");
    }
    if (isUnsigned) {
      return builder->CreateURem(lhs, rhs, "uremtmp");
    }
    return builder->CreateSRem(lhs, rhs, "modtmp");
  }
  case TokenType::DoubleEqual: {
    llvm::Value *cmp;
    if (isFloat) {
      cmp = this->builder->CreateFCmpOEQ(lhs, rhs, "feqtmp");
    } else {
      cmp = this->builder->CreateICmpEQ(lhs, rhs, "eqtmp");
    }
    return this->builder->CreateZExt(cmp, llvm::Type::getInt8Ty(context),
                                     "eqresult");
  }
  case TokenType::BangEqual: {
    llvm::Value *cmp;
    if (isFloat) {
      cmp = this->builder->CreateFCmpONE(lhs, rhs, "fnetmp");
    } else {
      cmp = this->builder->CreateICmpNE(lhs, rhs, "netmp");
    }
    return this->builder->CreateZExt(cmp, llvm::Type::getInt8Ty(context),
                                     "neresult");
  }
  case TokenType::LessThan: {
    llvm::Value *cmp;
    if (isFloat) {
      cmp = this->builder->CreateFCmpOLT(lhs, rhs, "flttmp");
    } else if (isUnsigned) {
      cmp = this->builder->CreateICmpULT(lhs, rhs, "ulttmp");
    } else {
      cmp = this->builder->CreateICmpSLT(lhs, rhs, "lttmp");
    }
    return this->builder->CreateZExt(cmp, llvm::Type::getInt8Ty(context),
                                     "ltresult");
  }
  case TokenType::LessEqual: {
    llvm::Value *cmp;
    if (isFloat) {
      cmp = this->builder->CreateFCmpOLE(lhs, rhs, "fletmp");
    } else if (isUnsigned) {
      cmp = this->builder->CreateICmpULE(lhs, rhs, "uletmp");
    } else {
      cmp = this->builder->CreateICmpSLE(lhs, rhs, "letmp");
    }
    return this->builder->CreateZExt(cmp, llvm::Type::getInt8Ty(context),
                                     "leresult");
  }
  case TokenType::GreaterThan: {
    llvm::Value *cmp;
    if (isFloat) {
      cmp = this->builder->CreateFCmpOGT(lhs, rhs, "fgttmp");
    } else if (isUnsigned) {
      cmp = this->builder->CreateICmpUGT(lhs, rhs, "ugttmp");
    } else {
      cmp = this->builder->CreateICmpSGT(lhs, rhs, "gttmp");
    }
    return this->builder->CreateZExt(cmp, llvm::Type::getInt8Ty(context),
                                     "gtresult");
  }
  case TokenType::GreaterEqual: {
    llvm::Value *cmp;
    if (isFloat) {
      cmp = this->builder->CreateFCmpOGE(lhs, rhs, "fgetmp");
    } else if (isUnsigned) {
      cmp = this->builder->CreateICmpUGE(lhs, rhs, "ugetmp");
    } else {
      cmp = this->builder->CreateICmpSGE(lhs, rhs, "getmp");
    }
    return this->builder->CreateZExt(cmp, llvm::Type::getInt8Ty(context),
                                     "geresult");
  }
  case TokenType::AndAnd: {
    llvm::Value *lhsBool = this->builder->CreateICmpNE(
        lhs, llvm::ConstantInt::get(lhs->getType(), 0), "lhsbool");
    llvm::Value *rhsBool = this->builder->CreateICmpNE(
        rhs, llvm::ConstantInt::get(rhs->getType(), 0), "rhsbool");
    llvm::Value *result = this->builder->CreateAnd(lhsBool, rhsBool, "andtmp");
    return this->builder->CreateZExt(result, llvm::Type::getInt8Ty(context),
                                     "andresult");
  }
  case TokenType::OrOr: {
    llvm::Value *lhsBool = this->builder->CreateICmpNE(
        lhs, llvm::ConstantInt::get(lhs->getType(), 0), "lhsbool");
    llvm::Value *rhsBool = this->builder->CreateICmpNE(
        rhs, llvm::ConstantInt::get(rhs->getType(), 0), "rhsbool");
    llvm::Value *result = this->builder->CreateOr(lhsBool, rhsBool, "ortmp");
    return this->builder->CreateZExt(result, llvm::Type::getInt8Ty(context),
                                     "orresult");
  }
  default: {
    fprintf(stderr, "Error: Unknown binary operator.\n");
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

  if (condVal->getType()->isIntegerTy(8)) {
    condVal = this->builder->CreateICmpNE(
        condVal, llvm::ConstantInt::get(condVal->getType(), 0), "tobool");
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

  // THEN block
  this->builder->SetInsertPoint(thenBB);
  for (auto *s : stmt->thenBranch) {
    this->genStatement(s);
  }

  // only add branch if block isn't already terminated
  if (!this->builder->GetInsertBlock()->getTerminator()) {
    this->builder->CreateBr(mergeBB);
  }

  // ELSE block
  if (elseBB) {
    this->builder->SetInsertPoint(elseBB);
    for (auto *s : stmt->elseBranch) {
      this->genStatement(s);
    }

    if (!this->builder->GetInsertBlock()->getTerminator()) {
      this->builder->CreateBr(mergeBB);
    }
  }

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

  if (condVal->getType()->isIntegerTy() &&
      !condVal->getType()->isIntegerTy(1)) {
    condVal = this->builder->CreateICmpNE(
        condVal, llvm::ConstantInt::get(condVal->getType(), 0), "tobool");
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
  if (expr->name == "malloc") {
    printf("Generating malloc for type %s\n", expr->type.c_str());

    if (expr->args.size() != 1) {
      fprintf(stderr,
              "Error: malloc<T>(count) requires exactly one argument.\n");
      std::abort();
    }

    if (expr->type.empty()) {
      fprintf(stderr, "Error: malloc requires a type parameter.\n");
      std::abort();
    }

    llvm::Type *elemTy = this->getLLVMType(expr->type, this->context);
    llvm::Value *countVal = this->genExpr(expr->args[0]);

    llvm::Value *elemSize = builder->CreateIntCast(
        llvm::ConstantInt::get(
            builder->getInt64Ty(),
            this->module->getDataLayout().getTypeAllocSize(elemTy)),
        countVal->getType(), false);

    llvm::Value *totalSize =
        builder->CreateMul(countVal, elemSize, "totalsize");

    // Cast totalSize to i64 for malloc
    totalSize = builder->CreateIntCast(totalSize, builder->getInt64Ty(), false);

    // Get or declare malloc
    llvm::Function *mallocFunc = module->getFunction("malloc");
    if (!mallocFunc) {
      llvm::FunctionType *mallocTy = llvm::FunctionType::get(
          builder->getPtrTy(), {builder->getInt64Ty()}, false);
      mallocFunc = llvm::Function::Create(
          mallocTy, llvm::Function::ExternalLinkage, "malloc", module.get());
    }

    // Call malloc
    llvm::Value *rawPtr =
        builder->CreateCall(mallocFunc, {totalSize}, "mallocCall");

    // Bitcast to proper type
    return builder->CreateBitCast(rawPtr, llvm::PointerType::get(elemTy, 0),
                                  "mallocCast");
  }

  if (expr->name == "free") {
    if (expr->args.size() != 1) {
      fprintf(stderr, "Error: free requires exactly one argument.\n");
      std::abort();
    }

    llvm::Value *ptrVal = this->genExpr(expr->args[0]);

    // Get or declare free
    llvm::Function *freeFunc = module->getFunction("free");
    if (!freeFunc) {
      llvm::FunctionType *ftype = llvm::FunctionType::get(
          builder->getVoidTy(), {builder->getPtrTy()}, false);
      freeFunc = llvm::Function::Create(ftype, llvm::Function::ExternalLinkage,
                                        "free", module.get());
    }

    // Bitcast pointer to i8*
    llvm::Value *casted =
        builder->CreateBitCast(ptrVal, builder->getPtrTy(), "freeCast");
    return builder->CreateCall(freeFunc, {casted});
  }

  std::string functionName = expr->name;

  if (!expr->moduleName.empty()) {
    // Qualified call: module.function
    auto it = this->importedModules.find(expr->moduleName);
    if (it == this->importedModules.end()) {
      fprintf(stderr, "Error: Module '%s' not imported.\n",
              expr->moduleName.c_str());
      std::abort();
    }

    const ExportedFunction *exportedFunc = it->second.findFunction(expr->name);
    if (!exportedFunc) {
      fprintf(stderr, "Error: Function '%s' not found in module '%s'.\n",
              expr->name.c_str(), expr->moduleName.c_str());
      std::abort();
    }

    functionName = expr->moduleName + "_" + expr->name;

    llvm::Function *callee = this->module->getFunction(functionName);
    if (!callee) {
      std::vector<llvm::Type *> paramTypes;
      for (const auto &param : exportedFunc->params) {
        paramTypes.push_back(this->getLLVMType(param.second, this->context));
      }

      llvm::Type *returnType =
          this->getLLVMType(exportedFunc->returnType, this->context);
      llvm::FunctionType *funcType =
          llvm::FunctionType::get(returnType, paramTypes, false);

      callee = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage,
                                      functionName, this->module.get());
    }

    std::vector<llvm::Value *> args;
    for (auto *argExpr : expr->args) {
      llvm::Value *argVal = this->genExpr(argExpr);
      if (!argVal) {
        fprintf(stderr, "Error: Invalid argument in call to '%s.%s'.\n",
                expr->moduleName.c_str(), expr->name.c_str());
        std::abort();
      }
      args.push_back(argVal);
    }

    return this->builder->CreateCall(
        callee, args, callee->getReturnType()->isVoidTy() ? "" : "calltmp");
  }

  // Unqualified call
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

llvm::Value *Codegen::genStringLiteral(const std::string &str) {
  llvm::Constant *strConst =
      llvm::ConstantDataArray::getString(this->context, str);

  llvm::GlobalVariable *gvar = new llvm::GlobalVariable(
      *this->module, strConst->getType(), true,
      llvm::GlobalVariable::ExternalLinkage, strConst, ".str");

  llvm::Value *zero =
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(this->context), 0);
  llvm::Value *indices[] = {zero, zero};

  return this->builder->CreateInBoundsGEP(strConst->getType(), gvar, indices,
                                          "srtptr");
};

llvm::Value *Codegen::genCharLiteral(char c) {
  return llvm::ConstantInt::get(llvm::Type::getInt8Ty(this->context), c);
}

llvm::Value *Codegen::genUnaryExpr(UnaryExpr *expr) {
  switch (expr->op) {
  case TokenType::Ampersand: { // &var
    if (auto *var = dynamic_cast<Variable *>(expr->operand)) {
      LocalVar *localVar = this->findVariable(var->name);

      if (localVar->alloca == nullptr) {
        if (auto *global = this->module->getGlobalVariable(var->name)) {
          return global;
        }
        fprintf(stderr, "Error: Global variable '%s' not found.\n",
                var->name.c_str());
        std::abort();
      }

      return localVar->alloca;
    }
  case TokenType::Star: { // *ptr
    if (auto *var = dynamic_cast<Variable *>(expr->operand)) {
      LocalVar *localVar = this->findVariable(var->name);

      std::string pointedToTypeStr = this->getPointedToType(localVar->typeStr);
      if (pointedToTypeStr.empty()) {
        fprintf(stderr,
                "Error: Attempt to dereference non-pointer variable '%s'.\n",
                var->name.c_str());
        std::abort();
      }

      llvm::Type *pointedToType =
          this->getLLVMType(pointedToTypeStr, this->context);
      llvm::Value *ptrVal = this->genExpr(expr->operand);

      return this->builder->CreateLoad(pointedToType, ptrVal, "deref");
    } else {
      fprintf(stderr,
              "Error: Dereference of complex expressions not yet supported.\n");
      std::abort();
    }
  }
  case TokenType::Bang: { // !bool
    llvm::Value *operand = this->genExpr(expr->operand);
    if (!operand) {
      fprintf(stderr, "Error: Invalid operand for logical NOT.\n");
      std::abort();
    }

    llvm::Value *boolVal = this->builder->CreateICmpNE(
        operand, llvm::ConstantInt::get(operand->getType(), 0), "tobool");
    llvm::Value *notVal = this->builder->CreateNot(boolVal, "nottmp");
    return this->builder->CreateZExt(notVal, llvm::Type::getInt8Ty(context),
                                     "notresult");
  }
  case TokenType::Minus: {
    llvm::Value *operand = this->genExpr(expr->operand);
    if (!operand) {
      fprintf(stderr, "Error: Invalid operand for negation.\n");
      std::abort();
    }
    if (operand->getType()->isFloatingPointTy()) {
      return this->builder->CreateFNeg(operand, "fnegtmp");
    }
    return this->builder->CreateNeg(operand, "negtmp");
  }
  default: {
    llvm::Value *operand = this->genExpr(expr->operand);
    if (!operand) {
      fprintf(stderr, "Error: Invalid operand in unary expression.\n");
      std::abort();
    }
    fprintf(stderr, "Error: Unknown unary operator.\n");
    std::abort();
  }
  }
  }
}

llvm::Value *Codegen::genLValue(Expr *expr) {
  if (auto *var = dynamic_cast<Variable *>(expr)) {
    LocalVar *localVar = this->findVariable(var->name);

    if (localVar->alloca == nullptr) {
      if (auto *global = this->module->getGlobalVariable(var->name)) {
        return global;
      }
      fprintf(stderr, "Error: Global variable '%s' not found.\n",
              var->name.c_str());
      std::abort();
    }

    return localVar->alloca;
  } else if (auto *unary = dynamic_cast<UnaryExpr *>(expr)) {
    if (unary->op == TokenType::Star) {
      llvm::Value *ptr = this->genExpr(unary->operand);
      if (!ptr) {
        fprintf(stderr, "Error: Cannot dereference null pointer.\n");
        std::abort();
      }
      return ptr; // pointer
    }
  }

  else if (auto *memberAccess = dynamic_cast<MemberAccessExpr *>(expr)) {
    llvm::Value *structPtr = nullptr;
    llvm::StructType *structType = nullptr;
    std::string structTypeName;

    // (p.x)
    if (auto *var = dynamic_cast<Variable *>(memberAccess->object)) {
      LocalVar *localVar = this->findVariable(var->name);
      structTypeName = localVar->typeStr;

      if (!structTypeName.empty() && structTypeName.back() == '*') {
        // Pointer to struct
        structTypeName = structTypeName.substr(0, structTypeName.size() - 1);
        structPtr =
            this->builder->CreateLoad(localVar->alloca->getAllocatedType(),
                                      localVar->alloca, var->name + "_load");
      } else {
        // Direct struct
        structPtr = localVar->alloca;
      }

      auto it = this->structTypes.find(structTypeName);
      if (it == this->structTypes.end()) {
        fprintf(stderr, "Error: Unknown struct type '%s' in member access.\n",
                structTypeName.c_str());
        std::abort();
      }
      structType = it->second;

      int fieldIndex = this->getFieldIndex(structTypeName, memberAccess->field);
      return this->builder->CreateStructGEP(structType, structPtr, fieldIndex,
                                            memberAccess->field + "_ptr");
    }
    // ((*ptr).x)
    else if (auto *unary = dynamic_cast<UnaryExpr *>(memberAccess->object)) {
      if (unary->op == TokenType::Star) {
        if (auto *ptrVar = dynamic_cast<Variable *>(unary->operand)) {
          LocalVar *localVar = this->findVariable(ptrVar->name);

          structTypeName = this->getPointedToType(localVar->typeStr);
          if (structTypeName.empty()) {
            fprintf(
                stderr,
                "Error: Cannot dereference non-pointer in member access.\n");
            std::abort();
          }

          auto it = this->structTypes.find(structTypeName);
          if (it == this->structTypes.end()) {
            fprintf(stderr,
                    "Error: Unknown struct type '%s' in member access.\n",
                    structTypeName.c_str());
            std::abort();
          }
          structType = it->second;

          structPtr = this->builder->CreateLoad(
              localVar->alloca->getAllocatedType(), localVar->alloca,
              ptrVar->name + "_load");

          int fieldIndex =
              this->getFieldIndex(structTypeName, memberAccess->field);
          return this->builder->CreateStructGEP(
              structType, structPtr, fieldIndex, memberAccess->field + "_ptr");
        } else {
          fprintf(stderr, "Error: Complex dereference in member access lvalue "
                          "not yet supported.\n");
          std::abort();
        }
      } else {
        fprintf(
            stderr,
            "Error: Unsupported unary operation in member access lvalue.\n");
        std::abort();
      }
    }

    fprintf(stderr,
            "Error: Complex member access as lvalue not yet supported.\n");
    std::abort();
  }

  fprintf(stderr, "Error: Expression is not an lvalue.\n");
  std::abort();
}

llvm::Value *Codegen::genExprLValue(Expr *expr) {
  if (auto *var = dynamic_cast<Variable *>(expr)) {
    LocalVar *localVar = this->findVariable(var->name);

    if (localVar->isConst) {
      fprintf(stderr, "Error: Cannot assign to constant variable '%s'.\n",
              var->name.c_str());
      std::abort();
    }

    if (localVar->alloca == nullptr) {
      if (auto *global = this->module->getGlobalVariable(var->name)) {
        fprintf(stderr, "Error: Global variable '%s' not found.\n",
                var->name.c_str());
        std::abort();
      }
    }

    return localVar->alloca;
  } else if (auto *un = dynamic_cast<UnaryExpr *>(expr)) {
    if (un->op == TokenType::Star) {
      return this->genExpr(un->operand);
    }
  } else if (auto *memberAccess = dynamic_cast<MemberAccessExpr *>(expr)) {
    return this->genLValue(memberAccess);
  }
  fprintf(stderr, "Error: Expression cannot be used as lvalue.\n");
  std::abort();
}

void Codegen::genBlockStatement(BlockStmt *stmt) {
  this->pushScope();
  for (auto *s : stmt->statements) {
    this->genStatement(s);
  }
  this->popScope();
}

void Codegen::genForStatement(ForStmt *stmt) {
  this->pushScope();

  if (stmt->initializer) {
    this->genStatement(stmt->initializer);
  }

  llvm::Function *func = this->builder->GetInsertBlock()->getParent();

  llvm::BasicBlock *condBB =
      llvm::BasicBlock::Create(this->context, "forcond", func);
  llvm::BasicBlock *bodyBB =
      llvm::BasicBlock::Create(this->context, "forbody", func);
  llvm::BasicBlock *incBB =
      llvm::BasicBlock::Create(this->context, "forinc", func);
  llvm::BasicBlock *afterBB =
      llvm::BasicBlock::Create(this->context, "afterfor", func);

  this->builder->CreateBr(condBB);

  this->builder->SetInsertPoint(condBB);
  if (stmt->condition) {
    llvm::Value *condVal = this->genExpr(stmt->condition);
    if (!condVal) {
      fprintf(stderr, "Error: Invalid for loop condition.\n");
      std::abort();
    }

    if (condVal->getType()->isIntegerTy(8)) {
      condVal = this->builder->CreateICmpNE(
          condVal, llvm::ConstantInt::get(condVal->getType(), 0), "tobool");
    } else if (!condVal->getType()->isIntegerTy(1)) {
      // If it's not i1 and not i8, convert any integer to i1
      condVal = this->builder->CreateICmpNE(
          condVal, llvm::ConstantInt::get(condVal->getType(), 0), "tobool");
    }

    this->builder->CreateCondBr(condVal, bodyBB, afterBB);
  } else {
    this->builder->CreateBr(bodyBB);
  }

  this->builder->SetInsertPoint(bodyBB);
  for (auto *s : stmt->body) {
    this->genStatement(s);
  }
  this->builder->CreateBr(incBB);

  this->builder->SetInsertPoint(incBB);
  if (stmt->increment) {
    this->genExpr(stmt->increment);
  }
  this->builder->CreateBr(condBB);

  this->builder->SetInsertPoint(afterBB);

  this->popScope();
}

// MARK: Scope mgmt

void Codegen::pushScope() {
  this->scopeStack.push_back(std::unordered_map<std::string, LocalVar>());
}

void Codegen::popScope() {
  if (this->scopeStack.empty()) {
    fprintf(stderr, "Error: Attempted to pop from an empty scope stack.\n");
    std::abort();
  }
  this->scopeStack.pop_back();
}

LocalVar *Codegen::findVariable(const std::string &name) {
  // note to self: top of the stack is the innermost scope
  // i sometimes forget this because im big dumb
  for (auto it = this->scopeStack.rbegin(); it != this->scopeStack.rend();
       ++it) {
    auto varIt = it->find(name);
    if (varIt != it->end()) {
      return &varIt->second;
    }
  }

  fprintf(stderr, "Error: Undefined variable '%s'.\n", name.c_str());
  std::abort();
}

void Codegen::addVariable(const std::string &name, const LocalVar &var) {
  if (this->scopeStack.empty()) {
    fprintf(stderr, "Error: No active scope to add the variable '%s'.\n",
            name.c_str());
    std::abort();
  }

  this->scopeStack.back()[name] = var;
}

// MARK: Structs

int Codegen::getFieldIndex(const std::string &structName,
                           const std::string &fieldName) {
  auto it = this->structFieldMetadata.find(structName);
  if (it == this->structFieldMetadata.end()) {
    fprintf(stderr, "Error: Unknown struct type '%s'.\n", structName.c_str());
    std::abort();
  }

  const auto &fields = it->second;
  for (size_t i = 0; i < fields.size(); ++i) {
    if (fields[i].first == fieldName) {
      return static_cast<int>(i);
    }
  }

  fprintf(stderr, "Error: Struct '%s' has no field named '%s'.\n",
          structName.c_str(), fieldName.c_str());
  std::abort();
}

void Codegen::genStructDecl(StructDecl *structDecl) {
  if (this->structTypes.find(structDecl->name) != this->structTypes.end()) {
    fprintf(stderr, "Error: Struct '%s' is already defined.\n",
            structDecl->name.c_str());
    std::abort();
  }

  std::vector<llvm::Type *> fieldTypes;
  for (const auto &field : structDecl->fields) {
    llvm::Type *fieldType = this->getLLVMType(field.second, this->context);
    if (!fieldType) {
      fprintf(
          stderr, "Error: Invalid type '%s' for field '%s' in struct '%s'.\n",
          field.second.c_str(), field.first.c_str(), structDecl->name.c_str());
      std::abort();
    }
    fieldTypes.push_back(fieldType);
  }

  llvm::StructType *structType =
      llvm::StructType::create(this->context, fieldTypes, structDecl->name);

  this->structTypes[structDecl->name] = structType;

  this->structFieldMetadata[structDecl->name] = structDecl->fields;

  if (structDecl->isExported) {
    ExportedStruct exportedStruct;
    exportedStruct.name = structDecl->name;
    exportedStruct.fields = structDecl->fields;
    this->currentModuleExports.structs.push_back(exportedStruct);
  }
}

llvm::Value *Codegen::genStructLiteral(StructLiteral *expr) {
  std::string structName = expr->typeName;
  if (!expr->moduleName.empty()) {
    auto it = this->importedModules.find(expr->moduleName);
    if (it == this->importedModules.end()) {
      fprintf(stderr, "Error: Module '%s' not imported.\n",
              expr->moduleName.c_str());
      std::abort();
    }

    const ExportedStruct *exportedStruct =
        it->second.findStruct(expr->typeName);
    if (!exportedStruct) {
      fprintf(stderr, "Error: Struct '%s' not found in module '%s'.\n",
              expr->typeName.c_str(), expr->moduleName.c_str());
      std::abort();
    }

    // Use the struct name directly (already loaded in loadImport)
    structName = expr->typeName;
  }

  auto it = this->structTypes.find(structName);
  if (it == this->structTypes.end()) {
    fprintf(stderr, "Error: Unknown struct type '%s'.\n", structName.c_str());
    std::abort();
  }

  llvm::StructType *structType = it->second;

  llvm::Function *func = this->builder->GetInsertBlock()->getParent();
  if (!func) {
    fprintf(stderr,
            "Error: Cannot create struct literal outside a function.\n");
    std::abort();
  }

  llvm::IRBuilder<>::InsertPoint oldIP = this->builder->saveIP();

  llvm::BasicBlock *entry = &func->getEntryBlock();
  this->builder->SetInsertPoint(entry, entry->begin());
  llvm::AllocaInst *alloca =
      this->builder->CreateAlloca(structType, nullptr, "structlit");

  this->builder->restoreIP(oldIP);

  auto metaIt = this->structFieldMetadata.find(structName);
  if (metaIt == this->structFieldMetadata.end()) {
    fprintf(stderr, "Error: No field metadata for struct '%s'.\n",
            structName.c_str());
    std::abort();
  }

  const auto &structFields = metaIt->second;

  if (expr->fields.size() != structFields.size()) {
    fprintf(stderr,
            "Error: Struct '%s' requires %zu fields, but %zu were provided.\n",
            structName.c_str(), structFields.size(), expr->fields.size());
    std::abort();
  }

  for (const auto &fieldInit : expr->fields) {
    const std::string &fieldName = fieldInit.first;
    Expr *fieldValue = fieldInit.second;

    int fieldIndex = this->getFieldIndex(structName, fieldName);

    llvm::Value *value = this->genExpr(fieldValue);
    if (!value) {
      fprintf(stderr, "Error: Invalid initializer for field '%s'.\n",
              fieldName.c_str());
      std::abort();
    }

    llvm::Value *fieldPtr = this->builder->CreateStructGEP(
        structType, alloca, fieldIndex, fieldName + "_ptr");

    this->builder->CreateStore(value, fieldPtr);
  }

  return this->builder->CreateLoad(structType, alloca, "structval");
}

llvm::Value *Codegen::genMemberAccessExpr(MemberAccessExpr *expr) {
  llvm::Value *structPtr = nullptr;
  llvm::StructType *structType = nullptr;
  std::string structTypeName;

  if (auto *var = dynamic_cast<Variable *>(expr->object)) {
    LocalVar *localVar = this->findVariable(var->name);

    structTypeName = localVar->typeStr;

    if (!structTypeName.empty() && structTypeName.back() == '*') {
      // It's a pointer to struct, load it
      structTypeName = structTypeName.substr(0, structTypeName.size() - 1);
      structPtr =
          this->builder->CreateLoad(localVar->alloca->getAllocatedType(),
                                    localVar->alloca, var->name + "_load");
    } else {
      // It's a direct struct value, get its address
      structPtr = localVar->alloca;
    }

    auto it = this->structTypes.find(structTypeName);
    if (it == this->structTypes.end()) {
      fprintf(stderr, "Error: Unknown struct type '%s' in member access.\n",
              structTypeName.c_str());
      std::abort();
    }
    structType = it->second;

  } else if (auto *unary = dynamic_cast<UnaryExpr *>(expr->object)) {
    // Handle (*ptr).x case
    if (unary->op == TokenType::Star) {
      if (auto *ptrVar = dynamic_cast<Variable *>(unary->operand)) {
        LocalVar *localVar = this->findVariable(ptrVar->name);

        structTypeName = this->getPointedToType(localVar->typeStr);
        if (structTypeName.empty()) {
          fprintf(stderr,
                  "Error: Cannot dereference non-pointer in member access.\n");
          std::abort();
        }

        auto it = this->structTypes.find(structTypeName);
        if (it == this->structTypes.end()) {
          fprintf(stderr, "Error: Unknown struct type '%s' in member access.\n",
                  structTypeName.c_str());
          std::abort();
        }
        structType = it->second;

        structPtr =
            this->builder->CreateLoad(localVar->alloca->getAllocatedType(),
                                      localVar->alloca, ptrVar->name + "_load");
      } else {
        fprintf(
            stderr,
            "Error: Complex dereference in member access not yet supported.\n");
        std::abort();
      }
    } else {
      fprintf(stderr, "Error: Unsupported unary operation in member access.\n");
      std::abort();
    }
  } else {
    fprintf(stderr,
            "Error: Complex expressions in member access not yet supported.\n");
    std::abort();
  }

  int fieldIndex = this->getFieldIndex(structTypeName, expr->field);

  auto metaIt = this->structFieldMetadata.find(structTypeName);
  if (metaIt == this->structFieldMetadata.end()) {
    fprintf(stderr, "Error: No field metadata for struct '%s'.\n",
            structTypeName.c_str());
    std::abort();
  }

  const std::string &fieldTypeStr = metaIt->second[fieldIndex].second;
  llvm::Type *fieldType = this->getLLVMType(fieldTypeStr, this->context);

  llvm::Value *fieldPtr = this->builder->CreateStructGEP(
      structType, structPtr, fieldIndex, expr->field + "_ptr");

  return this->builder->CreateLoad(fieldType, fieldPtr, expr->field);
}

// MARK: Modules

void Codegen::setModuleName(const std::string &moduleName) {
  this->currentModuleName = moduleName;
}

ModuleMetadata Codegen::getExportedSymbols() const {
  return this->currentModuleExports;
}

void Codegen::loadImport(const std::string &modulePath,
                         const std::string &baseDir) {
  if (this->importedModules.find(modulePath) != this->importedModules.end()) {
    return;
  }

  std::string metadataPath = baseDir;
  if (!metadataPath.empty() && metadataPath.back() != '/' &&
      metadataPath.back() != '\\') {
    metadataPath += "/";
  }
  metadataPath += modulePath + ".racm";

  ModuleMetadata metadata = ModuleMetadata::loadFromFile(metadataPath);

  if (metadata.moduleName.empty()) {
    fprintf(stderr, "Error: Failed to load module metadata from '%s'\n",
            metadataPath.c_str());
    std::abort();
  }

  this->importedModules[modulePath] = metadata;

  for (const auto &exportedStruct : metadata.structs) {
    if (this->structTypes.find(exportedStruct.name) !=
        this->structTypes.end()) {
      continue;
    }

    std::vector<llvm::Type *> fieldTypes;
    for (const auto &field : exportedStruct.fields) {
      llvm::Type *fieldType = this->getLLVMType(field.second, this->context);
      if (!fieldType) {
        fprintf(stderr,
                "Error: Invalid type '%s' for field '%s' in imported struct "
                "'%s'.\n",
                field.second.c_str(), field.first.c_str(),
                exportedStruct.name.c_str());
        std::abort();
      }
      fieldTypes.push_back(fieldType);
    }

    llvm::StructType *structType = llvm::StructType::create(
        this->context, fieldTypes, exportedStruct.name);

    this->structTypes[exportedStruct.name] = structType;
    this->structFieldMetadata[exportedStruct.name] = exportedStruct.fields;
  }
}
