#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
constexpr std::string_view version = "0.1.0";

std::string read_input(int argc, char** argv) {
    if (argc > 2) throw std::runtime_error("usage: flowparallel_graph_reference [semantic-report.json]");
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
    const auto found = input.find(marker, start);
    if (found == std::string_view::npos) return 0;
    auto position = found + marker.size();
    while (position < input.size() && (input[position] == ' ' || input[position] == '\t')) ++position;
    std::size_t value = 0;
    while (position < input.size() && input[position] >= '0' && input[position] <= '9') { value = value * 10 + static_cast<std::size_t>(input[position] - '0'); ++position; }
    return value;
}

std::string quote(std::string_view value) {
    std::string result = "\"";
    for (char character : value) { if (character == '\\' || character == '"') result.push_back('\\'); result.push_back(character); }
    result.push_back('"');
    return result;
}

std::string source_path(std::string_view input) {
    const auto source = input.find("\"source\":");
    if (source == std::string_view::npos) return {};
    const auto path = input.find("\"path\":", source);
    if (path == std::string_view::npos) return {};
    auto first = input.find('"', path + 7);
    if (first == std::string_view::npos) return {};
    ++first;
    std::string result;
    for (auto position = first; position < input.size(); ++position) { if (input[position] == '"' && input[position - 1] != '\\') return result; result.push_back(input[position]); }
    return {};
}

int run(std::string_view report) {
    if (!has_field(report, "format", "flowanalyst.semantic_report")) throw std::runtime_error("input is not a Flowanalyst semantic report");
    if (!has_field(report, "status", "ok")) {
        std::cout << "{\n  \"format\": \"flowparallel.graph_analysis\",\n  \"version\": 1,\n  \"status\": \"blocked\",\n  \"reason\": \"semantic report is not accepted\"\n}\n";
        return 2;
    }
    auto matrix = report.find("\"name\":\"region_dependency\"");
    if (matrix == std::string_view::npos) matrix = report.find("\"name\": \"region_dependency\"");
    if (matrix == std::string_view::npos) throw std::runtime_error("region_dependency matrix view is missing");
    const auto rows = number_after(report, "\"rows\":", matrix);
    const auto columns = number_after(report, "\"columns\":", matrix);
    if (rows == 0 || rows != columns || rows > 4096) throw std::runtime_error("unsupported graph matrix dimensions");
    std::vector<unsigned char> reach(rows * columns, 0);
    const auto entries = report.find("\"entries\":[", matrix);
    if (entries == std::string_view::npos) throw std::runtime_error("matrix entries are missing");
    const auto end = report.find(']', entries);
    if (end == std::string_view::npos) throw std::runtime_error("matrix entries are unterminated");
    for (auto position = entries; position < end;) {
        const auto row_marker = report.find("\"row\":", position);
        if (row_marker == std::string_view::npos || row_marker >= end) break;
        const auto column_marker = report.find("\"column\":", row_marker);
        if (column_marker == std::string_view::npos || column_marker >= end) throw std::runtime_error("matrix entry has no column");
        const auto row = number_after(report, "\"row\":", row_marker);
        const auto column = number_after(report, "\"column\":", column_marker);
        if (row >= rows || column >= columns) throw std::runtime_error("matrix entry is out of bounds");
        reach[row * columns + column] = 1;
        position = column_marker + 9;
    }
    for (std::size_t pivot = 0; pivot < rows; ++pivot)
        for (std::size_t row = 0; row < rows; ++row)
            if (reach[row * columns + pivot])
                for (std::size_t column = 0; column < columns; ++column)
                    reach[row * columns + column] = static_cast<unsigned char>(reach[row * columns + column] || reach[pivot * columns + column]);
    std::size_t reachable_pairs = 0;
    for (unsigned char value : reach) reachable_pairs += value != 0;
    std::cout << "{\n  \"format\": \"flowparallel.graph_analysis\",\n  \"version\": 1,\n  \"status\": \"verified\",\n  \"source\": {\"path\": " << quote(source_path(report)) << "},\n  \"operation\": \"reachability\",\n  \"input\": {\"format\": \"flowanalyst.analysis_graph\", \"matrix\": \"region_dependency\", \"rows\": " << rows << ", \"columns\": " << columns << "},\n  \"semantics\": {\"semiring\": \"boolean\", \"paths\": \"one-or-more-edges\"},\n  \"reachable_pairs\": " << reachable_pairs << ",\n  \"provider\": \"cpu.reference\"\n}\n";
    return 0;
}
}

int main(int argc, char** argv) {
    try {
        if (argc == 2 && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "-?" || std::string(argv[1]) == "--help")) { std::cout << "flowparallel_graph_reference - CPU reference graph reachability\n\nOptions: -h, -?, --help  show help\n         -a, --about    show about information\n         -v, --version  print the raw version number\n"; return 0; }
        if (argc == 2 && (std::string(argv[1]) == "-a" || std::string(argv[1]) == "--about")) { std::cout << "Flowparallel computes verified Boolean graph reachability as the CPU reference provider.\n"; return 0; }
        if (argc == 2 && (std::string(argv[1]) == "-v" || std::string(argv[1]) == "--version")) { std::cout << version << '\n'; return 0; }
        return run(read_input(argc, argv));
    } catch (const std::exception& error) { std::cerr << "flowparallel_graph_reference error: " << error.what() << '\n'; return 1; }
}
