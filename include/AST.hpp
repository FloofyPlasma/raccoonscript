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

struct FloatLiteral : Expr {
  float value;
  FloatLiteral(float v) : value(v) {}
};

struct StrLiteral : Expr {
  std::string value;
  StrLiteral(std::string v) : value(v) {}
};

struct CharLiteral : Expr {
  char value;
  CharLiteral(char v) : value(v) {}
};

struct UnaryExpr : Expr {
  TokenType op;
  Expr *operand;
  UnaryExpr(TokenType op, Expr *o) : op(op), operand(o) {}
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

struct CallExpr : Expr {
  std::string name;
  std::vector<Expr *> args;
  CallExpr(std::string n, std::vector<Expr *> a) : name(n), args(a) {}
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

struct IfStmt : Statement {
  Expr *condition;
  std::vector<Statement *> thenBranch;
  std::vector<Statement *> elseBranch; // optional
  IfStmt(Expr *c, std::vector<Statement *> t, std::vector<Statement *> e = {})
      : condition(c), thenBranch(t), elseBranch(e) {}
};

struct WhileStmt : Statement {
  Expr *condition;
  std::vector<Statement *> body;
  WhileStmt(Expr *c, std::vector<Statement *> b) : condition(c), body(b) {}
};

struct ForStmt : Statement {
  Statement *initializer; // usually VarDecl or ExprStmt
  Expr *condition;
  Expr *increment;
  std::vector<Statement *> body;
  ForStmt(Statement *i, Expr *c, Expr *inc, std::vector<Statement *> b)
      : initializer(i), condition(c), increment(inc), body(b) {}
};

struct ReturnStmt : Statement {
  Expr *value; // nullptr if no return expression
  ReturnStmt(Expr *v = nullptr) : value(v) {}
};

struct BlockStmt : Statement {
  std::vector<Statement *> statements;
  BlockStmt(std::vector<Statement *> s) : statements(s) {}
};
