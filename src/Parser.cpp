#include "Parser.hpp"
#include "AST.hpp"
#include "Token.hpp"
#include <string>

void Parser::advance() { this->current = this->lexer.nextToken(); }

Expr *Parser::parseExpression() {
  // For now: only integer literals and variables
  if (this->current.type == TokenType::IntLiteral) {
    int value = std::stoi(this->current.lexeme);
    this->advance();
    return new IntLiteral(value);
  } else if (this->current.type == TokenType::Identifier) {
    std::string name = this->current.lexeme;
    this->advance();
    return new Variable{name};
  }

  return nullptr;
}

Statement *Parser::parseVarDecl() {
  if (this->current.type == TokenType::Keyword &&
      this->current.lexeme == "let") {
    this->advance();

    if (this->current.type != TokenType::Identifier) {
      return nullptr; // error;
    }

    std::string name = this->current.lexeme;
    this->advance();

    std::string type = "i32"; // default type, TODO: Parse type annotation
    if (this->current.type == TokenType::Colon) {
      /* parse type */
    }

    Expr *initializer = nullptr;
    if (this->current.type == TokenType::Equal) {
      this->advance();

      initializer = this->parseExpression();
    }

    if (this->current.type != TokenType::Semicolon) {
      delete initializer;
      return nullptr; // error
    }

    this->advance();

    return new VarDecl(name, type, initializer);
  }

  return nullptr;
}
