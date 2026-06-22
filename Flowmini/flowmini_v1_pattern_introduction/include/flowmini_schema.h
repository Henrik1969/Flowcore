#ifndef FLOWMINI_SCHEMA_H
#define FLOWMINI_SCHEMA_H

#include <map>
#include <string>
#include <vector>

namespace flowmini {

struct Endpoint {
    std::string node;
    std::string port;
};

struct NodeDecl {
    std::string role; // producer, node, sink
    std::string id;
    std::string kind;
};

struct WireDecl {
    Endpoint from;
    Endpoint to;
};

struct ModuleSpec {
    std::string name;
    std::vector<NodeDecl> nodes;
    std::vector<WireDecl> wires;
};

struct AtomContract {
    std::string kind;
    std::map<std::string, std::string> inputs;
    std::map<std::string, std::string> outputs;
    std::vector<std::string> effects;
};

} // namespace flowmini

#endif // FLOWMINI_SCHEMA_H
