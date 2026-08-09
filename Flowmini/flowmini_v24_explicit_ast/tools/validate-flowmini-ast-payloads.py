#!/usr/bin/env python3

"""Validate canonical Flowmini AST payloads and compatibility projections."""

import json
import sys
from pathlib import Path


def fail(path: Path, expression_id: int, message: str) -> None:
    raise ValueError(f"{path}: expression {expression_id}: {message}")


def optional_id(payload: dict, name: str) -> list[int]:
    value = payload.get(name)
    if value is None:
        return []
    if not isinstance(value, int):
        raise TypeError(f"payload field {name!r} must be an expression id or null")
    return [value]


def id_array(payload: dict, name: str) -> list[int]:
    value = payload.get(name)
    if not isinstance(value, list) or not all(isinstance(item, int) for item in value):
        raise TypeError(f"payload field {name!r} must be an array of expression ids")
    return value


def payload_children(kind: str, payload: dict) -> list[int]:
    if kind in {
        "unknown",
        "identifier",
        "integer_literal",
        "float_literal",
        "string_literal",
        "bool_literal",
    }:
        return []
    if kind == "unary":
        return optional_id(payload, "operand")
    if kind == "binary":
        return optional_id(payload, "left") + optional_id(payload, "right")
    if kind == "call":
        return optional_id(payload, "base") + id_array(payload, "arguments")
    if kind == "index":
        return optional_id(payload, "base") + id_array(payload, "indexes")
    if kind == "field_access":
        field = payload.get("field")
        if not isinstance(field, str) or not field:
            raise TypeError("field_access payload requires a non-empty field name")
        return optional_id(payload, "base")
    if kind == "list_literal":
        return id_array(payload, "elements")
    if kind == "record_literal":
        fields = payload.get("fields")
        if not isinstance(fields, list):
            raise TypeError("record_literal payload field 'fields' must be an array")

        children: list[int] = []
        for field in fields:
            if not isinstance(field, dict):
                raise TypeError("record literal fields must be objects")
            if not isinstance(field.get("name"), str) or not field["name"]:
                raise TypeError("record literal fields require non-empty names")
            location = field.get("location")
            if not isinstance(location, dict) or not all(
                isinstance(location.get(component), int) for component in ("line", "column")
            ):
                raise TypeError("record literal fields require line/column locations")
            children.extend(optional_id(field, "value"))
        return children

    raise ValueError(f"unsupported expression kind {kind!r}")


def location(value: object, context: str) -> None:
    if not isinstance(value, dict) or not all(
        isinstance(value.get(component), int) for component in ("line", "column")
    ):
        raise TypeError(f"{context} requires a line/column location")


def string_segments(payload: dict, name: str) -> list[str]:
    value = payload.get(name)
    if not isinstance(value, list) or not value or not all(
        isinstance(segment, str) and segment for segment in value
    ):
        raise TypeError(f"type payload field {name!r} must be a non-empty string array")
    return value


def validate_type_ref(type_ref: object, context: str) -> str:
    if not isinstance(type_ref, dict):
        raise TypeError(f"{context}: canonical type_ref object is required")

    kind = type_ref.get("kind")
    text = type_ref.get("text")
    payload = type_ref.get("payload")
    if not isinstance(kind, str) or not isinstance(text, str) or not isinstance(payload, dict):
        raise TypeError(f"{context}: type_ref requires kind, text, and payload")
    location(type_ref.get("location"), f"{context}: type_ref")

    if kind == "unknown":
        canonical = payload.get("text")
        if not isinstance(canonical, str):
            raise TypeError(f"{context}: unknown type payload requires text")
    elif kind == "named":
        canonical = ".".join(string_segments(payload, "name_segments"))
    elif kind == "generic":
        constructor = ".".join(string_segments(payload, "constructor_segments"))
        arguments = payload.get("arguments")
        if not isinstance(arguments, list) or not arguments:
            raise TypeError(f"{context}: generic type requires one or more arguments")
        canonical = constructor + "<" + ",".join(
            validate_type_ref(argument, f"{context}: generic argument {index}")
            for index, argument in enumerate(arguments)
        ) + ">"
    elif kind == "array":
        element = payload.get("element_type")
        if element is None:
            raise TypeError(f"{context}: array type requires an element_type")
        canonical = "array<" + validate_type_ref(element, f"{context}: array element") + ">"
        extents = payload.get("extents")
        if not isinstance(extents, list):
            raise TypeError(f"{context}: array extents must be an array")
        extent_texts: list[str] = []
        for index, extent in enumerate(extents):
            if not isinstance(extent, dict) or not isinstance(extent.get("text"), str) or not extent["text"]:
                raise TypeError(f"{context}: array extent {index} requires non-empty text")
            location(extent.get("location"), f"{context}: array extent {index}")
            extent_texts.append(extent["text"])
        if extent_texts:
            canonical += "[" + ",".join(extent_texts) + "]"
    else:
        raise ValueError(f"{context}: unsupported type_ref kind {kind!r}")

    if canonical != text:
        raise ValueError(f"{context}: canonical type text {canonical!r} != projection {text!r}")
    return canonical


def validate_statement_types(statements: object, context: str, expression_count: int) -> None:
    if not isinstance(statements, list):
        raise TypeError(f"{context}: body_statements must be an array")
    for index, statement in enumerate(statements):
        statement_context = f"{context}: statement {index}"
        if not isinstance(statement, dict):
            raise TypeError(f"{statement_context} must be an object")

        expression_ids = statement.get("expression_ids", [])
        if not isinstance(expression_ids, list) or not all(
            isinstance(expression_id, int) for expression_id in expression_ids
        ):
            raise TypeError(f"{statement_context}: expression_ids must be an integer array")
        for expression_id in expression_ids:
            if expression_id < 0 or expression_id >= expression_count:
                raise ValueError(f"{statement_context}: dangling statement expression id {expression_id}")

        if statement.get("kind") == "let":
            initializer_expression_id = statement.get("initializer_expression_id")
            if initializer_expression_id is None:
                if expression_ids:
                    raise ValueError(
                        f"{statement_context}: let expression_ids require canonical initializer ownership"
                    )
            else:
                if not isinstance(initializer_expression_id, int):
                    raise TypeError(
                        f"{statement_context}: initializer_expression_id must be an integer"
                    )
                if initializer_expression_id < 0 or initializer_expression_id >= expression_count:
                    raise ValueError(
                        f"{statement_context}: dangling initializer expression id "
                        f"{initializer_expression_id}"
                    )
                if statement.get("has_initializer") is not True:
                    raise ValueError(
                        f"{statement_context}: initializer expression requires "
                        "has_initializer projection"
                    )
                if expression_ids != [initializer_expression_id]:
                    raise ValueError(
                        f"{statement_context}: initializer does not match expression_ids projection"
                    )

        if statement.get("kind") in {"return", "assignment"}:
            value_role = statement["kind"]
            value_expression_id = statement.get("value_expression_id")
            if value_expression_id is None:
                if statement.get("has_value") is True or expression_ids:
                    raise ValueError(
                        f"{statement_context}: valueless {value_role} disagrees with "
                        "compatibility projection"
                    )
            else:
                if not isinstance(value_expression_id, int):
                    raise TypeError(f"{statement_context}: value_expression_id must be an integer")
                if value_expression_id < 0 or value_expression_id >= expression_count:
                    raise ValueError(
                        f"{statement_context}: dangling {value_role} value expression id "
                        f"{value_expression_id}"
                    )
                if statement.get("has_value") is not True:
                    raise ValueError(
                        f"{statement_context}: {value_role} value requires has_value projection"
                    )
                if expression_ids != [value_expression_id]:
                    raise ValueError(
                        f"{statement_context}: {value_role} value does not match "
                        "expression_ids projection"
                    )

        if statement.get("kind") in {"if", "while"}:
            condition_role = statement["kind"]
            condition_expression_id = statement.get("condition_expression_id")
            if condition_expression_id is None:
                if statement.get("has_condition") is True or expression_ids:
                    raise ValueError(
                        f"{statement_context}: conditionless {condition_role} disagrees with "
                        "compatibility projection"
                    )
            else:
                if not isinstance(condition_expression_id, int):
                    raise TypeError(
                        f"{statement_context}: condition_expression_id must be an integer"
                    )
                if condition_expression_id < 0 or condition_expression_id >= expression_count:
                    raise ValueError(
                        f"{statement_context}: dangling {condition_role} condition expression id "
                        f"{condition_expression_id}"
                    )
                if statement.get("has_condition") is not True:
                    raise ValueError(
                        f"{statement_context}: {condition_role} condition requires "
                        "has_condition projection"
                    )
                if expression_ids != [condition_expression_id]:
                    raise ValueError(
                        f"{statement_context}: {condition_role} condition does not match "
                        "expression_ids projection"
                    )

        if "type" in statement or "type_ref" in statement:
            canonical = validate_type_ref(statement.get("type_ref"), statement_context)
            if statement.get("type") != canonical:
                raise ValueError(f"{statement_context}: type string disagrees with canonical type_ref")
        if "body_statements" in statement:
            validate_statement_types(
                statement["body_statements"], statement_context, expression_count
            )


def validate_declaration_types(document: dict, path: Path, expression_count: int) -> None:
    source_unit = document.get("source_unit")
    declarations = source_unit.get("declarations") if isinstance(source_unit, dict) else None
    if not isinstance(declarations, list):
        raise TypeError(f"{path}: source_unit declarations must be an array")

    for index, declaration in enumerate(declarations):
        context = f"{path}: declaration {index}"
        if not isinstance(declaration, dict):
            raise TypeError(f"{context} must be an object")
        kind = declaration.get("kind")
        if kind == "function":
            parameters = declaration.get("parameters")
            if not isinstance(parameters, list):
                raise TypeError(f"{context}: function parameters must be an array")
            for parameter_index, parameter in enumerate(parameters):
                parameter_context = f"{context}: parameter {parameter_index}"
                canonical = validate_type_ref(parameter.get("type_ref"), parameter_context)
                if parameter.get("type") != canonical:
                    raise ValueError(f"{parameter_context}: type string disagrees with canonical type_ref")
            return_type = validate_type_ref(declaration.get("return_type_ref"), f"{context}: return type")
            if declaration.get("return_type") != return_type:
                raise ValueError(f"{context}: return_type string disagrees with canonical type_ref")
            validate_statement_types(
                declaration.get("body_statements"), context, expression_count
            )
        elif kind == "record":
            fields = declaration.get("fields")
            if not isinstance(fields, list):
                raise TypeError(f"{context}: record fields must be an array")
            for field_index, field in enumerate(fields):
                field_context = f"{context}: field {field_index}"
                canonical = validate_type_ref(field.get("type_ref"), field_context)
                if field.get("type") != canonical:
                    raise ValueError(f"{field_context}: type string disagrees with canonical type_ref")
        elif kind == "type_alias":
            canonical = validate_type_ref(declaration.get("target_type_ref"), f"{context}: alias target")
            if declaration.get("target") != canonical:
                raise ValueError(f"{context}: target string disagrees with canonical type_ref")
        elif kind == "main_block":
            validate_statement_types(
                declaration.get("body_statements"), context, expression_count
            )


def validate(path: Path) -> None:
    with path.open(encoding="utf-8") as stream:
        document = json.load(stream)

    expressions = document.get("expression_pool")
    if not isinstance(expressions, list):
        raise TypeError(f"{path}: expression_pool must be an array")
    if document.get("expression_pool_size") != len(expressions):
        raise ValueError(f"{path}: expression_pool_size does not match expression_pool")

    validate_declaration_types(document, path, len(expressions))

    for expected_id, expression in enumerate(expressions):
        if expression.get("id") != expected_id:
            fail(path, expected_id, "expression IDs must match their pool positions")

        kind = expression.get("kind")
        payload = expression.get("payload")
        projected = expression.get("child_expressions")
        if not isinstance(kind, str) or not isinstance(payload, dict):
            fail(path, expected_id, "kind and canonical payload are required")
        if not isinstance(projected, list):
            fail(path, expected_id, "child_expressions compatibility projection is required")

        try:
            canonical = payload_children(kind, payload)
        except (TypeError, ValueError) as error:
            fail(path, expected_id, str(error))

        if canonical != projected:
            fail(path, expected_id, f"payload children {canonical} != projection {projected}")

        for child_id in canonical:
            if child_id < 0 or child_id >= len(expressions):
                fail(path, expected_id, f"dangling child expression id {child_id}")


def main() -> int:
    if len(sys.argv) != 2:
        print(f"usage: {Path(sys.argv[0]).name} AST_JSON", file=sys.stderr)
        return 2

    path = Path(sys.argv[1])
    try:
        validate(path)
    except (OSError, TypeError, ValueError, json.JSONDecodeError) as error:
        print(error, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
