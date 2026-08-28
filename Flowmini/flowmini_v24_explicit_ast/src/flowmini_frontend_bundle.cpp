#include "flowmini_frontend_bundle.h"

#include <iomanip>
#include <map>
#include <ostream>
#include <string>
#include <vector>

namespace flowmini::ast {
namespace {

void dump_json_string(std::ostream& out, const std::string_view value) {
    out << '"';
    for (const unsigned char ch : value) {
        switch (ch) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (ch < 0x20) {
                    out << "\\u00"
                        << std::hex << std::setw(2) << std::setfill('0')
                        << static_cast<unsigned int>(ch)
                        << std::dec << std::setfill(' ');
                } else {
                    out << static_cast<char>(ch);
                }
        }
    }
    out << '"';
}

} // namespace

void dump_frontend_bundle_json(std::ostream& out,
                               const AstModule& module,
                               const SymbolProjection& projection,
                               const std::string_view sourcePath,
                               const std::vector<FrontendSourceLineOrigin>& lineOrigins) {
    std::map<std::string, std::size_t> fileIds;
    std::vector<std::string> files;
    for (const auto& origin : lineOrigins) {
        if (origin.source_path.empty() || fileIds.contains(origin.source_path)) {
            continue;
        }
        const auto id = files.size();
        files.push_back(origin.source_path);
        fileIds.emplace(origin.source_path, id);
    }

    out << "{\n"
        << "  \"format\": \"flowmini.frontend_bundle\",\n"
        << "  \"version\": 1,\n"
        << "  \"source\": {\"path\": ";
    dump_json_string(out, sourcePath);
    out << "},\n"
        << "  \"source_map\": {\n"
        << "    \"coordinate_space\": \"expanded_lines\",\n"
        << "    \"files\": [\n";

    for (std::size_t index = 0; index < files.size(); ++index) {
        out << "      {\"id\": " << index << ", \"path\": ";
        dump_json_string(out, files[index]);
        out << '}';
        if (index + 1 < files.size()) {
            out << ',';
        }
        out << '\n';
    }

    out << "    ],\n"
        << "    \"lines\": [\n";

    for (std::size_t index = 0; index < lineOrigins.size(); ++index) {
        const auto& origin = lineOrigins[index];
        out << "      {\"expanded_line\": " << (index + 1)
            << ", \"source_id\": ";
        if (origin.source_path.empty()) {
            out << "null, \"source_line\": null";
        } else {
            out << fileIds.at(origin.source_path)
                << ", \"source_line\": " << origin.source_line;
        }
        out << '}';
        if (index + 1 < lineOrigins.size()) {
            out << ',';
        }
        out << '\n';
    }

    out << "    ]\n"
        << "  },\n"
        << "  \"ast\": ";
    dump_ast_json(out, module);
    out << ",\n"
        << "  \"symbol_table\": ";
    projection.table.dumpJson(out);
    out << ",\n"
        << "  \"symbol_origins\": [\n";

    for (std::size_t index = 0; index < projection.symbol_origins.size(); ++index) {
        const auto& origin = projection.symbol_origins[index];
        out << "    {\"symbol_id\": " << origin.symbol_id.value
            << ", \"ast_path\": ";
        dump_json_string(out, origin.ast_path);
        out << '}';
        if (index + 1 < projection.symbol_origins.size()) {
            out << ',';
        }
        out << '\n';
    }

    out << "  ],\n"
        << "  \"scope_origins\": [\n";

    for (std::size_t index = 0; index < projection.scope_origins.size(); ++index) {
        const auto& origin = projection.scope_origins[index];
        out << "    {\"scope_id\": " << origin.scope_id.value
            << ", \"ast_path\": ";
        dump_json_string(out, origin.ast_path);
        out << '}';
        if (index + 1 < projection.scope_origins.size()) {
            out << ',';
        }
        out << '\n';
    }

    out << "  ],\n"
        << "  \"diagnostics\": []\n"
        << "}\n";
}

} // namespace flowmini::ast
