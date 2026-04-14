"""Measure real token cost of UnrealMCP's tool schemas.

Loads every register_*_tools() into a sandbox FastMCP, introspects the
resulting tool schemas, counts tokens using tiktoken (cl100k_base, which
approximates Claude/GPT tokenization within ~5%).

Run from the Server/ directory:
    .venv/Scripts/python.exe scripts/measure_token_cost.py
"""
import json
import sys
from pathlib import Path

# Make Server/ importable so `from tools.* import ...` works
HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))

import tiktoken
from mcp.server.fastmcp import FastMCP

enc = tiktoken.get_encoding("cl100k_base")


def tok(s: str) -> int:
    return len(enc.encode(s))


def compact_schema(full: dict) -> dict:
    """Strip JSON-schema noise; keep essentials only."""
    props = full.get("properties", {})
    required = set(full.get("required", []))
    params = []
    for pname, spec in props.items():
        if pname == "ctx":
            continue
        p = {"name": pname, "type": spec.get("type", "any"),
             "required": pname in required}
        if "default" in spec:
            p["default"] = spec["default"]
        if "description" in spec:
            p["desc"] = spec["description"][:80]
        params.append(p)
    return {"params": params}


# Each (module name, register fn name) pair
REGISTRATIONS = [
    ("editor_tools", "register_editor_tools"),
    ("blueprint_tools", "register_blueprint_tools"),
    ("node_tools", "register_blueprint_node_tools"),
    ("project_tools", "register_project_tools"),
    ("umg_tools", "register_umg_tools"),
    ("material_tools", "register_material_tools"),
    ("material_graph_tools", "register_material_graph_tools"),
    ("metasound_tools", "register_metasound_tools"),
    ("niagara_tools", "register_niagara_tools"),
    ("asset_tools", "register_asset_tools"),
    ("blueprint_inspect_tools", "register_blueprint_inspect_tools"),
    ("blueprint_search_tools", "register_blueprint_search_tools"),
    ("blueprint_graph_tools", "register_blueprint_graph_tools"),
    ("blueprint_compound_tools", "register_blueprint_compound_tools"),
    ("blueprint_intelligence", "register_blueprint_intelligence_tools"),
    ("behavior_tree_tools", "register_behavior_tree_tools"),
    ("blackboard_tools", "register_blackboard_tools"),
    ("eqs_tools", "register_eqs_tools"),
    ("input_tools", "register_input_tools"),
    ("procedural_tools", "register_procedural_tools"),
    ("pcg_tools", "register_pcg_tools"),
]


def load_all_into_sandbox() -> FastMCP:
    sandbox = FastMCP("_sandbox")
    for mod_name, fn_name in REGISTRATIONS:
        try:
            module = __import__(f"tools.{mod_name}", fromlist=[fn_name])
            getattr(module, fn_name)(sandbox)
        except Exception as e:
            print(f"  ! failed to load {mod_name}: {e}", file=sys.stderr)
    return sandbox


def measure_tool(tool) -> dict:
    """Measure one tool's token cost across representations."""
    name = tool.name
    desc = tool.description or ""
    full_schema = tool.parameters or {}
    comp = compact_schema(full_schema)

    # Simulate what a client sees per tool in the tool list
    full_payload = json.dumps({
        "name": name,
        "description": desc,
        "inputSchema": full_schema,
    }, indent=None)

    compact_payload = json.dumps({
        "name": name,
        "desc": desc[:80],
        "params": comp["params"],
    }, indent=None)

    # Search result shape (tight, what we'd return from search_unreal_tools)
    search_payload = json.dumps({
        "name": name,
        "desc": desc[:80],
    }, indent=None)

    return {
        "name": name,
        "desc_len": len(desc),
        "param_count": len(full_schema.get("properties", {})),
        "full_tokens": tok(full_payload),
        "compact_tokens": tok(compact_payload),
        "search_tokens": tok(search_payload),
    }


def main():
    print("Loading all 21 tool modules into sandbox FastMCP...")
    sandbox = load_all_into_sandbox()
    tools = sandbox._tool_manager._tools
    print(f"  captured {len(tools)} tools")
    print()

    results = [measure_tool(t) for t in tools.values()]

    # Totals
    total_full = sum(r["full_tokens"] for r in results)
    total_compact = sum(r["compact_tokens"] for r in results)
    total_search = sum(r["search_tokens"] for r in results)

    # Per-tool averages
    n = len(results)
    avg_full = total_full / n
    avg_compact = total_compact / n
    avg_search = total_search / n

    # Distribution
    full_sorted = sorted(r["full_tokens"] for r in results)
    p50 = full_sorted[n // 2]
    p90 = full_sorted[int(n * 0.9)]
    p99 = full_sorted[int(n * 0.99)]

    print("=" * 60)
    print("PER-TOOL TOKEN COST (full MCP tool-list payload)")
    print("=" * 60)
    print(f"  tools measured:    {n}")
    print(f"  avg per tool:      {avg_full:.0f} tokens")
    print(f"  p50 (median):      {p50} tokens")
    print(f"  p90:               {p90} tokens")
    print(f"  p99:               {p99} tokens")
    print(f"  max:               {full_sorted[-1]} tokens")
    print(f"  min:               {full_sorted[0]} tokens")
    print()
    print("=" * 60)
    print("TOTAL UPFRONT CONTEXT COST (all tools loaded at once)")
    print("=" * 60)
    print(f"  Full schemas:         {total_full:>8,} tokens")
    print(f"  Compact schemas:      {total_compact:>8,} tokens  ({100*total_compact/total_full:.0f}% of full)")
    print(f"  Search-result shape:  {total_search:>8,} tokens  ({100*total_search/total_full:.0f}% of full)")
    print()
    print("=" * 60)
    print("TOP 10 HEAVIEST TOOLS")
    print("=" * 60)
    heavy = sorted(results, key=lambda r: -r["full_tokens"])[:10]
    for r in heavy:
        print(f"  {r['full_tokens']:>5}t  {r['param_count']:>2}params  {r['name']}")
    print()
    print("=" * 60)
    print("PER-MODULE BREAKDOWN")
    print("=" * 60)
    # regroup by module via sandbox tool tags (if present) — fall back to name prefix
    # simpler: just leave as a single total. module breakdown requires tracking at registration.


if __name__ == "__main__":
    main()
