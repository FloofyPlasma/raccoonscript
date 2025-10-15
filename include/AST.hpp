#pragma once

#include <string>
#include <utility>
#include <vector>

#include "Token.hpp"

struct Expr {
  virtual ~Expr() = default;
};

struct IntLiteral : Expr {
  int value;
  IntLiteral(int v) : value(v) {}
};

struct Variable : Expr {
  std::string name;
  Variable(std::string v) : name(v) {}
};

struct BinaryExpr : Expr {
  Expr *left;
  Expr *right;
  TokenType op;
  BinaryExpr(Expr *l, Expr *r, TokenType o) : left(l), right(r), op(o) {}
};

struct Statement {
  virtual ~Statement() = default;
};

struct VarDecl : Statement {
  std::string name;
  std::string type;
  Expr *initializer;
  VarDecl(std::string n, std::string t, Expr *i)
      : name(n), type(t), initializer(i) {}
};

struct FunctionDecl : Statement {
  std::string name;
  std::vector<std::pair<std::string, std::string>> params; // name + type
  std::vector<Statement *> body;
  std::string returnType;
  FunctionDecl(std::string n,
               std::vector<std::pair<std::string, std::string>> p,
               std::vector<Statement *> b, std::string r)
      : name(n), params(p), body(b), returnType(r) {}
};

struct ExprStmt : Statement {
  Expr *expr;
  ExprStmt(Expr *e) : expr(e) {}
};
