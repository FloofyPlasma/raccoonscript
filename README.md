
<div align="center">

pretend i have a logo here k thanks bye

# Raccoon Script

A programming language I guess. It uses [LLVM](https://github.com/llvm/llvm-project) on the backend.


[![MIT License](https://img.shields.io/badge/License-MIT-green.svg)](https://choosealicense.com/licenses/mit/) ![GitHub Issues or Pull Requests](https://img.shields.io/github/issues/floofyplasma/raccoonscript)

</div>

## Features

- WIP
- Compiles on pretty much every architecture that LLVM supports.
- Probably supports WASM.


## Roadmap

- A stdlib.
- Support for imports/exports
- Something else probably this is very incomplete.

## Contributing

Contributions are always welcome!

I'll eventually setup a `CONTRIBUTING.md` but yeah just try your best to fit existing stuff.
## Running Raccoon Script

Clone the project

```bash
  git clone https://github.com/floofyplasma/raccoonscript
```

Go to the project directory

```bash
  cd raccooncript
```

Build with CMake

```bash
  cmake -B build .
```

Compile a script to LLVM IR

```bash
  raccoonc codegen.rac > codegen.ll
```

Compile the LLVM IR to a binary

```bash
  clang codegen.ll -o codegen
```

Profit ???
