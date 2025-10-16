# Racoon Script Language Specification

## Syntax Overview

* **Statically typed**, with explicit types for variables and functions.
* **Manual memory management** via `malloc<T>(count)` and `free()`.
* **Function types** are first-class.
* **Block scoping** and **variable shadowing** allowed.
* **Each file is a module**; standard modules use `import std.name;`.
* **No garbage collector**, **no namespaces**, **no operator overloading**.

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
* `void`: Sugar for unit type `()`

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
fun add(i32 x, i32 y): i32 {
    return x + y;
}
```

### Lambdas

```racoon
let double = fun(i32 x) x * 2;
let verbose = fun(i32 x) {
    let y = x + 1;
    return y * y;
};
```

### First-Class Functions

```racoon
type Op = fun(i32, i32): i32;
fun apply(Op op, i32 a, i32 b): i32 {
    return op(a, b);
}
```

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

* No `foreach`.

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

### Const Field Rules

```racoon
const buf: Buffer = Buffer { data: malloc<u8>(1), len: 10 };
*buf.data = 42;    // OK
buf.len = 20;      // Error
```

* Fields of a `const` cannot be mutated unless they are pointers.

---

## Memory Management

```racoon
let p = malloc<i32>(1);
*p = 123;
free(p);
```

* No GC or ARC
* Use `malloc<T>(count)` and `free(ptr)` explicitly

---

## Closures

### Value Capture (Default)

```racoon
fun make_counter(): fun(): i32 {
    let ptr = malloc<i32>(1);
    *ptr = 0;
    return fun() {
        *ptr = *ptr + 1;
        return *ptr;
    };
}
```

### Reference Capture

```racoon
let code: i32;
get_error_code(fun&(x) code = x);
```

* `fun&(...)` captures by reference
* `fun&` closures **cannot escape** their defining scope

### Semantics

| Style  | Pointer Capture | Mutable | Can Escape |
| ------ | --------------- | ------- | ---------- |
| `fun`  | Yes (copy)      | No      | Yes        |
| `fun&` | Yes (borrow)    | Yes     | No         |

---

## Modules

Each `.rac` file is a module.

```racoon
// math.rac
export fun add(i32 x, i32 y): i32 {
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

## Namespaces and Currying

* No namespaces
* No currying

```racoon
fun add(i32 x, i32 y): i32 { return x + y; }
add(1); // Error

let add5 = fun(i32 y) add(5, y); // OK
```

---

## Void Type

```racoon
fun print_value(i32 x): void {
    print(x);
}
```

* `void` is sugar for the unit type `()`

---

## Example

```racoon
// math.rac
export fun make_adder(i32 x): fun(i32): i32 {
    return fun(i32 y) x + y;
}

export struct Vec2 {
    x: f32;
    y: f32;
}

export fun length(Vec2 v): f32 {
    return sqrt(v.x * v.x + v.y * v.y);
}
```

```racoon
// main.rac
import math;

fun main(): void {
    let add10 = math.make_adder(10);
    let result = add10(5);

    let v = math.Vec2 { x: 3.0, y: 4.0 };
    let len = math.length(v);

    if (len > 0.0) {
        print("Non-zero vector");
    }

    for (let i = 0; i < 3; i = i + 1) {
        print(result);
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
