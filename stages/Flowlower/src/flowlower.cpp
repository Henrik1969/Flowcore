#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace flowoptimize {

struct SourcePos {
    unsigned line{0};
    unsigned column{0};
};

struct Token {
    std::string type;
    std::string lexeme;
    SourcePos pos{};
};

struct Contract {
    std::string name;
    std::string payload;
    std::string abi;
    std::string protocol;
    std::string lane;
};

struct Port {
    std::string direction;
    std::string lane;
    std::string name;
    std::string contract;
};

struct Node {
    std::string name;
    std::string role;
    std::vector<std::string> effects;
    std::vector<Port> ports;
};

struct Wire {
    std::string name;
    std::string contract;
};

struct Instance {
    std::string name;
    std::string node_type;
    bool synthesized{false};
};

struct Connection {
    std::string from;
    std::string to;
    std::string origin;
};

struct Flow {
    std::string name;
    std::vector<std::string> effects;
    std::vector<Wire> wires;
    std::vector<Instance> instances;
    std::vector<Connection> connections;
};

struct CanonicalModule {
    std::string package_name;
    std::string module_name;
    std::vector<std::string> effects;
    std::vector<Contract> contracts;
    std::vector<Node> nodes;
    std::vector<Flow> flows;
};

[[nodiscard]] std::string json_escape(std::string_view text)
{
    std::string out;
    out.reserve(text.size() + 8);
    static constexpr char hex[] = "0123456789abcdef";
    for (const unsigned char c : text) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    out += "\\u00";
                    out += hex[(c >> 4U) & 0x0FU];
                    out += hex[c & 0x0FU];
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    return out;
}

[[nodiscard]] std::optional<std::string> read_json_string_at(
    const std::string& text,
    std::size_t object_start,
    std::string_view key)
{
    const std::string marker = "\"" + std::string(key) + "\":\"";
    const std::size_t start = text.find(marker, object_start);
    if (start == std::string::npos) {
        return std::nullopt;
    }

    std::size_t pos = start + marker.size();
    std::string out;
    while (pos < text.size()) {
        const char c = text[pos++];
        if (c == '"') {
            return out;
        }
        if (c != '\\') {
            out += c;
            continue;
        }
        if (pos >= text.size()) {
            throw std::runtime_error("truncated JSON escape");
        }
        const char escaped = text[pos++];
        switch (escaped) {
            case '"': out += '"'; break;
            case '\\': out += '\\'; break;
            case '/': out += '/'; break;
            case 'b': out += '\b'; break;
            case 'f': out += '\f'; break;
            case 'n': out += '\n'; break;
            case 'r': out += '\r'; break;
            case 't': out += '\t'; break;
            default:
                throw std::runtime_error("unsupported JSON escape in checked artifact");
        }
    }
    throw std::runtime_error("unterminated JSON string in checked artifact");
}

[[nodiscard]] std::optional<unsigned> read_json_unsigned_at(
    const std::string& text,
    std::size_t object_start,
    std::string_view key)
{
    const std::string marker = "\"" + std::string(key) + "\":";
    const std::size_t start = text.find(marker, object_start);
    if (start == std::string::npos) {
        return std::nullopt;
    }

    std::size_t pos = start + marker.size();
    std::size_t end = pos;
    while (end < text.size() && text[end] >= '0' && text[end] <= '9') {
        ++end;
    }
    if (end == pos) {
        return std::nullopt;
    }
    return static_cast<unsigned>(std::stoul(text.substr(pos, end - pos)));
}

[[nodiscard]] std::vector<Token> read_checked_tokens(std::istream& input)
{
    const std::string artifact{
        std::istreambuf_iterator<char>{input},
        std::istreambuf_iterator<char>{}
    };
    if (artifact.find("\"semantics\":\"ok\"") == std::string::npos) {
        throw std::runtime_error("checked artifact does not report semantics ok");
    }

    std::vector<Token> tokens;
    std::size_t pos = artifact.find("\"tokens\":[");
    if (pos == std::string::npos) {
        throw std::runtime_error("checked artifact has no token stream");
    }

    while (true) {
        pos = artifact.find("\"type\":\"", pos);
        if (pos == std::string::npos) {
            break;
        }
        Token token;
        token.type = read_json_string_at(artifact, pos, "type").value_or("");
        token.lexeme = read_json_string_at(artifact, pos, "lexeme").value_or("");
        token.pos.line = read_json_unsigned_at(artifact, pos, "line").value_or(0);
        token.pos.column = read_json_unsigned_at(artifact, pos, "column").value_or(0);
        tokens.push_back(std::move(token));
        pos += 8;
    }
    tokens.push_back(Token{"EOF", "", SourcePos{}});
    return tokens;
}

[[nodiscard]] bool is_keyword(const Token& token, std::string_view lexeme)
{
    return token.type == "KEYWORD" && token.lexeme == lexeme;
}

[[nodiscard]] bool is_name(const Token& token)
{
    return token.type == "IDENTIFIER" || token.type == "KEYWORD";
}

class Lowerer final {
public:
    explicit Lowerer(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

    [[nodiscard]] CanonicalModule lower()
    {
        while (!at_end()) {
            skip_newlines();
            if (at_end()) break;

            if (match_keyword("package")) {
                module_.package_name = read_qualified_name();
                continue;
            }
            if (match_keyword("module")) {
                module_.module_name = read_qualified_name();
                continue;
            }
            if (match_keyword("effect")) {
                if (auto name = consume_name()) module_.effects.push_back(*name);
                skip_to_newline_or("LEFT_BRACE");
                skip_balanced_body();
                continue;
            }
            if (match_keyword("contract")) {
                module_.contracts.push_back(read_contract());
                continue;
            }
            if (match_keyword("node")) {
                module_.nodes.push_back(read_node());
                continue;
            }
            if (match_keyword("flow")) {
                module_.flows.push_back(read_flow());
                continue;
            }
            skip_to_newline_or("LEFT_BRACE");
            skip_balanced_body();
        }
        return module_;
    }

private:
    [[nodiscard]] bool at_end() const { return current().type == "EOF"; }
    [[nodiscard]] const Token& current() const { return tokens_[position_]; }
    [[nodiscard]] const Token& peek(std::size_t lookahead = 1) const
    {
        return tokens_[std::min(position_ + lookahead, tokens_.size() - 1)];
    }
    void advance()
    {
        if (!at_end()) ++position_;
    }
    bool match(std::string_view type)
    {
        if (current().type != type) return false;
        advance();
        return true;
    }
    bool match_keyword(std::string_view lexeme)
    {
        if (!is_keyword(current(), lexeme)) return false;
        advance();
        return true;
    }
    void skip_newlines()
    {
        while (match("NEWLINE")) {
        }
    }
    void skip_to_newline_or(std::string_view token_type)
    {
        while (!at_end() && current().type != "NEWLINE" && current().type != token_type) advance();
    }
    void skip_balanced_body()
    {
        if (!match("LEFT_BRACE")) return;
        int depth = 1;
        while (!at_end() && depth > 0) {
            if (match("LEFT_BRACE")) ++depth;
            else if (match("RIGHT_BRACE")) --depth;
            else advance();
        }
    }
    [[nodiscard]] std::optional<std::string> consume_name()
    {
        if (!is_name(current())) return std::nullopt;
        std::string name = current().lexeme;
        advance();
        return name;
    }
    [[nodiscard]] std::string read_qualified_name()
    {
        std::string name;
        if (auto first = consume_name()) name = *first;
        while (match("DOT")) {
            if (auto part = consume_name()) {
                name += ".";
                name += *part;
            }
        }
        skip_to_newline_or("LEFT_BRACE");
        return name;
    }
    [[nodiscard]] std::string read_property_value()
    {
        std::string value;
        if (is_name(current()) || current().type == "INTEGER_LITERAL" ||
            current().type == "STRING_LITERAL" || current().type == "FLOAT_LITERAL") {
            value = current().lexeme;
            advance();
            while (match("DOT")) {
                if (is_name(current())) {
                    value += ".";
                    value += current().lexeme;
                    advance();
                }
            }
        }
        skip_to_newline_or("NEWLINE");
        return value;
    }
    [[nodiscard]] std::vector<std::string> read_effect_set()
    {
        std::vector<std::string> effects;
        if (!match("LEFT_BRACE")) return effects;
        while (!at_end() && !match("RIGHT_BRACE")) {
            if (is_name(current())) effects.push_back(current().lexeme);
            advance();
        }
        return effects;
    }
    [[nodiscard]] Contract read_contract()
    {
        Contract contract;
        if (auto name = consume_name()) contract.name = *name;
        skip_to_newline_or("LEFT_BRACE");
        if (!match("LEFT_BRACE")) return contract;
        int depth = 1;
        while (!at_end() && depth > 0) {
            if (match("LEFT_BRACE")) {
                ++depth;
                continue;
            }
            if (match("RIGHT_BRACE")) {
                --depth;
                continue;
            }
            if (depth == 1 && is_keyword(current(), "payload")) {
                advance();
                contract.payload = read_property_value();
                continue;
            }
            if (depth == 1 && is_keyword(current(), "abi")) {
                advance();
                contract.abi = read_property_value();
                continue;
            }
            if (depth == 1 && is_keyword(current(), "protocol")) {
                advance();
                contract.protocol = read_property_value();
                continue;
            }
            if (depth == 1 && is_keyword(current(), "lane")) {
                advance();
                contract.lane = read_property_value();
                continue;
            }
            advance();
        }
        return contract;
    }
    [[nodiscard]] Node read_node()
    {
        Node node;
        if (auto name = consume_name()) node.name = *name;
        if (match("COLON") && is_name(current())) {
            node.role = current().lexeme;
            advance();
        }
        if (!match("LEFT_BRACE")) return node;
        int depth = 1;
        while (!at_end() && depth > 0) {
            if (match("LEFT_BRACE")) {
                ++depth;
                continue;
            }
            if (match("RIGHT_BRACE")) {
                --depth;
                continue;
            }
            if (depth == 1 && (is_keyword(current(), "in") || is_keyword(current(), "out") ||
                               is_keyword(current(), "tap") || is_keyword(current(), "control"))) {
                node.ports.push_back(read_port());
                continue;
            }
            if (depth == 1 && match_keyword("effects")) {
                node.effects = read_effect_set();
                continue;
            }
            advance();
        }
        return node;
    }
    [[nodiscard]] Port read_port()
    {
        Port port;
        port.direction = current().lexeme;
        advance();
        if (match("DOT") && is_name(current())) {
            port.lane = current().lexeme;
            advance();
        }
        if (auto name = consume_name()) port.name = *name;
        if (match("COLON")) {
            if (auto contract = consume_name()) port.contract = *contract;
        }
        skip_to_newline_or("NEWLINE");
        return port;
    }
    [[nodiscard]] Flow read_flow()
    {
        Flow flow;
        if (auto name = consume_name()) flow.name = *name;
        if (match_keyword("effects")) {
            flow.effects = read_effect_set();
        }
        if (!match("LEFT_BRACE")) return flow;

        int sugar_index = 0;
        int depth = 1;
        while (!at_end() && depth > 0) {
            if (match("LEFT_BRACE")) {
                ++depth;
                continue;
            }
            if (match("RIGHT_BRACE")) {
                --depth;
                continue;
            }
            if (depth == 1 && match_keyword("wire")) {
                flow.wires.push_back(read_wire());
                continue;
            }
            if (depth == 1 && match_keyword("node")) {
                flow.instances.push_back(read_instance(false));
                continue;
            }
            if (depth == 1 && match_keyword("connect")) {
                flow.connections.push_back(read_connection("explicit"));
                continue;
            }
            if (depth == 1 && current().type == "IDENTIFIER" && peek().type == "BIND_LEFT") {
                lower_sugar(flow, ++sugar_index);
                continue;
            }
            advance();
        }
        return flow;
    }
    [[nodiscard]] Wire read_wire()
    {
        Wire wire;
        if (auto name = consume_name()) wire.name = *name;
        if (match("COLON")) {
            if (auto contract = consume_name()) wire.contract = *contract;
        }
        skip_to_newline_or("NEWLINE");
        return wire;
    }
    [[nodiscard]] Instance read_instance(bool synthesized)
    {
        Instance instance;
        if (auto name = consume_name()) instance.name = *name;
        if (match("COLON")) {
            if (auto node_type = consume_name()) instance.node_type = *node_type;
        }
        instance.synthesized = synthesized;
        skip_to_newline_or("NEWLINE");
        return instance;
    }
    [[nodiscard]] std::vector<std::string> read_endpoint()
    {
        std::vector<std::string> parts;
        if (!is_name(current())) return parts;
        parts.push_back(current().lexeme);
        advance();
        while (match("DOT")) {
            if (!is_name(current())) break;
            parts.push_back(current().lexeme);
            advance();
        }
        return parts;
    }
    [[nodiscard]] std::string join_endpoint(const std::vector<std::string>& parts)
    {
        std::string out;
        for (std::size_t index = 0; index < parts.size(); ++index) {
            if (index != 0) out += ".";
            out += parts[index];
        }
        return out;
    }
    [[nodiscard]] Connection read_connection(std::string origin)
    {
        Connection connection;
        connection.origin = std::move(origin);
        connection.from = join_endpoint(read_endpoint());
        match("GRAPH_ARROW");
        connection.to = join_endpoint(read_endpoint());
        skip_to_newline_or("NEWLINE");
        return connection;
    }
    [[nodiscard]] const Node* find_node(std::string_view name) const
    {
        const auto found = std::find_if(module_.nodes.begin(), module_.nodes.end(), [&](const Node& node) {
            return node.name == name;
        });
        return found == module_.nodes.end() ? nullptr : &*found;
    }
    void lower_sugar(Flow& flow, int sugar_index)
    {
        const std::string target_wire = current().lexeme;
        advance();
        match("BIND_LEFT");
        const std::string node_type = consume_name().value_or("");
        const std::string instance_name = "__sugar_" + std::to_string(sugar_index) + "_" + node_type;
        flow.instances.push_back(Instance{instance_name, node_type, true});

        std::vector<std::string> args;
        if (match("LEFT_PAREN")) {
            while (!at_end() && !match("RIGHT_PAREN")) {
                if (current().type == "IDENTIFIER") args.push_back(current().lexeme);
                advance();
            }
        }

        const Node* node = find_node(node_type);
        if (node != nullptr) {
            std::vector<Port> inputs;
            std::vector<Port> outputs;
            for (const Port& port : node->ports) {
                if (port.direction == "in") inputs.push_back(port);
                if (port.direction == "out") outputs.push_back(port);
            }
            for (std::size_t index = 0; index < args.size() && index < inputs.size(); ++index) {
                const Port& port = inputs[index];
                flow.connections.push_back(Connection{
                    args[index],
                    instance_name + "." + port.direction + "." + port.lane + "." + port.name,
                    "lowered_flow_sugar"
                });
            }
            const auto output = std::find_if(outputs.begin(), outputs.end(), [](const Port& port) {
                return port.lane == "data";
            });
            if (output != outputs.end()) {
                flow.connections.push_back(Connection{
                    instance_name + "." + output->direction + "." + output->lane + "." + output->name,
                    target_wire,
                    "lowered_flow_sugar"
                });
            }
        }
        skip_to_newline_or("NEWLINE");
    }

    std::vector<Token> tokens_;
    std::size_t position_{0};
    CanonicalModule module_;
};

void write_string_array(std::ostream& output, const std::vector<std::string>& values)
{
    output << "[";
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index != 0) output << ",";
        output << "\"" << json_escape(values[index]) << "\"";
    }
    output << "]";
}

void write_canonical(std::ostream& output, const CanonicalModule& module)
{
    output << "{\n";
    output << "  \"optimizer\":\"flowoptimize-canonical-cpp\",\n";
    output << "  \"ir\":\"flowcore.canonical_graph.v0\",\n";
    output << "  \"package\":\"" << json_escape(module.package_name) << "\",\n";
    output << "  \"module\":\"" << json_escape(module.module_name) << "\",\n";
    output << "  \"effects\":";
    write_string_array(output, module.effects);
    output << ",\n";

    output << "  \"contracts\":[";
    for (std::size_t index = 0; index < module.contracts.size(); ++index) {
        const Contract& contract = module.contracts[index];
        if (index != 0) output << ",";
        output << "\n    {\"name\":\"" << json_escape(contract.name)
               << "\",\"payload\":\"" << json_escape(contract.payload)
               << "\",\"abi\":\"" << json_escape(contract.abi)
               << "\",\"protocol\":\"" << json_escape(contract.protocol)
               << "\",\"lane\":\"" << json_escape(contract.lane) << "\"}";
    }
    if (!module.contracts.empty()) output << "\n  ";
    output << "],\n";

    output << "  \"nodes\":[";
    for (std::size_t node_index = 0; node_index < module.nodes.size(); ++node_index) {
        const Node& node = module.nodes[node_index];
        if (node_index != 0) output << ",";
        output << "\n    {\"name\":\"" << json_escape(node.name)
               << "\",\"role\":\"" << json_escape(node.role)
               << "\",\"effects\":";
        write_string_array(output, node.effects);
        output << ",\"ports\":[";
        for (std::size_t port_index = 0; port_index < node.ports.size(); ++port_index) {
            const Port& port = node.ports[port_index];
            if (port_index != 0) output << ",";
            output << "{\"direction\":\"" << json_escape(port.direction)
                   << "\",\"lane\":\"" << json_escape(port.lane)
                   << "\",\"name\":\"" << json_escape(port.name)
                   << "\",\"contract\":\"" << json_escape(port.contract) << "\"}";
        }
        output << "]}";
    }
    if (!module.nodes.empty()) output << "\n  ";
    output << "],\n";

    output << "  \"flows\":[";
    for (std::size_t flow_index = 0; flow_index < module.flows.size(); ++flow_index) {
        const Flow& flow = module.flows[flow_index];
        if (flow_index != 0) output << ",";
        output << "\n    {\"name\":\"" << json_escape(flow.name) << "\",\"effects\":";
        write_string_array(output, flow.effects);
        output << ",\"wires\":[";
        for (std::size_t index = 0; index < flow.wires.size(); ++index) {
            const Wire& wire = flow.wires[index];
            if (index != 0) output << ",";
            output << "{\"name\":\"" << json_escape(wire.name)
                   << "\",\"contract\":\"" << json_escape(wire.contract) << "\"}";
        }
        output << "],\"instances\":[";
        for (std::size_t index = 0; index < flow.instances.size(); ++index) {
            const Instance& instance = flow.instances[index];
            if (index != 0) output << ",";
            output << "{\"name\":\"" << json_escape(instance.name)
                   << "\",\"node_type\":\"" << json_escape(instance.node_type)
                   << "\",\"synthesized\":" << (instance.synthesized ? "true" : "false") << "}";
        }
        output << "],\"connections\":[";
        for (std::size_t index = 0; index < flow.connections.size(); ++index) {
            const Connection& connection = flow.connections[index];
            if (index != 0) output << ",";
            output << "{\"from\":\"" << json_escape(connection.from)
                   << "\",\"to\":\"" << json_escape(connection.to)
                   << "\",\"origin\":\"" << json_escape(connection.origin) << "\"}";
        }
        output << "]}";
    }
    if (!module.flows.empty()) output << "\n  ";
    output << "]\n";
    output << "}\n";
}

} // namespace flowoptimize

int main()
{
    try {
        auto tokens = flowoptimize::read_checked_tokens(std::cin);
        flowoptimize::Lowerer lowerer{std::move(tokens)};
        const flowoptimize::CanonicalModule module = lowerer.lower();
        flowoptimize::write_canonical(std::cout, module);
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "fatal optimizer error: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
