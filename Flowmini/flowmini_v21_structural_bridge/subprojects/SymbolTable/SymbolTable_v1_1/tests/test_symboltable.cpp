#include "symboltable/SymbolTable.hpp"
#include "symboltable/SymbolPolicy.hpp"
#include "symboltable/SymbolTreeView.hpp"

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>

using namespace symboltable;

static void test_global_scope_exists()
{
    SymbolTable table;
    const auto global = table.globalScope();
    assert(global.value == 1);
    assert(table.scope(global).kind == ScopeKind::Global);
}

static void test_insert_and_local_lookup()
{
    SymbolTable table;
    const auto global = table.globalScope();
    const auto foo = table.insertSymbol(global, "foo", SymbolKind::Function);

    const auto found = table.findLocal(global, "foo");
    assert(found.size() == 1);
    assert(found.front() == foo);
    assert(table.symbol(foo).name == "foo");
    assert(table.symbol(foo).kind == SymbolKind::Function);
}

static void test_overloads_or_duplicates_are_preserved()
{
    SymbolTable table;
    const auto global = table.globalScope();
    const auto foo1 = table.insertSymbol(global, "foo", SymbolKind::Function);
    const auto foo2 = table.insertSymbol(global, "foo", SymbolKind::Procedure);

    const auto found = table.findLocal(global, "foo");
    assert(found.size() == 2);
    assert(found.at(0) == foo1);
    assert(found.at(1) == foo2);
}

static void test_scope_resolution_upwards()
{
    SymbolTable table;
    const auto global = table.globalScope();
    const auto globalX = table.insertSymbol(global, "x", SymbolKind::Variable);

    const auto fn = table.insertSymbol(global, "do_work", SymbolKind::Function);
    const auto fnScope = table.createScope(ScopeKind::Function, global, fn, "do_work");
    const auto localX = table.insertSymbol(fnScope, "x", SymbolKind::Parameter);

    const auto found = table.resolveUpwards(fnScope, "x");
    assert(found.size() == 2);
    assert(found.at(0) == localX);
    assert(found.at(1) == globalX);
}

static void test_symbol_introduced_scope()
{
    SymbolTable table;
    const auto global = table.globalScope();
    const auto klass = table.insertSymbol(global, "Box", SymbolKind::Class);
    const auto klassScope = table.createScope(ScopeKind::Class, global, klass, "Box");

    assert(table.symbol(klass).introducedScope.has_value());
    assert(*table.symbol(klass).introducedScope == klassScope);
    assert(table.scope(klassScope).ownerSymbol.has_value());
    assert(*table.scope(klassScope).ownerSymbol == klass);
}

static void test_factoid_attachment()
{
    SymbolTable table;
    const auto global = table.globalScope();
    const auto sym = table.insertSymbol(global, "secret", SymbolKind::Field);

    table.symbol(sym).visibility = Visibility::Private;
    table.addFact(sym, Factoid{
        .kind = FactoidKind::AbiTag,
        .key = "abi",
        .value = std::string{"cdecl"}
    });

    assert(table.symbol(sym).visibility == Visibility::Private);
    assert(table.symbol(sym).facts.size() == 1);
    assert(table.symbol(sym).facts.front().key == "abi");
    assert(std::get<std::string>(table.symbol(sym).facts.front().value) == "cdecl");
}


static void test_tree_view_expose_all_policy()
{
    SymbolTable table;
    const auto global = table.globalScope();
    const auto publicSym = table.insertSymbol(global, "visible", SymbolKind::Variable);
    const auto privateSym = table.insertSymbol(global, "hidden", SymbolKind::Variable);
    table.symbol(privateSym).visibility = Visibility::Private;

    ExposeAllPolicy policy;
    SymbolTreeView view{table, policy, global};

    assert(view.resolveVisible("visible").size() == 1);
    assert(view.resolveVisible("visible").front() == publicSym);
    assert(view.resolveVisible("hidden").size() == 1);
    assert(view.resolveVisible("hidden").front() == privateSym);
}

static void test_tree_view_can_hide_private_factoids_without_mutating_table()
{
    SymbolTable table;
    const auto global = table.globalScope();

    const auto klass = table.insertSymbol(global, "Box", SymbolKind::Class);
    const auto klassScope = table.createScope(ScopeKind::Class, global, klass, "Box");

    const auto secret = table.insertSymbol(klassScope, "secret", SymbolKind::Field);
    table.symbol(secret).visibility = Visibility::Private;

    const auto externalFunction = table.insertSymbol(global, "external", SymbolKind::Function);
    const auto externalScope = table.createScope(ScopeKind::Function, global, externalFunction, "external");

    const auto rawFromClass = table.resolveUpwards(klassScope, "secret");
    assert(rawFromClass.size() == 1);
    assert(rawFromClass.front() == secret);

    HidePrivatePolicy policy;
    SymbolTreeView insideView{table, policy, klassScope};
    SymbolTreeView outsideView{table, policy, externalScope};

    const auto insideVisible = insideView.resolveVisible("secret");
    assert(insideVisible.size() == 1);
    assert(insideVisible.front() == secret);

    const auto outsideInspection = outsideView.inspectSymbolsIn(klassScope);
    assert(outsideInspection.size() == 1);
    assert(outsideInspection.front().symbol == secret);
    assert(outsideInspection.front().kind == LookupDecisionKind::Hidden);

    const auto outsideVisible = outsideView.visibleSymbolsIn(klassScope);
    assert(outsideVisible.empty());

    // The raw table still stores the fact. It has not enforced anything.
    assert(table.symbol(secret).visibility == Visibility::Private);
    assert(table.scope(klassScope).symbols.size() == 1);
}

static void test_dump()
{
    SymbolTable table;
    const auto global = table.globalScope();
    [[maybe_unused]] const auto foo = table.insertSymbol(global, "foo", SymbolKind::Function);

    std::ostringstream out;
    table.dump(out);
    const auto text = out.str();
    assert(text.find("SymbolTable dump") != std::string::npos);
    assert(text.find("foo") != std::string::npos);
}

int main()
{
    test_global_scope_exists();
    test_insert_and_local_lookup();
    test_overloads_or_duplicates_are_preserved();
    test_scope_resolution_upwards();
    test_symbol_introduced_scope();
    test_factoid_attachment();
    test_tree_view_expose_all_policy();
    test_tree_view_can_hide_private_factoids_without_mutating_table();
    test_dump();

    std::cout << "All SymbolTable v1.1 tests passed.\n";
    return 0;
}
