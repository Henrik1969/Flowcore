#include "hex_stage.h"
#include "flow_common.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

namespace {

void loadHexPoliciesFromArgs(int argc, char** argv, flow::PolicyBag& policies) {
    policies.set("hex.mode", std::string{"encode"});
    policies.set("hex.width", 80);
    policies.set("hex.uppercase", true);
    policies.set("hex.strict", true);
    policies.set("hex.ignore_whitespace", true);

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--mode") {
            policies.set("hex.mode", flow::requireArgValue(argc, argv, i, arg));
        } else if (arg == "--width") {
            policies.set("hex.width", flow::parseInt(flow::requireArgValue(argc, argv, i, arg)));
        } else if (arg == "--uppercase") {
            policies.set("hex.uppercase", flow::parseBool(flow::requireArgValue(argc, argv, i, arg)));
        } else if (arg == "--strict") {
            policies.set("hex.strict", flow::parseBool(flow::requireArgValue(argc, argv, i, arg)));
        } else if (arg == "--ignore-whitespace") {
            policies.set("hex.ignore_whitespace", flow::parseBool(flow::requireArgValue(argc, argv, i, arg)));
        } else if (arg == "--help" || arg == "-h") {
            std::cout
                << "Usage:\n"
                << "  flowhex [options] < input > output\n\n"
                << "Options:\n"
                << "  --mode encode|decode\n"
                << "  --width N\n"
                << "  --uppercase true|false\n"
                << "  --strict true|false\n"
                << "  --ignore-whitespace true|false\n";
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
        loadHexPoliciesFromArgs(argc, argv, ctx.policies);

        flow::StdinProducer stdinProducer(ctx);
        HexStage hex;
        flow::StdoutSink stdoutSink(std::cout);

        stdinProducer >> hex >> stdoutSink;

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
