#include "symboltable/SymbolTable.hpp"

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
    test_dump();

    std::cout << "All SymbolTable v1 tests passed.\n";
    return 0;
}
