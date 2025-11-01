#pragma once

#include <string>
#include <utility>
#include <vector>

#include "Token.hpp"

struct Expr {
  virtual ~Expr() = default;
};

struct IntLiteral : Expr {
  long long value;
  IntLiteral(long long v) : value(v) {}
};

struct FloatLiteral : Expr {
  float value;
  FloatLiteral(float v) : value(v) {}
};

struct BoolLiteral : Expr {
  bool value;
  BoolLiteral(bool v) : value(v) {}
};

struct StrLiteral : Expr {
  std::string value;
  StrLiteral(std::string v) : value(v) {}
};

struct CharLiteral : Expr {
  char value;
  CharLiteral(char v) : value(v) {}
};

struct StructLiteral : Expr {
  std::string typeName;
  std::vector<std::pair<std::string, Expr *>> fields; // field name + value
  std::string moduleName;
  StructLiteral(std::string t, std::vector<std::pair<std::string, Expr *>> f,
                std::string m = "")
      : typeName(t), fields(f), moduleName(m) {}
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
  std::string type;
  std::string moduleName;
  CallExpr(std::string n, std::vector<Expr *> a, std::string t,
           std::string m = "")
      : name(n), args(a), type(t), moduleName(m) {}
};

struct MemberAccessExpr : Expr {
  Expr *object;
  std::string field;
  MemberAccessExpr(Expr *obj, std::string f) : object(obj), field(f) {}
};

struct Statement {
  virtual ~Statement() = default;
};

struct VarDecl : Statement {
  std::string name;
  std::string type;
  Expr *initializer;
  bool isConst;
  VarDecl(std::string n, std::string t, Expr *i, bool isC)
      : name(n), type(t), initializer(i), isConst(isC) {}
};

struct FunctionDecl : Statement {
  std::string name;
  std::vector<std::pair<std::string, std::string>> params; // name + type
  std::vector<Statement *> body;
  std::string returnType;
  bool isExported;
  bool isExternal;

  FunctionDecl(std::string n,
               std::vector<std::pair<std::string, std::string>> p,
               std::vector<Statement *> b, std::string r, bool exported = false,
               bool ext = false)
      : name(n), params(p), body(b), returnType(r), isExported(exported),
        isExternal(ext) {}
};

struct StructDecl : Statement {
  std::string name;
  std::vector<std::pair<std::string, std::string>> fields; // name + type
  bool isExported;
  StructDecl(std::string n, std::vector<std::pair<std::string, std::string>> f,
             bool exported = false)
      : name(n), fields(f), isExported(exported) {}
};

struct ImportDecl : Statement {
  std::string modulePath;
  ImportDecl(std::string p) : modulePath(p) {}
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
