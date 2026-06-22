#include "flow_common.h"
#include "flowmini_lexer.h"
#include "flowmini_parser.h"
#include "flowmini_runtime.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

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

void printUsage(std::ostream& out) {
    out
        << "Usage:\n"
        << "  flowmini [--trace true|false] [--emit-flowir <file|->] <module.flow|module.flowir> < input\n\n"
        << "Human .flow sugar examples:\n"
        << "  module demo\n"
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

int main(int argc, char** argv) {
    flow::PipelineContext ctx;
    ctx.policies.set("runtime.trace", false);

    const flow::LogSink log{std::cerr};

    try {
        std::string sourcePath;
        std::string emitFlowIrPath;

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

            if (arg == "--emit-flowir") {
                emitFlowIrPath = flow::requireArgValue(argc, argv, i, arg);
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

        const std::string source = readFile(sourcePath);
        const auto tokens = flowmini::lexSource(source);
        const auto module = flowmini::parseModule(tokens);

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
