#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <string_view>
#include <stdexcept>
#include <sys/random.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>

namespace {

constexpr std::string_view VERSION = "0.1.0";

struct Result { std::string name; bool ok; int error = 0; };

Result check_syscall(const char* name, long value) {
    if (value >= 0) return {name, true, 0};
    return {name, false, errno};
}

Result readonly_probe() {
    auto pid = check_syscall("getpid", syscall(SYS_getpid));
    if (!pid.ok) return pid;

    struct timespec clock_value{};
    if (syscall(SYS_clock_gettime, CLOCK_MONOTONIC, &clock_value) < 0) return {"clock_gettime", false, errno};

    struct utsname host{};
    if (syscall(SYS_uname, &host) < 0) return {"uname", false, errno};

    std::uint8_t entropy = 0;
    const auto random_result = syscall(SYS_getrandom, &entropy, sizeof(entropy), GRND_NONBLOCK);
    if (random_result < 0 && errno != EAGAIN) return {"getrandom", false, errno};

    return {"readonly", true, 0};
}

Result tempfs_probe() {
    char directory_template[] = "/tmp/flowcore-kernel-probe-XXXXXX";
    const int directory = mkdtemp(directory_template) == nullptr ? -1 : open(directory_template, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (directory < 0) return {"mkdir/openat", false, errno};

    const int file = openat(directory, "probe.bin", O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, 0600);
    if (file < 0) { const int saved = errno; close(directory); rmdir(directory_template); return {"openat", false, saved}; }

    const char payload[] = "flowcore-kernel";
    const auto written = write(file, payload, sizeof(payload) - 1);
    if (written != static_cast<ssize_t>(sizeof(payload) - 1)) { const int saved = errno; close(file); unlinkat(directory, "probe.bin", 0); close(directory); rmdir(directory_template); return {"write", false, saved}; }
    if (lseek(file, 0, SEEK_SET) < 0) { const int saved = errno; close(file); unlinkat(directory, "probe.bin", 0); close(directory); rmdir(directory_template); return {"lseek", false, saved}; }

    char observed[sizeof(payload)]{};
    const auto read_count = read(file, observed, sizeof(payload) - 1);
    const bool content_ok = read_count == static_cast<ssize_t>(sizeof(payload) - 1) && std::memcmp(observed, payload, sizeof(payload) - 1) == 0;
    const int saved = content_ok ? 0 : errno;
    close(file);
    const bool unlinked = unlinkat(directory, "probe.bin", 0) == 0;
    close(directory);
    const bool removed = rmdir(directory_template) == 0;
    if (!content_ok) return {"read/verify", false, saved};
    if (!unlinked) return {"unlinkat", false, errno};
    if (!removed) return {"rmdir", false, errno};
    return {"tempfs", true, 0};
}

void print_result(const Result& result) {
    std::cout << "{\"name\":\"" << result.name << "\",\"status\":\"" << (result.ok ? "ok" : "error") << "\"";
    if (!result.ok) std::cout << ",\"errno\":" << result.error << ",\"message\":\"" << std::strerror(result.error) << "\"";
    std::cout << "}";
}

int run(std::string_view probe) {
    if (probe != "readonly" && probe != "tempfs" && probe != "all") throw std::runtime_error("unknown probe; choose readonly, tempfs, or all");
    const bool run_readonly = probe == "readonly" || probe == "all";
    const bool run_tempfs = probe == "tempfs" || probe == "all";
    const auto readonly = run_readonly ? readonly_probe() : Result{"", true, 0};
    const auto tempfs = run_tempfs ? tempfs_probe() : Result{"", true, 0};
    std::cout << "{\"format\":\"flowkernel.probe_report\",\"version\":1,\"probe\":\"" << probe << "\",\"effects\":[\"read-only-kernel\"" << (run_tempfs ? ",\"private-tempfs\"" : "") << "],\"results\":[";
    bool first = true;
    if (run_readonly) { print_result(readonly); first = false; }
    if (run_tempfs) { if (!first) std::cout << ','; print_result(tempfs); }
    std::cout << "] ,\"status\":\"" << (readonly.ok && tempfs.ok ? "ok" : "error") << "\"}\n";
    return readonly.ok && tempfs.ok ? 0 : 2;
}

}

int main(int argc, char** argv) {
    try {
        if (argc == 2) {
            const std::string option = argv[1];
            if (option == "-h" || option == "--help" || option == "-?") { std::cout << "flowkernel - isolated Linux kernel boundary probes\n\nUsage: flowkernel --probe readonly|tempfs|all\n\nOptions: -h, -?, --help  show help\n         -a, --about    show about information\n         -v, --version  print the raw version number\n\nMore help: Flowkernel/README.md\n"; return 0; }
            if (option == "-a" || option == "--about") { std::cout << "Flowkernel runs explicitly scoped, non-privileged Linux boundary probes.\nMore help: Flowkernel/README.md\n"; return 0; }
            if (option == "-v" || option == "--version") { std::cout << VERSION << '\n'; return 0; }
        }
        if (argc != 3 || std::string(argv[1]) != "--probe") throw std::runtime_error("usage: flowkernel --probe readonly|tempfs|all");
        return run(argv[2]);
    } catch (const std::exception& error) { std::cerr << "flowkernel error: " << error.what() << '\n'; return 1; }
}
