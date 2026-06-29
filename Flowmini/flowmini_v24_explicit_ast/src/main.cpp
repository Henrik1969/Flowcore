#include "flow_common.h"
#include "flowmini_lexer.h"
#include "flowmini_parser.h"
#include "flowmini_ast.h"
#include "flowmini_runtime.h"
#include "flowmini_structural.h"
#include "flowmini_token_tree_bridge.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace {

[[nodiscard]] std::string readFile(const std::string& path) {
    std::ifstream input{path};
    if (!input) {
        throw flow::DiagnosticError{"cli", "could not open source file: " + path};
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}


[[nodiscard]] auto trimCopy(const std::string value) -> std::string {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) { return {}; }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

[[nodiscard]] bool startsWithWord(const std::string& text, const std::string& word) {
    if (text.rfind(word, 0) != 0) { return false; }
    if (text.size() == word.size()) { return true; }
    const char next = text.at(word.size());
    return next == ' ' || next == '\t' || next == '{' || next == '(';
}


[[nodiscard]] std::string maskCommentsForImportScanner(const std::string& source) {
    enum class Mode { Code, String, LineComment, BlockComment };

    std::string out;
    out.reserve(source.size());

    Mode mode = Mode::Code;
    std::size_t depth = 0;
    int line = 1;
    int column = 1;
    int blockLine = 1;
    int blockColumn = 1;

    for (std::size_t i = 0; i < source.size();) {
        const char c = source[i];
        const char next = (i + 1 < source.size()) ? source[i + 1] : '\0';

        if (mode == Mode::Code) {
            if (c == '"') {
                mode = Mode::String;
                out.push_back(c);
                ++i;
                ++column;
                continue;
            }
            if (c == '/' && next == '/') {
                mode = Mode::LineComment;
                out.append("  ");
                i += 2;
                column += 2;
                continue;
            }
            if (c == '/' && next == '*') {
                mode = Mode::BlockComment;
                depth = 1;
                blockLine = line;
                blockColumn = column;
                out.append("  ");
                i += 2;
                column += 2;
                continue;
            }
            out.push_back(c);
            if (c == '\n') { ++line; column = 1; }
            else { ++column; }
            ++i;
            continue;
        }

        if (mode == Mode::String) {
            out.push_back(c);
            if (c == '\\' && i + 1 < source.size()) {
                out.push_back(source[i + 1]);
                i += 2;
                column += 2;
                continue;
            }
            if (c == '"') { mode = Mode::Code; }
            if (c == '\n') { ++line; column = 1; }
            else { ++column; }
            ++i;
            continue;
        }

        if (mode == Mode::LineComment) {
            if (c == '\n') {
                mode = Mode::Code;
                out.push_back('\n');
                ++i;
                ++line;
                column = 1;
                continue;
            }
            out.push_back(' ');
            ++i;
            ++column;
            continue;
        }

        if (mode == Mode::BlockComment) {
            if (c == '/' && next == '*') {
                ++depth;
                out.append("  ");
                i += 2;
                column += 2;
                continue;
            }
            if (c == '*' && next == '/') {
                --depth;
                out.append("  ");
                i += 2;
                column += 2;
                if (depth == 0) { mode = Mode::Code; }
                continue;
            }
            if (c == '\n') {
                out.push_back('\n');
                ++i;
                ++line;
                column = 1;
                continue;
            }
            out.push_back(' ');
            ++i;
            ++column;
            continue;
        }
    }

    if (mode == Mode::BlockComment) {
        throw flow::DiagnosticError{
            "import",
            "unterminated block comment starting at line " + std::to_string(blockLine) +
            ", column " + std::to_string(blockColumn)
        };
    }

    return out;
}

[[nodiscard]] bool parseImportLine(const std::string& line, std::string& importedPath) {
    const std::string t = trimCopy(line);
    if (!startsWithWord(t, "import")) { return false; }

    std::size_t i = std::string{"import"}.size();
    while (i < t.size() && (t[i] == ' ' || t[i] == '\t')) { ++i; }
    if (i >= t.size() || t[i] != '"') {
        throw flow::DiagnosticError{"import", "expected quoted path after import"};
    }
    ++i;

    std::string path;
    bool escape = false;
    for (; i < t.size(); ++i) {
        const char c = t[i];
        if (escape) { path.push_back(c); escape = false; continue; }
        if (c == '\\') { escape = true; continue; }
        if (c == '"') { ++i; break; }
        path.push_back(c);
    }
    if (path.empty()) { throw flow::DiagnosticError{"import", "empty import path"}; }
    while (i < t.size() && (t[i] == ' ' || t[i] == '\t')) { ++i; }
    if (i != t.size()) { throw flow::DiagnosticError{"import", "unexpected text after import path: " + t.substr(i)}; }
    importedPath = path;
    return true;
}

[[nodiscard]] std::filesystem::path canonicalImportPath(const std::filesystem::path& importerPath, const std::string& importText) {
    std::filesystem::path raw{importText};
    if (raw.is_relative()) {
        raw = importerPath.parent_path() / raw;
    }
    return std::filesystem::weakly_canonical(std::filesystem::absolute(raw));
}


enum class ImportState { Loading, Loaded };
enum class SourceUnitKind { None, LegacyModule, Program, Unit };

[[nodiscard]] SourceUnitKind detectUnitKind(const std::string& trimmed) {
    if (startsWithWord(trimmed, "program")) { return SourceUnitKind::Program; }
    if (startsWithWord(trimmed, "unit")) { return SourceUnitKind::Unit; }
    if (startsWithWord(trimmed, "module")) { return SourceUnitKind::LegacyModule; }
    return SourceUnitKind::None;
}

struct ImportExpander {
    std::map<std::string, ImportState> states;
    std::vector<std::string> stack;

    [[nodiscard]] std::string expandRoot(const std::string& sourcePath) {
        const auto rootPath = std::filesystem::weakly_canonical(std::filesystem::absolute(std::filesystem::path{sourcePath}));
        const std::string source = readFile(rootPath.string());
        const std::string scanSource = maskCommentsForImportScanner(source);
        std::istringstream input{source};
        std::istringstream scanInput{scanSource};

        std::vector<std::string> importExpansions;
        std::vector<std::string> rootBodyLines;
        std::string headerLine;
        SourceUnitKind rootKind = SourceUnitKind::None;
        bool sawHeader = false;
        bool sawMain = false;
        std::string line;

        std::string scanLine;
        while (std::getline(input, line) && std::getline(scanInput, scanLine)) {
            std::string importText;
            const std::string trimmed = trimCopy(scanLine);
            if (parseImportLine(scanLine, importText)) {
                if (sawHeader) {
                    throw flow::DiagnosticError{"import", "import statements must appear before program/unit declaration in: " + rootPath.string()};
                }
                importExpansions.push_back(expandLibrary(canonicalImportPath(rootPath, importText)));
                continue;
            }

            const SourceUnitKind kind = detectUnitKind(trimmed);
            if (kind != SourceUnitKind::None) {
                if (sawHeader) { throw flow::DiagnosticError{"import", "multiple program/unit declarations in: " + rootPath.string()}; }
                headerLine = line;
                rootKind = kind;
                sawHeader = true;
                continue;
            }

            if (startsWithWord(trimmed, "main")) {
                if (sawMain) { throw flow::DiagnosticError{"import", "multiple main definitions found in root file: " + rootPath.string()}; }
                sawMain = true;
            }

            if (!trimmed.empty() || sawHeader) {
                rootBodyLines.push_back(line);
            }
        }

        if (!sawHeader) { throw flow::DiagnosticError{"import", "root source has no program/unit declaration: " + rootPath.string()}; }
        if (rootKind == SourceUnitKind::Unit) {
            throw flow::DiagnosticError{"import", "root source is a unit; units are defining/importable units and cannot be executed directly: " + rootPath.string()};
        }
        if (!sawMain) { throw flow::DiagnosticError{"import", "root program has no main block: " + rootPath.string()}; }

        std::ostringstream out;
        out << headerLine << "\n\n";
        for (const auto& expanded : importExpansions) {
            if (!expanded.empty()) { out << expanded << "\n"; }
        }
        for (const auto& bodyLine : rootBodyLines) { out << bodyLine << "\n"; }
        return out.str();
    }

    [[nodiscard]] std::string expandLibrary(const std::filesystem::path& libPath) {
        const std::string key = libPath.string();
        const auto found = states.find(key);
        if (found != states.end()) {
            if (found->second == ImportState::Loaded) { return {}; }

            std::ostringstream cycle;
            cycle << "import cycle detected: ";
            const auto pos = std::ranges::find(stack, key);
            if (pos != stack.end()) {
                for (auto it = pos; it != stack.end(); ++it) { cycle << *it << " -> "; }
            }
            cycle << key;
            throw flow::DiagnosticError{"import", cycle.str()};
        }

        states[key] = ImportState::Loading;
        stack.push_back(key);

        const std::string source = readFile(key);
        const std::string scanSource = maskCommentsForImportScanner(source);
        std::istringstream input{source};
        std::istringstream scanInput{scanSource};
        std::vector<std::string> importExpansions;
        std::vector<std::string> bodyLines;
        SourceUnitKind libKind = SourceUnitKind::None;
        bool sawHeader = false;
        std::string line;

        std::string scanLine;
        while (std::getline(input, line) && std::getline(scanInput, scanLine)) {
            std::string importText;
            const std::string trimmed = trimCopy(scanLine);

            if (parseImportLine(scanLine, importText)) {
                if (sawHeader) {
                    throw flow::DiagnosticError{"import", "import statements must appear before unit declaration in imported file: " + key};
                }
                importExpansions.push_back(expandLibrary(canonicalImportPath(libPath, importText)));
                continue;
            }

            const SourceUnitKind kind = detectUnitKind(trimmed);
            if (kind != SourceUnitKind::None) {
                if (sawHeader) { throw flow::DiagnosticError{"import", "multiple program/unit declarations in imported file: " + key}; }
                libKind = kind;
                sawHeader = true;
                continue;
            }

            if (startsWithWord(trimmed, "main")) {
                throw flow::DiagnosticError{"import", "imported file defines main; only unit files may be imported: " + key};
            }

            if (!trimmed.empty() || sawHeader) {
                bodyLines.push_back(line);
            }
        }

        if (!sawHeader) { throw flow::DiagnosticError{"import", "imported file has no unit declaration: " + key}; }
        if (libKind == SourceUnitKind::Program) {
            throw flow::DiagnosticError{"import", "imported file is a program; only unit files may be imported: " + key};
        }

        std::ostringstream out;
        for (const auto& expanded : importExpansions) {
            if (!expanded.empty()) { out << expanded << "\n"; }
        }
        for (const auto& bodyLine : bodyLines) { out << bodyLine << "\n"; }

        stack.pop_back();
        states[key] = ImportState::Loaded;
        return out.str();
    }
};

[[nodiscard]] std::string expandImports(const std::string& sourcePath) {
    ImportExpander expander;
    return expander.expandRoot(sourcePath);
}

void printUsage(std::ostream& out) {
    out
        << "Usage:\n"
        << "  flowmini [--trace true|false] [--emit-flowir <file|->] [--dump-token-tree <file|->] [--dump-token-tree-bridge [json|simple]] [--dump-symbols <file|->] <program.flow|module.flowir> < input\n\n"
        << "Human .flow sugar examples:\n"
        << "  program demo\n"
        << "  stdin : stdin.text()\n"
        << "  one : int(1)\n"
        << "  arr : list<int>([1,2,3])\n"
        << "  add : int.add(lhs=\"sum\", rhs=\"current\", out=\"sum\")\n"
        << "  stdin.out => parse.in => init.in\n\n"
        << "Explicit .flowir remains valid:\n"
        << "  producer <id> : <atom.kind>\n"
        << "  node <id> : <atom.kind>\n"
        << "  sink <id> : <atom.kind>\n"
        << "  wire <from_node>.<from_port> => <to_node>.<to_port>\n"
        << "  policy <node>.<key> = <string|int|bool>\n";
}

void writeOutputFile(const std::string& path, const flowmini::ModuleSpec& module) {
    if (path == "-") {
        flowmini::writeFlowIr(module, std::cout);
        return;
    }

    std::ofstream out{path};
    if (!out) {
        throw flow::DiagnosticError{"cli", "could not open output file: " + path};
    }
    flowmini::writeFlowIr(module, out);
}

} // namespace

/**
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char** argv) {
    flow::PipelineContext ctx;
    ctx.policies.set("runtime.trace", false);

    const flow::LogSink log{std::cerr};

    try {
        std::string sourcePath;
        std::string emitFlowIrPath;
        std::string dumpTokenTreePath;

        bool dumpTokenTreeBridge = false;
        bool dumpAst = false;

        flowmini::TokenTreeBridgeDumpFormat dumpTokenTreeBridgeFormat = flowmini::TokenTreeBridgeDumpFormat::Json;
        std::string dumpSymbolsPath;
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];

            if (arg == "--help" || arg == "-h") {
                printUsage(std::cout);
                return 0;
            }

            if (arg == "--trace") {
                ctx.policies.set("runtime.trace", flow::parseBool(flow::requireArgValue(argc, argv, i, arg)));
                continue;
            }

            if (arg == "--dump-ast") {
                dumpAst = true;
                continue;
            }

            if (arg == "--emit-flowir") {
                emitFlowIrPath = flow::requireArgValue(argc, argv, i, arg);
                continue;
            }

            if (arg == "--dump-token-tree") {
                dumpTokenTreePath = flow::requireArgValue(argc, argv, i, arg);
                continue;
            }

            if (arg == "--dump-token-tree-bridge") {
                dumpTokenTreeBridge = true;
                if (i + 1 < argc) {
                    const std::string next = argv[i + 1];
                    if (next == "json" || next == "simple") {
                        dumpTokenTreeBridgeFormat = flowmini::parseTokenTreeBridgeDumpFormat(next);
                        ++i;
                    }
                }
                continue;
            }

            if (arg == "--dump-symbols") {
                dumpSymbolsPath = flow::requireArgValue(argc, argv, i, arg);
                continue;
            }


            if (!sourcePath.empty()) {
                throw flow::DiagnosticError{"cli", "unexpected extra argument: " + arg};
            }

            sourcePath = arg;
        }


        if (sourcePath.empty()) {
            printUsage(std::cerr);
            return 2;
        }

        const std::filesystem::path inputPath{sourcePath};
        const std::string source = (inputPath.extension() == ".flowir") ? readFile(sourcePath) : expandImports(sourcePath);
        const auto tokens = flowmini::lexSource(source);

        if (dumpAst) {
            const auto module = flowmini::ast::make_empty_ast_module();
            flowmini::ast::dump_ast_json(std::cout, module);
            return 0;
        }

        if (!dumpTokenTreePath.empty()) {
            if (dumpTokenTreePath == "-") {
                flowmini::writeTokenTreeDump(tokens, std::cout);
            } else {
                std::ofstream out{dumpTokenTreePath};
                if (!out) { throw flow::DiagnosticError{"cli", "could not open token tree output file: " + dumpTokenTreePath}; }
                flowmini::writeTokenTreeDump(tokens, out);
            }
            return 0;
        }

        if (dumpTokenTreeBridge) {
            flowmini::writeTokenTreeBridgeDump(tokens, dumpTokenTreeBridgeFormat, std::cout);
            return 0;
        }

        const auto module = flowmini::parseModule(tokens);

        if (!dumpSymbolsPath.empty()) {
            if (dumpSymbolsPath == "-") {
                flowmini::writeFlowIrSymbolTableDump(module, std::cout);
            } else {
                std::ofstream out{dumpSymbolsPath};
                if (!out) { throw flow::DiagnosticError{"cli", "could not open symbol dump output file: " + dumpSymbolsPath}; }
                flowmini::writeFlowIrSymbolTableDump(module, out);
            }
            return 0;
        }

        if (!emitFlowIrPath.empty()) {
            writeOutputFile(emitFlowIrPath, module);
            return 0;
        }

        auto registry = flowmini::makeCoreAtomRegistry();
        flowmini::runModule(module, ctx, registry);
        log.write(ctx);
        return 0;
    } catch (const flow::DiagnosticError& err) {
        log.writeFatal(err);
        log.write(ctx);
        return 1;
    } catch (const std::exception& err) {
        std::cerr << "fatal in unknown: " << err.what() << '\n';
        log.write(ctx);
        return 1;
    }
}
