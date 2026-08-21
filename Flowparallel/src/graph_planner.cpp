#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
constexpr std::string_view version = "0.1.0";
std::string read_file(const std::string& path) { std::ifstream file(path); if (!file) throw std::runtime_error("cannot open " + path); std::ostringstream text; text << file.rdbuf(); return text.str(); }
bool has(std::string_view text, std::string_view field, std::string_view value) { return text.find("\"" + std::string(field) + "\":\"" + std::string(value) + "\"") != std::string_view::npos || text.find("\"" + std::string(field) + "\": \"" + std::string(value) + "\"") != std::string_view::npos; }
std::size_t number_after(std::string_view text, std::string_view marker) { const auto at = text.find(marker); if (at == std::string_view::npos) return 0; auto p = at + marker.size(); while (p < text.size() && (text[p] == ' ' || text[p] == '\t')) ++p; std::size_t result = 0; while (p < text.size() && text[p] >= '0' && text[p] <= '9') { result = result * 10 + static_cast<std::size_t>(text[p] - '0'); ++p; } return result; }
std::string quote(std::string_view value) { std::string result = "\""; for (const char c : value) { if (c == '\\' || c == '"') result.push_back('\\'); result.push_back(c); } result.push_back('"'); return result; }
struct Options { std::string graph, capabilities, calibration; double density = 0.25; double min_speedup = 1.25; };
Options parse(int argc, char** argv) { Options o; for (int i = 1; i < argc; ++i) { const std::string arg = argv[i]; auto req = [&](const char* name) { if (++i >= argc) throw std::runtime_error(std::string(name) + " requires a value"); return std::string(argv[i]); }; if (arg == "--graph") o.graph = req("--graph"); else if (arg == "--capabilities") o.capabilities = req("--capabilities"); else if (arg == "--calibration") o.calibration = req("--calibration"); else if (arg == "--density-threshold") o.density = std::stod(req("--density-threshold")); else if (arg == "--min-speedup") o.min_speedup = std::stod(req("--min-speedup")); else throw std::runtime_error("unknown option: " + arg); } if (o.graph.empty() || o.capabilities.empty()) throw std::runtime_error("--graph and --capabilities are required"); if (o.density < 0.0 || o.density > 1.0 || o.min_speedup <= 0.0) throw std::runtime_error("invalid policy threshold"); return o; }
int run(const Options& o) {
    const auto graph = read_file(o.graph), capabilities = read_file(o.capabilities);
    if (!has(graph, "format", "flowanalyst.semantic_report") || !has(graph, "status", "ok")) throw std::runtime_error("graph is not an accepted semantic report");
    const auto matrix = graph.find("\"name\":\"region_dependency\"") != std::string::npos ? graph.find("\"name\":\"region_dependency\"") : graph.find("\"name\": \"region_dependency\"");
    if (matrix == std::string::npos) throw std::runtime_error("region_dependency matrix view is missing");
    const auto view = std::string_view(graph).substr(matrix); const auto rows = number_after(view, "\"rows\":"); const auto columns = number_after(view, "\"columns\":");
    if (rows == 0 || rows != columns) throw std::runtime_error("unsupported graph dimensions");
    const auto entries = graph.find("\"entries\":[", matrix); if (entries == std::string::npos) throw std::runtime_error("matrix entries are missing"); const auto end = graph.find(']', entries); std::size_t edges = 0;
    for (auto p = entries; p < end;) { p = graph.find("\"row\":", p); if (p == std::string::npos || p >= end) break; ++edges; p += 6; }
    const double density = static_cast<double>(edges) / static_cast<double>(rows * columns); const bool dense = density >= o.density; const bool cuda = has(capabilities, "status", "available") && number_after(capabilities, "\"device_count\":") > 0;
    bool calibrated = false; double speedup = 0.0;
    if (!o.calibration.empty()) { const auto calibration = read_file(o.calibration); const bool known_format = has(calibration, "format", "flowparallel.matrix_benchmark") || has(calibration, "format", "flowparallel.graph_cuda"); calibrated = known_format && has(calibration, "status", "verified"); const auto marker = calibration.find("\"end_to_end_speedup\":"); if (marker != std::string::npos) { const auto colon = calibration.find(':', marker); speedup = std::stod(calibration.substr(colon + 1)); } }
    const bool select_cuda = dense && cuda && calibrated && speedup >= o.min_speedup; const std::string provider = select_cuda ? "cuda.cublas.boolean_threshold" : "cpu.reference";
    const std::string reason = select_cuda ? "dense graph, CUDA available, and verified calibration exceeds threshold" : !dense ? "sparse graph below density threshold; retain CPU reference provider" : !cuda ? "CUDA is unavailable; retain required CPU fallback" : !calibrated ? "no verified CUDA calibration; retain required CPU fallback" : "verified CUDA calibration is below configured speedup threshold";
    std::cout << "{\n  \"format\": \"flowparallel.graph_provider_decision\",\n  \"version\": 1,\n  \"status\": \"verified\",\n  \"policy\": {\"density_threshold\": " << o.density << ", \"minimum_end_to_end_speedup\": " << o.min_speedup << "},\n  \"graph\": {\"rows\": " << rows << ", \"columns\": " << columns << ", \"edges\": " << edges << ", \"density\": " << density << "},\n  \"provider\": " << quote(provider) << ",\n  \"representation\": " << quote(dense ? "dense" : "sparse") << ",\n  \"reason\": " << quote(reason) << ",\n  \"fallback\": \"cpu.reference\"\n}\n";
    return 0;
}
}
int main(int argc, char** argv) { try { if (argc == 2 && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "-?" || std::string(argv[1]) == "--help")) { std::cout << "flowparallel_graph_planner - choose graph representation and provider\n"; return 0; } if (argc == 2 && (std::string(argv[1]) == "-a" || std::string(argv[1]) == "--about")) { std::cout << "Flowparallel applies explicit sparse/dense and runtime-provider policy to graph projections.\n"; return 0; } if (argc == 2 && (std::string(argv[1]) == "-v" || std::string(argv[1]) == "--version")) { std::cout << version << '\n'; return 0; } return run(parse(argc, argv)); } catch (const std::exception& error) { std::cerr << "flowparallel_graph_planner error: " << error.what() << '\n'; return 1; } }
