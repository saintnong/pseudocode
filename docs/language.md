# Supported Pseudocode Language Features

SCSA Pseudocode is a high-level, dynamically typed, object-oriented language. Below is the documentation of all supported features.

## Data Types
- Basic datatypes and a dynamic typing system:
    - `int`: e.g. `42`
    - `float`: e.g. `3.14`
    - `string`: e.g. `"hello"`
    - `bool`: `TRUE` or `FALSE`
    - `Null`: Represents absence of value.
- Functions and class instances can be treated as first-class variables.

## Scopes
- Separation between local and global scopes.
- Lexical scoping for functions and closures.

## Standard Library Functions
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

## Basic Operations
- Binary operations/comparisons: `+`, `-`, `*`, `/`, `MOD`, `%`, `>`, `<`, `>=`, `<=`, `==`, `!=`
- Logical operators: `AND`, `OR`, `NOT`
- Modulo operations: `MOD` or `%` (computes remainder of division, supports both integers and floats)

## Control Flow
- **While loops**:
  ```
  WHILE condition
      # statements
  END WHILE
  ```
- **For-In loops**:
  ```
  FOR item IN list
      # statements
  END FOR
  ```
- **For-To loops**:
  ```
  FOR i = start TO end
      # statements
  END FOR
  ```
- **Repeat-Until loops**:
  ```
  REPEAT
      # statements
  UNTIL condition
  ```
- **If statements**:
  ```
  IF condition THEN
      # statements
  ELSE_IF condition THEN
      # statements
  ELSE
      # statements
  END IF
  ```
- **CASE statements**:
  ```
  CASE OF variable
      value1:
          # statements
      value2, value3:
          # statements
      OTHERWISE:
          # statements
  END CASE
  ```

## Functions
- Declared using the `FUNCTION` keyword and ended with `END <name>`.
- Supports parameters and `RETURN` statements.
- Example:
  ```
  FUNCTION add(a, b)
      RETURN a + b
  END add
  ```

## Strings
- `string[i]`: Indexes into a string (0-indexed).
- `string.length`: Returns the length of a string.
- `string.slice(a, b)`: Returns the substring from index a to b (inclusive).
- String multiplication is supported (e.g. `"abc" * 3` yields `"abcabcabc"`; 0 and negative multipliers return an empty string).

## Arrays
- `array[i]`: Indexes into an array.
- `array.slice(a, b)`: Returns a shallow copy from index a to b (inclusive).
- `array.append(x)`: Appends value to the end of the array.
- `array.length`: Returns length of the array as an integer.
- Array multiplication is supported (e.g. `[1, 2] * 2` yields `[1, 2, 1, 2]`; 0 and negative multipliers return an empty array).

## Dictionaries
- `{key: value, key2: value2}`: Creates a dictionary literal.
- `dict[key]`: Indexes into a dictionary by key (keys must be strings, integers, or booleans).
- `dict.length`: Returns the number of entries.
- `dict.keys()`: Returns an array of all keys.
- `dict.values()`: Returns an array of all values.
- `dict.get(key, default?)`: Returns a value or optional default.
- `key IN dict`: Checks dictionary membership.
- `FOR key IN dict`: Iterates over dictionary keys.

## Object-Oriented Programming (OOP)
- **Classes**: Defined using the `CLASS` keyword.
- **Attributes & Methods**: Referencing object attributes or methods requires using `this` (e.g., `this.name`).
- **Constructors**: Defined with the same name as the class.
- **Supercalls**: `this.super()` is used to call parent constructor/methods/fields (can be chained).
- **Inheritance**: Single inheritance supported via `CLASS Subclass INHERITS Superclass`.
- **Circular inheritance**: Checked and rejected at compile time.

## Error Handling

Errors are categorized by type, each with a unique error code and a dedicated exception class. Runtime errors include source-location underlines pointing to the relevant expression.

| Error Type | Code  | Exception Class | Thrown When |
|---|---|---|---|
| `SyntaxError` | `E001` | — | Invalid token sequences, malformed syntax, missing delimiters |
| `TypeError` | `E002` | `TypeError` | Type mismatches (e.g. wrong operand type for an operator, division by zero, circular inheritance) |
| `NameError` | `E003` | `NameError` | Undefined variables, properties, or class names |
| `ArgumentError` | `E004` | `ArgumentError` | Wrong number of arguments passed to a function or constructor |
| `IndexError` | `E005` | `IndexError` | Array/string index out of bounds, dictionary key not found |
| `VMError` | `E006` | `VMError` | Internal VM errors (stack overflow/underflow, unknown opcode) |
