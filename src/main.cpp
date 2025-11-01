#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "AST.hpp"
#include "Codegen.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"

namespace fs = std::filesystem;

#include <llvm/Analysis/CGSCCPassManager.h>
#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>

struct CompilerOptions {
  std::vector<std::string> sourceFiles;
  std::string outputFile = "a.out";
  std::string targetTriple = llvm::sys::getDefaultTargetTriple();
  bool bareMetal = false;
  bool emitLLVM = false;
  bool emitObject = false;
  bool noLink = false;
  int optLevel = 0;
  bool generateDebugInfo = false;
  bool verbose = false;
  bool quiet = false;
  bool forceRecompile = false;
};

std::string getObjectFileName(const std::string &outputFile) {
  fs::path p(outputFile);

  if (p.extension() == ".o" || p.extension() == ".obj") {
    return outputFile;
  }

#ifdef _WIN32
  p.replace_extension(".obj");
#else
  p.replace_extension(".o");
#endif

  return p.string();
}

std::string getExecutableFileName(const std::string &outputFile) {
  fs::path p(outputFile);

  if (p.extension() == ".o" || p.extension() == ".obj") {
    p = p.stem();
  }

#ifdef _WIN32
  if (p.extension() != ".exe") {
    p.replace_extension(".exe");
  }
#endif

  return p.string();
}

std::string getDirectory(const std::string &filepath) {
  fs::path p(filepath);
  if (p.has_parent_path()) {
    return p.parent_path().string();
  }
  return ".";
}

std::string getBaseName(const std::string &filepath) {
  fs::path p(filepath);
  return p.stem().string();
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

bool fileExists(const std::string &filePath) { return fs::exists(filePath); }

std::string getMetadataPath(const std::string &sourceFile) {
  fs::path p(sourceFile);
  p.replace_extension(".racm");
  return p.string();
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

long getFileModificationTime(const std::string &filePath) {
  try {
    auto ftime = fs::last_write_time(filePath);
    return std::chrono::duration_cast<std::chrono::seconds>(
               ftime.time_since_epoch())
        .count();
  } catch (const fs::filesystem_error &) {
    return 0;
  }
}

bool needsRecompilation(const std::string &sourceFile,
                        const std::string &objectFile, bool forceRecompile) {
  if (forceRecompile)
    return true;
  if (!fileExists(objectFile))
    return true;

  long sourceTime = getFileModificationTime(sourceFile);
  long objectTime = getFileModificationTime(objectFile);

  return sourceTime > objectTime;
}

void log(const CompilerOptions &opts, const std::string &message) {
  if (!opts.quiet) {
    std::cout << message << "\n";
  }
}

void logVerbose(const CompilerOptions &opts, const std::string &message) {
  if (opts.verbose && !opts.quiet) {
    std::cout << "[VERBOSE] " << message << "\n";
  }
}

bool emitObjectFile(llvm::Module *module, const std::string &filename,
                    const CompilerOptions &opts) {
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmParsers();
  llvm::InitializeAllAsmPrinters();

  llvm::Triple targetTriple(opts.targetTriple);
  if (opts.bareMetal) {
    module->setTargetTriple(llvm::Triple("x86_64-pc-none-elf"));
  } else {
    module->setTargetTriple(targetTriple);
  }

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

  llvm::CodeGenOptLevel codegenOptLevel;
  switch (opts.optLevel) {
  case 0:
    codegenOptLevel = llvm::CodeGenOptLevel::None;
    break;
  case 1:
    codegenOptLevel = llvm::CodeGenOptLevel::Less;
    break;
  case 2:
    codegenOptLevel = llvm::CodeGenOptLevel::Default;
    break;
  case 3:
    codegenOptLevel = llvm::CodeGenOptLevel::Aggressive;
    break;
  default:
    codegenOptLevel = llvm::CodeGenOptLevel::None;
    break;
  }

  llvm::TargetMachine *targetMachine = nullptr;
  if (opts.bareMetal) {
    targetMachine = target->createTargetMachine(targetTriple, cpu, features,
                                                opt, llvm::Reloc::Static,
                                                std::nullopt, codegenOptLevel);
  } else {
    targetMachine = target->createTargetMachine(targetTriple, cpu, features,
                                                opt, llvm::Reloc::PIC_,
                                                std::nullopt, codegenOptLevel);
  }

  if (!targetMachine) {
    llvm::errs() << "Error: Could not create target machine\n";
    return false;
  }

  module->setDataLayout(targetMachine->createDataLayout());

  if (opts.optLevel > 0) {
    logVerbose(opts, "Applying optimization passes (level " +
                         std::to_string(opts.optLevel) + ")");

    llvm::LoopAnalysisManager LAM;
    llvm::FunctionAnalysisManager FAM;
    llvm::CGSCCAnalysisManager CGAM;
    llvm::ModuleAnalysisManager MAM;

    llvm::PassBuilder PB;
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    llvm::ModulePassManager MPM;

    switch (opts.optLevel) {
    case 1:
      MPM = PB.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O1);
      break;
    case 2:
      MPM = PB.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O2);
      break;
    case 3:
      MPM = PB.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O3);
      break;
    default:
      MPM = PB.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O0);
      break;
    }

    MPM.run(*module, MAM);
  }

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

  log(opts, "Object file written to " + filename);

  delete targetMachine;
  return true;
}

bool linkExecutable(const std::vector<std::string> &objectFiles,
                    const std::string &outputFile,
                    const CompilerOptions &opts) {
  auto tryCommand = [](const std::string &cmd) -> bool {
#ifdef _WIN32
    int result = std::system((cmd + " >nul 2>nul").c_str());
#else
    int result = std::system((cmd + " >/dev/null 2>&1").c_str());
#endif
    return result == 0;
  };

  std::string objects;
  for (const auto &obj : objectFiles) {
    objects += obj + " ";
  }

  std::string linkCmd;

#ifdef _WIN32
  if (tryCommand("clang.exe --version")) {
    linkCmd = "clang.exe " + objects + "-o " + outputFile;
  } else {
    linkCmd = "link.exe /OUT:" + outputFile + " " + objects;
  }
#else
  if (tryCommand("clang --version")) {
    linkCmd = "clang " + objects + "-o " + outputFile;
  } else {
    linkCmd = "gcc " + objects + "-o " + outputFile;
  }
#endif

  logVerbose(opts, "Linking with command: " + linkCmd);
  int result = std::system(linkCmd.c_str());
  if (result != 0) {
    std::cerr << "Linking failed (exit code " << result << ")\n";
    return false;
  }

  log(opts, "Linked executable written to " + outputFile);
  return true;
}

struct CompilationUnit {
  std::string sourceFile;
  std::string objectFile;
  std::string moduleName;
  std::vector<std::string> imports;
  std::vector<Statement *> program;
  bool compiled = false;
  bool isImported = false;
};

bool compileModule(CompilationUnit &unit, const CompilerOptions &opts,
                   std::map<std::string, CompilationUnit> &allUnits);

bool loadAndParseSource(CompilationUnit &unit, const CompilerOptions &opts) {
  std::ifstream file(unit.sourceFile);
  if (!file) {
    std::cerr << "Cannot open file: " << unit.sourceFile << '\n';
    return false;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string source = buffer.str();

  Lexer lexer(source);
  Parser parser(lexer);

  std::vector<std::string> errors;

  while (parser.current.type != TokenType::EndOfFile) {
    Statement *stmt = parser.parseStatement(false);
    if (!stmt) {

      std::string error =
          "Parse error at line " + std::to_string(parser.current.line);
      errors.push_back(error);
      parser.advance();
      continue;
    }
    unit.program.push_back(stmt);
  }

  if (!errors.empty()) {
    std::cerr << "Compilation of '" << unit.sourceFile << "' failed with "
              << errors.size() << " error(s):\n";
    for (const auto &err : errors) {
      std::cerr << "  " << err << "\n";
    }
    return false;
  }

  unit.imports = extractImports(unit.program);
  return true;
}

bool compileModule(CompilationUnit &unit, const CompilerOptions &opts,
                   std::map<std::string, CompilationUnit> &allUnits) {
  if (unit.compiled) {
    return true;
  }

  logVerbose(opts, "Compiling module: " + unit.moduleName);

  // Always parse the source to discover imports, even if we don't need to
  // recompile
  if (unit.program.empty()) {
    if (!loadAndParseSource(unit, opts)) {
      return false;
    }
  }

  fs::path baseDir = fs::path(unit.sourceFile).parent_path();
  if (baseDir.empty()) {
    baseDir = ".";
  }

  // Process dependencies first
  for (const auto &import : unit.imports) {
    fs::path importSourceFile = baseDir / (import + ".rac");
    fs::path importObjFile = baseDir / (import + ".o");

    if (allUnits.find(import) != allUnits.end()) {
      if (!compileModule(allUnits[import], opts, allUnits)) {
        return false;
      }
    } else {
      // Check if we need to compile this dependency
      // Even if metadata exists, we need the object file
      if (fs::exists(importSourceFile)) {
        logVerbose(opts, "Auto-compiling dependency: " + import);

        CompilationUnit depUnit;
        depUnit.sourceFile = importSourceFile.string();
        depUnit.objectFile = importObjFile.string();
        depUnit.moduleName = import;
        depUnit.isImported = true;

        allUnits[import] = depUnit;

        if (!compileModule(allUnits[import], opts, allUnits)) {
          std::cerr << "Error: Failed to compile dependency '" << import
                    << "'\n";
          return false;
        }
      } else {
        std::cerr << "Error: Module '" << import << "' not found\n";
        std::cerr << "Required file '" << importSourceFile.string()
                  << "' does not exist\n";
        return false;
      }
    }
  }

  // Now check if we need to actually compile this module
  if (!needsRecompilation(unit.sourceFile, unit.objectFile,
                          opts.forceRecompile)) {
    logVerbose(opts, "Skipping " + unit.sourceFile + " (up to date)");
    unit.compiled = true;
    return true;
  }

  log(opts, "Compiling " + unit.sourceFile + "...");

  Codegen codegen(unit.moduleName);
  codegen.setModuleName(unit.moduleName);

  for (const auto &import : unit.imports) {
    fs::path importBaseDir = fs::path(unit.sourceFile).parent_path();
    if (importBaseDir.empty()) {
      importBaseDir = ".";
    }
    codegen.loadImport(import, importBaseDir.string());
  }

  codegen.generate(unit.program);
  auto llvmModule = codegen.takeModule();

  std::string verifyError;
  llvm::raw_string_ostream verifyStream(verifyError);
  if (llvm::verifyModule(*llvmModule, &verifyStream)) {
    llvm::errs() << "Module verification failed for " << unit.sourceFile
                 << ":\n"
                 << verifyError << "\n";
    return false;
  }

  if (hasExports(unit.program)) {
    ModuleMetadata metadata = codegen.getExportedSymbols();
    std::string metadataPath = getMetadataPath(unit.sourceFile);
    metadata.saveToFile(metadataPath);
    logVerbose(opts, "Module metadata written to " + metadataPath);
  }

  if (!emitObjectFile(llvmModule.get(), unit.objectFile, opts)) {
    return false;
  }

  unit.compiled = true;
  return true;
}

void printUsage(const char *progName) {
  std::cerr
      << "Usage: " << progName << " <source_file> [options]\n"
      << "Options:\n"
      << "  -o <file>         Set output filename (default: a.out)\n"
      << "  --emit-llvm       Emit LLVM IR (.ll)\n"
      << "  --emit-object     Emit object file (.o), skip linking\n"
      << "  --no-link         Alias for --emit-object\n"
      << "  -O0, -O1, -O2, -O3  Set optimization level (default: -O0)\n"
      << "  -g                Generate debug information (not implemented)\n"
      << "  -v, --verbose     Enable verbose output\n"
      << "  -q, --quiet       Suppress non-error output\n"
      << "  -f, --force       Force recompilation of all files\n"
      << "  --help            Display this help message\n";
}

bool parseArguments(int argc, const char *argv[], CompilerOptions &opts) {
  if (argc < 2) {
    printUsage(argv[0]);
    return false;
  }

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (arg == "--help") {
      printUsage(argv[0]);
      return false;
    } else if (arg == "-o" && i + 1 < argc) {
      opts.outputFile = argv[++i];
    } else if (arg == "--emit-llvm") {
      opts.emitLLVM = true;
    } else if (arg == "--emit-object" || arg == "--no-link") {
      opts.emitObject = true;
      opts.noLink = true;
    } else if (arg == "-O0") {
      opts.optLevel = 0;
    } else if (arg == "-O1") {
      opts.optLevel = 1;
    } else if (arg == "-O2") {
      opts.optLevel = 2;
    } else if (arg == "-O3") {
      opts.optLevel = 3;
    } else if (arg == "-g") {
      opts.generateDebugInfo = true;
    } else if (arg == "-v" || arg == "--verbose") {
      opts.verbose = true;
    } else if (arg == "-q" || arg == "--quiet") {
      opts.quiet = true;
    } else if (arg == "-f" || arg == "--force") {
      opts.forceRecompile = true;
    } else if (arg[0] != '-') {
      opts.sourceFiles.push_back(arg);
    } else if (arg == "--target" && i + 1 < argc) {
      opts.targetTriple = argv[++i];
      if (opts.targetTriple == "x86_64-bios") {
        opts.bareMetal = true;
      }
    } else {
      std::cerr << "Unknown option: " << arg << "\n";
      printUsage(argv[0]);
      return false;
    }
  }

  if (opts.sourceFiles.empty()) {
    std::cerr << "Error: No source files specified\n";
    printUsage(argv[0]);
    return false;
  }

  if (opts.bareMetal) {
    log(opts, "[INFO] BIOS target detected; skipping host linker.");
    log(opts, "       Use ld -T linker.ld -nostdlib -o kernel.elf ...");
    opts.noLink = true;
  }

  return true;
}

int main(int argc, const char *argv[]) {
  CompilerOptions opts;

  if (!parseArguments(argc, argv, opts)) {
    return 1;
  }

  std::map<std::string, CompilationUnit> allUnits;
  std::vector<std::string> objectFiles;

  if (opts.emitLLVM) {
    if (opts.sourceFiles.size() > 1) {
      std::cerr << "Error: --emit-llvm only supports single source file\n";
      return 1;
    }

    std::string sourceFile = opts.sourceFiles[0];
    CompilationUnit unit;
    unit.sourceFile = sourceFile;
    unit.moduleName = getBaseName(sourceFile);

    if (!loadAndParseSource(unit, opts)) {
      return 1;
    }

    fs::path baseDir = fs::path(sourceFile).parent_path();
    if (baseDir.empty()) {
      baseDir = ".";
    }

    Codegen codegen(unit.moduleName);
    codegen.setModuleName(unit.moduleName);

    for (const auto &import : unit.imports) {
      codegen.loadImport(import, baseDir.string());
    }

    codegen.generate(unit.program);
    auto llvmModule = codegen.takeModule();

    std::string verifyError;
    llvm::raw_string_ostream verifyStream(verifyError);
    if (llvm::verifyModule(*llvmModule, &verifyStream)) {
      llvm::errs() << "Module verification failed:\n" << verifyError << "\n";
      return 1;
    }

    std::error_code ec;
    llvm::raw_fd_ostream dest(opts.outputFile, ec, llvm::sys::fs::OF_None);
    if (ec) {
      llvm::errs() << "Could not open file: " << ec.message() << "\n";
      return 1;
    }
    llvmModule->print(dest, nullptr);
    dest.flush();
    log(opts, "LLVM IR written to: " + opts.outputFile);

    for (auto stmt : unit.program) {
      delete stmt;
    }

    return 0;
  }

  for (const auto &sourceFile : opts.sourceFiles) {
    if (!fileExists(sourceFile)) {
      std::cerr << "Error: Source file '" << sourceFile << "' not found\n";
      return 1;
    }

    CompilationUnit unit;
    unit.sourceFile = sourceFile;
    unit.moduleName = getBaseName(sourceFile);

    if (opts.sourceFiles.size() == 1 && !opts.noLink) {
      unit.objectFile = getObjectFileName(opts.outputFile);
    } else {
      unit.objectFile = unit.moduleName + ".o";
    }

    allUnits[unit.moduleName] = unit;
  }

  for (auto &pair : allUnits) {
    if (!compileModule(pair.second, opts, allUnits)) {

      for (auto &unitPair : allUnits) {
        for (auto stmt : unitPair.second.program) {
          delete stmt;
        }
      }
      return 1;
    }
  }

  // Collect ALL object files from all compiled units (including dependencies)
  for (const auto &pair : allUnits) {
    if (pair.second.compiled && fileExists(pair.second.objectFile)) {
      objectFiles.push_back(pair.second.objectFile);
    }
  }

  if (!opts.noLink) {
    std::string execFile = getExecutableFileName(opts.outputFile);
    if (!linkExecutable(objectFiles, execFile, opts)) {

      for (auto &unitPair : allUnits) {
        for (auto stmt : unitPair.second.program) {
          delete stmt;
        }
      }
      return 1;
    }
  }

  for (auto &unitPair : allUnits) {
    for (auto stmt : unitPair.second.program) {
      delete stmt;
    }
  }

  return 0;
}
