# SCSA Pseudocode Interpreter (Very WIP as of now)
Pseudocode, based on the Western Australian ATAR Computer Science "Pseudocode" specification, which is so specific that it might as well be a real langauge.

Well guess what? It's real now, and it's horrifying. This is your fault SCSA, you bought this unto yourselves.

# Features
- Handwritten Lexer
- Pratt Parser w/ Operator precedence
- Tree walker interpreter
    - Uses shared pointers for garbage collection (slightly cursed)
- While and For-in loops
- If statements
- Functions
- Lists (append does not exist for now)

# WIP
- Object Oriented Programming✨
    - This was a new, painful, stupid addition to the SCSA pseudocode "standard", when they decided that their ancient Pascal based pseudocode had to be more like a real langauge like python
    - Classes technically exist, as in they won't error out until you try access any properties of them
- Lists appending

# Future Planned features
- Bytecode VM
- Actual Garbage Collector

