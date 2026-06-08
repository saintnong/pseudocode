<h1 align="center">SCSA Pseudocode Interpreter</h1>

<p align="center">
    <img src="https://img.shields.io/github/actions/workflow/status/SaintNong/pseudocode/ci.yml?branch=main&style=for-the-badge&logo=github&label=Build">
    <img src="https://img.shields.io/github/actions/workflow/status/SaintNong/pseudocode/ci.yml?branch=main&style=for-the-badge&logo=cplusplus&label=Tests">
    <img src="https://img.shields.io/badge/License-MIT-blue?style=for-the-badge">
    <img src="https://img.shields.io/github/v/release/SaintNong/pseudocode?style=for-the-badge&label=Version&color=orange">
</p>

<p align="center">
    SCSA Pseudocode is a high-level object-oriented interpreted programming language. <br>
    It is based on the WA School Curriculum and Standards Authority's ATAR Computer Science <a href="https://senior-secondary.scsa.wa.edu.au/__data/assets/pdf_file/0003/1090875/Year-11_12_Computer-Science_ATAR_Additional-syllabus-support-booklet-.PDF">"Pseudocode" (2024)</a>.
</p>

---

> "<em>This spec is so specific that it might as well be a real language...</em>"

This repository contains a full toolchain for Pseudocode development, including an optimized stack-based bytecode virtual machine (VM) and compiler written in C++, and a [VSCode extension](https://marketplace.visualstudio.com/items?itemName=SaintNong.scsa-pseudocode) with highlighting and snippets.

## Examples

#### Selection Sort in pseudocode (and syntax highlighting)!
![Selection sort in pseudocode](images/selection_sort.png)
![Selection sort output](images/selection_sort_result.png)
#### Integrated command line REPL
![Pseudocode REPL](images/REPL.png)

#### User-friendly error messages!
![Error message](images/error.png)

## Installation

- Follow the [Installation Guide](https://github.com/SaintNong/pseudocode/wiki/Installation-Guide) to get started with the interpreter!

- If you only want Syntax Highlighting/Snippets then check out the [VSCode Extension](https://marketplace.visualstudio.com/items?itemName=SaintNong.scsa-pseudocode) on the marketplace.

## Performance Benchmarks

To evaluate interpreter execution speed, there is an automated square matrix multiplication benchmark comparing SCSA Pseudocode against Python 3 and Node.js.

The bytecode virtual machine achieves a **4.5x speedup** over the legacy tree-walk interpreter.

The benchmark results are auto-generated on CI and can be viewed here:
- [Latest Benchmark Results](benchmarks/README.md)

To run the benchmarks locally:
```bash
python3 benchmarks/run_benchmarks.py
```

## Documentation

The language documentation and virtual machine design are split into separate files:

- 📖 [Supported Language Features](docs/language.md): Full list of supported statements, types, loops, arrays, dictionaries, and OOP structures.
- ⚙️ [Bytecode VM Architecture](docs/architecture.md): Details on compiler pipeline, opcodes, chunk layouts, stack design, and closures.

## Features
- Handwritten Lexer with locatable tokens
- Recursive descent parser for statements/blocks
- Pratt Parser for parsing expressions with operator precedence
- Optimized Stack-Based Bytecode VM & Compiler
  - Local variable stack management and call frame execution
  - Symmetric scope block cleanup in loops and conditional branches
  - Shared pointers for memory management
- Native Functions API
- Good error reporting anchored to nearest token
- Visual Studio Code Highlighting Extension
- Integration tests
- Circular inheritance check

## Integration Tests & Coverage
This project uses a small custom Python test harness integrated with CTest for CI/CD. Tests execute `.scsa` files and check that the interpreter output matches what is expected in the comments.

To run the integration tests:
1. Build the project ([see installation](https://github.com/SaintNong/pseudocode/wiki/Installation-Guide))
2. Run the tests from the `/tests` directory:
   ```bash
   python3 tester.py <path-to-your-binary>
   ```

### Measuring Code Coverage
To measure code coverage, you can use the coverage runner helper:
```bash
# From the repository root, runs tests with coverage instrumentation and generates a report
python3 tests/coverage.py
```

## Environment Variables
The interpreter supports the following environment variables:
- `NO_COLOR`: If set, suppresses all ANSI escape sequences for color output. Follows the [NO_COLOR](https://no-color.org) informal standard.

# Other stuff

## Future Plans
- Integrated file IO in standard library
- Subprocess in standard library
- Manual garbage collector (reference based)

## Credits
- Crafting Interpreters by Robert Nystrom
    - This book is an excellent start to writing interpreters, and was massively helpful for starting this project
- [tombl's scsa-pseudocode](https://github.com/tombl/scsa-pseudocode)
    - In highschool when my friends joked about making an interpreter, to our surprise some legend had already done it before, in Node.js of all languages!
    - This project is excellent, but is based on an older specification of pseudocode without OOP and when variable assignment was done with arrows

## Disclaimer
> [!NOTE]
> I am not affiliated with SCSA in any way.
> The SCSA logo is being used in this context in an educational setting without any commercial purpose (it's MIT Licensed). If someone at SCSA is unhappy with this usage they should contact me via email, or an issue on the attached GitHub repository and I will remove it immediately.
> All other material such as the language specification is derived from the Australian curriculum, and is thus under [CC4 BY](https://creativecommons.org/licenses/by/4.0/).
