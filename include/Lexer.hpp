#pragma once

#include "Token.hpp"
#include <string>

class Lexer {
public:
  Lexer(const std::string &src) : source(src) {}
  Token nextToken();
  Token peekToken();

private:
  const std::string &source;
  size_t pos = 0;
  int line = 1;
  int column = 1;

  size_t savedPos;
  int savedLine;
  int savedColumn;

  [[deprecated("Whitespace skipping is handled in Lexer::skipComment().")]]
  void skipWhitespace();
  void skipComment();

  Token number();
  Token identifier();
  Token stringLiteral();
  Token charLiteral();
};
