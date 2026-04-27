# MiniPTX Compiler

A domain-specific compiler that translates a C-like GPU kernel language (`.ptxc`) into NVIDIA PTX assembly. Built from scratch in C++17 with no external dependencies.

## Features

- **Hand-written recursive-descent parser** with detailed error messages (line/column)
- **SSA-form IR** with Three-Address Code, BasicBlocks, and CFG
- **Optimisation passes**: constant folding, common sub-expression elimination, dead code elimination
- **Chaitin-Briggs graph-coloring register allocator** with type-aware interference graphs
- **PTX 7.0 code generation** targeting `sm_80` with debug `.loc` directives
- **CUDA Driver API validation harness** for end-to-end GPU testing

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The compiler binary is at `build/Release/miniptx` (Windows) or `build/miniptx` (Linux/Mac).

## Usage

```
miniptx [options] <input.ptxc>
```

### Emit Options

| Flag | Description |
|---|---|
| `--emit-tokens` | Print lexer token stream and exit |
| `--emit-ast` | Print AST dump and exit |
| `--emit-ir` | Print IR before optimisation |
| `--emit-ir-opt` | Print IR after optimisation |
| `--emit-regalloc` | Print interference graph and register coloring |
| `--emit-ptx` | Print final PTX assembly *(default)* |

### Compilation Options

| Flag | Description |
|---|---|
| `--no-opt` | Skip all optimisation passes |
| `-o <file>` | Write PTX output to `<file>` |

### Examples

```bash
# Default: compile and print PTX to stdout
miniptx tests/vec_add.ptxc

# Save PTX to file
miniptx tests/vec_add.ptxc -o out/vec_add.ptx

# Inspect token stream
miniptx --emit-tokens tests/vec_add.ptxc

# Dump AST
miniptx --emit-ast tests/vec_add.ptxc

# View IR before/after optimisation
miniptx --emit-ir tests/vec_add.ptxc
miniptx --emit-ir-opt tests/vec_add.ptxc

# View register allocation details
miniptx --emit-regalloc tests/vec_add.ptxc

# Compile without optimisations
miniptx --no-opt tests/vec_add.ptxc -o out/vec_add_noopt.ptx
```

## Compiler Pipeline

```
┌─────────────────────────────────────────────────────────────────┐
│                     MiniPTX Compiler Pipeline                   │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  source.ptxc                                                    │
│       │                                                         │
│       ▼                                                         │
│  ┌─────────┐   Tokenizes source into typed tokens with          │
│  │  Lexer  │   line/column tracking. Handles comments,          │
│  │         │   keywords, GPU builtins (threadIdx.x, etc.)       │
│  └────┬────┘                                                    │
│       │ std::vector<Token>                                      │
│       ▼                                                         │
│  ┌─────────┐   Recursive-descent parser. Builds AST with       │
│  │ Parser  │   correct operator precedence (13 levels).         │
│  │         │   Right-associative assignments, for/if/return.    │
│  └────┬────┘                                                    │
│       │ Program* (AST)                                          │
│       ▼                                                         │
│  ┌─────────┐   Type checking: resolves int/float/ptr types,    │
│  │  Sema   │   validates variable declarations, checks         │
│  │         │   kernel parameter types.                          │
│  └────┬────┘                                                    │
│       │ Annotated AST                                           │
│       ▼                                                         │
│  ┌─────────┐   Lowers AST to Three-Address Code in SSA form.   │
│  │   IR    │   Builds CFG with BasicBlocks, links successors/  │
│  │ Builder │   predecessors, inserts PHI nodes via dominance    │
│  │         │   frontier algorithm.                              │
│  └────┬────┘                                                    │
│       │ std::vector<IRFunction>                                 │
│       ▼                                                         │
│  ┌─────────┐   Three passes (skippable with --no-opt):         │
│  │  Opt    │   • Constant Folding: evaluates const int exprs   │
│  │ Passes  │   • CSE: eliminates redundant computations        │
│  │         │   • DCE: removes unused definitions               │
│  └────┬────┘                                                    │
│       │ Optimised IR                                            │
│       ▼                                                         │
│  ┌─────────┐   Chaitin-Briggs graph coloring with:             │
│  │ Reg     │   • Iterative backward dataflow liveness          │
│  │ Alloc   │   • Type-aware interference graph (4 classes)     │
│  │         │   • Simplify → Spill → Select with K=128          │
│  └────┬────┘                                                    │
│       │ SSA name → PTX register map                             │
│       ▼                                                         │
│  ┌─────────┐   Emits valid PTX 7.0 assembly with:              │
│  │ Code    │   • .file / .loc debug directives                 │
│  │  Gen    │   • Typed instruction selection (setp, cvta, etc) │
│  │         │   • Register declarations from allocator          │
│  └────┬────┘                                                    │
│       │                                                         │
│       ▼                                                         │
│  output.ptx                                                     │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

## Register Allocator

MiniPTX uses the **Chaitin-Briggs** graph-coloring algorithm for register allocation, the same approach used in production compilers like GCC and LLVM.

### Register Classes

PTX has typed virtual registers. The allocator maintains four independent register pools:

| IR Type | PTX Class | Pool | Example |
|---|---|---|---|
| `INT` | `.s32` | `%r0, %r1, ...` | Integer arithmetic |
| `FLOAT` | `.f32` | `%f0, %f1, ...` | Floating-point values |
| `INT_PTR` / `FLOAT_PTR` | `.u64` | `%rd0, %rd1, ...` | Pointer addresses |
| `BOOL` | `.pred` | `%p0, %p1, ...` | Comparison results |

### Algorithm Steps

1. **Liveness Analysis** — Iterative backward dataflow computes live-in/live-out sets for every BasicBlock. Function parameters are treated as simultaneously live at entry.

2. **Interference Graph** — Two values interfere if their live ranges overlap. Edges are only added between values of the *same* register class. Use counts are tracked for spill cost estimation.

3. **Simplify** — Repeatedly push nodes with degree < K (K=128) onto a stack, removing them from the graph.

4. **Spill** — If no node has degree < K, the node with the lowest use count is optimistically pushed (Briggs improvement). If coloring fails, it gets spilled to `.local` memory.

5. **Select** — Pop nodes from the stack and assign the lowest available color not used by any already-colored neighbor.

### Example Output

For `vec_add` with 4 parameters and 10 temporaries, the allocator produces:
```
.reg .s32  %r<4>;     // 4 integer registers (reused via coloring)
.reg .f32  %f<2>;     // 2 float registers
.reg .u64  %rd<3>;    // 3 pointer registers (one per param)
.reg .pred %p<1>;     // 1 predicate register
```

## GPU Validation

A CUDA Driver API test harness validates the generated PTX on actual GPU hardware:

```bash
# Build and run (requires CUDA Toolkit)
cmake --build build --target validate

# Or manually:
nvcc -o validate tests/validate.cu -lcuda
./validate out/vec_add.ptx out/scalar_mul.ptx
```

The harness JIT-compiles PTX via `cuModuleLoadDataEx`, launches kernels with random input data, and asserts `max |gpu - cpu| < 1e-5`.

## Project Structure

```
mini-ptx/
├── CMakeLists.txt
├── README.md
├── src/
│   ├── main.cpp          # CLI driver
│   ├── lexer.hpp/cpp     # Tokenizer
│   ├── parser.hpp/cpp    # Recursive-descent parser
│   ├── ast.hpp           # AST node hierarchy
│   ├── sema.hpp/cpp      # Semantic analysis
│   ├── ir.hpp/cpp        # SSA IR + IRBuilder
│   ├── regalloc.hpp/cpp  # Chaitin-Briggs register allocator
│   ├── codegen.hpp/cpp   # PTX code generator
│   └── passes/
│       ├── const_fold.hpp/cpp
│       ├── cse.hpp/cpp
│       └── dce.hpp/cpp
├── tests/
│   ├── vec_add.ptxc      # Vector addition kernel
│   ├── scalar_mul.ptxc   # Element-wise squaring kernel
│   └── validate.cu       # CUDA Driver API test harness
└── out/                   # Generated .ptx files
```

## License

This project is for educational and demonstration purposes.
