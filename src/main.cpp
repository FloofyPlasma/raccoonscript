#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "AST.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"

// Simple AST printer
void printExpr(Expr *expr, int indent = 0) {
  std::string pad(indent, '\t');
  if (auto intLit = dynamic_cast<IntLiteral *>(expr)) {
    std::cout << pad << "IntLiteral: " << intLit->value << "\n";
  } else if (auto var = dynamic_cast<Variable *>(expr)) {
    std::cout << pad << "Variable: " << var->name << "\n";
  } else if (auto bin = dynamic_cast<BinaryExpr *>(expr)) {
    std::cout << pad << "BinaryExpr: " << static_cast<int>(bin->op) << "\n";
    printExpr(bin->left, indent + 2);
    printExpr(bin->right, indent + 2);
  }
}

void printStatement(Statement *stmt, int indent = 0) {
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
  }
}
int main(int argc, const char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage racoonc <source_file>\n";
    return 1;
  }

  std::ifstream file(argv[1]);
  if (!file) {
    std::cerr << "Cannot open file: " << argv[1] << '\n';
    return 1;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string source = buffer.str();

  Lexer lexer(source);
  Parser parser(lexer);

  std::vector<Statement *> program;
  while (parser.current.type != TokenType::EndOfFile) {
    Statement *stmt = parser.parseStatement(false);
    if (!stmt) {
      parser.advance();
      continue;
    }
    program.push_back(stmt);
  }

  std::cout << "AST:\n";
  for (auto stmt : program) {
    printStatement(stmt);
  }

  // Cleanup
  for (auto stmt : program) {
    delete stmt;
  }

  return 0;
}
