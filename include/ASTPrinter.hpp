#pragma once

#include "AST.hpp"

void printStatement(Statement *stmt, int indent = 0);
void printExpr(Expr *expr, int indent = 0);
void printIfStmt(IfStmt *ifs, int indent = 0);
void printWhileStmt(WhileStmt *whiles, int indent = 0);
void printForStmt(ForStmt *fors, int indent = 0);
