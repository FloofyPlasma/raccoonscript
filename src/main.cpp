#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "AST.hpp"
#include "Codegen.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>

std::string getObjectFileName(const std::string &outputFile) {
  std::string objectFile = outputFile;

#ifdef _WIN32
  if (objectFile.size() < 4 ||
      (objectFile.substr(objectFile.size() - 4) != ".obj" &&
       objectFile.substr(objectFile.size() - 2) != ".o")) {
    objectFile += ".obj";
  }
#else
  if (objectFile.size() < 2 ||
      (objectFile.substr(objectFile.size() - 2) != ".o" &&
       objectFile.substr(objectFile.size() - 4) != ".obj")) {
    objectFile += ".o";
  }
#endif

  return objectFile;
}

std::string getExecutableFileName(const std::string &outputFile) {
  std::string execFile = outputFile;

#ifdef _WIN32
  if (execFile.size() < 4 || execFile.substr(execFile.size() - 4) != ".exe") {
    execFile += ".exe";
  }
#else
  if (execFile.size() < 2 || execFile.substr(execFile.size() - 2) == ".o") {
    execFile = execFile.substr(0, execFile.size() - 2);
  }
#endif

  return execFile;
}

std::string getDirectory(const std::string &filepath) {
  size_t lastSlash = filepath.find_last_of("/\\");
  if (lastSlash != std::string::npos) {
    return filepath.substr(0, lastSlash);
  }
  return ".";
}

std::vector<std::string>
extractImports(const std::vector<Statement *> &program) {
  std::vector<std::string> imports;

  for (auto *stmt : program) {
    if (auto *importDecl = dynamic_cast<ImportDecl *>(stmt)) {
      imports.push_back(importDecl->modulePath);
    }
  }

  return imports;
}

bool fileExists(const std::string &filePath) {
  std::ifstream file(filePath);
  return file.good();
}

std::string getMetadataPath(const std::string &sourceFile) {
  std::string path = sourceFile;
  size_t lastDot = path.find_last_of('.');
  if (lastDot != std::string::npos) {
    path = path.substr(0, lastDot);
  }
  return path + ".racm";
}

bool hasExports(const std::vector<Statement *> &program) {
  for (auto *stmt : program) {
    if (auto *funcDecl = dynamic_cast<FunctionDecl *>(stmt)) {
      if (funcDecl->isExported)
        return true;
    }
    if (auto *structDecl = dynamic_cast<StructDecl *>(stmt)) {
      if (structDecl->isExported)
        return true;
    }
  }
  return false;
}

bool emitObjectFile(llvm::Module *module, const std::string &filename) {
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmParsers();
  llvm::InitializeAllAsmPrinters();

  llvm::Triple targetTriple = llvm::Triple(llvm::sys::getDefaultTargetTriple());
  module->setTargetTriple(targetTriple);

  std::string error;
  const llvm::Target *target =
      llvm::TargetRegistry::lookupTarget(targetTriple, error);

  if (!target) {
    llvm::errs() << "Error: " << error << "\n";
    return false;
  }

  std::string cpu = "generic";
  std::string features = "";
  llvm::TargetOptions opt;
  llvm::TargetMachine *targetMachine = target->createTargetMachine(
      targetTriple, cpu, features, opt, llvm::Reloc::PIC_);

  if (!targetMachine) {
    llvm::errs() << "Error: Could not create target machine\n";
    return false;
  }

  module->setDataLayout(targetMachine->createDataLayout());

  std::error_code ec;
  llvm::raw_fd_ostream dest(filename, ec, llvm::sys::fs::OF_None);

  if (ec) {
    llvm::errs() << "Error: Could not open file: " << ec.message() << "\n";
    return false;
  }

  llvm::legacy::PassManager pass;
  if (targetMachine->addPassesToEmitFile(pass, dest, nullptr,
                                         llvm::CodeGenFileType::ObjectFile)) {
    llvm::errs() << "Error: Target machine can't emit a file of this type\n";
    return false;
  }

  pass.run(*module);
  dest.flush();

  std::cout << "Object file written to " << filename << "\n";

  delete targetMachine;
  return true;
}

bool linkExecutable(const std::string &objectFile,
                    const std::string &outputFile) {
  auto tryCommand = [](const std::string &cmd) -> bool {
#ifdef _WIN32
    int result = std::system((cmd + " >nul 2>nul").c_str());
#else
    int result = std::system((cmd + " >/dev/null 2>&1").c_str());
#endif
    return result == 0;
  };

  std::string linkCmd;

#ifdef _WIN32
  // Prefer clang
  if (tryCommand("clang.exe --version")) {
    linkCmd = "clang.exe " + objectFile + " -o " + outputFile;
  } else {
    linkCmd = "cl.exe /Fe:" + outputFile + " " + objectFile;
  }
#else
  if (tryCommand("clang --version")) {
    linkCmd = "clang " + objectFile + " -o " + outputFile;
  } else {
    linkCmd = "gcc " + objectFile + " -o " + outputFile;
  }
#endif

  std::cout << "Linking with command: " << linkCmd << "\n";
  int result = std::system(linkCmd.c_str());
  if (result != 0) {
    std::cerr << "Linking failed (exit code " << result << ")\n";
    return false;
  }

  std::cout << "Linked executable written to " << outputFile << "\n";
  return true;
}

int main(int argc, const char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: raccoonc <source_file> [options]\n"
              << "Options:\n"
              << "  -o <file>         Set output filename (default: a.out)\n"
              << "  --emit-llvm       Emit LLVM IR (.ll)\n"
              << "  --emit-object     Emit object file (.o), skip linking\n"
              << "  --no-link         Alias for --emit-object\n";
    return 1;
  }

  std::string sourceFile = argv[1];
  std::string outputFile = "a.out";
  bool emitLLVM = false;
  bool emitObject = false;
  bool noLink = false;

  for (int i = 2; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-o" && i + 1 < argc) {
      outputFile = argv[++i];
    } else if (arg == "--emit-llvm") {
      emitLLVM = true;
    } else if (arg == "--emit-object" || arg == "--no-link") {
      emitObject = true;
      noLink = true;
    }
  }

  std::ifstream file(sourceFile);
  if (!file) {
    std::cerr << "Cannot open file: " << sourceFile << '\n';
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
    } else {
      program.push_back(stmt);
    }
  }

  std::vector<std::string> imports = extractImports(program);
  std::string baseDir = getDirectory(sourceFile);

  for (const auto &import : imports) {
    std::string importSourceFile =
        baseDir == "." ? import + ".rac" : baseDir + "/" + import + ".rac";

    std::string metadataFile =
        baseDir == "." ? import + ".racm" : baseDir + "/" + import + ".racm";

    if (!fileExists(metadataFile)) {
      std::cerr << "Error: Module '" << import << "' not compiled\n";

      if (fileExists(importSourceFile)) {
        std::cerr << "File '" << importSourceFile << "' found but '"
                  << metadataFile << "' missing\n\n";
        std::cerr << "Run: " << argv[0] << " " << importSourceFile;

        std::string suggestedObj = import + ".o";
        std::cerr << " -o " << suggestedObj << "\n";
        std::cerr << "Then: " << argv[0] << " " << sourceFile << " -o "
                  << outputFile << " --no-link\n";
      } else {
        std::cerr << "Required file '" << importSourceFile << "' not found\n";
      }

      return 1;
    }
  }

  std::string moduleName = sourceFile;
  size_t lastSlash = moduleName.find_last_of("/\\");
  if (lastSlash != std::string::npos) {
    moduleName = moduleName.substr(lastSlash + 1);
  }
  size_t lastDot = moduleName.find_last_of('.');
  if (lastDot != std::string::npos) {
    moduleName = moduleName.substr(0, lastDot);
  }

  Codegen codegen(moduleName);
  codegen.setModuleName(moduleName);
  for (const auto &import : imports) {
    codegen.loadImport(import, baseDir);
  }

  codegen.generate(program);
  auto llvmModule = codegen.takeModule();

  std::string verifyError;
  llvm::raw_string_ostream verifyStream(verifyError);
  if (llvm::verifyModule(*llvmModule, &verifyStream)) {
    llvm::errs() << "Module verification failed:\n" << verifyError << "\n";
  }

  if (hasExports(program)) {
    ModuleMetadata metadata = codegen.getExportedSymbols();
    std::string metadataPath = getMetadataPath(sourceFile);
    metadata.saveToFile(metadataPath);
    std::cout << "Module metadata written to " << metadataPath << "\n";
  }

  if (emitLLVM) {
    std::error_code ec;
    llvm::raw_fd_ostream dest(outputFile, ec, llvm::sys::fs::OF_None);
    if (ec) {
      llvm::errs() << "Could not open file: " << ec.message() << "\n";
      return 1;
    }
    llvmModule->print(dest, nullptr);
    dest.flush();
    std::cout << "LLVM IR written to: " << outputFile << "\n";
  } else {
    std::string objFile = getObjectFileName(outputFile);
    if (!emitObjectFile(llvmModule.get(), objFile)) {
      return 1;
    }

    if (!noLink) {
      std::string execFile = getExecutableFileName(outputFile);
      if (!linkExecutable(objFile, execFile)) {
        return 1;
      }
    }
  }

  for (auto stmt : program) {
    delete stmt;
  }

  return 0;
}
