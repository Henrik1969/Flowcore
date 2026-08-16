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

    [[maybe_unused]] const auto importSymbol =
        table.insertSymbol(moduleScope, name, symboltable::SymbolKind::Import);
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
        [[maybe_unused]] const auto variableSymbol =
            table.insertSymbol(owningScope,
                               binding->name,
                               symboltable::SymbolKind::Variable);
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

    const auto fnScope =
        table.createScope(symboltable::ScopeKind::Function,
                          moduleScope,
                          fnSymbol,
                          name);

    for (const auto& parameter : decl.parameters) {
        if (parameter.name.empty()) {
            continue;
        }

        [[maybe_unused]] const auto parameterSymbol =
            table.insertSymbol(fnScope,
                               parameter.name,
                               symboltable::SymbolKind::Parameter);
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

    const auto recordScope =
        table.createScope(symboltable::ScopeKind::Struct,
                          moduleScope,
                          recordSymbol,
                          name);

    for (const auto& field : decl.fields) {
        if (field.name.empty()) {
            continue;
        }

        [[maybe_unused]] const auto fieldSymbol =
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

    [[maybe_unused]] const auto aliasSymbol =
        table.insertSymbol(moduleScope, name, symboltable::SymbolKind::Alias);
}

void project_main_block(symboltable::SymbolTable& table,
                        const AstModule& module,
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
