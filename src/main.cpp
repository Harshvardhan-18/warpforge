#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <filesystem>

#include "lexer.hpp"
#include "parser.hpp"
#include "sema.hpp"
#include "ir.hpp"
#include "passes/dce.hpp"
#include "passes/const_fold.hpp"
#include "passes/cse.hpp"
#include "regalloc.hpp"
#include "codegen.hpp"

namespace fs = std::filesystem;

static std::string readFile(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        std::cerr << "error: cannot open '" << path << "'\n";
        std::exit(1);
    }
    std::ostringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

static std::string fileName(const std::string& path) {
    return fs::path(path).filename().string();
}

// ═══════════════════════════════════════════════════════════════════
//  CLI options
// ═══════════════════════════════════════════════════════════════════

struct Options {
    std::string input;
    std::string output;        // -o <file>
    bool emitTokens  = false;  // --emit-tokens
    bool emitAst     = false;  // --emit-ast
    bool emitIr      = false;  // --emit-ir
    bool emitIrOpt   = false;  // --emit-ir-opt
    bool emitRegalloc= false;  // --emit-regalloc
    bool emitPtx     = true;   // --emit-ptx (default)
    bool noOpt       = false;  // --no-opt
};

static Options parseArgs(int argc, char* argv[]) {
    Options opts;
    bool anyEmit = false;

    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--emit-tokens"))  { opts.emitTokens = true; anyEmit = true; }
        else if (!std::strcmp(argv[i], "--emit-ast"))     { opts.emitAst = true; anyEmit = true; }
        else if (!std::strcmp(argv[i], "--emit-ir"))      { opts.emitIr = true; anyEmit = true; }
        else if (!std::strcmp(argv[i], "--emit-ir-opt"))  { opts.emitIrOpt = true; anyEmit = true; }
        else if (!std::strcmp(argv[i], "--emit-regalloc")){ opts.emitRegalloc = true; anyEmit = true; }
        else if (!std::strcmp(argv[i], "--emit-ptx"))     { opts.emitPtx = true; anyEmit = true; }
        else if (!std::strcmp(argv[i], "--no-opt"))       { opts.noOpt = true; }
        else if (!std::strcmp(argv[i], "-o") && i + 1 < argc) { opts.output = argv[++i]; }
        else if (argv[i][0] == '-') {
            std::cerr << "unknown option: " << argv[i] << "\n"; std::exit(1);
        }
        else { opts.input = argv[i]; }
    }

    // If any specific --emit-* was given (besides ptx), disable default ptx
    if (anyEmit && !opts.emitPtx) opts.emitPtx = false;

    return opts;
}

static void printUsage() {
    std::cerr <<
        "WarpForge Compiler v0.3\n\n"
        "Usage: warpforge [options] <input.ptxc>\n\n"
        "Emit options (if none given, --emit-ptx is default):\n"
        "  --emit-tokens    Print lexer token stream and exit\n"
        "  --emit-ast       Print AST dump and exit\n"
        "  --emit-ir        Print IR before optimisation\n"
        "  --emit-ir-opt    Print IR after optimisation\n"
        "  --emit-regalloc  Print interference graph and coloring\n"
        "  --emit-ptx       Print final PTX assembly (default)\n\n"
        "Compilation options:\n"
        "  --no-opt         Skip all optimisation passes\n"
        "  -o <file>        Write PTX output to <file>\n";
}

// ═══════════════════════════════════════════════════════════════════
//  Main compiler driver
// ═══════════════════════════════════════════════════════════════════

int main(int argc, char* argv[]) {
    if (argc < 2) { printUsage(); return 1; }

    Options opts = parseArgs(argc, argv);
    if (opts.input.empty()) { printUsage(); return 1; }

    std::string source = readFile(opts.input);
    std::string srcName = fileName(opts.input);

    // ── Lex ──
    auto tokens = miniptx::Lexer(source).tokenizeAll();

    if (opts.emitTokens) {
        std::cout << "=== Tokens: " << srcName << " (" << tokens.size() << ") ===\n";
        miniptx::Lexer lex(source);
        miniptx::Token t;
        do { t = lex.nextToken(); std::cout << t << "\n"; }
        while (t.type != miniptx::TokenType::END_OF_FILE);
        return 0;
    }

    // ── Parse ──
    std::unique_ptr<miniptx::Program> prog;
    try { prog = miniptx::Parser(std::move(tokens)).parseProgram(); }
    catch (const miniptx::ParseError& e) {
        std::cerr << srcName << ": parse error: " << e.what() << "\n";
        return 1;
    }

    if (opts.emitAst) {
        std::cout << "=== AST: " << srcName << " ===\n";
        prog->dump();
        return 0;
    }

    // ── Sema ──
    miniptx::Sema sema;
    try { sema.analyse(*prog); }
    catch (const miniptx::SemaError& e) {
        std::cerr << srcName << ": semantic error: " << e.what() << "\n";
        return 1;
    }

    // ── IR lowering ──
    miniptx::IRBuilder builder;
    auto fns = builder.lowerProgram(*prog);

    if (opts.emitIr) {
        std::cout << "=== IR (before opt): " << srcName << " ===\n";
        for (auto& fn : fns) fn.dump(std::cout);
        if (!opts.emitIrOpt && !opts.emitRegalloc && !opts.emitPtx)
            return 0;
    }

    // ── Optimisation ──
    if (!opts.noOpt) {
        miniptx::runConstFold(fns);
        miniptx::runCSE(fns);
        miniptx::runDCE(fns);
    }

    if (opts.emitIrOpt) {
        std::cout << "=== IR (after opt): " << srcName << " ===\n";
        for (auto& fn : fns) fn.dump(std::cout);
        if (!opts.emitRegalloc && !opts.emitPtx)
            return 0;
    }

    // ── Register allocation ──
    miniptx::RegAlloc ra;
    ra.allocate(fns);

    if (opts.emitRegalloc) {
        ra.dump(std::cout);
        if (!opts.emitPtx) return 0;
    }

    // ── Code generation ──
    miniptx::CodeGen cg(ra);
    cg.setSourceFile(srcName);

    // Determine output path
    std::string outPath = opts.output;
    if (outPath.empty() && opts.emitPtx) {
        // Default: print to stdout
        std::cout << "=== PTX: " << srcName << " ===\n";
        cg.generate(fns, std::cout);
    }

    if (!outPath.empty()) {
        // Ensure directory exists
        fs::path outDir = fs::path(outPath).parent_path();
        if (!outDir.empty()) fs::create_directories(outDir);

        std::ofstream ofs(outPath);
        if (!ofs.is_open()) {
            std::cerr << "error: cannot write '" << outPath << "'\n";
            return 1;
        }
        cg.generate(fns, ofs);
        ofs.close();
    }

    // ── Compilation summary ──
    int totalInsts = cg.countInstructions(fns);
    auto decls = ra.getRegDecls();
    for (auto& fn : fns) {
        std::cout << "\nCompiled " << fn.name << ": "
                  << totalInsts << " instructions, ";
        int totalRegs = 0;
        for (auto& d : decls) totalRegs += d.count;
        std::cout << totalRegs << " registers (";
        bool first = true;
        for (auto& d : decls) {
            if (!first) std::cout << ", ";
            std::cout << miniptx::regClassName(d.rc) << ": " << d.count;
            first = false;
        }
        std::cout << ")\n";
    }

    if (!outPath.empty())
        std::cout << "PTX written to '" << outPath << "'\n";

    return 0;
}
