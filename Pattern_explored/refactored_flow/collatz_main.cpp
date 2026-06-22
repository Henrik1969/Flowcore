#include "collatz_pipeline.h"
#include "flow_common.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <string>

namespace {

void loadCollatzPoliciesFromArgs(int argc, char** argv, flow::PolicyBag& policies) {
    policies.set("collatz.max_steps", 10000);
    policies.set("collatz.stop_on_overflow", true);
    policies.set("format.include_steps", true);
    policies.set("runtime.trace", false);

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--max-steps") {
            policies.set("collatz.max_steps", flow::parseInt(flow::requireArgValue(argc, argv, i, arg)));
        } else if (arg == "--stop-on-overflow") {
            policies.set("collatz.stop_on_overflow", flow::parseBool(flow::requireArgValue(argc, argv, i, arg)));
        } else if (arg == "--include-steps") {
            policies.set("format.include_steps", flow::parseBool(flow::requireArgValue(argc, argv, i, arg)));
        } else if (arg == "--trace") {
            policies.set("runtime.trace", flow::parseBool(flow::requireArgValue(argc, argv, i, arg)));
        } else if (arg == "--help" || arg == "-h") {
            std::cout
                << "Usage:\n"
                << "  flowexec [options] < input\n\n"
                << "Options:\n"
                << "  --max-steps N\n"
                << "  --stop-on-overflow true|false\n"
                << "  --include-steps true|false\n"
                << "  --trace true|false\n";
            std::exit(EXIT_SUCCESS);
        } else {
            throw flow::DiagnosticError{"cli", "unknown argument: " + arg};
        }
    }
}

} // namespace

int main(int argc, char** argv) {
    flow::PipelineContext ctx;
    flow::LogSink log(std::cerr);

    try {
        loadCollatzPoliciesFromArgs(argc, argv, ctx.policies);

        collatz::StdinTextProducer stdin(ctx);
        collatz::RuntimeGraph graph;

        graph.addNode("parse", std::make_unique<collatz::ParseUIntNode>());
        graph.addNode("check", std::make_unique<collatz::CheckDoneNode>());
        graph.addNode("parity", std::make_unique<collatz::ParityBranchNode>());
        graph.addNode("even", std::make_unique<collatz::EvenStepNode>());
        graph.addNode("odd", std::make_unique<collatz::OddStepNode>());
        graph.addNode("format", std::make_unique<collatz::FormatOutputNode>());
        graph.addNode("stdout", std::make_unique<collatz::StdoutNode>());

        graph.connect("stdin", "out", "parse");
        graph.connect("parse", "ok", "check");
        graph.connect("check", "continue", "parity");
        graph.connect("check", "done", "format");
        graph.connect("parity", "even", "even");
        graph.connect("parity", "odd", "odd");
        graph.connect("even", "out", "check");
        graph.connect("odd", "out", "check");
        graph.connect("format", "out", "stdout");

        graph.startFromProducer("stdin", "out", stdin.produce());

        log.write(ctx);
    } catch (const flow::DiagnosticError& err) {
        log.writeFatal(err);
        log.write(ctx);
        return EXIT_FAILURE;
    } catch (const std::exception& err) {
        std::cerr << "fatal internal error: " << err.what() << '\n';
        log.write(ctx);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
