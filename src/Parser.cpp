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

  if (this->current.type == TokenType::Keyword &&
      this->current.lexeme == "if") {
    return this->parseIfStatement();
  }

  if (this->current.type == TokenType::Keyword &&
      this->current.lexeme == "while") {
    return this->parseWhileStatement();
  }

  if (this->current.type == TokenType::Keyword &&
      this->current.lexeme == "for") {
    return this->parseForStatement();
  }

  if (this->current.type == TokenType::Keyword &&
      this->current.lexeme == "return") {
    return this->parseReturnStatement();
  }

  if (this->current.type == TokenType::LeftBrace) {
    return this->parseBlockStatement();
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
    if (this->current.type != TokenType::Identifier && this->current.type != TokenType::Keyword) { // FIXME: Should void remain a keyword?
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
    if (this->current.type == TokenType::Semicolon ||
        this->current.type == TokenType::RightParen ||
        this->current.type == TokenType::RightBrace ||
        this->current.type == TokenType::Comma ||
        this->current.type == TokenType::EndOfFile) {
      break;
    }

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

Statement *Parser::parseIfStatement() {
  this->advance(); // consume 'if'
  if (this->current.type != TokenType::LeftParen) {
    return nullptr; // error
  }
  this->advance(); // consume '('

  Expr *condition = this->parseExpression();
  if (!condition) {
    return nullptr; // error
  }

  // TODO: Seems this->parseExpresion() may consume the ')', should this be
  // kept?
  //   if (this->current.type != TokenType::RightParen) {
  //     delete condition;
  //     return nullptr; // error
  //   }
  //   this->advance(); // consume ')'

  if (this->current.type != TokenType::LeftBrace) {
    delete condition;
    return nullptr; // error
  }
  this->advance(); // consume '{'

  std::vector<Statement *> thenBranch;
  while (this->current.type != TokenType::RightBrace &&
         this->current.type != TokenType::EndOfFile) {
    Statement *stmt = this->parseStatement(true);
    if (!stmt) {
      this->advance(); // skip erroneous token
      continue;
    }
    thenBranch.push_back(stmt);
  }

  if (this->current.type != TokenType::RightBrace) {
    delete condition;
    for (auto s : thenBranch) {
      delete s;
    }
    return nullptr; // error
  }
  this->advance(); // consume '}'

  std::vector<Statement *> elseBranch;
  if (this->current.type == TokenType::Keyword &&
      this->current.lexeme == "else") {
    this->advance(); // consume 'else'
    if (this->current.type != TokenType::LeftBrace) {
      delete condition;
      for (auto s : thenBranch) {
        delete s;
      }
      return nullptr; // error
    }
    this->advance(); // consume '{'

    while (this->current.type != TokenType::RightBrace &&
           this->current.type != TokenType::EndOfFile) {
      Statement *stmt = this->parseStatement(true);
      if (!stmt) {
        this->advance(); // skip erroneous token
        continue;
      }
      elseBranch.push_back(stmt);
    }
    if (this->current.type != TokenType::RightBrace) {
      delete condition;
      for (auto s : thenBranch) {
        delete s;
      }
      for (auto s : elseBranch) {
        delete s;
      }
      return nullptr; // error
    }
    this->advance(); // consume '}'
  }

  return new IfStmt(condition, thenBranch, elseBranch);
}

Statement *Parser::parseWhileStatement() {
  this->advance(); // consume 'while'
  if (this->current.type != TokenType::LeftParen) {
    return nullptr; // error
  }
  this->advance(); // consume '('

  Expr *condition = this->parseExpression();
  if (!condition) {
    return nullptr; // error
  }

  // TODO: Seems this->parseExpresion() may consume the ')', should this be
  // kept?
  //   if (this->current.type != TokenType::RightParen) {
  //     delete condition;
  //     return nullptr; // error
  //   }
  //   this->advance(); // consume ')'

  if (this->current.type != TokenType::LeftBrace) {
    delete condition;
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
    delete condition;
    for (auto s : body) {
      delete s;
    }
    return nullptr; // error
  }

  this->advance(); // consume '}'

  return new WhileStmt{condition, body};
}

Statement *Parser::parseForStatement() {
  this->advance(); // consume 'for'

  if (this->current.type != TokenType::LeftParen) {
    return nullptr; // error
  }
  this->advance(); // consume '('

  Statement *initializer = nullptr;
  if (this->current.lexeme == "let") {
    initializer = this->parseVarDecl();
  } else if (this->current.type != TokenType::Semicolon) {
    initializer = this->parseExpressionStatement();
  }

  Expr *condition = nullptr;
  if (this->current.type != TokenType::Semicolon) {
    condition = this->parseExpression();
  }

  if (this->current.type != TokenType::Semicolon) {
    delete initializer;
    delete condition;
    return nullptr; // error
  }
  this->advance(); // consume ';'

  Expr *increment = nullptr;
  if (this->current.type != TokenType::RightParen) {
    increment = this->parseExpression();
  }

  if (this->current.type != TokenType::RightParen) {
    delete initializer;
    delete condition;
    delete increment;
    return nullptr; // error
  }
  this->advance(); // consume ')'

  if (this->current.type != TokenType::LeftBrace) {
    delete initializer;
    delete condition;
    delete increment;
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
    delete initializer;
    delete condition;
    delete increment;
    for (auto s : body) {
      delete s;
    }
    return nullptr; // error
  }
  this->advance(); // consume '}'

  return new ForStmt(initializer, condition, increment, body);
}

Statement *Parser::parseReturnStatement() {
  this->advance(); // consume 'return'

  Expr *value = nullptr;
  if (this->current.type != TokenType::Semicolon) {
    value = this->parseExpression();
    if (!value) {
      return nullptr; // We return nothing
    }
  }

  if (this->current.type != TokenType::Semicolon) {
    delete value;
    this->advance(); // skip erroneous token
    return nullptr;  // error
  }
  this->advance(); // consume ';'

  return new ReturnStmt(value);
}

Statement *Parser::parseBlockStatement() {
  this->advance(); // consume '{'
  std::vector<Statement *> statements;

  while (this->current.type != TokenType::RightBrace &&
         this->current.type != TokenType::EndOfFile) {
    Statement *stmt = this->parseStatement(true);
    if (!stmt) {
      this->advance(); // skip erroneous token
      continue;
    }
    statements.push_back(stmt);
  }

  if (this->current.type != TokenType::RightBrace) {
    for (auto s : statements) {
      delete s;
    }
    return nullptr; // error
  }
  this->advance(); // consume '}'

  return new BlockStmt(statements);
}
