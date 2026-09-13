#!/usr/bin/env python3
"""Attack the frontend-bundle boundary with isolated and malformed inputs."""

from __future__ import annotations

import copy
import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any, Callable


def fail(message: str) -> None:
    raise RuntimeError(message)


def run_consumer(consumer: Path, bundle: Path, cwd: Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str(consumer), str(bundle)],
        cwd=cwd,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )


def first_mapped_location(value: Any) -> dict[str, Any] | None:
    if isinstance(value, dict):
        location = value.get("location")
        if isinstance(location, dict) and isinstance(location.get("line"), int):
            if location["line"] > 0:
                return location
        for child in value.values():
            found = first_mapped_location(child)
            if found is not None:
                return found
    elif isinstance(value, list):
        for child in value:
            found = first_mapped_location(child)
            if found is not None:
                return found
    return None


def mutate_generated_ast_line(bundle: dict[str, Any]) -> None:
    location = first_mapped_location(bundle["ast"])
    if location is None:
        fail("attack fixture contains no mapped AST location")
    for line in bundle["source_map"]["lines"]:
        if line["expanded_line"] == location["line"]:
            line["source_id"] = None
            line["source_line"] = None
            return
    fail("attack fixture AST location is absent from the source map")


def mutate_missing_symbol_origin(bundle: dict[str, Any]) -> None:
    if not bundle["symbol_origins"]:
        fail("attack fixture contains no symbol origins")
    bundle["symbol_origins"].pop()


def mutate_dangling_ast_pointer(bundle: dict[str, Any]) -> None:
    if not bundle["symbol_origins"]:
        fail("attack fixture contains no symbol origins")
    bundle["symbol_origins"][0]["ast_path"] = "/declaration_pool/999999"


def mutate_unknown_scope_parent(bundle: dict[str, Any]) -> None:
    global_id = bundle["symbol_table"]["global_scope_id"]
    for scope in bundle["symbol_table"]["scopes"]:
        if scope["id"] != global_id:
            scope["parent_id"] = 999999
            return
    fail("attack fixture contains no non-global scope")


def mutate_unknown_source(bundle: dict[str, Any]) -> None:
    for line in bundle["source_map"]["lines"]:
        if line["source_id"] is not None:
            line["source_id"] = 999999
            return
    fail("attack fixture contains no mapped source line")


def mutate_pool_size(bundle: dict[str, Any]) -> None:
    bundle["ast"]["declaration_pool_size"] += 1


def mutate_duplicate_symbol(bundle: dict[str, Any]) -> None:
    symbols = bundle["symbol_table"]["symbols"]
    if not symbols:
        fail("attack fixture contains no symbols")
    symbols.append(copy.deepcopy(symbols[0]))


def mutate_bundle_version(bundle: dict[str, Any]) -> None:
    bundle["version"] = 999


def mutate_ast_format(bundle: dict[str, Any]) -> None:
    bundle["ast"]["format"] = "flowmini.ast.unknown"


def mutate_invalid_fact_value(bundle: dict[str, Any]) -> None:
    for symbol in bundle["symbol_table"]["symbols"]:
        if symbol["facts"]:
            symbol["facts"][0]["value"] = {
                "type": "double",
                "value": "host_specific_infinity",
            }
            return
    fail("attack fixture contains no symbol facts")


ATTACKS: list[tuple[str, Callable[[dict[str, Any]], None]]] = [
    ("unsupported bundle version", mutate_bundle_version),
    ("unsupported AST format", mutate_ast_format),
    ("missing symbol origin", mutate_missing_symbol_origin),
    ("dangling AST origin pointer", mutate_dangling_ast_pointer),
    ("unknown scope parent", mutate_unknown_scope_parent),
    ("unknown source-map source", mutate_unknown_source),
    ("AST location mapped to generated text", mutate_generated_ast_line),
    ("declaration pool size mismatch", mutate_pool_size),
    ("duplicate symbol ID", mutate_duplicate_symbol),
    ("invalid typed fact value", mutate_invalid_fact_value),
]


def main() -> int:
    if len(sys.argv) != 3:
        print(f"usage: {Path(sys.argv[0]).name} CONSUMER BUNDLE", file=sys.stderr)
        return 2

    consumer = Path(sys.argv[1]).resolve()
    fixture = Path(sys.argv[2]).resolve()
    try:
        original = json.loads(fixture.read_text(encoding="utf-8"))
        with tempfile.TemporaryDirectory(prefix="flowmini-bundle-attack-") as raw_dir:
            isolated = Path(raw_dir)
            isolated_consumer = isolated / "consume.py"
            isolated_bundle = isolated / "frontend-bundle.json"
            shutil.copyfile(consumer, isolated_consumer)
            shutil.copyfile(fixture, isolated_bundle)

            positive = run_consumer(isolated_consumer, isolated_bundle, isolated)
            if positive.returncode != 0:
                fail(f"isolated consumer rejected valid bundle: {positive.stderr.strip()}")
            skeleton = json.loads(positive.stdout)
            if skeleton.get("format") != "flowmini.lowering_skeleton":
                fail("isolated consumer did not emit a lowering skeleton")

            for index, (name, mutate) in enumerate(ATTACKS, start=1):
                attacked = copy.deepcopy(original)
                mutate(attacked)
                attacked_path = isolated / f"attack-{index}.json"
                attacked_path.write_text(
                    json.dumps(attacked, ensure_ascii=False), encoding="utf-8"
                )
                result = run_consumer(isolated_consumer, attacked_path, isolated)
                if result.returncode == 0:
                    fail(f"consumer accepted malformed bundle: {name}")

        print(
            "Frontend bundle attacks: PASS "
            f"(1 isolated positive, {len(ATTACKS)} malformed negatives)"
        )
        return 0
    except (OSError, json.JSONDecodeError, RuntimeError) as error:
        print(f"frontend bundle attack error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
