#pragma once

#include "AST.hpp"
#include "Lexer.hpp"
#include "Token.hpp"

class Parser {
    Lexer& lexer;
    Token current;

    public:
    Parser(Lexer& l) : lexer(l) {this->advance();}

    void advance();

    Expr* parseExpression();

    Statement* parseVarDecl();
};
