#include <dlfcn.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
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
    template <typename Function> Function get(const char* name) const {
        auto* symbol = dlsym(handle, name);
        if (!symbol) throw std::runtime_error(std::string("missing symbol: ") + name);
        return reinterpret_cast<Function>(symbol);
    }
};

void check(error_t value, const char* operation) { if (value != success) throw std::runtime_error(std::string(operation) + " failed: " + std::to_string(value)); }
struct Options { int size = 512; int iterations = 5; };

Options parse(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--size" || arg == "--iterations") {
            if (++i >= argc) throw std::runtime_error(arg + " requires a value");
            int value = std::stoi(argv[i]);
            if (arg == "--size") options.size = value; else options.iterations = value;
        } else if (arg == "-h" || arg == "-?" || arg == "--help") {
            std::cout << "flowparallel_matrix_benchmark - CPU/CUDA matrix benchmark\n\nOptions: --size N --iterations N\n         -h, -?, --help  show help\n         -a, --about    show about information\n         -v, --version  print the raw version number\n"; std::exit(0);
        } else if (arg == "-a" || arg == "--about") { std::cout << "Flowparallel compares a single-thread CPU matrix baseline with CUDA cuBLAS.\n"; std::exit(0); }
        else if (arg == "-v" || arg == "--version") { std::cout << "0.1.0\n"; std::exit(0); }
        else throw std::runtime_error("unknown option '" + arg + "'");
    }
    if (options.size < 32 || options.size > 2048 || options.iterations < 2 || options.iterations > 100) throw std::runtime_error("benchmark dimensions are outside safe bounds");
    return options;
}

double checksum(const std::vector<float>& values) { double result = 0.0; for (float value : values) result += value; return result; }

int run(const Options& options) {
    const int n = options.size;
    const std::size_t elements = static_cast<std::size_t>(n) * n;
    const std::size_t bytes = elements * sizeof(float);
    std::vector<float> a(elements), b(elements), cpu(elements, 0.0F), gpu(elements, 0.0F);
    for (int row = 0; row < n; ++row) for (int column = 0; column < n; ++column) {
        a[static_cast<std::size_t>(row) * n + column] = row == column ? 2.0F : 1.0F;
        b[static_cast<std::size_t>(row) * n + column] = row == column ? 3.0F : 1.0F;
    }

    const auto cpu_start = std::chrono::steady_clock::now();
    for (int iteration = 0; iteration < options.iterations; ++iteration) {
        for (int row = 0; row < n; ++row) for (int column = 0; column < n; ++column) {
            float value = 0.0F;
            for (int inner = 0; inner < n; ++inner) value += a[static_cast<std::size_t>(row) * n + inner] * b[static_cast<std::size_t>(inner) * n + column];
            cpu[static_cast<std::size_t>(row) * n + column] = value;
        }
    }
    const double cpu_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - cpu_start).count() / options.iterations;

    Library runtime("libcudart.so.12");
    Library blas("libcublas.so.12");
    using malloc_fn = error_t (*)(void**, std::size_t); using free_fn = error_t (*)(void*); using memcpy_fn = error_t (*)(void*, const void*, std::size_t, int); using sync_fn = error_t (*)();
    using create_fn = error_t (*)(handle_t*); using destroy_fn = error_t (*)(handle_t); using gemm_fn = error_t (*)(handle_t, int, int, int, int, int, const float*, const float*, int, const float*, int, const float*, float*, int);
    const auto cuda_malloc = runtime.get<malloc_fn>("cudaMalloc"); const auto cuda_free = runtime.get<free_fn>("cudaFree"); const auto cuda_memcpy = runtime.get<memcpy_fn>("cudaMemcpy"); const auto cuda_sync = runtime.get<sync_fn>("cudaDeviceSynchronize");
    const auto create = blas.get<create_fn>("cublasCreate_v2"); const auto destroy = blas.get<destroy_fn>("cublasDestroy_v2"); const auto gemm = blas.get<gemm_fn>("cublasSgemm_v2");
    void* da = nullptr; void* db = nullptr; void* dc = nullptr; handle_t handle = nullptr;
    check(cuda_malloc(&da, bytes), "cudaMalloc(A)"); check(cuda_malloc(&db, bytes), "cudaMalloc(B)"); check(cuda_malloc(&dc, bytes), "cudaMalloc(C)");
    check(cuda_memcpy(da, a.data(), bytes, host_to_device), "cudaMemcpy(A)"); check(cuda_memcpy(db, b.data(), bytes, host_to_device), "cudaMemcpy(B)"); check(create(&handle), "cublasCreate");
    const float alpha = 1.0F; const float beta = 0.0F;
    gemm(handle, 0, 0, n, n, n, &alpha, static_cast<const float*>(da), n, static_cast<const float*>(db), n, &beta, static_cast<float*>(dc), n);
    check(cuda_sync(), "warmup");
    const auto gpu_start = std::chrono::steady_clock::now();
    for (int iteration = 0; iteration < options.iterations; ++iteration) check(gemm(handle, 0, 0, n, n, n, &alpha, static_cast<const float*>(da), n, static_cast<const float*>(db), n, &beta, static_cast<float*>(dc), n), "cublasSgemm");
    check(cuda_sync(), "cudaDeviceSynchronize");
    const double gpu_compute_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - gpu_start).count() / options.iterations;
    const auto end_to_end_start = std::chrono::steady_clock::now();
    for (int iteration = 0; iteration < options.iterations; ++iteration) {
        check(cuda_memcpy(da, a.data(), bytes, host_to_device), "cudaMemcpy(A)");
        check(cuda_memcpy(db, b.data(), bytes, host_to_device), "cudaMemcpy(B)");
        check(gemm(handle, 0, 0, n, n, n, &alpha, static_cast<const float*>(da), n, static_cast<const float*>(db), n, &beta, static_cast<float*>(dc), n), "cublasSgemm");
        check(cuda_sync(), "cudaDeviceSynchronize");
        check(cuda_memcpy(gpu.data(), dc, bytes, device_to_host), "cudaMemcpy(C)");
    }
    const double end_to_end_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - end_to_end_start).count() / options.iterations;
    check(destroy(handle), "cublasDestroy"); check(cuda_free(dc), "cudaFree(C)"); check(cuda_free(db), "cudaFree(B)"); check(cuda_free(da), "cudaFree(A)");
    double max_error = 0.0; for (std::size_t i = 0; i < elements; ++i) max_error = std::max(max_error, std::fabs(static_cast<double>(cpu[i]) - gpu[i]));
    std::cout << std::setprecision(10) << "{\n  \"format\": \"flowparallel.matrix_benchmark\",\n  \"status\": \"verified\",\n  \"matrix_size\": " << n << ",\n  \"iterations\": " << options.iterations << ",\n  \"cpu_single_thread_ms\": " << cpu_ms << ",\n  \"cuda_cublas_compute_ms\": " << gpu_compute_ms << ",\n  \"cuda_end_to_end_ms\": " << end_to_end_ms << ",\n  \"compute_speedup\": " << cpu_ms / gpu_compute_ms << ",\n  \"end_to_end_speedup\": " << cpu_ms / end_to_end_ms << ",\n  \"max_error\": " << max_error << ",\n  \"cpu_checksum\": " << checksum(cpu) << ",\n  \"cuda_checksum\": " << checksum(gpu) << "\n}\n";
    return max_error < 0.001 ? 0 : 2;
}
}

int main(int argc, char** argv) { try { return run(parse(argc, argv)); } catch (const std::exception& error) { std::cerr << "flowparallel_matrix_benchmark error: " << error.what() << '\n'; return 1; } }
