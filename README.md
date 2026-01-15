<h1 align="center">SCSA Pseudocode Interpreter</h1>

<p align="center">
    <img src="https://img.shields.io/github/actions/workflow/status/SaintNong/pseudocode/ci.yml?branch=main&style=for-the-badge&logo=github&label=Build">
    <img src="https://img.shields.io/badge/License-MIT-blue?style=for-the-badge">
    <img src="https://img.shields.io/github/v/release/SaintNong/pseudocode?style=for-the-badge&label=Version&color=orange">
</p>

<p align="center">
    Pseudocode is a high-level interpreted language with support for procedural and object oriented programming.
    Based on the WA School Curriculum and Standards Authority's (SCSA) ATAR Computer Science <a href="https://senior-secondary.scsa.wa.edu.au/__data/assets/pdf_file/0003/1090875/Year-11_12_Computer-Science_ATAR_Additional-syllabus-support-booklet-.PDF">"Pseudocode" (2024)</a>.
</p>

---

> "<em>This spec is so specific that it might as well be a real language...</em>"

This repository contains a full toolchain for Pseudocode development, including an interpreter, VSCode extension with highlighting and snippets.

## Examples

#### Working Bubble Sort in SCSA Pseudocode (and syntax highlights)!
![Bubble Sort in Pseudocode](images/bubblesort.png)
#### Integrated REPL
![Pseudocode REPL](images/REPL.png)

#### Good error messages!
![Error message](images/error.png)

## Installation

Follow our [Installation Guide](https://github.com/SaintNong/pseudocode/wiki/Installation-Guide) to get started!

## Features
- Handwritten Lexer with locatable tokens
- Recursive descent parser for statements/blocks
- Pratt Parser for parsing expressions with operator precedence
- Working tree walk interpreter
    - Shared pointers for garbage collection
    - Variable scopes
- Native Functions API
- Good error reporting which is anchored to nearest token for easy debugging
- Visual Studio Code Highlighting Extension
### Supported Pseudocode Language Features
- Basic datatypes and a dynamic typing system
    - [int, float, string, bool, Null]
    - Functions and instances as variables
- Local/Global scope separation
- Standard Library Functions:
    - `PRINT(...args)`: Prints values followed by a newline.
    - `OUTPUT(...args)`: Prints values without a trailing newline.
    - `INPUT(prompt?)`: Reads a line of input from the user.
    - `INT(value)`: Converts a value to an integer.
    - `FLOAT(value)`: Converts a value to a float.
    - `STRING(value)`: Converts a value to its string representation.
    - `BOOL(value)`: Converts a value to a boolean.
    - `RANDOM(min, max)`: Returns a random integer between min and max.
    - `TIME()`: Returns current system time in seconds.
    - `TYPE(value)`: Returns the type of the value as a string.
- Binary operations/comparisons [+, -, *, /, >, <, >=, <=, ==, !=]
- Logical operators [AND, OR, NOT]
- While and For-in loops
- If & If-Else statements
- Functions
- Lists (Append and length is WIP)
- ✨✨Object Oriented Programming✨✨
    - Most features working, but the syntax requires 'this' to reference object attributes/methods
    - No inheritance and polymorphism available as of now

# Other stuff
## Currently WIP Features
- ✨✨Object Oriented Programming✨✨
    - This was a new, painful recent addition to the SCSA pseudocode "standard", when they decided that their ancient Pascal based pseudocode had to be more **"modern"**
    - Polymorphism and inheritance are WIP
- Case statement
- List appending & length
- ELSE IF statements
- Better string manipulation

## Environment Variables
The interpreter supports the following environment variables:
- `NO_COLOR`: If set, suppresses all ANSI escape sequences for color output. Follows the [NO_COLOR](https://no-color.org) informal standard.

## Future Plans
- Unit and integration testing with CI/CD
- Custom Bytecode VM which requires:
    - Custom stack-based bytecode
    - Bytecode compiler
    - Bytecode virtual machine
    - Some bytecode optimisation
- Manual garbage collector (reference based)

## Credits
- Crafting Interpreters by Robert Nystrom
    - This book is an excellent start to writing interpreters, and was massively helpful for starting this project
- [tombl's scsa-pseudocode](https://github.com/tombl/scsa-pseudocode)
    - In highschool when my friends joked about making an interpreter, to our surprise some legend had already done it before, in Node.js of all languages!
    - This project is excellent, but is based on an older specification of pseudocode without OOP and when variable assignment was done with arrows