#include "ModuleMetadata.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

void ModuleMetadata::saveToFile(const std::string &filepath) const {
  std::ofstream file(filepath);
  if (!file) {
    std::cerr << "Error: Cannot write metadata file: " << filepath << std::endl;
    return;
  }

  // Boring text format:
  // MODULE <name>
  // FUNCTION <name> <returnType> <paramCount>
  //   PARAM <name> <type>
  //   ...
  // STRUCT <name> <fieldCount>
  //   FIELD <name> <type>
  //   ...
  file << "MODULE " << moduleName << "\n";

  for (const auto &func : functions) {
    file << "FUNCTION " << func.name << " " << func.returnType << " "
         << func.params.size() << "\n";
    for (const auto &param : func.params) {
      file << "  PARAM " << param.first << " " << param.second << "\n";
    }
  }

  for (const auto &st : structs) {
    file << "STRUCT " << st.name << " " << st.fields.size() << "\n";
    for (const auto &field : st.fields) {
      file << "  FIELD " << field.first << " " << field.second << "\n";
    }
  }

  file.close();
}

ModuleMetadata ModuleMetadata::loadFromFile(const std::string &filepath) {
  ModuleMetadata metadata;
  std::ifstream file(filepath);

  if (!file) {
    std::cerr << "Error: Cannot read metadata file: " << filepath << "\n";
    return metadata;
  }

  std::string line;
  while (std::getline(file, line)) {
    std::istringstream iss(line);
    std::string keyword;
    iss >> keyword;

    if (keyword == "MODULE") {
      iss >> metadata.moduleName;
    } else if (keyword == "FUNCTION") {
      ExportedFunction func;
      int paramCount;
      iss >> func.name >> func.returnType >> paramCount;

      // Read parameters
      for (int i = 0; i < paramCount; ++i) {
        std::getline(file, line);
        std::istringstream paramIss(line);
        std::string paramKeyword, paramName, paramType;
        paramIss >> paramKeyword >> paramName >> paramType;
        if (paramKeyword == "PARAM") {
          func.params.push_back({paramName, paramType});
        }
      }

      metadata.functions.push_back(func);
    } else if (keyword == "STRUCT") {
      ExportedStruct st;
      int fieldCount;
      iss >> st.name >> fieldCount;

      // Read fields
      for (int i = 0; i < fieldCount; ++i) {
        std::getline(file, line);
        std::istringstream fieldIss(line);
        std::string fieldKeyword, fieldName, fieldType;
        fieldIss >> fieldKeyword >> fieldName >> fieldType;
        if (fieldKeyword == "FIELD") {
          st.fields.push_back({fieldName, fieldType});
        }
      }

      metadata.structs.push_back(st);
    }
  }

  file.close();
  return metadata;
}

const ExportedFunction *
ModuleMetadata::findFunction(const std::string &name) const {
  for (const auto &func : functions) {
    if (func.name == name) {
      return &func;
    }
  }
  return nullptr;
}

const ExportedStruct *
ModuleMetadata::findStruct(const std::string &name) const {
  for (const auto &st : structs) {
    if (st.name == name) {
      return &st;
    }
  }
  return nullptr;
}
