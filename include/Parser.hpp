#pragma once

#include "AST.hpp"
#include "Lexer.hpp"
#include "Token.hpp"

class Parser {
  Lexer &lexer;

public:
  Token current;
  Parser(Lexer &l) : lexer(l) { this->advance(); }

  void advance();
  Token peek();

  Expr *parseExpression(int precedence = 0);

  Statement *parseStatement(bool insideFunction = false);
  Statement *parseVarDecl(bool isConst);
  Statement *parseFunctionDecl();
  Statement *parseIfStatement();
  Statement *parseWhileStatement();
  Statement *parseForStatement();
  Statement *parseReturnStatement();
  Statement *parseBlockStatement();
  Statement *parseStructDecl();
  Statement* parseImportDecl();

  Expr *parsePrimary();

  Expr *parseInitializer();

  Expr *parseUnary();

private:
  int getPrecedence(TokenType type);

  Statement *parseExpressionStatement();
};
