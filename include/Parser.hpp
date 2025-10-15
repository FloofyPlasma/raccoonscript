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

  Expr *parseExpression(int precedence = 0);

  Statement *parseStatement(bool insideFunction = false);

  Statement *parseVarDecl();

  Statement *parseFunctionDecl();

  Statement *parseIfStatement();

  Statement *parseWhileStatement();

  Statement *parseForStatement();

  Expr *parsePrimary();

  Expr *parseInitializer();

private:
  int getPrecedence(TokenType type);

  Statement *parseExpressionStatement();
};
