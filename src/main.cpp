#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "AST.hpp"
#include "ASTPrinter.hpp"
#include "Codegen.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"

int main(int argc, const char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage racoonc <source_file>\n";
    return 1;
  }

  std::ifstream file(argv[1]);
  if (!file) {
    std::cerr << "Cannot open file: " << argv[1] << '\n';
    return 1;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string source = buffer.str();

  Lexer lexer(source);
  Parser parser(lexer);

  std::vector<Statement *> program;
  while (parser.current.type != TokenType::EndOfFile) {
    Statement *stmt = parser.parseStatement(false);
    if (!stmt) {
      parser.advance();
      continue;
    }
    program.push_back(stmt);
  }

  //   std::cout << "AST:\n";
  //   for (auto stmt : program) {
  //     printStatement(stmt);
  //   }

  //   std::cout << "\n\n====================\nBeginning "
  //                "codegen...\n====================\n\n";
  Codegen codegen("RaccoonModule");
  codegen.generate(program);

  auto llvmModule = codegen.takeModule();

  llvmModule->print(llvm::outs(), nullptr);

  // Cleanup
  for (auto stmt : program) {
    delete stmt;
  }

  return 0;
}
