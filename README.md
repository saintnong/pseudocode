<h1 align="center">SCSA Pseudocode Interpreter</h1>

<p align="center">
    <img src="https://img.shields.io/github/actions/workflow/status/SaintNong/pseudocode/ci.yml?branch=main&style=for-the-badge&logo=github&label=Build">
    <img src="https://img.shields.io/badge/License-MIT-blue?style=for-the-badge">
    <img src="https://img.shields.io/github/v/release/SaintNong/pseudocode?style=for-the-badge&label=Version&color=orange">
</p>

<p align="center">
    Pseudocode is a high-level interpreted language with support for procedural and object oriented programming (WIP).
    Based on the WA School Curriculum and Standards Authority's (SCSA) ATAR Computer Science <a href="https://senior-secondary.scsa.wa.edu.au/__data/assets/pdf_file/0003/1090875/Year-11_12_Computer-Science_ATAR_Additional-syllabus-support-booklet-.PDF">"Pseudocode" (2024)</a>.
</p>

---

> "<em>This spec is so specific that it might as well be a real language...</em>"


## Features
- Handwritten Lexer with locatable tokens
- Recursive descent parser for statements/blocks
- Pratt Parser for parsing expressions with operator precedence
- Working tree walk interpreter
    - Shared pointers for garbage collection
    - Variable scopes
- Good error reporting which is anchored to nearest token for easy debugging
### Supported Pseudocode Language Features
- Basic datatypes and a dynamic typing system
    - [int, float, string, bool, Null]
    - Functions and instances as variables
- Local/Global scope separation
- PRINT library function
- Binary operations/comparisons [+, -, *, /, >, <, >=, <=, ==]
- While and For-in loops
- If & If-Else statements
- Functions
- Lists (Append and length is WIP)

## Requirements
- A C++ compiler with support for C++17
- Terminal with ANSI colour support (Not required but highly recommended)
- Operating systems tested:
    - Ubuntu 24.04
    - Windows 11

## Currently WIP Features
- ✨✨Object Oriented Programming✨✨
    - This was a new, painful recent addition to the SCSA pseudocode "standard", when they decided that their ancient Pascal based pseudocode had to be more **"modern"**
    - Class lexing and parsing is mostly working, but interpreter does not interpret binding functions/variables to class declarations correctly yet
- Case statement
- List appending
- ELSE IF statements
- Better string manipulation

## Future Plans
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