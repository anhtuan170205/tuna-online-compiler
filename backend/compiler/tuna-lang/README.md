# Tuna Language

**Author:** Tuan Tran Anh

**Tuna** is a small interpreted programming language implemented in **C** using **Flex** (lexer) and **Bison** (parser).

---

## Requirements
- **C compiler**: `gcc`
- **Build system**: `make`
- **Lexer generator**: `flex`
- **Parser generator**: `bison`
- **Operating system**: Linux or macOS

On macOS
``` 
brew install gcc flex bison
```

On Linux
``` 
sudo apt install gcc make flex bison
```

---

## Project Structure 
```
tuna-lang/
├── src/
│ ├── main.c
│ ├── ast.c
│ ├── env.c
│ ├── interp.c
│ ├── tuna.l # Flex lexer
│ └── tuna.y # Bison parser
│
├── include/
│ ├── ast.h
│ ├── env.h
│ ├── interp.h
│ ├── runtime.h
│ └── value.h
│
├── tests/ 
│ ├── black_jack.tuna
│ ├── game_of_life.tuna
│ ├── test.tuna
│ ├── test_func.tuna
│ └── run_length_decoder.tuna
│
├── build/ 
├── Makefile
└── README.md
```

---

## Build
From the project root
```
make
```

This produces the executable:

```
./tuna
```

To clean generated files:
```
make clean
```

---

## Run Tests
Run a test by number
```
./tuna 1
```

Available test mappings:
| Number | File                          |
| -----: | ----------------------------- |
|      1 | tests/black_jack.tuna         |
|      2 | tests/game_of_life.tuna       |
|      3 | tests/language_demo.tuna      |
|      4 | tests/test_func.tuna          |
|      5 | tests/run_length_decoder.tuna |

Running without arguments prints this list automatically.

Run a specific `.tuna` file
```
./tuna tests/game_of_life.tuna
```

--- 

## Implementation Overview
- **Lexer:** `src/tuna.l` (Flex)
Splits the source code into tokens (identifiers, keywords, literals, operators) and passes them to the parser.
- **Parser:** `src/tuna.y` (Bison, generates `tuna.tab.c` and `tuna.tab.h`)
Reads the token stream, checks it against the grammar rules, and builds the program structure.
- **AST:** `include/ast.h`, `src/ast.c`
Defines the Abstract Syntax Tree (AST) node types (statements/expressions) and provides constructors/helpers to create and manage AST nodes produced by the parser.
- **Environment/Scope:** `include/env.h`, `src/env.c`
Stores variable bindings (name -> value) and handles scope rules 
- **Interpreter:** `include/interp.h`, `src/interp.c`
Traverses the AST and executes it: evaluates expressions, runs statements, updates the environment, and produces results.
- **Runtime Error Checker:** `include/runtime.h`
Centralizes runtime error reporting. The interpreter calls this when it detects an execution-time violation.

---

## Compiler Pipeline Diagram
```
.tuna source
   |
   v
[Lexer: Flex]  (src/tuna.l)
   |
   v
[Parser: Bison] (src/tuna.y)  ---> generates tuna.tab.c / tuna.tab.h
   |
   v
[AST] (ast.c / ast.h)
   |
   v
[Interpreter] (interp.c)
   |        \
   |         -> [Runtime Error Checker] (runtime.h)
   |
   -> [Environment / Scope] (env.c / env.h)
   |
   v
Output / Exit
```

## Tests 
All `.tuna` files in `tests/` are **language test programs**
They are used to validate parsing, semantic correctness, and runtime error handling.

Tests can be executed individually or via numeric selection.

## Future Improvements
- Dynamic data structures (lists, dictionaries, tuples)
- Enhanced control flow (`for ... in ...`, `break`, `continue`)
- Function extensions (default parameters, named arguments)
- Basic standard library (`len`, `type`, `range`, `print`)
- Improved runtime error messages (line/column, error categories)

## License
This project is for educational purposes only.