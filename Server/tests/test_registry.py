"""Unit tests for the ToolRegistry + RecordingProxy pipeline.

Does not require a running Unreal Editor — everything is exercised via
the sandbox FastMCP. Runs in under ~3 seconds total.

Run from Server/:
    .venv/Scripts/python.exe -m pytest tests/test_registry.py -v

Or without pytest:
    .venv/Scripts/python.exe tests/test_registry.py
"""
from __future__ import annotations

import sys
import time
from pathlib import Path

# Make Server/ importable so `from tool_registry import ...` works
HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))

from tool_registry import (
    ToolRegistry,
    RecordingProxy,
    ToolMeta,
    compact_schema,
    keywords_for,
    register_meta_tools,
)
from tool_categories import MODULE_CATEGORIES, CORE_TOOLS, KEYWORD_OVERRIDES


# ---------------------------------------------------------------------------
# Fixture: build a full registry from every real tool module
# ---------------------------------------------------------------------------

# Each tuple: (friendly_name, module_file, register_fn_name)
# friendly_name must match a key in MODULE_CATEGORIES so the category
# filter test can resolve tools correctly.
_REGISTRATIONS = [
    ("editor",                 "editor_tools",            "register_editor_tools"),
    ("blueprint",              "blueprint_tools",         "register_blueprint_tools"),
    ("blueprint_nodes",        "node_tools",              "register_blueprint_node_tools"),
    ("project",                "project_tools",           "register_project_tools"),
    ("widget",                 "umg_tools",               "register_umg_tools"),
    ("material",               "material_tools",          "register_material_tools"),
    ("material_graph",         "material_graph_tools",    "register_material_graph_tools"),
    ("metasound",              "metasound_tools",         "register_metasound_tools"),
    ("niagara",                "niagara_tools",           "register_niagara_tools"),
    ("asset",                  "asset_tools",             "register_asset_tools"),
    ("blueprint_inspect",      "blueprint_inspect_tools", "register_blueprint_inspect_tools"),
    ("blueprint_search",       "blueprint_search_tools",  "register_blueprint_search_tools"),
    ("blueprint_graph",        "blueprint_graph_tools",   "register_blueprint_graph_tools"),
    ("blueprint_compound",     "blueprint_compound_tools","register_blueprint_compound_tools"),
    ("blueprint_intelligence", "blueprint_intelligence",  "register_blueprint_intelligence_tools"),
    ("behavior_tree",          "behavior_tree_tools",     "register_behavior_tree_tools"),
    ("blackboard",             "blackboard_tools",        "register_blackboard_tools"),
    ("eqs",                    "eqs_tools",               "register_eqs_tools"),
    ("input",                  "input_tools",             "register_input_tools"),
    ("procedural",             "procedural_tools",        "register_procedural_tools"),
    ("pcg",                    "pcg_tools",               "register_pcg_tools"),
]


def build_full_registry() -> ToolRegistry:
    """Load every real tool module through a RecordingProxy into one registry."""
    registry = ToolRegistry()
    for friendly, mod_name, fn_name in _REGISTRATIONS:
        category = MODULE_CATEGORIES.get(friendly, "misc")
        proxy = RecordingProxy(
            registry=registry,
            category=category,
            core_names=CORE_TOOLS,
            keyword_overrides=KEYWORD_OVERRIDES,
        )
        module = __import__(f"tools.{mod_name}", fromlist=[fn_name])
        getattr(module, fn_name)(proxy)
    return registry


# ---------------------------------------------------------------------------
# Registry population
# ---------------------------------------------------------------------------

def test_registration_captures_all_tools():
    registry = build_full_registry()
    # Measured baseline: 449 tools across 21 modules
    assert len(registry) == 449, (
        f"expected 449 tools, got {len(registry)} — has a module been added/removed?"
    )


def test_no_duplicate_tool_names():
    # Registry.add() raises on duplicate; if build_full_registry() completes,
    # no duplicates. Test explicit raise:
    registry = ToolRegistry()
    meta = ToolMeta(name="foo", func=lambda: None, schema={}, compact={}, description="", category="test")
    registry.add(meta)
    try:
        registry.add(meta)
    except ValueError:
        return
    raise AssertionError("duplicate registration should have raised")


def test_core_tools_exist_and_marked():
    registry = build_full_registry()
    missing = [n for n in CORE_TOOLS if n not in registry]
    assert not missing, f"core tools missing from registry: {missing}"
    not_marked = [n for n in CORE_TOOLS if not registry._tools[n].is_core]
    assert not not_marked, f"core tools not marked as core: {not_marked}"


def test_dynamic_tools_not_marked_core():
    registry = build_full_registry()
    wrongly_core = [m.name for m in registry.dynamic_tools() if m.is_core]
    assert not wrongly_core, f"these tools are flagged core but not in CORE_TOOLS: {wrongly_core}"


# ---------------------------------------------------------------------------
# compact_schema
# ---------------------------------------------------------------------------

def test_compact_schema_strips_ctx():
    full = {
        "properties": {
            "ctx": {"type": "object"},
            "name": {"type": "string", "description": "actor name"},
        },
        "required": ["name"],
    }
    c = compact_schema(full)
    assert all(p["name"] != "ctx" for p in c["params"])


def test_compact_schema_preserves_required_and_default():
    full = {
        "properties": {
            "name": {"type": "string"},
            "scale": {"type": "number", "default": 1.0},
        },
        "required": ["name"],
    }
    c = compact_schema(full)
    name_param = next(p for p in c["params"] if p["name"] == "name")
    scale_param = next(p for p in c["params"] if p["name"] == "scale")
    assert name_param["required"] is True
    assert scale_param["required"] is False
    assert scale_param["default"] == 1.0


def test_compact_schema_truncates_description():
    full = {"properties": {"n": {"type": "int", "description": "x" * 300}}}
    c = compact_schema(full)
    assert len(c["params"][0]["desc"]) == 80


# ---------------------------------------------------------------------------
# keywords_for
# ---------------------------------------------------------------------------

def test_keywords_for_uses_override():
    overrides = {"add_button_to_widget": ["button", "click"]}
    assert keywords_for("add_button_to_widget", overrides) == ["button", "click"]


def test_keywords_for_falls_back_to_snake_tokens():
    got = keywords_for("add_button_to_widget", {})
    assert "add" in got and "button" in got and "widget" in got
    # Short tokens filtered out
    assert "to" not in got


# ---------------------------------------------------------------------------
# search
# ---------------------------------------------------------------------------

def test_search_finds_tool_by_name_substring():
    registry = build_full_registry()
    results = registry.search("button")
    assert any(r["name"] == "add_button_to_widget" for r in results), \
        f"expected add_button_to_widget in results, got {[r['name'] for r in results]}"


def test_search_uses_keyword_overrides():
    registry = build_full_registry()
    # "click" is a curated keyword for add_button_to_widget, not in the tool name
    results = registry.search("click")
    assert any(r["name"] == "add_button_to_widget" for r in results), \
        "keyword override 'click' should surface add_button_to_widget"


def test_search_respects_category_filter():
    registry = build_full_registry()
    results = registry.search("", category="material", limit=100)
    assert len(results) > 0
    # All returned tools must actually be in the material category
    for r in results:
        meta = registry._tools[r["name"]]
        assert meta.category == "material", f"{r['name']} is in {meta.category}, not material"


def test_search_excludes_core_tools():
    registry = build_full_registry()
    # spawn_actor is core — should never appear in search results
    results = registry.search("spawn_actor")
    assert all(r["name"] != "spawn_actor" for r in results), \
        "core tool spawn_actor should be filtered from search"


def test_search_returns_compact_shape():
    registry = build_full_registry()
    results = registry.search("button")
    assert results, "expected at least one result"
    r = results[0]
    assert set(r.keys()) == {"name", "desc"}, f"unexpected keys in result: {r.keys()}"
    assert len(r["desc"]) <= 80


def test_search_limit_respected():
    registry = build_full_registry()
    results = registry.search("widget", limit=3)
    assert len(results) <= 3


# ---------------------------------------------------------------------------
# describe
# ---------------------------------------------------------------------------

def test_describe_returns_params_for_known_tool():
    registry = build_full_registry()
    [r] = registry.describe(["add_button_to_widget"])
    assert r["name"] == "add_button_to_widget"
    assert "params" in r
    # Each param must have at minimum {name, type, required}
    for p in r["params"]:
        assert "name" in p and "type" in p and "required" in p


def test_describe_reports_unknown_tool():
    registry = build_full_registry()
    [r] = registry.describe(["no_such_tool_exists"])
    assert "error" in r


def test_describe_hard_caps_at_five():
    registry = build_full_registry()
    names = ["add_button_to_widget"] * 20
    out = registry.describe(names)
    assert len(out) == 5, f"expected cap of 5, got {len(out)}"


# ---------------------------------------------------------------------------
# execute
# ---------------------------------------------------------------------------

def test_execute_unknown_tool_returns_error():
    registry = build_full_registry()
    r = registry.execute("no_such_tool", {})
    assert r["success"] is False
    assert "unknown tool" in r["error"]


def test_execute_bad_params_surfaces_expected_params():
    """On TypeError, execute returns expected_params so the LLM can self-correct."""
    registry = ToolRegistry()

    def demo(ctx, name: str, count: int) -> dict:
        return {"ok": True}

    # Manually build a meta for this demo — skip the sandbox dance
    registry.add(ToolMeta(
        name="demo",
        func=demo,
        schema={
            "properties": {
                "ctx": {}, "name": {"type": "string"}, "count": {"type": "integer"},
            },
            "required": ["name", "count"],
        },
        compact=compact_schema({
            "properties": {
                "ctx": {}, "name": {"type": "string"}, "count": {"type": "integer"},
            },
            "required": ["name", "count"],
        }),
        description="demo tool",
        category="test",
    ))
    r = registry.execute("demo", {"name": "x"})   # missing required `count`
    assert r["success"] is False
    assert "expected_params" in r
    param_names = {p["name"] for p in r["expected_params"]}
    assert "count" in param_names


def test_execute_increments_hits():
    registry = ToolRegistry()

    def demo(ctx, x: int) -> int:
        return x * 2

    registry.add(ToolMeta(
        name="demo", func=demo,
        schema={"properties": {"x": {"type": "integer"}}, "required": ["x"]},
        compact={"params": [{"name": "x", "type": "integer", "required": True}]},
        description="demo", category="test",
    ))
    registry.execute("demo", {"x": 5})
    registry.execute("demo", {"x": 6})
    assert registry.hits() == {"demo": 2}


# ---------------------------------------------------------------------------
# Performance
# ---------------------------------------------------------------------------

def test_startup_time_under_3s():
    """Measured baseline is 1.3s; 3s cap gives generous headroom."""
    t0 = time.perf_counter()
    build_full_registry()
    elapsed = time.perf_counter() - t0
    assert elapsed < 3.0, f"registry build took {elapsed:.2f}s — regression?"


# ---------------------------------------------------------------------------
# Entrypoint without pytest
# ---------------------------------------------------------------------------

def _run_all():
    import inspect
    mod = sys.modules[__name__]
    failures: list[tuple[str, str]] = []
    tests = [
        (name, fn) for name, fn in inspect.getmembers(mod, inspect.isfunction)
        if name.startswith("test_")
    ]
    for name, fn in tests:
        try:
            fn()
            print(f"  PASS  {name}")
        except Exception as e:
            failures.append((name, f"{type(e).__name__}: {e}"))
            print(f"  FAIL  {name}: {e}")
    print()
    print(f"{len(tests) - len(failures)}/{len(tests)} passed")
    return 0 if not failures else 1


if __name__ == "__main__":
    sys.exit(_run_all())
