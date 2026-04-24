<div align="center">
 
              __/)
           .-(__(=:
        |\ |    \)
        \ ||
         \||
          \|
           |
 
An elegant, dynamically typed educational programming language.
 
[![C++17](https://img.shields.io/badge/C++-17-blue.svg?style=flat-square)](#)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg?style=flat-square)](#)
[![Interpreter: AST](https://img.shields.io/badge/Architecture-AST--Walker-success.svg?style=flat-square)](#)
 
</div>
 
 
**lilies** is a lightweight, dynamically typed programming language built from scratch in modern C++17. It features a clean C-style syntax, first-class functions, lexical scoping (closures), and automatic memory management.
 
Instead of relying on clunky manual memory management, lilies utilizes a modern AST-walking architecture powered by `std::variant` and `std::shared_ptr` to ensure a safe, clean, and highly readable codebase.
 
## Features
 
* **Clean Syntax**: Inspired by C, JavaScript, and Rust.
* **Dynamic Typing**: Numbers, Strings, Booleans, Functions, and `nil`.
* **First-Class Functions**: Pass functions around as variables and return them.
* **Lexical Scoping**: Full support for closures and nested environments.
* **Memory Safe**: Zero raw pointers. Completely garbage-collected via modern C++ smart pointers.
* **Interactive REPL**: Read-Eval-Print Loop for quick testing and prototyping.
 
## Getting Started
 
### Prerequisites
You need a modern C++ compiler that supports **C++17** (like GCC, Clang, or MSVC).
 
### Building from Source
 
1. Clone the repository:
   ```bash
   git clone https://github.com/anelya0333/liliescpp
   cd lilies
   ```
 
2. Compile the interpreter with optimizations:
   ```bash
   g++ -std=c++17 -O3 -Wall lilies.cpp -o lilies
   ```
 
### Running lilies
 
To start the interactive REPL, simply run the compiled executable:
```bash
./lilies
```
 
*(You can also run files by piping them into the interpreter: `./lilies < script.lil`)*
  
## Language Tour
 
Here is a quick overview of what lilies can do!
 
### 1. Variables and Types
Variables are declared using the `let` keyword. lilies supports numbers (doubles), strings, booleans, and `nil`.
 
```javascript
let greeting = "Hello, World!";
let age = 25;
let isProgrammer = true;
let emptyValue = nil;
 
print greeting;
```
 
### 2. Math & Logic
lilies supports standard arithmetic operations and string concatenation.
 
```javascript
let math = (10 + 5) * 2 / 3;
let concatenated = "lilies is " + "awesome!";
 
print math;
print concatenated;
```
 
### 3. Control Flow
Standard `if` / `else` logic and `while` loops are fully supported.
 
```javascript
let count = 5;
while (count > 0) {
    print count;
    let count = count - 1;
    if (count == 0) {
        print "Liftoff!";
    }
}
```
 
### 4. Functions & Recursion
Functions are declared using the `fn` keyword. They can take multiple arguments and return values.
 
```javascript
fn fib(n) {
    if (n <= 1) { 
        return n; 
    }
    return fib(n - 1) + fib(n - 2);
}
 
print "Fibonacci of 10 is:";
print fib(10);
```
 
### 5. Closures & Lexical Scoping
lilies respects scope. Inner functions can capture and remember variables from the block they were defined in.
 
```javascript
fn makeCounter() {
    let i = 0;
    fn count() {
        let i = i + 1;
        print i;
    }
    return count;
}
 
// Support for returning and executing closures 
// (Currently variables hold the function, execution requires standard calls)
```
 
## Architecture Under the Hood
 
For those interested in language design, lilies is built to be an educational reference for modern C++ techniques:
 
* **Values**: Implemented using `std::variant<std::nullptr_t, bool, double, std::string, std::shared_ptr<Callable>>`. No unions or void pointers.
* **Garbage Collection**: Replaced traditional Mark-and-Sweep with C++ `std::shared_ptr`. Environments and AST nodes are automatically cleaned up when they fall out of scope.
* **Error Handling**: Uses C++ Exceptions to cleanly unwind the call stack during runtime errors and `return` statements.
 
## License
 
This project is open-source and available under the MIT License.