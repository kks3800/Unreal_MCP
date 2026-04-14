# UnrealMCP

**Control Unreal Engine with natural language through any MCP-compatible AI assistant.**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Unreal Engine 5.7](https://img.shields.io/badge/Unreal%20Engine-5.7-blue.svg)](https://www.unrealengine.com/)
[![Python 3.10+](https://img.shields.io/badge/Python-3.10%2B-green.svg)](https://www.python.org/)
[![MCP](https://img.shields.io/badge/Model%20Context%20Protocol-compatible-purple.svg)](https://modelcontextprotocol.io/)
[![Status: WIP](https://img.shields.io/badge/status-heavy%20WIP-orange.svg)](#project-status)

UnrealMCP lets AI assistants like **Claude Code**, **Claude Desktop**, **Cursor**, and **Windsurf** directly drive the Unreal Engine Editor. Spawn actors, build blueprints, author materials, compose UMG widgets, design MetaSounds, wire behavior trees, and more -- all from a prompt.

> **449 tools across 21 domains.** Full editor automation, not just a toy demo.

---

## Project Status

> [!WARNING]
> **This is heavy work-in-progress.** Things will break, some tools are incomplete, some are slow, and some subsystems (notably **Niagara**) are not functional yet. Expect rough edges and use at your own risk. Issues and PRs welcome.
>
> **Built with AI.** A large portion of this codebase was written with AI assistance -- primarily **Claude** (Anthropic). Code quality varies accordingly: some parts are polished, others need a human pass. Every tool is exercised in real Unreal projects before shipping, but coverage is uneven.
>
> **Unreal Engine 5.7 only.** That's what it's developed and tested against. It may work on older 5.x versions, but this is unverified -- if you try it on 5.5 / 5.6, expect to fix things yourself.

---

## Table of Contents

- [Project Status](#project-status)
- [What It Can Do](#what-it-can-do)
- [How It Works](#how-it-works)
- [Requirements](#requirements)
- [Quick Install (AI-Assisted)](#quick-install-ai-assisted)
- [Manual Install](#manual-install)
- [Your First Prompt](#your-first-prompt)
- [Reducing Context Cost](#reducing-context-cost)
- [Feature Reference](#feature-reference)
- [Repository Layout](#repository-layout)
- [Security Model](#security-model)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)
- [Credits](#credits)
- [License](#license)

---

## What It Can Do

Ask an AI assistant to:

- **"Spawn a grid of 10x10 cubes with random colors."**
- **"Create a lobby HUD with player avatars, names, ready states, and a start button."**
- **"Make a translucent UI material with a radial gradient and soft glow."**
- **"Add a MetaSound with an oscillator, low-pass filter, and ADSR envelope."**
- **"Design a behavior tree for an AI that patrols, detects the player, then chases."**
- **"Generate an EQS query that finds cover positions away from the player."**
- **"Procedurally build a spiral staircase with 30 steps."**
- **"Analyze my blueprint graph for complexity hotspots."**
- **"Write a PCG graph that scatters meshes on a landscape with density rules."**

...and it will happen inside your running Unreal Editor.

---

## How It Works

```
 +--------------------------+
 |  AI Assistant            |   Claude Code / Desktop, Cursor, Windsurf, ...
 |  (MCP Client)            |
 +-----------+--------------+
             | MCP protocol (stdio)
             v
 +--------------------------+
 |  Python FastMCP Server   |   449 @mcp.tool() wrappers
 |  Server/                 |
 +-----------+--------------+
             | TCP JSON (127.0.0.1:55557)
             v
 +--------------------------+
 |  C++ Editor Plugin       |   21 command groups
 |  Source/UnrealMCP/       |   Reflection-driven, session-based
 +-----------+--------------+
             | UE Editor APIs
             v
 +--------------------------+
 |  Unreal Engine Editor    |
 +--------------------------+
```

**Two moving parts:**

1. **C++ Editor Plugin** (`Source/`, `UnrealMCP.uplugin`) -- an Unreal editor plugin that listens on TCP port `55557` and executes commands inside the running editor using reflection, session-based editing, and deterministic multi-step operations.
2. **Python FastMCP Server** (`Server/`) -- a FastMCP server exposing 449 MCP tools to AI assistants and translating them into TCP commands for the plugin.

Both ship in the same repo -- the entire repository is the UE plugin folder.

---

## Requirements

| Requirement | Version | Notes |
|-------------|---------|-------|
| Unreal Engine | **5.7** | Only 5.7 is tested. Older 5.x versions may work but are unsupported. |
| Python | **3.10+** | For the FastMCP server |
| `uv` package manager | latest | `pip install uv` or see [astral.sh/uv](https://astral.sh/uv) |
| Build toolchain | MSVC / Xcode / Clang | Required to compile the C++ plugin |
| MCP client | any | Claude Code, Claude Desktop, Cursor, Windsurf, etc. |

---

## Quick Install (AI-Assisted)

If you use **Claude Code**, paste this prompt and Claude will handle the rest:

> Install UnrealMCP into my project. The UnrealMCP repo is at `<PATH_TO_THIS_REPO>`. My Unreal project is at `<PATH_TO_YOUR_UE_PROJECT>`. Follow the instructions in the UnrealMCP README under "Manual Install", starting at Step 1.

Replace the two paths. Claude will clone, set up the venv, create the `.mcp.json`, and copy optional agents.

---

## Manual Install

### Step 1 -- Install the plugin

The repository **is** the plugin. Clone or copy it directly into your project's `Plugins/` folder:

```bash
mkdir -p <UE_PROJECT>/Plugins
git clone https://github.com/kks3800/Unreal_MCP <UE_PROJECT>/Plugins/UnrealMCP
```

Verify the structure:

```
<UE_PROJECT>/Plugins/UnrealMCP/
  UnrealMCP.uplugin       <-- must exist
  Source/UnrealMCP/
  Server/
```

Then **regenerate project files** (right-click `.uproject` > "Generate Visual Studio project files") and **build** from your IDE.

### Step 2 -- Set up the Python server

```bash
cd <UE_PROJECT>/Plugins/UnrealMCP/Server

# Create virtual environment
uv venv

# Install dependencies (editable mode)
uv pip install -e .
```

Quick sanity check:

```bash
# Windows:
.venv/Scripts/python -c "import mcp; print('OK')"

# macOS / Linux:
.venv/bin/python -c "import mcp; print('OK')"
```

### Step 3 -- Create the MCP config

Create `.mcp.json` at your **UE project root** (the folder containing `.uproject`):

**Windows**
```json
{
  "mcpServers": {
    "unreal-mcp": {
      "type": "stdio",
      "command": "<UE_PROJECT>/Plugins/UnrealMCP/Server/.venv/Scripts/python.exe",
      "args": ["<UE_PROJECT>/Plugins/UnrealMCP/Server/unreal_mcp_server.py"],
      "env": {}
    }
  }
}
```

**macOS / Linux**
```json
{
  "mcpServers": {
    "unreal-mcp": {
      "type": "stdio",
      "command": "<UE_PROJECT>/Plugins/UnrealMCP/Server/.venv/bin/python",
      "args": ["<UE_PROJECT>/Plugins/UnrealMCP/Server/unreal_mcp_server.py"],
      "env": {}
    }
  }
}
```

Use **absolute paths with forward slashes** (even on Windows). Replace `<UE_PROJECT>` with your actual project root.

The `env` block is where you set optional flags — see [Reducing Context Cost](#reducing-context-cost) below for `DYNAMIC_MODE` and `ENABLED_MODULES`. For a first run, the empty `{}` is fine.

> For **Claude Desktop**, merge the `unreal-mcp` entry into your `claude_desktop_config.json`.
> For **Cursor / Windsurf**, follow their MCP config docs.

### Step 4 -- (Optional) Install the Claude Code widget agent

Copies a specialized agent that helps Claude Code build UMG widgets with better pattern awareness:

```bash
mkdir -p <UE_PROJECT>/.claude/agents
cp <UE_PROJECT>/Plugins/UnrealMCP/Claude/agents/mcp-widget-expert.md \
   <UE_PROJECT>/.claude/agents/
```

### Step 5 -- Start using it

1. Open your Unreal project in the editor. The plugin starts its TCP server automatically.
2. Restart your MCP client (so it reads the new `.mcp.json`).
3. Ask the AI to do something (see [Your First Prompt](#your-first-prompt)).

---

## Your First Prompt

Try these in order to verify the connection:

1. **"Spawn a cube at the origin."**  -- tests basic actor spawning.
2. **"Create a blueprint actor called BP_TestRotator with a static mesh and a rotating movement component."**  -- tests BP authoring.
3. **"Build a UMG widget called WBP_HelloWorld with a centered title text and a button labelled 'Click me'."**  -- tests widget layout.

If all three work, you're ready.

---

## Reducing Context Cost

UnrealMCP exposes **449 tools costing ~95k tokens** when loaded in full. On 200k-context Claude models that's tight-but-workable; on 128k-context models (GPT-4, many Cursor variants) it doesn't fit.

Two independent knobs, both set in `.mcp.json` under the server's `env` block:

| Knob | What it does | When to use |
|------|--------------|-------------|
| `DYNAMIC_MODE=1` | Exposes only 22 tools (19 core + 3 meta-tools `search_unreal_tools` / `describe_unreal_tools` / `execute_unreal_tool`). Drops upfront cost from ~95k → ~3k tokens (**~97% reduction**). AI discovers non-core tools on demand. | Any client that isn't Claude Code. Highly recommended. |
| `ENABLED_MODULES=...` | Loads only selected tool modules. Reduces everything (including what's reachable via search in dynamic mode). | When you know you only need certain domains. |

Use one, the other, or both. They compose cleanly.

### Option A — Dynamic mode (recommended)

```json
{
  "mcpServers": {
    "unreal-mcp": {
      "command": "...",
      "args": ["..."],
      "env": {
        "DYNAMIC_MODE": "1"
      }
    }
  }
}
```

Only 22 tools visible to the AI client: 19 always-on core tools (spawn, delete, compile, CVars, etc.) plus 3 meta-tools that search/describe/execute the other 430. The AI reads the `search_unreal_tools` docstring to learn what domains exist, searches for what it needs, then invokes via `execute_unreal_tool`.

Per-task cost: ~1-2k tokens of accumulated search/describe responses. Break-even vs legacy mode at ~60 novel tool calls per session — well beyond typical usage.

### Option B — Module presets

```json
{
  "mcpServers": {
    "unreal-mcp": {
      "command": "...",
      "args": ["..."],
      "env": {
        "ENABLED_MODULES": "editor,blueprint,material"
      }
    }
  }
}
```

```json
{
  "mcpServers": {
    "unreal-mcp": {
      "command": "...",
      "args": ["..."],
      "env": {
        "ENABLED_MODULES": "editor,blueprint,material"
      }
    }
  }
}
```

### Preset recipes

| Role | `ENABLED_MODULES` | ~Tools | ~Tokens |
|------|-------------------|--------|---------|
| **Minimal** — actors + blueprints only | `editor,blueprint,blueprint_nodes` | ~30 | ~15k |
| **Level designer** — widgets + materials | `editor,blueprint,blueprint_nodes,widget_core,widget_commonui,material` | ~200 | ~42k |
| **AI / gameplay programmer** — BT + blackboard + EQS | `editor,blueprint,behavior_tree,blackboard,eqs` | ~120 | ~25k |
| **Material / shader work** | `editor,material,material_graph` | ~35 | ~8k |
| **Audio / MetaSound** | `editor,metasound` | ~40 | ~9k |
| **Full** — everything | `all` (or omit the var) | 449 | ~95k |

### Available module names

`editor`, `blueprint`, `blueprint_nodes`, `blueprint_inspect`, `blueprint_search`, `blueprint_graph`, `blueprint_compound`, `blueprint_intelligence`, `widget` (or individual sub-modules: `widget_core`, `widget_commonui`, `widget_batch`, `widget_discovery`, `widget_style`, `widget_input`, `widget_animation`, `widget_commonui_ext`, `widget_readonly`), `material`, `material_graph`, `metasound`, `niagara` (scaffolded, not functional), `asset`, `behavior_tree`, `blackboard`, `eqs`, `pcg`, `input`, `procedural`, `project`.

After changing `ENABLED_MODULES` or `DYNAMIC_MODE`, **fully quit and restart your MCP client** so it re-reads the config and fetches the new tool catalog. Just reconnecting (e.g. Claude Code's `/mcp`) is often not enough — most clients cache tool schemas per session.

---

## Feature Reference

**449 MCP tools across 21 domains.** Every tool maps to a typed Python wrapper in `Server/tools/` and a C++ handler in `Source/UnrealMCP/Private/Commands/`.

### Editor & Scene (15 tools)

Spawn actors, move / rotate / scale them, set properties via reflection, inspect material assignments, take screenshots, read/write console variables (CVars), start and stop Play-In-Editor.

### Blueprints (101 tools total)

| Area | Tools | What It Does |
|------|-------|--------------|
| Blueprint core | 10 | Create BP actors/classes, add components, set defaults, compile |
| Node graph | 8 | Work with event graphs, function graphs, macro graphs |
| Low-level nodes | 55 | Every node type: branch, loop, switch, cast, delay, timeline, dispatchers, interface messages, reroute, comment... |
| Compound operations | 10 | Higher-level "create variable + getter + setter" style helpers |
| Inspection | 11 | Read any BP's graphs, nodes, pins, connections, variables |
| Search | 5 | Action catalog, category browsing, find-by-name |
| Intelligence | 2 | Graph complexity analysis, pattern detection |

Plus the **blueprint graph builder** -- a higher-level tool that takes a structured description and constructs a complete graph in one call.

### UMG Widgets (120 tools)

The biggest surface area. Full UMG + Common UI support:

- **Containers** -- HorizontalBox, VerticalBox, GridPanel, UniformGridPanel, Overlay, WrapBox, ScrollBox, SizeBox, ScaleBox, Border, Spacer, WidgetSwitcher
- **Common UI widgets** -- CommonButton, CommonButtonBase, CommonTextBlock, CommonRichTextBlock, CommonNumericTextBlock, CommonDateTimeTextBlock, CommonBorder, CommonLazyImage, CommonListView, CommonTileView, CommonTreeView, CommonAnimatedSwitcher, CommonVisibilitySwitcher, CommonHierarchicalScrollBox, CommonActivatableWidget, CommonActionWidget, CommonLoadGuard, CommonWidgetCarousel, CommonVideoPlayer, CommonRotator
- **Input** -- Button, CheckBox, Slider, AnalogSlider, ComboBox, EditableText
- **Display** -- Image, TextBlock, ProgressBar, CircularThrobber, Throbber
- **Layout tools** -- alignment, padding, slot properties per container, widget reparenting
- **Styling** -- BorderStyle, ButtonStyle, TextBlockStyle, ProgressBarStyle, image brushes, opacity, visibility
- **Animations** -- widget animations, float/color/transform tracks, keyframes, timeline control
- **Events** -- BindWidget events, variable exposure, event dispatchers
- **Introspection** -- full hierarchy dumps, bounds, properties, widget search

### Materials (31 tools)

- Create materials and material instances, set PBR properties (domain, blend mode, shading model, two-sided, etc.)
- 100+ material node types via `add_material_node` (multiply, add, lerp, texture sample, noise, fresnel, custom HLSL, ...)
- Connect nodes, connect to material outputs, set pin defaults, auto-layout the graph
- Material functions with input/output pins
- Material instance parameter overrides (scalar, vector, texture)
- Material hierarchy inspection, unconnected pin detection, graph fixup
- Recompile and preview

Plus the **material graph builder** -- takes a JSON tree description and constructs a full material graph.

### MetaSounds (26 tools)

- Create MetaSound sources and presets
- Add nodes by class path (oscillators, filters, envelopes, math, conversions, audio I/O)
- Connect audio / trigger / value pins
- Declare inputs, outputs, and graph variables
- Set default values on inputs and node pins
- Auto-build metasound graphs from a description

### Niagara (38 tools) -- NOT WORKING YET

> [!CAUTION]
> **Niagara support is scaffolded but not functional.** The tool surface exists (create systems, add emitters/renderers/modules, configure CPU/GPU modes, parameter exposure, emitter control, etc.) but the underlying C++ commands do not produce working VFX in the current state. **Do not rely on these tools.** Fixing this is on the roadmap -- PRs welcome.

### Behavior Trees (51 tools)

- Create / open / delete behavior tree and blackboard assets
- Every standard composite (Sequence, Selector, SimpleParallel)
- Every standard decorator (Blackboard, CompareBB, ConeCheck, Cooldown, TagCooldown, TimeLimit, Loop, ForceSuccess, DoesPathExist, IsAtLocation)
- Every standard service (DefaultFocus, RunEQS)
- Every standard task (MoveTo, Wait, PlaySound, PlayAnimation, RotateToFace, RunBehavior, RunEQS, SetKeyValue, MakeNoise, FinishWithResult)
- Generic "add any node by class name" via reflection
- Runtime control: run, pause, resume, restart, stop, inspect runtime state
- Auto-arrange tree layout

### Blackboards (8 tools)

Create and manage blackboards, add/remove/modify keys, get/set/clear values at runtime, inspect full state.

### EQS -- Environment Query System (14 tools)

Full EQS authoring: create queries, add generators, add tests (Distance, Dot, Trace, Overlap, Project, custom project subclasses), set generator/test properties via reflection (supports enum arrays, structs, soft references). Open queries in the editor. Discover available options dynamically.

### PCG (25 tools) -- Procedural Content Generation

Reflection-driven PCG graph authoring:

- Create / delete / duplicate / rename graphs, list all graphs
- Begin/end editing sessions for deterministic batch operations
- Add nodes by class (any PCG node type, auto-discovered)
- Connect / disconnect pins with validation
- Move nodes, auto-layout columns by topology
- Schema introspection: node pins, input/output types
- **Full property marshaler** -- scalar, struct, array, map, set, enum, object/class/soft-reference, FMapProperty (with offset fix)
- Array manipulation: add, remove, clear items
- Property path parser: `Mesh.Entries[2].Descriptor`
- Inverse marshaling (FProperty -> JSON) for round-trip reads

### Input (10 tools)

Create Enhanced Input mappings, add action / axis bindings, configure input modifiers and triggers.

### Assets (17 tools)

Bulk operations: list, move, rename, duplicate, batch-move, analyze folder organization, fix redirectors, find references, save assets. Protected-path validation prevents accidental modification of engine content.

### Procedural Geometry (6 tools)

One-shot geometry generators: walls, stairs, towers, arches, pyramids, mazes. Each returns a spawned actor or blueprint.

### Project Settings (1 tool)

Common UI configuration. (CVar operations are in the Editor group.)

---

## Repository Layout

```
UnrealMCP/                        # repo root IS the UE plugin folder
|-- UnrealMCP.uplugin             # plugin descriptor
|-- Source/UnrealMCP/
|   |-- UnrealMCP.Build.cs
|   |-- Public/                   # 4 core headers + 21 command group headers
|   |   |-- MCPCore.h
|   |   |-- MCPServerRunnable.h
|   |   |-- UnrealMCPBridge.h
|   |   |-- UnrealMCPModule.h
|   |   +-- Commands/             # per-domain command headers
|   +-- Private/                  # implementations (~40k lines)
|       +-- Commands/             # per-domain command implementations
|-- Server/
|   |-- pyproject.toml            # uv-managed Python project
|   |-- unreal_mcp_server.py      # FastMCP entry point
|   |-- tools/                    # 21 tool modules (~20k lines, 449 tools)
|   +-- scripts/                  # standalone test scripts (no MCP required)
|-- Claude/
|   |-- .mcp.json.template        # MCP config template
|   +-- agents/
|       +-- mcp-widget-expert.md  # optional Claude Code agent
|-- Config/
|   +-- FilterPlugin.ini
|-- README.md
+-- LICENSE
```

Unreal Engine only reads `UnrealMCP.uplugin` and `Source/` -- everything else coexists harmlessly inside the plugin directory.

---

## Security Model

The C++ plugin runs a **localhost-only TCP server** on `127.0.0.1:55557` with **no authentication**. Any process on your machine can send commands and have them executed in the Unreal Editor. This is standard for local developer tooling, but be aware:

- **Do not run on machines where untrusted code may execute.** Any local process can spawn actors, modify blueprints, or read project data through the socket.
- **Do not expose port 55557 to a network.** The socket is bound to `127.0.0.1` by default; keep it that way.
- Screenshot commands are restricted to paths under your project directory.
- Asset commands use a protected-path validator to block accidental modification of engine content.

This plugin is for **local development use only.**

---

## Troubleshooting

**MCP client shows no tools after install.**
Restart the MCP client after creating `.mcp.json`. Claude Code and Claude Desktop only read the config at startup.

**"Not connected to Unreal Engine" errors.**
The Unreal Editor must be running with the plugin loaded. Check the editor log for `UnrealMCP TCP server listening on port 55557`.

**Build fails after cloning.**
Regenerate project files (right-click `.uproject` > "Generate Visual Studio project files") before building. The plugin adds new modules UBT needs to see.

**Port 55557 already in use.**
Close any other running UE editor with the plugin loaded, or change the port in `Source/UnrealMCP/Private/MCPServerRunnable.cpp` and rebuild.

**Plugin loads but AI tools hang.**
The TCP server runs in a background thread but dispatches work to the game thread. If the editor is busy (e.g. importing assets), requests queue. Wait for the editor to idle, then retry.

**Test scripts without MCP.**
`Server/scripts/` contains standalone Python scripts that speak directly to the TCP server -- handy for debugging without involving an AI client:

```bash
cd Plugins/UnrealMCP/Server
python scripts/actors/test_cube.py
python scripts/blueprints/test_create_and_spawn_cube_blueprint.py
```

---

## Contributing

Pull requests welcome. Follow the existing patterns:

- **C++ command headers** -> `Source/UnrealMCP/Public/Commands/`
- **C++ command implementations** -> `Source/UnrealMCP/Private/Commands/`
- **Python tool modules** -> `Server/tools/` (one module per domain)
- **Register new C++ handlers** in `Source/UnrealMCP/Private/UnrealMCPBridge.cpp` (include header, `MakeShared` in constructor, `Reset()` in destructor, else-if branch in `ExecuteCommand()`)
- **Register new Python tool modules** in `Server/unreal_mcp_server.py` (import + call `register_*_tools(mcp)`)

Tests live in `Server/scripts/` (functional smoke tests) and `Server/tests/` (end-to-end smoke tests).

Please keep commits focused and commit messages descriptive.

---

## Credits

Originally inspired by [chongdashu/unreal-mcp](https://github.com/chongdashu/unreal-mcp) (MIT), which provided the initial proof-of-concept for the Python FastMCP -> TCP -> C++ plugin architecture.

This project has been **substantially rewritten and expanded** since, adding:

- A new command-group architecture with 21 reflection-driven C++ command modules
- 449 MCP tools (from the original small set)
- Session-based editing for deterministic multi-step operations
- Blueprint graph builder, material graph builder, and blueprint intelligence tooling
- PCG, MetaSounds, Behavior Trees, Blackboards, EQS, and Input support (Niagara is scaffolded but not yet functional)
- A generic property marshaler handling structs, arrays, maps, sets, enums, object/class/soft-references, and `FMapProperty`
- 100+ UMG / Common UI widget types
- Full blueprint inspection and search

### Built With AI

A large portion of this codebase was written with AI assistance -- primarily **Claude (Anthropic)** via Claude Code. Reviewing, testing, and direction come from a human; a lot of the mechanical C++ / Python was AI-generated. This means quality is uneven, and some areas need human cleanup. If you spot rough code, a PR is the fastest way to fix it.

---

## License

MIT License -- see [LICENSE](LICENSE).
