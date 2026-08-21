#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

struct Options {
    std::uint64_t items = 2000000;
    unsigned workers = 1;
};

std::uint64_t mix(std::uint64_t value) {
    value ^= value >> 30;
    value *= 0xbf58476d1ce4e5b9ULL;
    value ^= value >> 27;
    value *= 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}

std::uint64_t serial_sum(std::uint64_t items) {
    std::uint64_t result = 0;
    for (std::uint64_t index = 0; index < items; ++index) result += mix(index);
    return result;
}

std::uint64_t parallel_sum(std::uint64_t items, unsigned workers) {
    if (workers == 0) throw std::runtime_error("workers must be greater than zero");
    const auto actual_workers = static_cast<unsigned>(std::min<std::uint64_t>(workers, items == 0 ? 1 : items));
    std::vector<std::uint64_t> partial(actual_workers, 0);
    std::vector<std::thread> threads;
    threads.reserve(actual_workers);
    for (unsigned worker = 0; worker < actual_workers; ++worker) {
        const auto begin = items * worker / actual_workers;
        const auto end = items * (worker + 1) / actual_workers;
        threads.emplace_back([&, worker, begin, end] {
            for (auto index = begin; index < end; ++index) partial[worker] += mix(index);
        });
    }
    for (auto& thread : threads) thread.join();
    std::uint64_t result = 0;
    for (const auto value : partial) result += value;
    return result;
}

Options parse(int argc, char** argv) {
    Options options;
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        auto next_number = [&](const char* name) {
            if (++index >= argc) throw std::runtime_error(std::string(name) + " requires a value");
            char* end = nullptr;
            const auto value = std::strtoull(argv[index], &end, 10);
            if (!end || *end != '\0' || value > std::numeric_limits<std::uint64_t>::max()) throw std::runtime_error(std::string(name) + " requires an unsigned integer");
            return value;
        };
        if (argument == "--items") options.items = next_number("--items");
        else if (argument == "--workers") {
            const auto value = next_number("--workers");
            if (value > std::numeric_limits<unsigned>::max()) throw std::runtime_error("--workers is too large");
            options.workers = static_cast<unsigned>(value);
        } else if (argument == "-h" || argument == "--help" || argument == "-?") {
            std::cout << "flowparallel_smoketest - deterministic serial/parallel reduction\n\n"
                         "Options: --items N --workers N\n"
                         "         -h, -?, --help  show help\n"
                         "         -a, --about      show about information\n"
                         "         -v, --version    print the raw version number\n";
            std::exit(0);
        } else if (argument == "-a" || argument == "--about") { std::cout << "A deterministic segmented reduction smoke test for Flowparallel providers.\n"; std::exit(0); }
        else if (argument == "-v" || argument == "--version") { std::cout << "0.1.0\n"; std::exit(0); }
        else throw std::runtime_error("unknown option '" + argument + "'");
    }
    return options;
}

} // namespace

int main(int argc, char** argv) {
    try {
        const auto options = parse(argc, argv);
        const auto start = std::chrono::steady_clock::now();
        const auto result = options.workers == 1 ? serial_sum(options.items) : parallel_sum(options.items, options.workers);
        const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count();
        std::cout << "{\n  \"format\": \"flowparallel.smoketest\",\n"
                     "  \"version\": 1,\n"
                     "  \"status\": \"ok\",\n"
                     "  \"items\": " << options.items << ",\n"
                     "  \"workers\": " << options.workers << ",\n"
                     "  \"result\": " << result << ",\n"
                     "  \"elapsed_ns\": " << elapsed << "\n}\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "flowparallel_smoketest error: " << error.what() << '\n';
        return 2;
    }
}
