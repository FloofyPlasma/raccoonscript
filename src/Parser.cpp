#include "Parser.hpp"
#include "AST.hpp"
#include "Token.hpp"
#include <iostream>
#include <string>

void Parser::advance() { this->current = this->lexer.nextToken(); }

Statement *Parser::parseStatement(bool insideFunction) {
  if (this->current.type == TokenType::Keyword &&
      (this->current.lexeme == "let" || this->current.lexeme == "const")) {
    bool isConst = (this->current.lexeme == "const");

    this->advance(); // consume 'let' or 'const'

    return this->parseVarDecl(isConst);
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

Statement *Parser::parseVarDecl(bool isConst) {
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
    while (this->current.type == TokenType::Star) {
      type += "*";
      this->advance(); // consume star
    }
  }

  Expr *initializer = nullptr;
  if (this->current.type == TokenType::Equal) {
    this->advance(); // consume '='
    initializer = this->parseExpression();
    if (!initializer) {
      std::cerr << "Failed to parse initializer for variable " << name << "\n";
      return nullptr;
    }

    // Log for debugging
    if (auto *call = dynamic_cast<CallExpr *>(initializer)) {
      std::cerr << "Parsed CallExpr for " << name << " to function "
                << call->name << " with typeArg: " << call->type << "\n";
    }
  }

  if (this->current.type != TokenType::Semicolon) {
    delete initializer;
    return nullptr; // error
  }

  this->advance(); // consume ';'

  return new VarDecl{name, type, initializer, isConst};
}

Statement *Parser::parseFunctionDecl() {
  // Consume 'fun'
  this->advance();
  if (this->current.type != TokenType::Identifier) {
    return nullptr;
  } // missing function name

  std::string name = this->current.lexeme;
  this->advance(); // consume function name

  if (this->current.type != TokenType::LeftParen) {
    return nullptr;
  } // missing '('
  this->advance(); // consume '('

  std::vector<std::pair<std::string, std::string>> params;

  // Parse zero or more parameters
  while (this->current.type != TokenType::RightParen &&
         this->current.type != TokenType::EndOfFile) {
    if (this->current.type != TokenType::Identifier) {
      return nullptr;
    } // expected parameter name
    std::string paramName = this->current.lexeme;
    this->advance(); // consume parameter name

    if (this->current.type != TokenType::Colon) {
      return nullptr;
    } // expected ':'
    this->advance(); // consume ':'

    if (this->current.type != TokenType::Identifier) {
      return nullptr;
    } // expected type
    std::string paramType = this->current.lexeme;
    this->advance(); // consume type

    // Handle pointer stars
    while (this->current.type == TokenType::Star) {
      paramType += "*";
      this->advance();
    }

    params.push_back({paramName, paramType});

    if (this->current.type == TokenType::Comma) {
      this->advance();
    } // consume ',' and continue
    else {
      break;
    }
  }

  if (this->current.type != TokenType::RightParen) {
    return nullptr;
  } // missing ')'
  this->advance(); // consume ')'

  // Parse optional return type
  std::string returnType = "void";
  if (this->current.type == TokenType::Colon) {
    this->advance(); // consume ':'
    if (this->current.type != TokenType::Identifier &&
        this->current.type != TokenType::Keyword) {
      return nullptr;
    }
    returnType = this->current.lexeme;
    this->advance(); // consume return type
  }

  // Parse function body
  if (this->current.type != TokenType::LeftBrace) {
    return nullptr;
  } // missing '{'
  this->advance(); // consume '{'

  std::vector<Statement *> body;
  while (this->current.type != TokenType::RightBrace &&
         this->current.type != TokenType::EndOfFile) {
    Statement *stmt = this->parseStatement(true);
    if (!stmt) {
      this->advance(); // skip bad token
      continue;
    }
    body.push_back(stmt);
  }

  if (this->current.type != TokenType::RightBrace) {
    return nullptr;
  } // missing '}'
  this->advance(); // consume '}'

  return new FunctionDecl(name, params, body, returnType);
}

Expr *Parser::parseExpression(int precedence) {
  Expr *left = this->parseUnary();
  if (!left) {
    return nullptr; // error
  }

  while (true) {
    // Stop at semicolons, braces, parentheses, commas
    if (current.type == TokenType::Semicolon ||
        current.type == TokenType::RightParen ||
        current.type == TokenType::RightBrace ||
        current.type == TokenType::Comma ||
        current.type == TokenType::EndOfFile)
      break;

    // Only handle operators relevant for expressions
    int opPrecedence = this->getPrecedence(current.type);
    if (opPrecedence < precedence)
      break;

    TokenType op = current.type;
    this->advance(); // consume operator
    Expr *right = parseExpression(opPrecedence + 1);
    if (!right)
      break;
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
  if (this->current.type == TokenType::FloatLiteral) {
    float value = std::stof(this->current.lexeme);
    this->advance(); // consume float literal
    return new FloatLiteral(value);
  }
  if (this->current.type == TokenType::Keyword &&
      (this->current.lexeme == "true" || this->current.lexeme == "false")) {
    bool value = (this->current.lexeme == "true");
    this->advance(); // consume boolean literal
    return new BoolLiteral(value);
  }
  if (this->current.type == TokenType::CharLiteral) {
    char value = this->current.lexeme[0];
    this->advance(); // consume char literal
    return new CharLiteral(value);
  }
  if (this->current.type == TokenType::StringLiteral) {
    std::string value = this->current.lexeme;
    this->advance(); // consume string literal
    return new StrLiteral(value);
  }
  if (this->current.type == TokenType::Identifier) {
    std::string name = this->current.lexeme;

    // Do not advance if at top-level and current token is 'fun'
    if (!(this->current.type == TokenType::Keyword &&
          this->current.lexeme == "fun")) {
      this->advance();
    }

    if (this->current.type == TokenType::LeftParen) {
      this->advance();
      std::vector<Expr *> args;
      while (this->current.type != TokenType::RightParen &&
             this->current.type != TokenType::EndOfFile) {
        Expr *arg = this->parseExpression();
        if (!arg)
          return nullptr;
        args.push_back(arg);
        if (this->current.type == TokenType::Comma)
          this->advance();
      }
      if (this->current.type != TokenType::RightParen) {
        for (auto a : args)
          delete a;
        return nullptr;
      }
      this->advance();
      return new CallExpr(name, args, "");
    }

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
  if (current.type == TokenType::Keyword &&
      (current.lexeme == "malloc" || current.lexeme == "free")) {
    std::string name = current.lexeme;
    this->advance(); // consume keyword

    std::string typeArg;
    if (current.type == TokenType::LessThan) {
      this->advance();
      if (current.type != TokenType::Identifier)
        return nullptr;
      typeArg = current.lexeme;
      this->advance();
      if (current.type != TokenType::GreaterThan)
        return nullptr;
      this->advance();
    }

    if (current.type != TokenType::LeftParen)
      return nullptr;
    this->advance();
    std::vector<Expr *> args;
    while (current.type != TokenType::RightParen &&
           current.type != TokenType::EndOfFile) {
      Expr *arg = parseExpression();
      if (!arg)
        return nullptr;
      args.push_back(arg);
      if (current.type == TokenType::Comma)
        advance();
    }
    if (current.type != TokenType::RightParen)
      return nullptr;
    advance();
    return new CallExpr(name, args, typeArg);
  }

  return nullptr;
}

int Parser::getPrecedence(TokenType type) {
  switch (type) {
  case TokenType::Star:
  case TokenType::Slash:
  case TokenType::Percent:
    return 30;
  case TokenType::Plus:
  case TokenType::Minus:
    return 20;
  case TokenType::LessThan:
  case TokenType::LessEqual:
  case TokenType::GreaterThan:
  case TokenType::GreaterEqual:
    return 10;
  case TokenType::DoubleEqual:
  case TokenType::BangEqual:
    return 9;
  case TokenType::AndAnd:
    return 6;
  case TokenType::OrOr:
    return 5;
  case TokenType::Equal:
    return 2;
  default:
    return -1;
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
  switch (this->current.type) {
  case TokenType::IntLiteral:
  case TokenType::FloatLiteral:
  case TokenType::CharLiteral:
  case TokenType::StringLiteral:
  case TokenType::Identifier:
    return this->parsePrimary();
  default:
    return nullptr;
  }
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

  if (this->current.type != TokenType::RightParen) {
    delete condition;
    return nullptr; // error
  }
  this->advance(); // consume ')'

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

  if (this->current.type != TokenType::RightParen) {
    delete condition;
    return nullptr; // error
  }
  this->advance(); // consume ')'

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
  if (this->current.type == TokenType::Keyword &&
      this->current.lexeme == "let") {
    this->advance();
    initializer = this->parseVarDecl(false);
  } else if (this->current.type == TokenType::Keyword &&
             this->current.lexeme == "const") {
    this->advance();
    initializer = this->parseVarDecl(true);
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
  if (this->current.type != TokenType::Semicolon &&
      this->current.type != TokenType::RightBrace) {
    value = this->parseExpression();
    if (!value) {
      this->advance();
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

Expr *Parser::parseUnary() {
  if (this->current.type == TokenType::Minus ||
      this->current.type == TokenType::Bang ||
      this->current.type == TokenType::Ampersand ||
      this->current.type == TokenType::Star) {
    TokenType op = this->current.type;
    this->advance(); // consume unary operator
    Expr *operand = this->parseUnary();
    if (!operand) {
      return nullptr;
    }

    return new UnaryExpr(op, operand);
  }

  return this->parsePrimary();
}
