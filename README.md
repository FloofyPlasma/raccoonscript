<div align="center">

pretend i have a logo here k thanks bye

# Raccoon

A statically-typed, systems programming language with manual memory management, and a focus on simplicity. Uses [LLVM](https://github.com/llvm/llvm-project) as the backend.

[![MIT License](https://img.shields.io/badge/License-MIT-green.svg)](https://choosealicense.com/licenses/mit/) ![GitHub Issues or Pull Requests](https://img.shields.io/github/issues/floofyplasma/raccoonscript) ![GitHub Actions Workflow Status](https://img.shields.io/github/actions/workflow/status/floofyplasma/raccoonscript/ci.yml)

</div>

## Features

- Statically typed with explicit type annotations
- Manual memory management (`malloc`/`free`) with pointer support
- Structs with stack or heap allocation
- Module-based architecture with explicit exports/imports
- Recursion and zero-cost abstractions
- Cross-platform via LLVM (x86-64, ARM64)
- Simple syntax: no arrow operator (`->`), no garbage collection, no implicit type conversions

## Roadmap

**High Priority**
- Arrays and slices with indexing
- Pointer arithmetic
- Compound assignment operators (`+=`, `-=`, etc.)

**Medium Priority**
- Generics for functions and structs
- Built-in string type
- Standard library with features and stuff

**Low Priority**
- FFI/C interop
- Compile-time evaluation (`const` functions)
- CLI toolchain: package manager, formatter, linter

**Future Ideas**
- Inline assembly
- Enums and pattern matching
- Traits/interfaces for polymorphism
- Collections: Vec, HashMap, etc.

## Contributing

Contributions are welcome. Follow existing code style and project structure. A `CONTRIBUTING.md` will be added.

## Running Raccoon

Clone the project:

```bash
git clone https://github.com/floofyplasma/raccoon
cd raccoon
```

Build with CMake:

```bash
cmake -B build .
cmake --build build
```

Compile a module:

```bash
raccoonc main.rac -o program
```

Run the compiled binary:

```bash
./program
```
