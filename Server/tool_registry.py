"""Dynamic tool registry for UnrealMCP.

Implements the v1 dynamic-toolset pattern described in
Docs/DYNAMIC_TOOLSETS_PLAN.md. The registry captures every
@mcp.tool()-decorated function from the existing tool modules via a
recording proxy, stores them with schema + category + keyword metadata,
and exposes three meta-tools (search / describe / execute) for
client-agnostic dynamic dispatch.

Enable with env var DYNAMIC_MODE=1 in the MCP client config.
"""
from __future__ import annotations

import logging
from dataclasses import dataclass, field
from typing import Any, Callable

from mcp.server.fastmcp import FastMCP

logger = logging.getLogger("UnrealMCP")


# ---------------------------------------------------------------------------
# Tool metadata
# ---------------------------------------------------------------------------

@dataclass
class ToolMeta:
    """Everything the registry knows about a single tool."""

    name: str
    func: Callable
    schema: dict                        # full Pydantic-generated JSON schema
    compact: dict                       # stripped form for describe()
    description: str
    category: str
    keywords: list[str] = field(default_factory=list)
    is_core: bool = False


# ---------------------------------------------------------------------------
# Schema / keyword helpers
# ---------------------------------------------------------------------------

def compact_schema(full_schema: dict) -> dict:
    """Strip JSON-schema noise down to essential fields.

    Measured savings: ~51% vs the full FastMCP/Pydantic schema. Drops
    the ``ctx`` param (FastMCP injects it; LLMs don't supply it) and
    noisy metadata fields like $schema, title, examples.
    """
    props = full_schema.get("properties", {}) or {}
    required = set(full_schema.get("required", []))
    params = []
    for pname, spec in props.items():
        if pname == "ctx":
            continue
        spec = spec or {}
        param: dict[str, Any] = {
            "name": pname,
            "type": spec.get("type", "any"),
            "required": pname in required,
        }
        if "default" in spec:
            param["default"] = spec["default"]
        if "description" in spec and spec["description"]:
            param["desc"] = str(spec["description"])[:80]
        params.append(param)
    return {"params": params}


def keywords_for(tool_name: str, overrides: dict[str, list[str]]) -> list[str]:
    """Return curated keywords if present, otherwise snake_case tokens.

    Speakeasy V2 Lesson 3: auto-extraction from names is fragile.
    Curated overrides for high-value tools compensate; fall back to
    snake_case tokenisation for the long tail.
    """
    if tool_name in overrides:
        return list(overrides[tool_name])
    return [tok for tok in tool_name.split("_") if len(tok) > 2]


# ---------------------------------------------------------------------------
# Registry
# ---------------------------------------------------------------------------

class ToolRegistry:
    """In-memory catalog of all captured tools.

    The three public methods (search / describe / execute) back the
    three meta-tools exposed to MCP clients. Additional state:
    ``_hits`` tracks per-tool invocation counts for later calibration
    of the core-tool allow-list.
    """

    def __init__(self) -> None:
        self._tools: dict[str, ToolMeta] = {}
        self._hits: dict[str, int] = {}

    # -- registration -------------------------------------------------------

    def add(self, meta: ToolMeta) -> None:
        if meta.name in self._tools:
            raise ValueError(f"duplicate tool registration: {meta.name}")
        self._tools[meta.name] = meta

    def __contains__(self, name: str) -> bool:
        return name in self._tools

    def __len__(self) -> int:
        return len(self._tools)

    def core_tools(self) -> list[ToolMeta]:
        return [m for m in self._tools.values() if m.is_core]

    def dynamic_tools(self) -> list[ToolMeta]:
        return [m for m in self._tools.values() if not m.is_core]

    # -- meta-tool backends -------------------------------------------------

    def search(
        self,
        query: str,
        category: str | None = None,
        limit: int = 10,
    ) -> list[dict]:
        """Keyword-scored search over non-core tools.

        Returns compact results — name + 80-char description only.
        Core tools are excluded because they're already visible.
        """
        q = (query or "").lower().strip()
        scored: list[tuple[int, ToolMeta]] = []
        for m in self._tools.values():
            if m.is_core:
                continue
            if category and m.category != category:
                continue
            score = 0
            name_lc = m.name.lower()
            desc_lc = (m.description or "").lower()
            if q and q in name_lc:
                score += 10
            if q and q in desc_lc:
                score += 3
            for kw in m.keywords:
                if q and q in kw:
                    score += 5
            for tok in q.split():
                if len(tok) > 2 and tok in desc_lc:
                    score += 1
            # Empty query with category filter: return everything in-category
            if not q and category:
                score = 1
            if score > 0:
                scored.append((score, m))
        scored.sort(key=lambda x: -x[0])
        return [
            {"name": m.name, "desc": (m.description or "")[:80]}
            for _, m in scored[:max(1, limit)]
        ]

    def describe(self, names: list[str]) -> list[dict]:
        """Return compact schemas for up to 5 tools at once.

        Hard cap of 5 keeps response token cost bounded. For more,
        make multiple calls.
        """
        names = list(names or [])[:5]
        out: list[dict] = []
        for n in names:
            m = self._tools.get(n)
            if m:
                out.append({"name": n, "params": m.compact.get("params", [])})
            else:
                out.append({"name": n, "error": "unknown tool"})
        return out

    def execute(self, name: str, params: dict | None = None) -> Any:
        """Invoke a registered tool by name.

        Passes ctx=None (validated: UnrealMCP tools don't use ctx).
        On TypeError, returns expected_params so the LLM can self-correct
        without another describe call.
        """
        params = params or {}
        m = self._tools.get(name)
        if m is None:
            return {"success": False, "error": f"unknown tool: {name}"}
        self._hits[name] = self._hits.get(name, 0) + 1
        try:
            return m.func(ctx=None, **params)
        except TypeError as e:
            return {
                "success": False,
                "error": f"bad params: {e}",
                "expected_params": m.compact.get("params", []),
            }
        except Exception as e:
            return {
                "success": False,
                "error": f"{type(e).__name__}: {e}",
            }

    # -- telemetry ----------------------------------------------------------

    def hits(self) -> dict[str, int]:
        return dict(self._hits)


# ---------------------------------------------------------------------------
# Recording proxy — captures @mcp.tool() registrations into the registry
# ---------------------------------------------------------------------------

class RecordingProxy:
    """Quacks like FastMCP for the purposes of register_*_tools().

    Each existing ``register_*_tools(mcp)`` call can be redirected to a
    RecordingProxy instead of the real FastMCP server. The proxy forwards
    @mcp.tool() calls to an internal sandbox FastMCP (to reuse its Pydantic
    schema extraction) and captures the resulting tools into a ToolRegistry
    rather than exposing them to the real server.
    """

    def __init__(
        self,
        registry: ToolRegistry,
        category: str,
        core_names: set[str],
        keyword_overrides: dict[str, list[str]],
    ):
        self._registry = registry
        self._category = category
        self._core_names = core_names
        self._keyword_overrides = keyword_overrides
        # Throwaway FastMCP solely to reuse its tool-schema extraction
        self._sandbox = FastMCP("_sandbox")

    def tool(self, *args, **kwargs):
        """Mirror of FastMCP.tool() decorator, capturing into the registry."""
        def decorator(func):
            wrapped = self._sandbox.tool(*args, **kwargs)(func)
            name = func.__name__
            sandbox_tool = self._sandbox._tool_manager._tools.get(name)
            schema = sandbox_tool.parameters if sandbox_tool else {}
            try:
                self._registry.add(
                    ToolMeta(
                        name=name,
                        func=func,
                        schema=schema,
                        compact=compact_schema(schema),
                        description=(func.__doc__ or "").strip(),
                        category=self._category,
                        keywords=keywords_for(name, self._keyword_overrides),
                        is_core=(name in self._core_names),
                    )
                )
            except ValueError as e:
                logger.warning(f"skipping duplicate tool {name}: {e}")
            return wrapped
        return decorator

    # Pass any other attribute access (prompt(), resource(), etc.) through
    # to the sandbox so register_*_tools() code that touches them still works.
    def __getattr__(self, attr):
        return getattr(self._sandbox, attr)


# ---------------------------------------------------------------------------
# Meta-tool registration
# ---------------------------------------------------------------------------

def register_meta_tools(mcp: FastMCP, registry: ToolRegistry) -> None:
    """Register the 3 meta-tools (search / describe / execute) on the real mcp.

    The search_unreal_tools docstring deliberately embeds a full domain
    overview — Speakeasy V2 Lesson 1: without a visible menu the LLM
    doesn't realise dynamic tools exist and never searches.
    """

    @mcp.tool()
    def search_unreal_tools(
        query: str,
        category: str = "",
        limit: int = 10,
    ) -> list:
        """Search hundreds of additional UnrealMCP tools beyond the 20 core ones.

        Use this when a core tool doesn't cover what you need. Returns
        compact results (name + short description). Call describe_unreal_tools
        next to get parameter schemas, then execute_unreal_tool to invoke.

        Available domains (rough tool counts in parens):
        - editor (15): actor spawn/delete, transforms, CVars, PIE, screenshots
        - blueprint (~100): create/compile blueprints, graph editing, 50+ node
          types, inspection, search, complexity analysis
        - widget (~120): UMG + Common UI — containers, buttons, text, lists,
          tree/tile views, styling, animations, event binding
        - material (~30): PBR materials, material graph nodes, instance
          parameters, material functions
        - metasound (~25): EXPERIMENTAL — oscillators, filters, envelopes,
          audio routing. Simple graphs work; complex cases often break.
        - niagara (~35): NOT WORKING YET — scaffolded only, do not rely on
        - behavior_tree (~50): BT composites, decorators, services, tasks,
          runtime control
        - blackboard (8): BB keys and values, runtime state
        - eqs (~15): environment queries, generators, tests
        - pcg (~25): EXPERIMENTAL — procedural content generation nodes,
          property marshaler. Trivial graphs only; AI gets pin semantics
          and data-layer rules wrong often.
        - asset (~15): bulk rename/move, redirectors, folder analysis,
          references
        - input (~10): Enhanced Input mappings, action/axis bindings
        - procedural (6): walls, stairs, towers, arches, pyramids, mazes
        - project (1): Common UI config

        Examples:
        - search("button")                 → add_button_to_widget, CommonButton
        - search("damage")                 → BP nodes dealing with damage
        - search("blackboard key")         → blackboard_tools entries
        - search("", category="material")  → every material tool
        - search("timeline")               → blueprint timeline nodes
        """
        return registry.search(query, category or None, limit)

    @mcp.tool()
    def describe_unreal_tools(names: list[str]) -> list:
        """Get compact parameter schemas for up to 5 tools by name.

        Call after search_unreal_tools. Returns
        {name, params: [{name, type, required, default, desc}]}.
        Hard-capped at 5 tools per call to keep response size bounded —
        make multiple calls if you need more.
        """
        return registry.describe(names)

    @mcp.tool()
    def execute_unreal_tool(name: str, params: dict) -> dict:
        """Execute a non-core UnrealMCP tool by name with a params dict.

        Use describe_unreal_tools first to learn the expected params.
        On parameter error the response includes ``expected_params`` so
        you can self-correct without another describe call. For the 20
        core tools, call them directly — they're already loaded.
        """
        return registry.execute(name, params or {})
