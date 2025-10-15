#include "ASTPrinter.hpp"

#include <iostream>

std::ostream &operator<<(std::ostream &out, const TokenType &value) {
  return out << [value]() -> std::string {
#define PROCESS_VAL(p)                                                         \
  case (p):                                                                    \
    return std::string(#p);
    switch (value) {
      PROCESS_VAL(TokenType::Plus);
      PROCESS_VAL(TokenType::Minus);
      PROCESS_VAL(TokenType::Star);
      PROCESS_VAL(TokenType::Slash);
      PROCESS_VAL(TokenType::Equal);
      PROCESS_VAL(TokenType::LessThan);
      PROCESS_VAL(TokenType::GreaterThan);
      PROCESS_VAL(TokenType::LessEqual);
      PROCESS_VAL(TokenType::GreaterEqual);
      PROCESS_VAL(TokenType::DoubleEqual);
      PROCESS_VAL(TokenType::BangEqual);
      PROCESS_VAL(TokenType::Bang);
    default:
      return std::to_string(static_cast<int>(value));
    }
#undef PROCESS_VAL
  }();
}

void printExpr(Expr *expr, int indent) {
  std::string pad(indent, '\t');
  if (auto intLit = dynamic_cast<IntLiteral *>(expr)) {
    std::cout << pad << "IntLiteral: " << intLit->value << "\n";
  } else if (auto var = dynamic_cast<Variable *>(expr)) {
    std::cout << pad << "Variable: " << var->name << "\n";
  } else if (auto bin = dynamic_cast<BinaryExpr *>(expr)) {
    std::cout << pad << "BinaryExpr: " << bin->op << "\n";
    printExpr(bin->left, indent + 2);
    printExpr(bin->right, indent + 2);
  } else if (auto call = dynamic_cast<CallExpr *>(expr)) {
    std::cout << pad << "CallExpr: " << call->name << "\n";
    for (auto &arg : call->args) {
      printExpr(arg, indent + 2);
    }
  }
}

void printIfStmt(IfStmt *ifs, int indent) {
  std::string pad(indent, '\t');
  std::cout << pad << "IfStmt:\n";
  std::cout << pad << "\tCondition:\n";
  printExpr(ifs->condition, indent + 2);
  std::cout << pad << "\tThenBranch:\n";
  for (auto &s : ifs->thenBranch) {
    printStatement(s, indent + 2);
  }
  if (!ifs->elseBranch.empty()) {
    std::cout << pad << "\tElseBranch:\n";
    for (auto &s : ifs->elseBranch) {
      printStatement(s, indent + 2);
    }
  }
}

void printWhileStmt(WhileStmt *whiles, int indent) {
  std::string pad(indent, '\t');
  std::cout << pad << "WhileStmt:\n";
  std::cout << pad << "\tCondition:\n";
  printExpr(whiles->condition, indent + 2);
  std::cout << pad << "\tBody:\n";
  for (auto &s : whiles->body) {
    printStatement(s, indent + 2);
  }
}

void printForStmt(ForStmt *fors, int indent) {
  std::string pad(indent, '\t');
  std::cout << pad << "ForStmt:\n";
  std::cout << pad << "\tInitializer:\n";
  printStatement(fors->initializer, indent + 2);
  std::cout << pad << "\tCondition:\n";
  printExpr(fors->condition, indent + 2);
  std::cout << pad << "\tIncrement:\n";
  printExpr(fors->increment, indent + 2);
  std::cout << pad << "\tBody:\n";
  for (auto &s : fors->body) {
    printStatement(s, indent + 2);
  }
}

void printStatement(Statement *stmt, int indent) {
  std::string pad(indent, '\t');
  if (auto varDecl = dynamic_cast<VarDecl *>(stmt)) {
    std::cout << pad << "VarDecl: " << varDecl->name << " : " << varDecl->type
              << "\n";
    if (varDecl->initializer)
      printExpr(varDecl->initializer, indent + 2);
  } else if (auto exprStmt = dynamic_cast<ExprStmt *>(stmt)) {
    std::cout << pad << "ExprStmt:\n";
    printExpr(exprStmt->expr, indent + 2);
  } else if (auto funcDecl = dynamic_cast<FunctionDecl *>(stmt)) {
    std::cout << pad << "FunctionDecl: " << funcDecl->name << " -> "
              << funcDecl->returnType << "\n";
    for (auto &param : funcDecl->params) {
      std::cout << pad << "\tParam: " << param.first << " : " << param.second
                << "\n";
    }
    std::cout << pad << "\tBody:\n";
    for (auto &s : funcDecl->body)
      printStatement(s, indent + 2);
  } else if (auto ifs = dynamic_cast<IfStmt *>(stmt)) {
    printIfStmt(ifs, indent);
  } else if (auto whiles = dynamic_cast<WhileStmt *>(stmt)) {
    printWhileStmt(whiles, indent);
  } else if (auto fors = dynamic_cast<ForStmt *>(stmt)) {
    printForStmt(fors, indent);
  } else if (auto ret = dynamic_cast<ReturnStmt *>(stmt)) {
    std::cout << pad << "ReturnStmt:\n";
    if (ret->value) {
      printExpr(ret->value, indent + 2);
    } else {
      std::cout << pad << "\t(void)\n";
    }
  } else if (auto block = dynamic_cast<BlockStmt *>(stmt)) {
    std::cout << pad << "BlockStmt:\n";
    for (auto &s : block->statements) {
      printStatement(s, indent + 2);
    }
  }
}
