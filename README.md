<h1 align="center">SCSA Pseudocode Interpreter</h1>

<p align="center">
  <a href="https://github.com/SaintNong/pseudocode/actions/workflows/ci.yml">
    <img src="https://img.shields.io/github/actions/workflow/status/SaintNong/pseudocode/ci.yml?branch=main&style=for-the-badge&logo=github&label=Build">
  </a>
  <a href="https://opensource.org/licenses/MIT">
    <img src="https://img.shields.io/badge/License-MIT-blue?style=for-the-badge">
  </a>
</p>

<p align="center">
  Based on the Western Australian School Curriculum and Standards Authority's (SCSA) ATAR Computer Science <a href="https://senior-secondary.scsa.wa.edu.au/__data/assets/pdf_file/0003/1090875/Year-11_12_Computer-Science_ATAR_Additional-syllabus-support-booklet-.PDF">"Pseudocode" specification (2024)</a>.
</p>

---

<em>This spec is so specific that it might as well be a real language...</em>


## Features
- Handwritten Lexer
- Recursive descent parser for statements/blocks
- Pratt Parser for parsing expressions with operator precedence
- Working tree walk interpreter
    - Shared pointers for garbage collection
    - Variable scopes
- Language features implemented
    - Basic datatypes and a dynamic typing system
        - [int, float, string, bool, Null]
    - While and For-in loops
    - If statements
    - Functions
    - Lists (Append is WIP)

## Currently WIP Features
- ✨✨Object Oriented Programming✨✨
    - This was a new, painful recent addition to the SCSA pseudocode "standard", when they decided that their ancient Pascal based pseudocode had to be more **"modern"**
    - Class lexing and parsing is mostly working, but interpreter does not interpret binding functions/variables to class declarations correctly yet
- Case statement
- List appending
- Better string manipulation

## Future Plans
- Custom Bytecode VM:
    - Custom stack-based bytecode
    - Bytecode compiler
    - Bytecode virtual machine
    - Some bytecode optimisation
- Manual garbage collector (reference based)

## Credits
- Crafting Interpreters by Robert Nystrom
    - This book is an excellent start to writing interpreters, and was massively helpful for this project
- [tombl's scsa-pseudocode](https://github.com/tombl/scsa-pseudocode)
    - In highschool when my friends joked about making an interpreter, to our surprise some legend had already done it before, in Node.js of all languages!
    - This project is excellent, but is based on an older specification of pseudocode without OOP and when variable assignment was done with arrows