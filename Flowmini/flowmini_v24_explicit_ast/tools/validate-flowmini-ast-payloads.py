#!/usr/bin/env python3

"""Validate canonical Flowmini expression payloads and compatibility projections."""

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


def validate(path: Path) -> None:
    with path.open(encoding="utf-8") as stream:
        document = json.load(stream)

    expressions = document.get("expression_pool")
    if not isinstance(expressions, list):
        raise TypeError(f"{path}: expression_pool must be an array")
    if document.get("expression_pool_size") != len(expressions):
        raise ValueError(f"{path}: expression_pool_size does not match expression_pool")

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
