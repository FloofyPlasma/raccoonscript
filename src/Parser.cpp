#include "Parser.hpp"
#include "AST.hpp"
#include "Token.hpp"
#include <iostream>
#include <string>

void Parser::advance() { this->current = this->lexer.nextToken(); }

Statement *Parser::parseStatement(bool insideFunction) {
  if (this->current.type == TokenType::Keyword &&
      this->current.lexeme == "let") {
    return this->parseVarDecl();
  }

  if (!insideFunction && this->current.type == TokenType::Keyword &&
      this->current.lexeme == "fun") {
    return this->parseFunctionDecl();
  }

  return this->parseExpressionStatement();
}

Statement *Parser::parseVarDecl() {
  this->advance(); // consume 'let'
  if (this->current.type != TokenType::Identifier) {
    return nullptr; // error
  }

  std::string name = this->current.lexeme;
  this->advance(); // consume identifier

  std::string type = "i32"; // default type
  if (this->current.type == TokenType::Colon) {
    this->advance(); // consume ':'
    if (this->current.type != TokenType::Identifier) {
      return nullptr; // error
    }
    type = this->current.lexeme;
    this->advance(); // consume type identifier
  }

  Expr *initializer = nullptr;
  if (this->current.type == TokenType::Equal) {
    this->advance(); // consume '='
    initializer = this->parseInitializer();
    if (!initializer) {
      return nullptr; // error
    }
  }

  if (this->current.type != TokenType::Semicolon) {
    delete initializer;
    return nullptr; // error
  }

  this->advance(); // consume ';'

  return new VarDecl{name, type, initializer};
}

Statement *Parser::parseFunctionDecl() {
  this->advance(); // consume 'fun'
  if (this->current.type != TokenType::Identifier) {
    return nullptr; // error
  }

  std::string name = this->current.lexeme;
  this->advance(); // consume function name

  if (this->current.type != TokenType::LeftParen) {
    return nullptr; // error
  }
  this->advance(); // consume '('

  std::vector<std::pair<std::string, std::string>> params;
  while (this->current.type != TokenType::RightParen) {
    if (this->current.type != TokenType::Identifier) {
      return nullptr; // error
    }
    std::string paramName = this->current.lexeme;
    this->advance(); // consume parameter name

    if (this->current.type != TokenType::Colon) {
      return nullptr; // error
    }
    this->advance(); // consume ':'

    if (this->current.type != TokenType::Identifier) {
      return nullptr; // error
    }
    std::string paramType = this->current.lexeme;
    this->advance(); // consume parameter type

    params.push_back({paramName, paramType});

    if (this->current.type == TokenType::Comma) {
      this->advance(); // consume ','
    } else if (this->current.type == TokenType::RightParen) {
      break;
    } else {
      return nullptr; // error
    }
  }
  this->advance(); // consume ')'

  std::string returnType = "void";
  if (this->current.type == TokenType::Colon) {
    this->advance(); // consume ':'
    if (this->current.type != TokenType::Identifier) {
      return nullptr; // error
    }
    returnType = this->current.lexeme;
    this->advance(); // consume return type
  }

  if (this->current.type != TokenType::LeftBrace) {
    return nullptr; // error
  }
  this->advance(); // consume '{'

  std::vector<Statement *> body;
  while (this->current.type != TokenType::RightBrace &&
         this->current.type != TokenType::EndOfFile) {
    Statement *stmt = this->parseStatement(true);
    if (!stmt) {
      this->advance(); // skip erroneous token
      continue;
    }
    body.push_back(stmt);
  }

  if (this->current.type != TokenType::RightBrace) {
    return nullptr; // error
  }
  this->advance(); // consume '}'

  return new FunctionDecl(name, params, body, returnType);
}

Expr *Parser::parseExpression(int precedence) {
  Expr *left = this->parsePrimary();
  if (!left) {
    return nullptr; // error
  }

  while (true) {
    int opPrecedence = this->getPrecedence(this->current.type);
    if (opPrecedence < precedence) {
      break;
    }

    TokenType op = this->current.type;
    this->advance(); // consume operator
    Expr *right = this->parseExpression(opPrecedence + 1);
    if (!right) {
      break; // stop parsing if right side fails
    }
    left = new BinaryExpr(left, right, op);
  }
  return left;
}

Expr *Parser::parsePrimary() {
  if (this->current.type == TokenType::IntLiteral) {
    int value = std::stoi(this->current.lexeme);
    this->advance(); // consume integer literal
    return new IntLiteral(value);
  }
  if (this->current.type == TokenType::Identifier) {
    std::string name = this->current.lexeme;
    this->advance(); // consume identifier
    return new Variable(name);
  }
  if (this->current.type == TokenType::LeftParen) {
    this->advance(); // consume '('
    Expr *expr = this->parseExpression();
    if (this->current.type != TokenType::RightParen) {
      delete expr;
      return nullptr; // error
    }
    this->advance(); // consume ')'
    return expr;
  }

  return nullptr;
}

int Parser::getPrecedence(TokenType type) {
  switch (type) {
  case TokenType::Star:
  case TokenType::Slash:
    return 10;
  case TokenType::Plus:
  case TokenType::Minus:
    return 5;
  default:
    return 0;
  }
}

Statement *Parser::parseExpressionStatement() {
  Expr *expr = this->parseExpression();
  if (!expr) {
    this->advance(); // consume erroneous token
    return nullptr;  // error
  }

  if (this->current.type != TokenType::Semicolon) {
    delete expr;
    this->advance(); // skip to recover
    return nullptr;  // error
  }
  this->advance(); // consume ';'
  return new ExprStmt(expr);
}

Expr *Parser::parseInitializer() {
  if (this->current.type == TokenType::IntLiteral) {
    int value = std::stoi(this->current.lexeme);
    this->advance(); // consume initializer
    return new IntLiteral(value);
  }
  if (this->current.type == TokenType::Identifier) {
    std::string name = this->current.lexeme;
    this->advance(); // consume name
    return new Variable(name);
  }
  // TODO: More cases (FloatLiteral, BoolLiteral etc).
  return nullptr;

  return nullptr;
}
