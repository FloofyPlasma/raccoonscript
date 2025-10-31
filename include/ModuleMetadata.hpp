#pragma once

#include <string>
#include <unordered_map>
#include <vector>

struct ExportedFunction {
  std::string name;
  std::vector<std::pair<std::string, std::string>> params; // (name, type)
  std::string returnType;
};

struct ExportedStruct {
  std::string name;
  std::vector<std::pair<std::string, std::string>> fields; // (name, type)
};

struct ModuleMetadata {
  std::string moduleName;
  std::vector<ExportedFunction> functions;
  std::vector<ExportedStruct> structs;

  void saveToFile(const std::string &filepath) const;
  static ModuleMetadata loadFromFile(const std::string &filepath);
  const ExportedFunction *findFunction(const std::string &name) const;
  const ExportedStruct *findStruct(const std::string &name) const;
};
