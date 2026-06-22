#include "bitart_stage.h"
#include "flow_common.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

namespace {

void loadBitArtPoliciesFromArgs(int argc, char** argv, flow::PolicyBag& policies) {
    policies.set("bitart.mode", std::string{"pack"});
    policies.set("bitart.width", 8);
    policies.set("bitart.one", std::string{"#"});
    policies.set("bitart.zero", std::string{"."});
    policies.set("bitart.strict", true);
    policies.set("bitart.ignore_whitespace", true);

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--mode") {
            policies.set("bitart.mode", flow::requireArgValue(argc, argv, i, arg));
        } else if (arg == "--width") {
            policies.set("bitart.width", flow::parseInt(flow::requireArgValue(argc, argv, i, arg)));
        } else if (arg == "--one") {
            policies.set("bitart.one", flow::requireArgValue(argc, argv, i, arg));
        } else if (arg == "--zero") {
            policies.set("bitart.zero", flow::requireArgValue(argc, argv, i, arg));
        } else if (arg == "--strict") {
            policies.set("bitart.strict", flow::parseBool(flow::requireArgValue(argc, argv, i, arg)));
        } else if (arg == "--ignore-whitespace") {
            policies.set("bitart.ignore_whitespace", flow::parseBool(flow::requireArgValue(argc, argv, i, arg)));
        } else if (arg == "--help" || arg == "-h") {
            std::cout
                << "Usage:\n"
                << "  bitart [options] < input > output\n\n"
                << "Options:\n"
                << "  --mode pack|unpack\n"
                << "  --width N\n"
                << "  --one CHAR\n"
                << "  --zero CHAR\n"
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
        loadBitArtPoliciesFromArgs(argc, argv, ctx.policies);

        flow::StdinProducer stdinProducer(ctx);
        BitArtStage bitart;
        flow::StdoutSink stdoutSink(std::cout);

        stdinProducer >> bitart >> stdoutSink;

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
