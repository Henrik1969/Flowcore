#include "flowmini_symbol_projection.h"

#include <cstdint>
#include <string>
#include <utility>
#include <variant>

namespace flowmini::ast {
namespace {

symboltable::SourceLocation project_source_location(const SourceLocation& location) {
    return symboltable::SourceLocation{
        .file = {},
        .line = static_cast<std::uint32_t>(location.line),
        .column = static_cast<std::uint32_t>(location.column),
    };
}

void set_declaration_location(symboltable::SymbolTable& table,
                              const symboltable::SymbolId symbol,
                              const SourceLocation& location) {
    table.symbol(symbol).declarationLocation = project_source_location(location);
}

void add_string_fact(symboltable::SymbolTable& table,
                     const symboltable::SymbolId symbol,
                     const symboltable::FactoidKind kind,
                     std::string key,
                     std::string value) {
    table.addFact(symbol, symboltable::Factoid{
        .kind = kind,
        .key = std::move(key),
        .value = std::move(value),
    });
}

void add_type_spelling_fact(symboltable::SymbolTable& table,
                            const symboltable::SymbolId symbol,
                            std::string key,
                            const TypeRef& type) {
    auto spelling = type_ref_text(type);
    if (spelling.empty()) {
        return;
    }

    add_string_fact(table,
                    symbol,
                    symboltable::FactoidKind::TypeReference,
                    std::move(key),
                    std::move(spelling));
}

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

    const auto importSymbol =
        table.insertSymbol(moduleScope, name, symboltable::SymbolKind::Import);
    set_declaration_location(table, importSymbol, decl.location);
}


std::string statement_scope_debug_name(const Statement& statement) {
    switch (statement_kind(statement)) {
        case StatementKind::If:    return "if";
        case StatementKind::While: return "while";
        default:                   return "block";
    }
}

void project_block(symboltable::SymbolTable& table,
                   const AstModule& module,
                   symboltable::ScopeId owningScope,
                   BlockId blockId);

void project_statement_binding(symboltable::SymbolTable& table,
                               const AstModule& module,
                               const symboltable::ScopeId owningScope,
                               const StatementId statementId) {
    if (statementId >= module.statement_pool.size()) {
        return;
    }
    const auto& statement = module.statement_pool[statementId];

    if (const auto* binding = std::get_if<LetStatement>(&statement.payload);
        binding && !binding->name.empty()) {
        const auto variableSymbol =
            table.insertSymbol(owningScope,
                               binding->name,
                               symboltable::SymbolKind::Variable);
        set_declaration_location(table, variableSymbol, statement.location);
        add_type_spelling_fact(table,
                               variableSymbol,
                               "declared_type_spelling",
                               binding->type);
    }

    std::optional<BlockId> body;
    const std::optional<ElseArm>* elseArm = nullptr;
    if (const auto* ifStatement = std::get_if<IfStatement>(&statement.payload)) {
        body = ifStatement->then_block;
        elseArm = &ifStatement->else_arm;
    } else if (const auto* whileStatement = std::get_if<WhileStatement>(&statement.payload)) {
        body = whileStatement->body_block;
    }

    if (body && *body < module.block_pool.size()) {
        const auto blockScope =
            table.createScope(symboltable::ScopeKind::Block,
                              owningScope,
                              std::nullopt,
                              statement_scope_debug_name(statement));

        project_block(table, module, blockScope, *body);
    }

    if (elseArm && *elseArm) {
        if (const auto* elseBlock = std::get_if<ElseBlock>(&**elseArm)) {
            if (elseBlock->block < module.block_pool.size()) {
                const auto elseScope =
                    table.createScope(symboltable::ScopeKind::Block,
                                      owningScope,
                                      std::nullopt,
                                      "else");
                project_block(table, module, elseScope, elseBlock->block);
            }
        } else if (const auto* elseIf = std::get_if<ElseIf>(&**elseArm)) {
            project_statement_binding(table, module, owningScope, elseIf->if_statement);
        }
    }
}

void project_block(symboltable::SymbolTable& table,
                   const AstModule& module,
                   const symboltable::ScopeId owningScope,
                   const BlockId blockId) {
    if (blockId >= module.block_pool.size()) {
        return;
    }
    for (const auto statementId : module.block_pool[blockId].statements) {
        project_statement_binding(table, module, owningScope, statementId);
    }
}


void project_function_decl(symboltable::SymbolTable& table,
                           const AstModule& module,
                           const symboltable::ScopeId moduleScope,
                           const FunctionDecl& decl) {
    const auto name = decl.name.empty()
        ? std::string{"<anonymous-function>"}
        : decl.name;

    const auto fnSymbol =
        table.insertSymbol(moduleScope, name, symboltable::SymbolKind::Function);
    set_declaration_location(table, fnSymbol, decl.location);
    add_type_spelling_fact(table,
                           fnSymbol,
                           "return_type_spelling",
                           decl.return_type);

    const auto fnScope =
        table.createScope(symboltable::ScopeKind::Function,
                          moduleScope,
                          fnSymbol,
                          name);

    for (const auto& parameter : decl.parameters) {
        if (parameter.name.empty()) {
            continue;
        }

        const auto parameterSymbol =
            table.insertSymbol(fnScope,
                               parameter.name,
                               symboltable::SymbolKind::Parameter);
        set_declaration_location(table, parameterSymbol, parameter.location);
        add_type_spelling_fact(table,
                               parameterSymbol,
                               "declared_type_spelling",
                               parameter.type);
    }

    if (decl.body) {
        project_block(table, module, fnScope, *decl.body);
    }
}

void project_record_decl(symboltable::SymbolTable& table,
                         const symboltable::ScopeId moduleScope,
                         const RecordDecl& decl) {
    const auto name = decl.name.empty()
        ? std::string{"<anonymous-record>"}
        : decl.name;

    const auto recordSymbol =
        table.insertSymbol(moduleScope, name, symboltable::SymbolKind::Struct);
    set_declaration_location(table, recordSymbol, decl.location);

    const auto recordScope =
        table.createScope(symboltable::ScopeKind::Struct,
                          moduleScope,
                          recordSymbol,
                          name);

    for (const auto& field : decl.fields) {
        if (field.name.empty()) {
            continue;
        }

        const auto fieldSymbol =
            table.insertSymbol(recordScope,
                               field.name,
                               symboltable::SymbolKind::Field);
        set_declaration_location(table, fieldSymbol, field.location);
        add_type_spelling_fact(table,
                               fieldSymbol,
                               "declared_type_spelling",
                               field.type);
    }
}

void project_type_alias_decl(symboltable::SymbolTable& table,
                             const symboltable::ScopeId moduleScope,
                             const TypeAliasDecl& decl) {
    const auto name = decl.name.empty()
        ? std::string{"<anonymous-alias>"}
        : decl.name;

    const auto aliasSymbol =
        table.insertSymbol(moduleScope, name, symboltable::SymbolKind::Alias);
    set_declaration_location(table, aliasSymbol, decl.location);
    add_type_spelling_fact(table,
                           aliasSymbol,
                           "target_type_spelling",
                           decl.target);
}

void project_main_block(symboltable::SymbolTable& table,
                        const AstModule& module,
                        const symboltable::ScopeId moduleScope,
                        const MainBlock& decl) {
    constexpr auto name = "main";

    const auto mainSymbol =
        table.insertSymbol(moduleScope, name, symboltable::SymbolKind::Procedure);
    set_declaration_location(table, mainSymbol, decl.location);

    const auto mainScope =
        table.createScope(symboltable::ScopeKind::Function,
                          moduleScope,
                          mainSymbol,
                          name);

    if (decl.body) {
        project_block(table, module, mainScope, *decl.body);
    }
}

struct ProjectTopLevelDecl {
    symboltable::SymbolTable& table;
    const AstModule& module;
    symboltable::ScopeId moduleScope;

    void operator()(const ImportDecl& decl) const {
        project_import_decl(table, moduleScope, decl);
    }

    void operator()(const FunctionDecl& decl) const {
        project_function_decl(table, module, moduleScope, decl);
    }

    void operator()(const RecordDecl& decl) const {
        project_record_decl(table, moduleScope, decl);
    }

    void operator()(const TypeAliasDecl& decl) const {
        project_type_alias_decl(table, moduleScope, decl);
    }

    void operator()(const MainBlock& decl) const {
        project_main_block(table, module, moduleScope, decl);
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
    set_declaration_location(table, moduleSymbol, module.source_unit.location);
    add_string_fact(table,
                    moduleSymbol,
                    symboltable::FactoidKind::Custom,
                    "source_unit_kind",
                    to_string(module.source_unit.kind));

    const auto moduleScope =
        table.createScope(symboltable::ScopeKind::Module,
                          global,
                          moduleSymbol,
                          moduleName);

    for (const auto& declaration : module.source_unit.declarations) {
        std::visit(ProjectTopLevelDecl{table, module, moduleScope}, declaration);
    }

    return table;
}

} // namespace flowmini::ast
