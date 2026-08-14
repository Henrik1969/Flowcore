#include "flowmini_symbol_projection.h"

#include <string>
#include <variant>

namespace flowmini::ast {
namespace {

std::string source_unit_debug_name(const SourceUnit& unit) {
    if (!unit.name.empty()) {
        return unit.name;
    }

    switch (unit.kind) {
        case SourceUnitKind::Program: return "<anonymous-program>";
        case SourceUnitKind::Unit:    return "<anonymous-unit>";
        default:                      return "<anonymous-source-unit>";
    }
}

symboltable::SymbolKind source_unit_symbol_kind(const SourceUnitKind kind) {
    switch (kind) {
        case SourceUnitKind::Program:
        case SourceUnitKind::Unit:
            return symboltable::SymbolKind::Module;

        default:
            return symboltable::SymbolKind::Unknown;
    }
}

void project_import_decl(symboltable::SymbolTable& table,
                         const symboltable::ScopeId moduleScope,
                         const ImportDecl& decl) {
    const auto name = decl.module_name.empty()
        ? std::string{"<anonymous-import>"}
        : decl.module_name;

    table.insertSymbol(moduleScope, name, symboltable::SymbolKind::Import);
}


std::string statement_scope_debug_name(const Statement& statement) {
    switch (statement.kind) {
        case StatementKind::If:    return "if";
        case StatementKind::While: return "while";
        default:                   return "block";
    }
}

void project_statement_bindings(symboltable::SymbolTable& table,
                                symboltable::ScopeId owningScope,
                                const std::vector<Statement>& statements);

void project_statement_binding(symboltable::SymbolTable& table,
                               const symboltable::ScopeId owningScope,
                               const Statement& statement) {
    if (statement.kind == StatementKind::Let && !statement.name.empty()) {
        table.insertSymbol(owningScope,
                           statement.name,
                           symboltable::SymbolKind::Variable);
    }

    if (statement.has_body) {
        const auto blockScope =
            table.createScope(symboltable::ScopeKind::Block,
                              owningScope,
                              std::nullopt,
                              statement_scope_debug_name(statement));

        project_statement_bindings(table, blockScope, statement.body);
    }

    if (statement.else_location.has_value()) {
        const auto elseScope =
            table.createScope(symboltable::ScopeKind::Block,
                              owningScope,
                              std::nullopt,
                              "else");

        project_statement_bindings(table, elseScope, statement.else_body);
    }
}

void project_statement_bindings(symboltable::SymbolTable& table,
                                const symboltable::ScopeId owningScope,
                                const std::vector<Statement>& statements) {
    for (const auto& statement : statements) {
        project_statement_binding(table, owningScope, statement);
    }
}


void project_function_decl(symboltable::SymbolTable& table,
                           const symboltable::ScopeId moduleScope,
                           const FunctionDecl& decl) {
    const auto name = decl.name.empty()
        ? std::string{"<anonymous-function>"}
        : decl.name;

    const auto fnSymbol =
        table.insertSymbol(moduleScope, name, symboltable::SymbolKind::Function);

    const auto fnScope =
        table.createScope(symboltable::ScopeKind::Function,
                          moduleScope,
                          fnSymbol,
                          name);

    for (const auto& parameter : decl.parameters) {
        if (parameter.name.empty()) {
            continue;
        }

        table.insertSymbol(fnScope,
                           parameter.name,
                           symboltable::SymbolKind::Parameter);
    }

    project_statement_bindings(table, fnScope, decl.body);
}

void project_record_decl(symboltable::SymbolTable& table,
                         const symboltable::ScopeId moduleScope,
                         const RecordDecl& decl) {
    const auto name = decl.name.empty()
        ? std::string{"<anonymous-record>"}
        : decl.name;

    const auto recordSymbol =
        table.insertSymbol(moduleScope, name, symboltable::SymbolKind::Struct);

    const auto recordScope =
        table.createScope(symboltable::ScopeKind::Struct,
                          moduleScope,
                          recordSymbol,
                          name);

    for (const auto& field : decl.fields) {
        if (field.name.empty()) {
            continue;
        }

        table.insertSymbol(recordScope,
                           field.name,
                           symboltable::SymbolKind::Field);
    }
}

void project_type_alias_decl(symboltable::SymbolTable& table,
                             const symboltable::ScopeId moduleScope,
                             const TypeAliasDecl& decl) {
    const auto name = decl.name.empty()
        ? std::string{"<anonymous-alias>"}
        : decl.name;

    table.insertSymbol(moduleScope, name, symboltable::SymbolKind::Alias);
}

void project_main_block(symboltable::SymbolTable& table,
                        const symboltable::ScopeId moduleScope,
                        [[maybe_unused]] const MainBlock& decl) {
    constexpr auto name = "main";

    const auto mainSymbol =
        table.insertSymbol(moduleScope, name, symboltable::SymbolKind::Procedure);

    const auto mainScope =
        table.createScope(symboltable::ScopeKind::Function,
                          moduleScope,
                          mainSymbol,
                          name);

    project_statement_bindings(table, mainScope, decl.body);
}

struct ProjectTopLevelDecl {
    symboltable::SymbolTable& table;
    symboltable::ScopeId moduleScope;

    void operator()(const ImportDecl& decl) const {
        project_import_decl(table, moduleScope, decl);
    }

    void operator()(const FunctionDecl& decl) const {
        project_function_decl(table, moduleScope, decl);
    }

    void operator()(const RecordDecl& decl) const {
        project_record_decl(table, moduleScope, decl);
    }

    void operator()(const TypeAliasDecl& decl) const {
        project_type_alias_decl(table, moduleScope, decl);
    }

    void operator()(const MainBlock& decl) const {
        project_main_block(table, moduleScope, decl);
    }
};

} // namespace

symboltable::SymbolTable build_symbol_table_projection(const AstModule& module) {
    symboltable::SymbolTable table;

    const auto global = table.globalScope();
    const auto moduleName = source_unit_debug_name(module.source_unit);

    const auto moduleSymbol =
        table.insertSymbol(global,
                           moduleName,
                           source_unit_symbol_kind(module.source_unit.kind));

    const auto moduleScope =
        table.createScope(symboltable::ScopeKind::Module,
                          global,
                          moduleSymbol,
                          moduleName);

    for (const auto& declaration : module.source_unit.declarations) {
        std::visit(ProjectTopLevelDecl{table, moduleScope}, declaration);
    }

    return table;
}

} // namespace flowmini::ast
