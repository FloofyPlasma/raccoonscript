#pragma once

#include <string>

enum class TokenType {
  Identifier,
  Keyword,
  IntLiteral,
  StringLiteral,
  CharLiteral,
  FloatLiteral,
  Plus,
  Minus,
  Star,
  Slash,
  Equal,
  Semicolon,
  Colon,
  Comma,
  Dot,
  LeftParen,
  RightParen,
  LeftBrace,
  RightBrace,
  LessThan,
  GreaterThan,
  LessEqual,
  GreaterEqual,
  DoubleEqual,
  BangEqual,
  Bang,
  Ampersand,
  AndAnd,
  Pipe,
  OrOr,
  Percent,
  EndOfFile
};

struct Token {
  TokenType type;
  std::string lexeme;
  int line;
  int column;
};
