#pragma once

#include "Token.hpp"
#include <string>

class Lexer {
  const std::string &source;
  size_t pos = 0;
  int line = 1;
  int column = 1;

public:
  Lexer(const std::string &src) : source(src) {}
  Token nextToken();

private:
  void skipWhitespace();
  void skipComment();

  Token number();
  Token identifier();
  Token stringLiteral();
  Token charLiteral();
};
