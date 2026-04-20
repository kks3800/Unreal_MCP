"""Static metadata for UnrealMCP's dynamic toolset mode.

Three pieces of data:
  - MODULE_CATEGORIES: maps each register_*_tools() module name to a
    user-facing category string used in search filters.
  - CORE_TOOLS: the small allow-list of always-visible tools. Starting
    point is a measured guess; refined by ``ToolRegistry._hits`` data
    after rollout.
  - KEYWORD_OVERRIDES: hand-curated keywords for tools where the
    snake_case name doesn't capture natural search terms. Speakeasy's
    V2 writeup warns against depending on auto-extracted metadata, so
    this list is intentionally grown rather than auto-generated.
"""
from __future__ import annotations


# ---------------------------------------------------------------------------
# Module → category
# ---------------------------------------------------------------------------
# Keys match the names used in unreal_mcp_server.MODULE_REGISTRY.
# Values are the category string exposed in search() filters.

MODULE_CATEGORIES: dict[str, str] = {
    "editor":                  "editor",
    "blueprint":               "blueprint",
    "blueprint_nodes":         "blueprint",
    "blueprint_inspect":       "blueprint",
    "blueprint_search":        "blueprint",
    "blueprint_graph":         "blueprint",
    "blueprint_multigraph":    "blueprint",
    "blueprint_compound":      "blueprint",
    "blueprint_intelligence":  "blueprint",
    "widget":                  "widget",
    "widget_core":             "widget",
    "widget_commonui":         "widget",
    "widget_batch":            "widget",
    "widget_discovery":        "widget",
    "widget_style":            "widget",
    "widget_input":            "widget",
    "widget_animation":        "widget",
    "widget_commonui_ext":     "widget",
    "widget_readonly":         "widget",
    "material":                "material",
    "material_graph":          "material",
    "metasound":               "metasound",
    "niagara":                 "niagara",
    "asset":                   "asset",
    "behavior_tree":           "bt",
    "blackboard":              "blackboard",
    "eqs":                     "eqs",
    "input":                   "input",
    "procedural":              "procedural",
    "pcg":                     "pcg",
    "project":                 "project",
}


# ---------------------------------------------------------------------------
# Core tools — always visible in DYNAMIC_MODE, never surfaced via search()
# ---------------------------------------------------------------------------
# Target: stay below ~20 entries. Claude's tool-selection accuracy
# degrades past 30-50 visible tools; with 3 meta-tools added, 20 core
# leaves 23 visible total — comfortable headroom.

CORE_TOOLS: set[str] = {
    # Actor / level basics — 5
    "spawn_actor",
    "delete_actor",
    "set_actor_property",
    "get_actors_in_level",
    "find_actors_by_name",

    # Editor state & diagnostics — 5
    "take_editor_screenshot",
    "get_editor_context",
    "start_pie",
    "stop_pie",
    "is_pie_active",

    # Blueprint essentials — 3
    "create_blueprint",
    "compile_blueprint",
    "open_blueprint",

    # Asset lookup — 2
    "list_assets",
    "get_asset_info",

    # CVars — 2
    "get_cvar",
    "set_cvar",

    # Project overview — 2
    "get_folder_structure",
    "get_viewport_camera",
}


# ---------------------------------------------------------------------------
# Keyword overrides — curated natural-language synonyms
# ---------------------------------------------------------------------------
# Seed set, not exhaustive. Grow as real searches reveal gaps.
# Format: tool_name → list[str] of keywords (each stored lowercase).

KEYWORD_OVERRIDES: dict[str, list[str]] = {
    # Widgets — natural LLM vocabulary for UI intent
    "add_button_to_widget":             ["button", "click", "press", "tap", "interactive"],
    "bind_widget_event":                ["click", "event", "callback", "press", "interact", "handler"],
    "add_text_block_to_widget":         ["text", "label", "caption", "string", "write"],
    "add_image_to_widget":              ["image", "picture", "texture", "icon", "sprite"],
    "add_progress_bar_to_widget":       ["progress", "bar", "loading", "percentage", "health"],
    "add_slider_to_widget":             ["slider", "range", "volume", "setting"],
    "add_checkbox_to_widget":           ["checkbox", "toggle", "bool", "option"],
    "add_editable_text_to_widget":      ["input", "textbox", "editable", "entry", "field"],
    "add_common_button":                ["button", "click", "common", "ui"],
    "add_common_text_block":            ["text", "common", "label"],
    "add_common_list_view":             ["list", "common", "scrollable", "items"],
    "create_umg_widget_blueprint":      ["widget", "ui", "hud", "umg", "blueprint"],

    # Blueprints — graph semantics the names bury
    "add_blueprint_event_node":         ["event", "trigger", "callback", "hook", "begin", "play"],
    "add_blueprint_function_node":      ["function", "call", "invoke", "method", "execute"],
    "add_branch_node":                  ["if", "condition", "branch", "switch", "decision"],
    "add_for_each_loop_node":           ["loop", "iterate", "foreach", "array", "each"],
    "add_while_loop_node":              ["while", "loop", "repeat", "iterate"],
    "add_cast_node":                    ["cast", "convert", "type", "as"],
    "add_dynamic_cast_node":            ["cast", "convert", "type", "as", "pure", "impure"],
    "add_delay_node":                   ["delay", "wait", "pause", "timer", "sleep"],
    "add_timeline_node":                ["timeline", "animate", "tween", "interp", "lerp"],
    "connect_blueprint_nodes":          ["connect", "wire", "link", "pin", "autocast"],
    "add_blueprint_variable":           ["variable", "var", "property", "field"],
    "add_make_struct_node":             ["struct", "make", "compose", "vector", "rotator"],
    "add_break_struct_node":            ["struct", "break", "split", "decompose", "vector"],
    "describe_node_pins":               ["describe", "introspect", "pins", "preview", "layout", "discover"],
    "add_component_bound_event_node":   ["bound", "event", "component", "delegate", "overlap", "clicked", "bind"],

    # Blueprint multigraph (Phase 8) — function/macro/dispatcher/interface authoring
    "create_function_graph_ex":         ["function", "method", "graph", "pure", "const", "callable", "editor", "replication", "rpc"],
    "create_macro_graph_ex":            ["macro", "tunnel", "graph", "reusable"],
    "create_event_dispatcher":          ["dispatcher", "delegate", "event", "multicast", "assignable", "broadcast"],
    "implement_interface":              ["interface", "implement", "inherit", "contract", "bpi"],
    "add_local_variable_ex":            ["local", "variable", "scope", "function", "temporary"],
    "add_bind_delegate_node":           ["bind", "delegate", "dispatcher", "subscribe", "listen", "hook"],
    "add_call_delegate_node":           ["call", "dispatcher", "delegate", "broadcast", "fire", "invoke"],
    "add_remove_delegate_node":         ["unbind", "remove", "delegate", "dispatcher", "unsubscribe"],

    # Materials
    "create_material":                  ["material", "shader", "pbr", "surface"],
    "add_material_node":                ["node", "shader", "expression", "material"],
    "create_material_instance":         ["instance", "material", "inst", "mi"],
    "set_material_instance_parameter":  ["parameter", "param", "tweak", "instance", "material"],

    # Behavior trees
    "add_bt_task_move_to":              ["move", "walk", "pathfind", "navigate", "goto"],
    "add_bt_task_wait":                 ["wait", "delay", "pause", "bt"],
    "add_bt_decorator_cooldown":        ["cooldown", "wait", "delay", "timer"],
    "add_bt_decorator_blackboard":      ["blackboard", "check", "condition", "bb"],
    "add_bt_selector":                  ["selector", "or", "bt", "composite"],
    "add_bt_sequence":                  ["sequence", "and", "bt", "composite"],
    "create_behavior_tree":             ["bt", "behavior", "tree", "ai"],

    # Blackboard
    "set_blackboard_value":             ["blackboard", "bb", "set", "store", "variable"],
    "get_blackboard_value":             ["blackboard", "bb", "get", "read"],
    "add_blackboard_key":               ["blackboard", "bb", "key", "variable"],

    # Assets / organisation
    "move_asset":                       ["move", "relocate", "reorganise", "asset"],
    "rename_asset":                     ["rename", "name", "asset"],
    "duplicate_asset":                  ["duplicate", "copy", "clone", "asset"],
    "fix_redirectors":                  ["redirector", "cleanup", "fix", "moved"],
    "find_asset_references":            ["references", "dependencies", "used", "where"],
    "analyze_folder_organization":      ["organise", "folder", "cleanup", "structure"],

    # Niagara (scaffolded, but still worth discovery for future)
    "create_niagara_system":            ["particle", "vfx", "effect", "niagara"],

    # Procedural
    "create_staircase":                 ["stairs", "staircase", "steps", "procedural"],
    "create_wall":                      ["wall", "barrier", "procedural"],
    "create_tower":                     ["tower", "procedural", "building"],
    "create_arch":                      ["arch", "doorway", "procedural"],
    "create_pyramid":                   ["pyramid", "procedural", "structure"],
    "create_maze":                      ["maze", "labyrinth", "procedural"],

    # MetaSound
    "create_metasound_source":          ["metasound", "sound", "audio", "oscillator"],
    "add_metasound_node":               ["metasound", "node", "audio", "dsp"],

    # PCG
    "create_pcg_graph":                 ["pcg", "procedural", "content", "graph"],
    "add_pcg_node":                     ["pcg", "node", "procedural"],
}
