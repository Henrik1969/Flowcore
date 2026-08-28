#!/usr/bin/env python3
"""Validate a Flowmini frontend bundle and emit a lowering skeleton.

This consumer intentionally uses only Python's standard library. It does not
read Flowmini source, import Flowmini modules, or depend on C++ headers.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any


class BundleError(ValueError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise BundleError(message)


def require_object(value: Any, label: str) -> dict[str, Any]:
    require(isinstance(value, dict), f"{label} must be an object")
    return value


def require_array(value: Any, label: str) -> list[Any]:
    require(isinstance(value, list), f"{label} must be an array")
    return value


def indexed_by_id(items: list[Any], label: str) -> dict[int, dict[str, Any]]:
    result: dict[int, dict[str, Any]] = {}
    for item in items:
        obj = require_object(item, f"{label} entry")
        item_id = obj.get("id")
        require(isinstance(item_id, int), f"{label} id must be an integer")
        require(item_id not in result, f"duplicate {label} id {item_id}")
        result[item_id] = obj
    return result


def resolve_json_pointer(document: Any, pointer: str) -> Any:
    require(isinstance(pointer, str), "AST origin path must be a string")
    require(pointer == "" or pointer.startswith("/"),
            f"invalid JSON pointer {pointer!r}")
    current = document
    if pointer == "":
        return current
    for raw_segment in pointer[1:].split("/"):
        segment = raw_segment.replace("~1", "/").replace("~0", "~")
        if isinstance(current, list):
            require(segment.isdigit(),
                    f"array segment {segment!r} is not an index in {pointer!r}")
            index = int(segment)
            require(0 <= index < len(current),
                    f"array index {index} is out of range in {pointer!r}")
            current = current[index]
        elif isinstance(current, dict):
            require(segment in current,
                    f"object key {segment!r} is missing in {pointer!r}")
            current = current[segment]
        else:
            raise BundleError(f"pointer {pointer!r} descends through a scalar")
    return current


def validate_location(value: Any, label: str) -> None:
    if value is None:
        return
    location = require_object(value, label)
    require(isinstance(location.get("file"), str), f"{label}.file must be a string")
    require(isinstance(location.get("line"), int), f"{label}.line must be an integer")
    require(isinstance(location.get("column"), int),
            f"{label}.column must be an integer")


def validate_mapped_location(value: Any, label: str,
                             source_lines: dict[int, dict[str, Any]]) -> None:
    if value is None:
        return
    location = require_object(value, label)
    if "file" in location:
        require(isinstance(location.get("file"), str), f"{label}.file must be a string")
    require(isinstance(location.get("line"), int), f"{label}.line must be an integer")
    require(isinstance(location.get("column"), int),
            f"{label}.column must be an integer")
    line = location.get("line")
    if line == 0:
        return
    require(line in source_lines, f"{label} line {line} is outside the source map")
    require(source_lines[line].get("source_id") is not None,
            f"{label} line {line} maps to generated text")


def validate_ast_locations(value: Any, source_lines: dict[int, dict[str, Any]],
                           path: str = "ast") -> None:
    if isinstance(value, dict):
        for key, child in value.items():
            child_path = f"{path}.{key}"
            if key == "location":
                validate_mapped_location(child, child_path, source_lines)
            else:
                validate_ast_locations(child, source_lines, child_path)
    elif isinstance(value, list):
        for index, child in enumerate(value):
            validate_ast_locations(child, source_lines, f"{path}[{index}]")


def resolve_source_location(value: Any,
                            source_lines: dict[int, dict[str, Any]],
                            source_files: dict[int, dict[str, Any]]) -> Any:
    if value is None or value.get("line") == 0:
        return None
    mapped_line = source_lines[value["line"]]
    source_id = mapped_line["source_id"]
    return {
        "source_id": source_id,
        "path": source_files[source_id]["path"],
        "line": mapped_line["source_line"],
        "column": value["column"],
    }


def validate_fact(fact: Any, label: str, symbol_ids: set[int], scope_ids: set[int]) -> None:
    obj = require_object(fact, label)
    require(isinstance(obj.get("kind"), str), f"{label}.kind must be a string")
    require(isinstance(obj.get("key"), str), f"{label}.key must be a string")
    wrapped = require_object(obj.get("value"), f"{label}.value")
    value_type = wrapped.get("type")
    require(value_type in {
        "null", "bool", "int64", "double", "string",
        "symbol_id", "scope_id", "source_location",
    }, f"{label} has unknown value type {value_type!r}")
    value = wrapped.get("value")
    if value_type == "null":
        require(value is None, f"{label} null value must be null")
    elif value_type == "bool":
        require(isinstance(value, bool), f"{label} bool value must be boolean")
    elif value_type == "int64":
        require(isinstance(value, int) and not isinstance(value, bool),
                f"{label} int64 value must be an integer")
    elif value_type == "double":
        require(
            (isinstance(value, (int, float)) and not isinstance(value, bool))
            or value in {"nan", "positive_infinity", "negative_infinity"},
            f"{label} double value has invalid representation",
        )
    elif value_type == "string":
        require(isinstance(value, str), f"{label} string value must be a string")
    elif value_type == "symbol_id":
        require(value in symbol_ids, f"{label} references unknown symbol {value}")
    elif value_type == "scope_id":
        require(value in scope_ids, f"{label} references unknown scope {value}")
    elif value_type == "source_location":
        validate_location(value, f"{label}.value")


SYMBOL_ORIGIN_CONTRACT: dict[str, tuple[str, str, set[str]]] = {
    "source_unit": ("Module", "source_unit", {"program", "unit"}),
    "import_declaration": ("Import", "declaration", {"import"}),
    "function_declaration": ("Function", "declaration", {"function"}),
    "function_parameter": ("Parameter", "parameter", set()),
    "record_declaration": ("Struct", "declaration", {"record"}),
    "record_field": ("Field", "field", set()),
    "refined_type_declaration": ("Type", "declaration", {"refined_type"}),
    "abi_declaration": ("Contract", "declaration", {"abi"}),
    "abi_type": ("Type", "abi_member", {"type"}),
    "abi_struct": ("Struct", "abi_member", {"struct"}),
    "abi_struct_field": ("Field", "field", set()),
    "extern_function": ("Function", "abi_member", {"extern_function"}),
    "extern_parameter": ("Parameter", "parameter", set()),
    "main_declaration": ("Procedure", "declaration", {"main_block"}),
    "target_declaration": ("Namespace", "declaration", {"target"}),
    "local_binding": ("Variable", "statement", {"let"}),
}


SCOPE_ORIGIN_CONTRACT: dict[str, tuple[str, str, set[str]]] = {
    "module_scope": ("Module", "source_unit", {"program", "unit"}),
    "function_scope": ("Function", "declaration", {"function"}),
    "record_scope": ("Struct", "declaration", {"record"}),
    "abi_scope": ("Contract", "declaration", {"abi"}),
    "abi_struct_scope": ("Struct", "abi_member", {"struct"}),
    "extern_function_scope": ("Function", "abi_member", {"extern_function"}),
    "main_scope": ("Function", "declaration", {"main_block"}),
    "target_scope": ("Namespace", "declaration", {"target"}),
    "if_then_scope": ("Block", "block", set()),
    "while_body_scope": ("Block", "block", set()),
    "else_block_scope": ("Block", "block", set()),
}


def location_coordinates(value: Any, label: str) -> tuple[int, int]:
    location = require_object(value, label)
    line = location.get("line")
    column = location.get("column")
    require(isinstance(line, int), f"{label}.line must be an integer")
    require(isinstance(column, int), f"{label}.column must be an integer")
    return line, column


def validate_origin_path(entity_kind: str, path: str, ast_id: Any, label: str) -> None:
    patterns = {
        "source_unit": r"/source_unit",
        "declaration": r"/declaration_pool/(\d+)",
        "statement": r"/statement_pool/(\d+)",
        "block": r"/block_pool/(\d+)",
        "parameter": r"/declaration_pool/\d+/(?:parameters/\d+|members/\d+/parameters/\d+)",
        "field": r"/declaration_pool/\d+/(?:fields/\d+|members/\d+/fields/\d+)",
        "abi_member": r"/declaration_pool/\d+/members/\d+",
    }
    require(entity_kind in patterns, f"{label} has unknown entity_kind {entity_kind!r}")
    match = re.fullmatch(patterns[entity_kind], path)
    require(match is not None,
            f"{label} path {path!r} does not match entity_kind {entity_kind!r}")
    if entity_kind in {"declaration", "statement", "block"}:
        require(isinstance(ast_id, int) and ast_id == int(match.group(1)),
                f"{label}.ast_id does not match its canonical pool path")
    else:
        require(ast_id is None,
                f"{label}.ast_id must be null for parent-owned {entity_kind}")


def block_origin_causes(statements: dict[int, dict[str, Any]]) -> dict[int, tuple[str, dict[str, Any]]]:
    causes: dict[int, tuple[str, dict[str, Any]]] = {}

    def add(block_id: Any, role: str, statement: dict[str, Any]) -> None:
        require(isinstance(block_id, int), f"{role} block id must be an integer")
        require(block_id not in causes, f"block {block_id} has multiple structural causes")
        causes[block_id] = (role, statement)

    for statement in statements.values():
        payload = statement.get("payload")
        if not isinstance(payload, dict):
            continue
        if statement.get("kind") == "if":
            add(payload.get("then_block"), "if_then_scope", statement)
            else_arm = payload.get("else_arm")
            if isinstance(else_arm, dict) and else_arm.get("kind") == "else_block":
                add(else_arm.get("block"), "else_block_scope", statement)
        elif statement.get("kind") == "while":
            add(payload.get("body_block"), "while_body_scope", statement)
    return causes


def validate_origin_entry(
    origin: dict[str, Any],
    label: str,
    expected_contract: tuple[str, str, set[str]],
    ast: dict[str, Any],
    source_lines: dict[int, dict[str, Any]],
    block_causes: dict[int, tuple[str, dict[str, Any]]],
) -> tuple[Any, tuple[int, int]]:
    _expected_owner_kind, expected_entity_kind, expected_target_kinds = expected_contract
    entity_kind = origin.get("entity_kind")
    role = origin.get("role")
    path = origin.get("ast_path")
    require(isinstance(path, str), f"{label}.ast_path must be a string")
    require(entity_kind == expected_entity_kind,
            f"{label} role {role!r} requires entity_kind {expected_entity_kind!r}")
    validate_origin_path(entity_kind, path, origin.get("ast_id"), label)
    target = resolve_json_pointer(ast, path)
    target_obj = require_object(target, f"{label} target")
    if expected_target_kinds:
        require(target_obj.get("kind") in expected_target_kinds,
                f"{label} resolves to wrong AST kind {target_obj.get('kind')!r}")

    parent_kinds = {
        "function_parameter": "function",
        "record_field": "record",
        "abi_struct_field": "struct",
        "extern_parameter": "extern_function",
    }
    if role in parent_kinds:
        parent_path = path.rsplit("/", 2)[0]
        parent = require_object(resolve_json_pointer(ast, parent_path),
                                f"{label} parent target")
        require(parent.get("kind") == parent_kinds[role],
                f"{label} parent has wrong AST kind {parent.get('kind')!r}")

    location = origin.get("source_location")
    validate_mapped_location(location, f"{label}.source_location", source_lines)
    coordinates = location_coordinates(location, f"{label}.source_location")

    if entity_kind == "block":
        block_id = origin.get("ast_id")
        require(block_id in block_causes, f"{label} block has no structural cause")
        cause_role, cause_statement = block_causes[block_id]
        require(role == cause_role,
                f"{label} role {role!r} disagrees with block cause {cause_role!r}")
        target_location = cause_statement.get("location")
    else:
        target_location = target_obj.get("location")
    require(target_location is not None, f"{label} AST target has no source location")
    require(location_coordinates(target_location, f"{label} target location") == coordinates,
            f"{label} source location disagrees with its AST origin")
    return target, coordinates


def validate_and_build(bundle: dict[str, Any]) -> dict[str, Any]:
    require(bundle.get("format") == "flowmini.frontend_bundle",
            "unsupported frontend bundle format")
    require(bundle.get("version") == 2, "unsupported frontend bundle version")

    source = require_object(bundle.get("source"), "source")
    require(isinstance(source.get("path"), str), "source.path must be a string")
    diagnostics = require_array(bundle.get("diagnostics"), "diagnostics")

    source_map = require_object(bundle.get("source_map"), "source_map")
    require(source_map.get("coordinate_space") == "expanded_lines",
            "unsupported source-map coordinate space")
    source_files = indexed_by_id(require_array(source_map.get("files"),
                                               "source-map files"),
                                 "source file")
    require(source_files, "source map must identify at least one source file")
    for source_id, source_file in source_files.items():
        require(isinstance(source_file.get("path"), str) and source_file.get("path"),
                f"source file {source_id} path must be a nonempty string")

    source_lines: dict[int, dict[str, Any]] = {}
    for entry in require_array(source_map.get("lines"), "source-map lines"):
        line_entry = require_object(entry, "source-map line")
        expanded_line = line_entry.get("expanded_line")
        require(isinstance(expanded_line, int) and expanded_line > 0,
                "expanded source line must be a positive integer")
        require(expanded_line not in source_lines,
                f"duplicate expanded source line {expanded_line}")
        source_id = line_entry.get("source_id")
        source_line = line_entry.get("source_line")
        if source_id is None:
            require(source_line is None,
                    f"generated expanded line {expanded_line} must have null source_line")
        else:
            require(source_id in source_files,
                    f"expanded line {expanded_line} references unknown source {source_id}")
            require(isinstance(source_line, int) and source_line > 0,
                    f"expanded line {expanded_line} source_line must be positive")
        source_lines[expanded_line] = line_entry
    require(set(source_lines) == set(range(1, len(source_lines) + 1)),
            "expanded source-map lines must be contiguous from one")

    ast = require_object(bundle.get("ast"), "ast")
    require(ast.get("format") == "flowmini.ast.v2", "unsupported AST format")
    validate_ast_locations(ast, source_lines)
    source_unit = require_object(ast.get("source_unit"), "ast.source_unit")

    declaration_pool = require_array(ast.get("declaration_pool"), "AST declaration pool")
    expression_pool = require_array(ast.get("expression_pool"), "AST expression pool")
    statement_pool = require_array(ast.get("statement_pool"), "AST statement pool")
    block_pool = require_array(ast.get("block_pool"), "AST block pool")
    declarations = indexed_by_id(declaration_pool, "declaration")
    expressions = indexed_by_id(expression_pool, "expression")
    statements = indexed_by_id(statement_pool, "statement")
    blocks = indexed_by_id(block_pool, "block")
    block_causes = block_origin_causes(statements)
    require(len(declarations) == ast.get("declaration_pool_size"),
            "declaration pool size mismatch")
    require(len(expressions) == ast.get("expression_pool_size"),
            "expression pool size mismatch")
    require(len(statements) == ast.get("statement_pool_size"),
            "statement pool size mismatch")
    require(len(blocks) == ast.get("block_pool_size"), "block pool size mismatch")

    source_declaration_ids = require_array(source_unit.get("declaration_ids"),
                                           "source declaration ids")
    require(len(source_declaration_ids) == len(set(source_declaration_ids)),
            "source unit repeats a declaration id")

    owned_declaration_ids: list[int] = []

    def collect_declaration(declaration_id: Any, owner: str) -> None:
        require(isinstance(declaration_id, int),
                f"{owner} declaration id must be an integer")
        require(declaration_id in declarations,
                f"{owner} references unknown declaration {declaration_id}")
        require(declaration_id not in owned_declaration_ids,
                f"declaration {declaration_id} has multiple structural parents")
        owned_declaration_ids.append(declaration_id)
        declaration = declarations[declaration_id]
        if declaration.get("kind") == "target":
            children = require_array(declaration.get("declaration_ids"),
                                     f"target {declaration_id} declaration ids")
            for child_id in children:
                collect_declaration(child_id, f"target {declaration_id}")

    for declaration_id in source_declaration_ids:
        collect_declaration(declaration_id, "source unit")
    require(set(owned_declaration_ids) == set(declarations),
            "source/target structure must own the complete declaration pool exactly once")

    snapshot = require_object(bundle.get("symbol_table"), "symbol_table")
    require(snapshot.get("format") == "symboltable.snapshot",
            "unsupported SymbolTable snapshot format")
    require(snapshot.get("version") == 1, "unsupported SymbolTable snapshot version")
    scopes = indexed_by_id(require_array(snapshot.get("scopes"), "scopes"), "scope")
    symbols = indexed_by_id(require_array(snapshot.get("symbols"), "symbols"), "symbol")
    global_scope_id = snapshot.get("global_scope_id")
    require(global_scope_id in scopes, "global scope id is missing")

    for scope_id, scope in scopes.items():
        parent_id = scope.get("parent_id")
        owner_id = scope.get("owner_symbol_id")
        require(parent_id is None or parent_id in scopes,
                f"scope {scope_id} has unknown parent {parent_id}")
        require(owner_id is None or owner_id in symbols,
                f"scope {scope_id} has unknown owner symbol {owner_id}")
        for child_id in require_array(scope.get("child_scope_ids"),
                                      f"scope {scope_id} child ids"):
            require(child_id in scopes, f"scope {scope_id} has unknown child {child_id}")
            require(scopes[child_id].get("parent_id") == scope_id,
                    f"scope {scope_id} child {child_id} has inconsistent parent")
        for symbol_id in require_array(scope.get("symbol_ids"),
                                       f"scope {scope_id} symbol ids"):
            require(symbol_id in symbols, f"scope {scope_id} has unknown symbol {symbol_id}")
            require(symbols[symbol_id].get("owning_scope_id") == scope_id,
                    f"scope {scope_id} symbol {symbol_id} has inconsistent owner")

    symbol_ids = set(symbols)
    scope_ids = set(scopes)
    for symbol_id, symbol in symbols.items():
        require(symbol.get("owning_scope_id") in scopes,
                f"symbol {symbol_id} has unknown owning scope")
        introduced_scope = symbol.get("introduced_scope_id")
        require(introduced_scope is None or introduced_scope in scopes,
                f"symbol {symbol_id} introduces unknown scope")
        if introduced_scope is not None:
            require(scopes[introduced_scope].get("owner_symbol_id") == symbol_id,
                    f"symbol {symbol_id} introduced scope has inconsistent owner")
        validate_mapped_location(symbol.get("declaration_location"),
                                 f"symbol {symbol_id} declaration location",
                                 source_lines)
        validate_mapped_location(symbol.get("definition_location"),
                                 f"symbol {symbol_id} definition location",
                                 source_lines)
        for fact_index, fact in enumerate(require_array(symbol.get("facts"),
                                                        f"symbol {symbol_id} facts")):
            validate_fact(fact, f"symbol {symbol_id} fact {fact_index}",
                          symbol_ids, scope_ids)

    origin_contract = require_object(bundle.get("origin_contract"), "origin_contract")
    require(origin_contract.get("format") == "flowmini.structural_origins",
            "unsupported structural-origin format")
    require(origin_contract.get("version") == 1,
            "unsupported structural-origin version")

    symbol_origins: dict[int, dict[str, Any]] = {}
    for entry in require_array(bundle.get("symbol_origins"), "symbol_origins"):
        origin = require_object(entry, "symbol origin")
        symbol_id = origin.get("symbol_id")
        require(symbol_id in symbols, f"origin references unknown symbol {symbol_id}")
        require(symbol_id not in symbol_origins, f"duplicate origin for symbol {symbol_id}")
        role = origin.get("role")
        require(role in SYMBOL_ORIGIN_CONTRACT,
                f"symbol origin {symbol_id} has unknown role {role!r}")
        expected_symbol_kind = SYMBOL_ORIGIN_CONTRACT[role][0]
        require(symbols[symbol_id].get("kind") == expected_symbol_kind,
                f"symbol {symbol_id} kind disagrees with origin role {role!r}")
        _, coordinates = validate_origin_entry(
            origin,
            f"symbol origin {symbol_id}",
            SYMBOL_ORIGIN_CONTRACT[role],
            ast,
            source_lines,
            block_causes,
        )
        require(location_coordinates(symbols[symbol_id].get("declaration_location"),
                                     f"symbol {symbol_id} declaration location") == coordinates,
                f"symbol {symbol_id} declaration location disagrees with its AST origin")
        symbol_origins[symbol_id] = origin
    require(set(symbol_origins) == set(symbols),
            "every projected symbol must have exactly one AST origin")

    scope_origins: dict[int, dict[str, Any]] = {}
    for entry in require_array(bundle.get("scope_origins"), "scope_origins"):
        origin = require_object(entry, "scope origin")
        scope_id = origin.get("scope_id")
        require(scope_id in scopes, f"origin references unknown scope {scope_id}")
        require(scope_id != global_scope_id, "global scope must not claim an AST origin")
        require(scope_id not in scope_origins, f"duplicate origin for scope {scope_id}")
        role = origin.get("role")
        require(role in SCOPE_ORIGIN_CONTRACT,
                f"scope origin {scope_id} has unknown role {role!r}")
        expected_scope_kind = SCOPE_ORIGIN_CONTRACT[role][0]
        require(scopes[scope_id].get("kind") == expected_scope_kind,
                f"scope {scope_id} kind disagrees with origin role {role!r}")
        validate_origin_entry(
            origin,
            f"scope origin {scope_id}",
            SCOPE_ORIGIN_CONTRACT[role],
            ast,
            source_lines,
            block_causes,
        )
        scope_origins[scope_id] = origin
    require(set(scope_origins) == set(scopes) - {global_scope_id},
            "every non-global scope must have exactly one AST origin")

    for scope_id, origin in scope_origins.items():
        owner_symbol_id = scopes[scope_id].get("owner_symbol_id")
        if owner_symbol_id is not None:
            require(symbol_origins[owner_symbol_id].get("ast_path") == origin.get("ast_path"),
                    f"scope {scope_id} and owner symbol {owner_symbol_id} have different origins")

    lowering_declarations = []
    for declaration_id in owned_declaration_ids:
        declaration = declarations[declaration_id]
        lowering_declarations.append({
            "id": declaration_id,
            "kind": declaration.get("kind"),
            "name": declaration.get("name"),
            "symbol_ids": [
                symbol_id for symbol_id, origin in symbol_origins.items()
                if origin.get("ast_path") == f"/declaration_pool/{declaration_id}"
            ],
        })

    lowering_symbols = []
    for symbol_id in sorted(symbols):
        symbol = symbols[symbol_id]
        lowering_symbols.append({
            "id": symbol_id,
            "name": symbol.get("name"),
            "kind": symbol.get("kind"),
            "owning_scope_id": symbol.get("owning_scope_id"),
            "introduced_scope_id": symbol.get("introduced_scope_id"),
            "origin": {
                key: value for key, value in symbol_origins[symbol_id].items()
                if key != "symbol_id"
            },
            "source_location": resolve_source_location(
                symbol.get("declaration_location"), source_lines, source_files
            ),
            "facts": symbol.get("facts", []),
        })

    lowering_scopes = []
    for scope_id in sorted(scopes):
        scope = scopes[scope_id]
        lowering_scopes.append({
            "id": scope_id,
            "kind": scope.get("kind"),
            "parent_id": scope.get("parent_id"),
            "owner_symbol_id": scope.get("owner_symbol_id"),
            "origin": None if scope_id == global_scope_id else {
                key: value for key, value in scope_origins[scope_id].items()
                if key != "scope_id"
            },
            "symbol_ids": scope.get("symbol_ids"),
        })

    lowering_statements = []
    for statement_id in sorted(statements):
        statement = statements[statement_id]
        payload = statement.get("payload") if isinstance(statement.get("payload"), dict) else {}
        target = payload.get("target") if isinstance(payload.get("target"), dict) else {}
        lowering_statements.append({
            "id": statement_id,
            "kind": statement.get("kind"),
            "source_form": payload.get("source_form"),
            "target_kind": target.get("kind"),
            "source_location": resolve_source_location(
                statement.get("location"), source_lines, source_files
            ),
        })

    return {
        "format": "flowmini.lowering_skeleton",
        "version": 1,
        "source": source,
        "source_files": [source_files[source_id] for source_id in sorted(source_files)],
        "expanded_line_count": len(source_lines),
        "source_unit": {
            "kind": source_unit.get("kind"),
            "name": source_unit.get("name"),
        },
        "diagnostic_count": len(diagnostics),
        "declarations": lowering_declarations,
        "scopes": lowering_scopes,
        "symbols": lowering_symbols,
        "origin_index": [
            {
                "ast_path": path,
                "symbol_ids": sorted(
                    symbol_id for symbol_id, origin in symbol_origins.items()
                    if origin.get("ast_path") == path
                ),
                "scope_ids": sorted(
                    scope_id for scope_id, origin in scope_origins.items()
                    if origin.get("ast_path") == path
                ),
            }
            for path in sorted({
                *(origin.get("ast_path") for origin in symbol_origins.values()),
                *(origin.get("ast_path") for origin in scope_origins.values()),
            })
        ],
        "blocks": [
            {"id": block_id, "statement_ids": blocks[block_id].get("statement_ids", [])}
            for block_id in sorted(blocks)
        ],
        "statements": lowering_statements,
        "expression_count": len(expressions),
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Validate a Flowmini frontend bundle and emit a lowering skeleton"
    )
    parser.add_argument("bundle", nargs="?", default="-",
                        help="bundle JSON file, or - for stdin")
    parser.add_argument("--require-declaration-kind", action="append", default=[],
                        help="fail unless the skeleton contains this declaration kind")
    parser.add_argument("--require-statement-kind", action="append", default=[],
                        help="fail unless the skeleton contains this statement kind")
    parser.add_argument("--minimum-source-files", type=int, default=1,
                        help="fail unless at least this many source files are mapped")
    parser.add_argument("--require-symbol-origin-role", action="append", default=[],
                        help="fail unless a symbol origin has this structural role")
    parser.add_argument("--require-scope-origin-role", action="append", default=[],
                        help="fail unless a scope origin has this structural role")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        if args.bundle == "-":
            bundle = json.load(sys.stdin)
        else:
            with Path(args.bundle).open("r", encoding="utf-8") as stream:
                bundle = json.load(stream)
        skeleton = validate_and_build(require_object(bundle, "bundle"))
        declaration_kinds = {item.get("kind") for item in skeleton["declarations"]}
        statement_kinds = {item.get("kind") for item in skeleton["statements"]}
        symbol_origin_roles = {
            item.get("origin", {}).get("role") for item in skeleton["symbols"]
        }
        scope_origin_roles = {
            item.get("origin", {}).get("role")
            for item in skeleton["scopes"] if item.get("origin") is not None
        }
        require(len(skeleton["source_files"]) >= args.minimum_source_files,
                f"expected at least {args.minimum_source_files} mapped source files")
        for required_kind in args.require_declaration_kind:
            require(required_kind in declaration_kinds,
                    f"required declaration kind {required_kind!r} is missing")
        for required_kind in args.require_statement_kind:
            require(required_kind in statement_kinds,
                    f"required statement kind {required_kind!r} is missing")
        for required_role in args.require_symbol_origin_role:
            require(required_role in symbol_origin_roles,
                    f"required symbol origin role {required_role!r} is missing")
        for required_role in args.require_scope_origin_role:
            require(required_role in scope_origin_roles,
                    f"required scope origin role {required_role!r} is missing")
        json.dump(skeleton, sys.stdout, indent=2, ensure_ascii=False)
        sys.stdout.write("\n")
        return 0
    except (BundleError, json.JSONDecodeError, OSError) as error:
        print(f"frontend bundle consumer error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
