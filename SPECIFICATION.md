# Racoon Script Language Specification

## Syntax Overview

* **Statically typed**, with explicit types for variables and functions.
* **Manual memory management** via `malloc<T>(count)` and `free(ptr)`.
* **Block scoping** and **variable shadowing** allowed.
* **Each file is a module**; standard modules use `import std.name;`.
* **No garbage collector**, **no namespaces**, **no operator overloading**.
* Strings are pointer types (`i8*`) or `char*`.

---

## Keywords

Reserved words:

```
fun let const struct return if else while for import export malloc free true false void
```

---

## Types

### Primitive Types

* `i8`, `i16`, `i32`, `i64`: Signed integers
* `u8`, `u16`, `u32`, `u64`: Unsigned integers
* `f32`, `f64`: Floating-point
* `bool`: Boolean
* `usize`: Unsigned pointer-sized integer
* `void`: Void as defined by LLVM

### Pointers

```racoon
ptr<T>        // pointer to T
*ptr          // dereference
&val          // address-of
```

---

## Variables

```racoon
let x: i32 = 5;       // mutable
const y: i32 = 10;    // immutable
```

* Type annotation is optional.
* Variables are block-scoped and can be shadowed.

```racoon
let x = 5;
{
    let x = 10; // shadows outer x
}
```

---

## Functions

### Declaration

```racoon
fun add(x: i32, y: i32): i32 {
    return x + y;
}
```

* Functions must be declared at the top level (not inside other functions or blocks).
* Function parameters and return types must be explicitly specified.
* Functions can call other functions defined in the same module or imported modules.

---

## Control Flow

### If / Else

```racoon
if (x > 0) {
    print("positive");
} else {
    print("non-positive");
}
```

* Conditions must be in parentheses.

### While

```racoon
while (x < 5) {
    x = x + 1;
}
```

### For

```racoon
for (let i = 0; i < 10; i = i + 1) {
    print(i);
}
```

---

## Structs

```racoon
struct Point {
    x: f32;
    y: f32;
}
```

* Stack or heap allocated
* Fields accessed with dot syntax (`p.x`)
* Const fields cannot be mutated unless pointers.

---

## Memory Management

```racoon
let p = malloc<i32>(1);
*p = 123;
free(p);
```

* Heap types must match pointer type (i32* to i32 memory).
* Use `malloc<T>(count)` and `free(ptr)` explicitly
* String literals are constant arrays `[X * i8]` with global pointers.

---

## Modules

Each `.rac` file is a module.

```racoon
// math.rac
export fun add(x: i32, y: i32): i32 {
    return x + y;
}

// main.rac
import math;
fun main(): void {
    let sum = math.add(2, 3);
}
```

* `export` exposes symbols to other modules
* `import` resolves relative to `main.rac`
* Standard modules: `import std.math;`

---

## Namespaces

* No namespaces - use module prefixes for organization

---

## Void Type

```racoon
fun print_value(x: i32): void {
    print(x);
}
```

* `void` is sugar for the unit type `()`

---

## Example

```racoon
// math.rac
export struct Vec2 {
    x: f32;
    y: f32;
}

export fun length(v: Vec2): f32 {
    return sqrt(v.x * v.x + v.y * v.y);
}

export fun add_vectors(a: Vec2, b: Vec2): Vec2 {
    return Vec2 { x: a.x + b.x, y: a.y + b.y };
}
```

```racoon
// main.rac
import math;

fun main(): void {
    let v1 = math.Vec2 { x: 3.0, y: 4.0 };
    let v2 = math.Vec2 { x: 1.0, y: 2.0 };
    
    let len = math.length(v1);
    let sum = math.add_vectors(v1, v2);

    if (len > 0.0) {
        print("Non-zero vector");
    }

    for (let i = 0; i < 3; i = i + 1) {
        print(sum.x);
    }
}
```

---

## Potential Features

* Traits / Interfaces
* Enums and Pattern Matching
* Error Handling
* Arrays, Slices, Collections
* CLI Toolchain
* Pointer Arithmetic (TBD)
* LLVM + C Interop
