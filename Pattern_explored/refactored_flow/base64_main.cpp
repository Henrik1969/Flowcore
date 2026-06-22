#include "base64_stage.h"
#include "flow_common.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

namespace {

void loadBase64PoliciesFromArgs(int argc, char** argv, flow::PolicyBag& policies) {
    policies.set("base64.mode", std::string{"encode"});
    policies.set("base64.strict", true);
    policies.set("base64.ignore_whitespace", true);
    policies.set("base64.wrap", 0);

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--mode") {
            policies.set("base64.mode", flow::requireArgValue(argc, argv, i, arg));
        } else if (arg == "--strict") {
            policies.set("base64.strict", flow::parseBool(flow::requireArgValue(argc, argv, i, arg)));
        } else if (arg == "--ignore-whitespace") {
            policies.set("base64.ignore_whitespace", flow::parseBool(flow::requireArgValue(argc, argv, i, arg)));
        } else if (arg == "--wrap") {
            policies.set("base64.wrap", flow::parseInt(flow::requireArgValue(argc, argv, i, arg)));
        } else if (arg == "--help" || arg == "-h") {
            std::cout
                << "Usage:\n"
                << "  flowbase64 [options] < input > output\n\n"
                << "Options:\n"
                << "  --mode encode|decode\n"
                << "  --strict true|false\n"
                << "  --ignore-whitespace true|false\n"
                << "  --wrap N\n";
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
        loadBase64PoliciesFromArgs(argc, argv, ctx.policies);

        flow::StdinProducer stdinProducer(ctx);
        Base64Stage base64;
        flow::StdoutSink stdoutSink(std::cout);

        stdinProducer >> base64 >> stdoutSink;

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
