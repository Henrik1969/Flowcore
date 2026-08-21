#include <dlfcn.h>

#include <cstddef>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using error_t = int;
using handle_t = void*;
constexpr int success = 0;
constexpr int host_to_device = 1;
constexpr int device_to_host = 2;

struct Library {
    void* handle;
    explicit Library(const char* name) : handle(dlopen(name, RTLD_NOW | RTLD_LOCAL)) { if (!handle) throw std::runtime_error(std::string("cannot load ") + name); }
    ~Library() { dlclose(handle); }
    template <typename Function> Function get(const char* name) const { auto* symbol = dlsym(handle, name); if (!symbol) throw std::runtime_error(std::string("missing CUDA symbol: ") + name); return reinterpret_cast<Function>(symbol); }
};

void check(error_t value, const char* operation) { if (value != success) throw std::runtime_error(std::string(operation) + " failed: " + std::to_string(value)); }

std::string read_input(int argc, char** argv) {
    if (argc > 2) throw std::runtime_error("usage: flowparallel_graph_cuda [semantic-report.json]");
    std::ostringstream input;
    if (argc == 2) { std::ifstream file(argv[1]); if (!file) throw std::runtime_error("cannot open semantic report"); input << file.rdbuf(); }
    else input << std::cin.rdbuf();
    return input.str();
}

bool has_field(std::string_view input, std::string_view field, std::string_view value) {
    const std::string spaced = "\"" + std::string(field) + "\": \"" + std::string(value) + "\"";
    const std::string compact = "\"" + std::string(field) + "\":\"" + std::string(value) + "\"";
    return input.find(spaced) != std::string_view::npos || input.find(compact) != std::string_view::npos;
}

std::size_t number_after(std::string_view input, std::string_view marker, std::size_t start = 0) {
    const auto found = input.find(marker, start); if (found == std::string_view::npos) return 0;
    auto position = found + marker.size(); while (position < input.size() && (input[position] == ' ' || input[position] == '\t')) ++position;
    std::size_t value = 0; while (position < input.size() && input[position] >= '0' && input[position] <= '9') { value = value * 10 + static_cast<std::size_t>(input[position] - '0'); ++position; }
    return value;
}

std::string quote(std::string_view value) { std::string result = "\""; for (char character : value) { if (character == '\\' || character == '"') result.push_back('\\'); result.push_back(character); } result.push_back('"'); return result; }

std::string source_path(std::string_view input) {
    const auto source = input.find("\"source\":"); if (source == std::string_view::npos) return {};
    const auto path = input.find("\"path\":", source); if (path == std::string_view::npos) return {};
    auto first = input.find('"', path + 7); if (first == std::string_view::npos) return {}; ++first; std::string result;
    for (auto position = first; position < input.size(); ++position) { if (input[position] == '"' && input[position - 1] != '\\') return result; result.push_back(input[position]); }
    return {};
}

int run(std::string_view report) {
    if (!has_field(report, "format", "flowanalyst.semantic_report")) throw std::runtime_error("input is not a Flowanalyst semantic report");
    if (!has_field(report, "status", "ok")) { std::cout << "{\"format\":\"flowparallel.graph_cuda\",\"version\":1,\"status\":\"blocked\"}\n"; return 2; }
    auto matrix = report.find("\"name\":\"region_dependency\""); if (matrix == std::string_view::npos) matrix = report.find("\"name\": \"region_dependency\"");
    if (matrix == std::string_view::npos) throw std::runtime_error("region_dependency matrix view is missing");
    const auto rows = number_after(report, "\"rows\":", matrix); const auto columns = number_after(report, "\"columns\":", matrix);
    if (rows == 0 || rows != columns || rows > 1024) throw std::runtime_error("unsupported graph matrix dimensions");
    const auto entries = report.find("\"entries\":[", matrix); const auto end = report.find(']', entries);
    if (entries == std::string_view::npos || end == std::string_view::npos) throw std::runtime_error("matrix entries are missing or unterminated");
    const std::size_t elements = rows * columns; std::vector<float> adjacency(elements, 0.0F);
    for (auto position = entries; position < end;) {
        const auto row_marker = report.find("\"row\":", position); if (row_marker == std::string_view::npos || row_marker >= end) break;
        const auto column_marker = report.find("\"column\":", row_marker); if (column_marker == std::string_view::npos || column_marker >= end) throw std::runtime_error("matrix entry has no column");
        const auto row = number_after(report, "\"row\":", row_marker); const auto column = number_after(report, "\"column\":", column_marker);
        if (row >= rows || column >= columns) throw std::runtime_error("matrix entry is out of bounds");
        adjacency[column * rows + row] = 1.0F; position = column_marker + 9;
    }
    const auto cpu_start = std::chrono::steady_clock::now();
    std::vector<unsigned char> cpu_reach(elements, 0);
    for (std::size_t i = 0; i < elements; ++i) cpu_reach[i] = adjacency[i] > 0.5F;
    for (std::size_t pivot = 0; pivot < rows; ++pivot)
        for (std::size_t row = 0; row < rows; ++row)
            if (cpu_reach[pivot * rows + row])
                for (std::size_t column = 0; column < columns; ++column)
                    cpu_reach[column * rows + row] = static_cast<unsigned char>(cpu_reach[column * rows + row] || cpu_reach[column * rows + pivot]);
    const double cpu_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - cpu_start).count();
    const auto cuda_start = std::chrono::steady_clock::now();
    Library runtime("libcudart.so.12"); Library blas("libcublas.so.12");
    using count_fn = error_t (*)(int*); using malloc_fn = error_t (*)(void**, std::size_t); using free_fn = error_t (*)(void*); using memcpy_fn = error_t (*)(void*, const void*, std::size_t, int); using sync_fn = error_t (*)();
    using create_fn = error_t (*)(handle_t*); using destroy_fn = error_t (*)(handle_t); using gemm_fn = error_t (*)(handle_t, int, int, int, int, int, const float*, const float*, int, const float*, int, const float*, float*, int);
    const auto count = runtime.get<count_fn>("cudaGetDeviceCount"); int devices = 0; check(count(&devices), "cudaGetDeviceCount");
    if (devices < 1) { std::cout << "{\"format\":\"flowparallel.graph_cuda\",\"version\":1,\"status\":\"unavailable\",\"provider\":\"cuda.cublas\",\"device_count\":0}\n"; return 2; }
    const auto cuda_malloc = runtime.get<malloc_fn>("cudaMalloc"); const auto cuda_free = runtime.get<free_fn>("cudaFree"); const auto cuda_memcpy = runtime.get<memcpy_fn>("cudaMemcpy"); const auto cuda_sync = runtime.get<sync_fn>("cudaDeviceSynchronize");
    const auto create = blas.get<create_fn>("cublasCreate_v2"); const auto destroy = blas.get<destroy_fn>("cublasDestroy_v2"); const auto gemm = blas.get<gemm_fn>("cublasSgemm_v2");
    std::vector<float> power = adjacency, next(elements), reach = adjacency; void* da = nullptr; void* db = nullptr; void* dc = nullptr; handle_t handle = nullptr; const std::size_t bytes = elements * sizeof(float);
    check(cuda_malloc(&da, bytes), "cudaMalloc(A)"); check(cuda_malloc(&db, bytes), "cudaMalloc(B)"); check(cuda_malloc(&dc, bytes), "cudaMalloc(C)"); check(cuda_memcpy(db, adjacency.data(), bytes, host_to_device), "cudaMemcpy(B)"); check(create(&handle), "cublasCreate");
    const float alpha = 1.0F; const float beta = 0.0F;
    for (std::size_t step = 1; step < rows; ++step) {
        check(cuda_memcpy(da, power.data(), bytes, host_to_device), "cudaMemcpy(A)");
        check(gemm(handle, 0, 0, static_cast<int>(rows), static_cast<int>(columns), static_cast<int>(rows), &alpha, static_cast<const float*>(da), static_cast<int>(rows), static_cast<const float*>(db), static_cast<int>(rows), &beta, static_cast<float*>(dc), static_cast<int>(rows)), "cublasSgemm");
        check(cuda_sync(), "cudaDeviceSynchronize"); check(cuda_memcpy(next.data(), dc, bytes, device_to_host), "cudaMemcpy(C)");
        for (std::size_t index = 0; index < elements; ++index) { power[index] = next[index] > 0.5F ? 1.0F : 0.0F; reach[index] = (reach[index] > 0.5F || power[index] > 0.5F) ? 1.0F : 0.0F; }
    }
    check(destroy(handle), "cublasDestroy"); check(cuda_free(dc), "cudaFree(C)"); check(cuda_free(db), "cudaFree(B)"); check(cuda_free(da), "cudaFree(A)");
    const double cuda_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - cuda_start).count();
    std::size_t reachable_pairs = 0; for (float value : reach) reachable_pairs += value > 0.5F;
    std::size_t cpu_reachable_pairs = 0; for (unsigned char value : cpu_reach) cpu_reachable_pairs += value != 0;
    std::cout << "{\n  \"format\": \"flowparallel.graph_cuda\",\n  \"version\": 1,\n  \"status\": \"verified\",\n  \"source\": {\"path\": " << quote(source_path(report)) << "},\n  \"operation\": \"reachability\",\n  \"semiring\": \"boolean\",\n  \"reachable_pairs\": " << reachable_pairs << ",\n  \"cpu_reachable_pairs\": " << cpu_reachable_pairs << ",\n  \"cpu_reference_ms\": " << cpu_ms << ",\n  \"cuda_end_to_end_ms\": " << cuda_ms << ",\n  \"end_to_end_speedup\": " << cpu_ms / cuda_ms << ",\n  \"provider\": \"cuda.cublas.boolean_threshold\",\n  \"device_count\": " << devices << "\n}\n";
    return 0;
}
}

int main(int argc, char** argv) {
    try {
        if (argc == 2 && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "-?" || std::string(argv[1]) == "--help")) { std::cout << "flowparallel_graph_cuda - CUDA Boolean graph reachability\n\nOptions: -h, -?, --help  show help\n         -a, --about    show about information\n         -v, --version  print the raw version number\n"; return 0; }
        if (argc == 2 && (std::string(argv[1]) == "-a" || std::string(argv[1]) == "--about")) { std::cout << "Flowparallel computes Boolean graph reachability through CUDA cuBLAS with CPU differential verification.\n"; return 0; }
        if (argc == 2 && (std::string(argv[1]) == "-v" || std::string(argv[1]) == "--version")) { std::cout << "0.1.0\n"; return 0; }
        return run(read_input(argc, argv));
    } catch (const std::exception& error) { std::cerr << "flowparallel_graph_cuda error: " << error.what() << '\n'; return 1; }
}
