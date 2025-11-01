# Raccoon Language Specification v1.0

## Overview
Raccoon is a statically-typed, systems programming language with manual memory management and a focus on simplicity and performance.

### Core Principles
* **Statically typed** with explicit type annotations
* **Manual memory management** (no garbage collector)
* **Block scoping** with variable shadowing support
* **Module-based** architecture (one module per file)
* **No runtime overhead** from complex language features
* **LLVM backend** for native code generation

---

## Keywords
Reserved words that cannot be used as identifiers:
```
fun let const struct return if else while for 
import export malloc free true false void extern
```

---

## Type System

### Primitive Types
| Type | Description | Size |
|------|-------------|------|
| `i8`, `i16`, `i32`, `i64` | Signed integers | 8, 16, 32, 64 bits |
| `u8`, `u16`, `u32`, `u64` | Unsigned integers | 8, 16, 32, 64 bits |
| `f32`, `f64` | Floating-point numbers | 32, 64 bits |
| `bool` | Boolean (`true` or `false`) | stored as i8 |
| `usize` | Unsigned pointer-sized integer | Platform-dependent |
| `void` | Unit type (no value) | 0 bits |

### Pointer Types
```raccoon
T*            // Pointer to type T
*ptr          // Dereference operator
&val          // Address-of operator
```

**Examples:**
```raccoon
let x: i32 = 42;
let p: i32* = &x;
let value: i32 = *p;  // value = 42
```

### Type Aliases (Future Feature)
```raccoon
type String = i8*;
type CharPtr = i8*;
```

---

## Variables and Constants

### Variable Declaration
```raccoon
let x: i32 = 5;           // Mutable variable with explicit type
let y: i32 = 10;          // Explicit type required
const PI: f32 = 3.14159;  // Immutable constant
```

### Scoping Rules
* Variables are **block-scoped**
* **Shadowing is allowed** within nested scopes
* **Constants cannot be reassigned** after initialization

**Example:**
```raccoon
let x: i32 = 5;
{
    let x: i32 = 10;    // Shadows outer x
}
```

---

## Functions

### Function Declaration
```raccoon
fun add(x: i32, y: i32): i32 {
    return x + y;
}

fun greet(name: i8*): void {
    print(name);
}
```

### Rules
* Functions must be declared at **top level** (no nested functions)
* **Parameter types** and **return type** must be explicitly specified
* Functions without a return value use `void`
* Early return is supported with `return` keyword
* Non-exported functions are internal to the module

### Function Calls
```raccoon
let sum: i32 = add(5, 10);
greet("Hello, Raccoon!");
```

---

## Control Flow

### Conditional Statements
```raccoon
if (x > 0) {
    print("positive");
} else {
    if (x < 0) {
        print("negative");
    } else {
        print("zero");
    }
}
```

* Conditions must be enclosed in parentheses
* Braces are required for blocks

### While Loops
```raccoon
let i: i32 = 0;
while (i < 5) {
    print(i);
    i = i + 1;
}
```

### For Loops
```raccoon
for (let i: i32 = 0; i < 10; i = i + 1) {
    print(i);
}
```

* Initialization, condition, and increment are required
* Loop variable is scoped to the loop body
* Type must be specified in initialization

---

## Structs

### Definition
```raccoon
struct Point {
    x: f32;
    y: f32;
}

struct Node {
    value: i32;
    next: Node*;
}
```

### Usage
```raccoon
// Stack allocation
let p: Point = Point { x: 1.0, y: 2.0 };

// Heap allocation
let node: Node* = malloc<Node>(1);
(*node).value = 42;
(*node).next = malloc<Node>(0);

// Field access
let x_coord: f32 = p.x;
(*node).value = 100;

free(node);
```

### Rules
* Structs can be allocated on **stack** or **heap**
* Fields are accessed with **dot notation** (`.`)
* Pointer-to-struct fields require explicit dereference: `(*ptr).field`
* Structs can contain pointers to themselves (for linked structures)

---

## Memory Management

### Manual Allocation
```raccoon
// Allocate single value
let p: i32* = malloc<i32>(1);
*p = 123;
free(p);

// Allocate struct
let node: Node* = malloc<Node>(1);
(*node).value = 50;
free(node);

// Allocate zero-initialized (null pointer)
let next: Node* = malloc<Node>(0);
```

### Rules
* **No garbage collection** - manual `free()` required
* **Type safety** - allocated type must match pointer type
* **Dangling pointers** - programmer's responsibility to avoid
* **String literals** are `i8*` pointers to constant data
* `malloc<T>(0)` creates a null pointer

### Best Practices
```raccoon
fun process_data(): void {
    let data: i32* = malloc<i32>(1);
    // ... use data ...
    free(data);  // Always free before returning
}
```

---

## Modules and Imports

### Module Structure
Each `.rac` file is a module. The filename becomes the module name.

**math.rac:**
```raccoon
// Basic math module for testing
export fun add(x: i32, y: i32): i32 {
    return x + y;
}

export fun multiply(x: i32, y: i32): i32 {
    return x * y;
}

export fun subtract(x: i32, y: i32): i32 {
    return x - y;
}

// Non-exported helper (should not be accessible)
fun internal_helper(x: i32): i32 {
    return x + 1;
}
```

**main.rac:**
```raccoon
import math;

fun main(): i32 {
    let result: i32 = math.add(5, 3);
    return result;
}
```

### Import Rules
* `export` makes functions/structs visible to other modules
* `import module_name;` imports a module
* Module members are accessed via dot notation: `module.symbol`
* Import paths are resolved relative to `main.rac`
* Standard library: `import std.io;`, `import std.math;`

### No Namespace Pollution
* No `using` or wildcard imports
* All imported symbols require module prefix
* Prevents naming conflicts

---

## Operators

### Arithmetic
```raccoon
+  -  *  /  %  (add, subtract, multiply, divide, modulo)
```

### Comparison
```raccoon
==  !=  <  >  <=  >=
```

### Logical
```raccoon
&&  ||  !  (and, or, not)
```

### Assignment
```raccoon
=  (assignment only - no compound assignments)
```

### Pointer Operations
```raccoon
&  *  (address-of, dereference)
```

---

## Complete Examples

### Example 1: Calculator Module
**calculator.rac:**
```raccoon
// Calculator module demonstrating more complex logic
export fun divide(a: i32, b: i32): i32 {
    if (b == 0) {
        return 0;
    }
    return a / b;
}

export fun modulo(a: i32, b: i32): i32 {
    if (b == 0) {
        return 0;
    }
    return a % b;
}

export fun is_even(n: i32): i32 {
    if (n % 2 == 0) {
        return 1;
    }
    return 0;
}

export fun power_of_two(n: i32): i32 {
    let result: i32 = 1;
    for (let i: i32 = 0; i < n; i = i + 1) {
        result = result * 2;
    }
    return result;
}
```

**main.rac:**
```raccoon
import calculator;

fun main(): i32 {
    let div: i32 = calculator.divide(20, 5);   // 4
    let mod: i32 = calculator.modulo(17, 5);   // 2
    let even: i32 = calculator.is_even(10);    // 1
    let pow: i32 = calculator.power_of_two(3); // 8
    
    if (div == 4) {
        if (mod == 2) {
            if (even == 1) {
                return pow;
            }
        }
    }
    
    return 0;
}
```

### Example 2: Linked List Node
**data.rac:**
```raccoon
// Module with pointers and heap allocation
export struct Node {
    value: i32;
    next: Node*;
}

export fun create_node(val: i32): Node* {
    let n: Node* = malloc<Node>(1);
    (*n).value = val;
    (*n).next = malloc<Node>(0);
    return n;
}

export fun get_value(n: Node*): i32 {
    return (*n).value;
}

export fun set_value(n: Node*, val: i32): void {
    (*n).value = val;
}

export fun free_node(n: Node*): void {
    free(n);
}
```

**main.rac:**
```raccoon
import data;

fun main(): i32 {
    let node: data.Node* = data.create_node(50);
    
    data.set_value(node, 100);
    
    let val: i32 = data.get_value(node);
    
    data.free_node(node);
    
    return val;  // Returns 100
}
```

### Example 3: Geometry Module
**geometry.rac:**
```raccoon
// Geometry module with struct exports
export struct Point {
    x: f32;
    y: f32;
}

export struct Rect {
    width: f32;
    height: f32;
}

export fun area(r: Rect): f32 {
    return r.width * r.height;
}

export fun distance_squared(p1: Point, p2: Point): f32 {
    let dx: f32 = p2.x - p1.x;
    let dy: f32 = p2.y - p1.y;
    return dx * dx + dy * dy;
}

export fun make_point(x: f32, y: f32): Point {
    return Point { x: x, y: y };
}
```

**main.rac:**
```raccoon
import geometry;

fun main(): i32 {
    let p1: geometry.Point = geometry.make_point(0.0, 0.0);
    let p2: geometry.Point = geometry.make_point(3.0, 4.0);
    
    let dist_sq: f32 = geometry.distance_squared(p1, p2);
    
    let rect: geometry.Rect = geometry.Rect { width: 10.0, height: 5.0 };
    let area: f32 = geometry.area(rect);
    
    return 0;
}
```

### Example 4: Recursion
**main.rac:**
```raccoon
fun factorial(n: i32): i32 {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

fun main(): i32 {
    return factorial(5);  // 5! = 120
}
```

### Example 5: Local Functions
**main.rac:**
```raccoon
fun max(a: i32, b: i32): i32 {
    if (a > b) {
        return a;
    }
    return b;
}

fun main(): i32 {
    let x: i32 = max(3, 1);
    let y: i32 = max(2, 5);
    
    if (x == 3) {
        if (y == 5) {
            return 3;
        }
    }
    
    return 0;
}
```

---

## Standard Library (Planned)

### std.io
```raccoon
import std.io;

fun main(): void {
    std.io.print("Hello");
    std.io.println("World");
}
```

### std.math
```raccoon
import std.math;

fun calculate(): f32 {
    return std.math.sqrt(16.0);
}
```

---

## Future Features (Roadmap)

### High Priority
* **Arrays and Slices** - first-class array types with indexing
* **Pointer Arithmetic** - for low-level operations and manual array access
* **Compound Assignment** - `+=`, `-=`, `*=`, etc.

### Medium Priority
* **Generics** - type-parameterized functions and structs
* **String Type** - built-in string handling

### Low Priority
* **FFI/C Interop** - calling C libraries
* **Compile-time Evaluation** - const functions
* **CLI Toolchain** - package manager, formatter, linter

### Potential Ideas
* **Inline Assembly** - for performance-critical code
* **Enums and Pattern Matching** - for safer state representation
* **Traits/Interfaces** - for polymorphism
* **Collections** - Vec, HashMap, etc.

---

## Design Decisions

### What Raccoon Has
✓ Static typing (explicit annotations required)  
✓ Manual memory management  
✓ Structs with associated functions  
✓ Module system with explicit exports  
✓ Pointers and explicit dereferencing  
✓ Zero-cost abstractions  
✓ Recursion support  
✓ Self-referential structs (Node*)  

### What Raccoon Doesn't Have (Yet)
✗ Arrays (planned - high priority)  
✗ Arrow operator `->` (use explicit dereference)  
✗ Compound assignment operators  
✗ Type conversions/casting (LLVM handles internally when needed)  
✗ Garbage collection  
✗ Operator overloading  
✗ Inheritance  
✗ Exceptions  
✗ Reflection  
✗ Macros  
✗ Implicit type conversions  

---

## Grammar (Simplified BNF)

BNF (Backus-Naur Form) is a formal notation for describing the syntax rules of Raccoon. Each line defines how language constructs are formed.

### Notation
* `::=` means "is defined as"
* `|` means "or" (alternative)
* `*` means "zero or more"
* `?` means "optional"
* `( )` groups items together
* `" "` indicates literal keywords

### Raccoon Grammar
```bnf
program        ::= (import | function | struct | export)*
import         ::= "import" identifier ";"
export         ::= "export" (function | struct)
function       ::= "fun" identifier "(" params ")" ":" type block
struct         ::= "struct" identifier "{" fields "}"
params         ::= (identifier ":" type ("," identifier ":" type)*)?
fields         ::= (identifier ":" type ";")*
block          ::= "{" statement* "}"
statement      ::= var_decl | assignment | return | if | while | for | expr ";"
var_decl       ::= ("let" | "const") identifier ":" type "=" expr ";"
assignment     ::= identifier "=" expr ";"
return         ::= "return" expr? ";"
if             ::= "if" "(" expr ")" block ("else" (if | block))?
while          ::= "while" "(" expr ")" block
for            ::= "for" "(" var_decl expr ";" assignment ")" block
expr           ::= literal | identifier | call | binary_op | unary_op
type           ::= primitive | identifier | type "*"
primitive      ::= "i32" | "f32" | "bool" | "void" | ...
```

---

## Compiler Implementation Notes

### Target Backend
* **LLVM** - primary code generation target
* **Target platforms**: x86-64, ARM64

### Compilation Phases
1. **Lexical Analysis** - tokenization
2. **Parsing** - AST generation
3. **Semantic Analysis** - name resolution and type checking
4. **IR Generation** - LLVM IR emission
5. **Optimization** - LLVM optimization passes
6. **Code Generation** - native machine code

### Type Checking Rules
* All types must be explicitly declared (no type inference)
* All types must be known at compile time
* Types must match exactly - mixing types in operations is not allowed
* Pointer types must match allocation types
* Function signatures must match at call sites
* Module-qualified types must match import declarations
* LLVM handles any necessary low-level conversions internally

### Pointer Syntax
* Use `T*` for pointer types (e.g., `i32*`, `Node*`)
* Always use explicit dereference `(*ptr).field` for struct access
* No arrow operator `->` in current version

---

## Notes on Language Design

### Why "Raccoon"?
The double-C in Raccoon emphasizes the language's connection to C and compiled systems programming, while maintaining a friendly, approachable identity.

### Why Explicit Types?
Raccoon requires explicit type annotations for clarity and to simplify the compiler implementation. This makes code more readable and reduces compilation time. Types must match exactly - you cannot mix `i32` and `i64`, or `i32` and `f32` in operations. LLVM handles any necessary low-level conversions internally.

### Why Manual Memory Management?
Manual memory management gives programmers full control over performance and resource usage, making Raccoon suitable for systems programming and embedded development.

### Why No Arrow Operator?
Currently, Raccoon requires explicit dereference `(*ptr).field` instead of `ptr->field` to keep the syntax simple and explicit. This may be added as syntactic sugar in future versions.

### Module System Philosophy
Each file is a module to keep the project structure simple and predictable. Export/import are explicit to prevent namespace pollution and make dependencies clear.

---

## License
MIT License

Copyright (c) 2025 FloofyPlasma

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

---

## Version History
* **v1.0** - Initial specification (2025)
