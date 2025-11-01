#include "Lexer.hpp"
#include "Token.hpp"
#include <cctype>
#include <unordered_set>

Token Lexer::nextToken() {
  this->skipComment();

  if (pos >= this->source.size()) {
    return {TokenType::EndOfFile, "", this->line, this->column};
  }

  char c = this->source[this->pos];

  if (std::isdigit(c)) {
    return number();
  }

  if (std::isalpha(c) || c == '_') {
    return identifier();
  }

  switch (c) {
  case '+': {
    this->pos++;
    return {TokenType::Plus, "+", this->line, this->column++};
  }
  case '-': {
    this->pos++;
    return {TokenType::Minus, "-", this->line, this->column++};
  }
  case '*': {
    this->pos++;
    return {TokenType::Star, "*", this->line, this->column++};
  }
  case '/': {
    this->pos++;
    return {TokenType::Slash, "/", this->line, this->column++};
  }
  case '.': {
    this->pos++;
    return {TokenType::Dot, ".", this->line, this->column++};
  }
  case '=': {
    this->pos++;
    if (this->pos < this->source.size() && this->source[pos] == '=') {
      this->pos++;
      return {TokenType::DoubleEqual, "==", this->line, this->column++};
    }
    return {TokenType::Equal, "=", this->line, this->column++};
  }
  case '!': {
    this->pos++;
    if (this->pos < this->source.size() && this->source[pos] == '=') {
      this->pos++;
      return {TokenType::BangEqual, "!=", this->line, this->column++};
    }
    return {TokenType::Bang, "!", this->line, this->column++};
  }
  case ';': {
    this->pos++;
    return {TokenType::Semicolon, ";", this->line, this->column++};
  }
  case ':': {
    this->pos++;
    return {TokenType::Colon, ":", this->line, this->column++};
  }
  case ',': {
    this->pos++;
    return {TokenType::Comma, ",", this->line, this->column++};
  }
  case '(': {
    this->pos++;
    return {TokenType::LeftParen, "(", this->line, this->column++};
  }
  case ')': {
    this->pos++;
    return {TokenType::RightParen, ")", this->line, this->column++};
  }
  case '{': {
    this->pos++;
    return {TokenType::LeftBrace, "{", this->line, this->column++};
  }
  case '}': {
    this->pos++;
    return {TokenType::RightBrace, "}", this->line, this->column++};
  }
  case '<': {
    this->pos++;
    if (this->pos < this->source.size() && this->source[pos] == '=') {
      this->pos++;
      return {TokenType::LessEqual, "<=", this->line, this->column++};
    }
    return {TokenType::LessThan, "<", this->line, this->column++};
  }
  case '>': {
    this->pos++;
    if (this->pos < this->source.size() && this->source[pos] == '=') {
      this->pos++;
      return {TokenType::GreaterEqual, ">=", this->line, this->column++};
    }
    return {TokenType::GreaterThan, ">", this->line, this->column++};
  }
  case '&': {
    this->pos++;
    if (this->pos < this->source.size() && this->source[pos] == '&') {
      this->pos++;
      return {TokenType::AndAnd, "&&", this->line, this->column++};
    }
    return {TokenType::Ampersand, "&", this->line, this->column++};
  }
  case '|': {
    this->pos++;
    if (this->pos < this->source.size() && this->source[pos] == '|') {
      this->pos++;
      return {TokenType::OrOr, "||", this->line, this->column++};
    }
    return {TokenType::Pipe, "|", this->line, this->column++};
  }
  case '%': {
    this->pos++;
    return {TokenType::Percent, "%", this->line, this->column++};
  }
  case '"': {
    return this->stringLiteral();
  }
  case '\'': {
    return this->charLiteral();
  }
  default: {
    this->pos++;
    return {TokenType::EndOfFile, "", this->line, this->column++};
  }
  }
}

void Lexer::skipComment() {
  while (pos < this->source.size()) {
    while (pos < this->source.size() && std::isspace(this->source[pos])) {
      if (this->source[pos] == '\n') {
        this->line++;
        this->column = 1;
      } else {
        this->column++;
      }
      pos++;
    }

    if (pos < this->source.size() && this->source[pos] == '/' &&
        pos + 1 < this->source.size()) {

      // Single-line comment //
      if (this->source[pos + 1] == '/') {
        pos += 2;
        column += 2;
        while (pos < this->source.size() && this->source[pos] != '\n') {
          pos++;
          column++;
        }

        continue;
      }

      // Multi-line comment /* */
      else if (this->source[pos + 1] == '*') {
        pos += 2;
        column += 2;
        while (pos + 1 < this->source.size() &&
               !(this->source[pos] == '*' && this->source[pos + 1] == '/')) {
          if (this->source[pos] == '\n') {
            line++;
            column = 1;
          } else {
            column++;
          }
          pos++;
        }
        if (pos + 1 < this->source.size()) {
          pos += 2; // skip closing */
          column += 2;
        }

        continue;
      }
    }

    // No more comments or whitespace to skip
    break;
  }
}

Token Lexer::number() {
  size_t start = this->pos;

  while (this->pos < this->source.size() &&
         std::isdigit(this->source[this->pos])) {
    this->pos++;
  }

  if (this->pos < this->source.size() && this->source[pos] == '.') {
    this->pos++;

    while (this->pos < this->source.size() &&
           std::isdigit(this->source[this->pos])) {
      this->pos++;
    }

    std::string lexeme = this->source.substr(start, this->pos - start);

    return {TokenType::FloatLiteral, lexeme, this->line, this->column};
  } else {
    std::string lexeme = this->source.substr(start, this->pos - start);

    return {TokenType::IntLiteral, lexeme, this->line, this->column};
  }
}

Token Lexer::stringLiteral() {
  size_t start = this->pos;
  this->pos++;

  while (this->pos < this->source.size() && this->source[this->pos] != '"') {
    if (this->source[this->pos] == '\\' &&
        this->pos + 1 < this->source.size()) {
      this->pos += 2; // skip escape sequence
    } else {
      this->pos++;
    }
  }

  std::string lexeme = this->source.substr(start + 1, this->pos - start - 1);
  this->pos++;

  return {TokenType::StringLiteral, lexeme, this->line, this->column};
}

Token Lexer::charLiteral() {
  size_t start = this->pos;
  this->pos++;

  char c = this->source[this->pos++];
  if (c == '\\' && this->pos < this->source.size()) {
    c = this->source[this->pos++];
  }

  this->pos++;
  return {TokenType::CharLiteral, std::string(1, c), this->line, this->column};
}

Token Lexer::identifier() {
  size_t start = this->pos;

  while (this->pos < this->source.size() &&
         (std::isalnum(this->source[this->pos]) || this->source[pos] == '_')) {
    this->pos++;
  }

  std::string lexeme = source.substr(start, this->pos - start);

  // Check for keyword
  static const std::unordered_set<std::string> keywords = {
      "fun",  "let",   "const", "struct", "return", "if",
      "else", "while", "for",   "import", "export", "malloc",
      "free", "true",  "false", "void",   "extern"};

  TokenType type =
      keywords.count(lexeme) ? TokenType::Keyword : TokenType::Identifier;

  return {type, lexeme, this->line, this->column};
}

Token Lexer::peekToken() {
  size_t oldPos = this->pos;
  int oldLine = this->line;
  int oldColumn = this->column;

  Token token = this->nextToken();

  this->pos = oldPos;
  this->line = oldLine;
  this->column = oldColumn;

  return token;
}
