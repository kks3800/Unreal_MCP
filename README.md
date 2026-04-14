# UnrealMCP

**Model Context Protocol (MCP) integration for Unreal Engine 5.5+**

UnrealMCP lets AI assistants (Claude, Cursor, Windsurf, etc.) directly interact with the Unreal Engine Editor -- creating blueprints, materials, widgets, particle systems, MetaSounds, behavior trees, and more through natural language.

## Architecture

```
AI Assistant (Claude Code / Cursor / etc.)
    |
    | MCP Protocol (stdio)
    v
Python FastMCP Server (Server/)
    |
    | TCP JSON on port 55557
    v
C++ Editor Plugin (Source/)
    |
    | UE Editor APIs
    v
Unreal Engine Editor
```

The system has two parts:

1. **C++ Plugin** (`Source/`, `UnrealMCP.uplugin`) - An Unreal Engine editor plugin that listens on TCP port 55557 and executes commands inside the editor (spawn actors, create blueprints, edit materials, manage behavior trees, etc.)
2. **Python Server** (`Server/`) - A FastMCP server that exposes 430+ tools to AI assistants and translates them into TCP commands for the plugin

## Repo Structure

```
UnrealMCP/                      # This IS the UE plugin folder
├── UnrealMCP.uplugin           # Plugin descriptor
├── Source/UnrealMCP/           # C++ plugin source
│   ├── UnrealMCP.Build.cs
│   ├── Public/                 # Headers (4 core + 19 command modules)
│   └── Private/                # Implementation (4 core + 19 command modules)
├── Server/                     # Python FastMCP server
│   ├── pyproject.toml
│   ├── unreal_mcp_server.py    # Main server with connection management
│   ├── tools/                  # 21 tool modules
│   └── scripts/                # Test scripts
├── Claude/                     # Claude Code integration (optional)
│   ├── .mcp.json.template
│   └── agents/                 # Specialized AI agents
├── README.md
├── LICENSE
└── .gitignore
```

UE only cares about `Source/` and `UnrealMCP.uplugin` -- it ignores everything else, so `Server/`, `Claude/`, `README.md` etc. coexist harmlessly inside the plugin directory.

## Capabilities

| Category | Tools | Examples |
|----------|-------|---------|
| **Editor** | Actors, properties, transforms, screenshots, PIE control | Spawn actors, set properties, find by name, take screenshots |
| **Blueprints** | Create, compile, components, graph nodes, analysis | Create BP actors, add events/functions/variables, analyze complexity |
| **Blueprint Inspection** | Read graphs, nodes, pins, connections, snapshots | Introspect any blueprint's structure and logic |
| **Blueprint Search** | Action search, category browsing, node finding | Find available BP actions, search by category |
| **Blueprint Intelligence** | Graph analysis, complexity metrics | Analyze blueprint graphs for complexity and patterns |
| **Materials** | Materials, instances, functions, graph builder | Create PBR materials, connect node graphs, build complete material graphs |
| **UMG Widgets** | Widget blueprints, Common UI, styling, animations | Build full UIs with 50+ widget types, layout, styling, events |
| **MetaSounds** | Sources, presets, nodes, variables, connections | Create audio graphs with oscillators, filters, envelopes |
| **Niagara** | Systems, emitters, renderers, modules, parameters | Create particle effects with sprite/mesh/ribbon/light renderers |
| **Behavior Trees** | Trees, nodes, decorators, services, tasks | Build complete AI behavior trees with all node types |
| **Blackboards** | Keys, values, runtime state | Create and manage AI blackboard data |
| **EQS** | Queries, generators, tests | Create Environment Query System queries for AI |
| **Input** | Input mappings, action/axis bindings | Configure input actions and axis mappings |
| **Assets** | List, move, rename, duplicate, organize, analyze | Bulk operations, folder analysis, redirector cleanup |
| **Procedural** | Walls, stairs, mazes, towers, arches, pyramids | Generate procedural geometry in the editor |
| **Project** | Project settings, CVars | Read/modify project settings and console variables |

## Quick Install (AI-Assisted)

If you're using **Claude Code**, paste this prompt:

> Install UnrealMCP into my project. The UnrealMCP repo is at `<PATH_TO_THIS_REPO>`. My Unreal project is at `<PATH_TO_YOUR_UE_PROJECT>`. Follow the instructions in the UnrealMCP README under "Claude Code Installation Instructions".

Replace the two paths and Claude will handle the rest.

---

## Claude Code Installation Instructions

**This section is written for Claude Code (or any AI coding assistant) to follow step-by-step.**

You will need two paths from the user:
- `UNREALMCP_REPO` - Where this UnrealMCP repository was cloned (or the path to this directory)
- `UE_PROJECT` - The root of their Unreal Engine project (contains the `.uproject` file)

### Prerequisites

Before starting, verify:
1. Python 3.10+ is installed and available on PATH (`python --version`)
2. `uv` is installed (`uv --version`). If not, install it: `pip install uv` or `curl -LsSf https://astral.sh/uv/install.sh | sh`
3. The Unreal project directory exists and contains a `.uproject` file

### Step 1: Install the Plugin

This repo IS the plugin. Clone or copy it directly into the project's `Plugins/` directory:

```bash
# Create Plugins dir if it doesn't exist
mkdir -p "<UE_PROJECT>/Plugins"

# Clone directly into the Plugins folder
git clone <REPO_URL> "<UE_PROJECT>/Plugins/UnrealMCP"

# OR if installing from a local copy:
cp -r "<UNREALMCP_REPO>" "<UE_PROJECT>/Plugins/UnrealMCP"
```

After this, verify the structure:
```
<UE_PROJECT>/Plugins/UnrealMCP/
  UnrealMCP.uplugin        <-- Must exist at this level
  Source/UnrealMCP/
  Server/
```

> **Note:** The user must regenerate project files and rebuild after this step. Do NOT run build commands -- just tell the user to rebuild.

### Step 2: Set Up the Python Server

The Python server is inside the plugin at `Plugins/UnrealMCP/Server/`. Set up its virtual environment:

```bash
cd "<UE_PROJECT>/Plugins/UnrealMCP/Server"

# Create virtual environment
uv venv

# Install dependencies
uv pip install -e .
```

Verify the install succeeded:
```bash
# Windows:
.venv/Scripts/python -c "import mcp; print('MCP installed successfully')"
# macOS/Linux:
.venv/bin/python -c "import mcp; print('MCP installed successfully')"
```

### Step 3: Create the MCP Configuration

Create a `.mcp.json` file at the **UE project root** with absolute paths to the Python venv and server script.

Detect the platform and write the appropriate config:

**Windows:**
```json
{
  "mcpServers": {
    "unreal-mcp": {
      "type": "stdio",
      "command": "<UE_PROJECT>/Plugins/UnrealMCP/Server/.venv/Scripts/python.exe",
      "args": [
        "<UE_PROJECT>/Plugins/UnrealMCP/Server/unreal_mcp_server.py"
      ],
      "env": {}
    }
  }
}
```

**macOS/Linux:**
```json
{
  "mcpServers": {
    "unreal-mcp": {
      "type": "stdio",
      "command": "<UE_PROJECT>/Plugins/UnrealMCP/Server/.venv/bin/python",
      "args": [
        "<UE_PROJECT>/Plugins/UnrealMCP/Server/unreal_mcp_server.py"
      ],
      "env": {}
    }
  }
}
```

**Important:** Use absolute paths with forward slashes (even on Windows). Replace `<UE_PROJECT>` with the actual path.

### Step 4: Install Claude Code Agents (Optional)

If the user wants the specialized MCP agents for Claude Code:

```bash
# Create .claude/agents directory if it doesn't exist
mkdir -p "<UE_PROJECT>/.claude/agents"

# Copy agent definitions
cp "<UE_PROJECT>/Plugins/UnrealMCP/Claude/agents/mcp-widget-expert.md" "<UE_PROJECT>/.claude/agents/"
```

This agent gives Claude Code specialized knowledge about building UMG widgets through MCP tools.

### Step 5: Verify Installation

Run these checks to confirm everything is in place:

1. **Plugin exists:** `ls "<UE_PROJECT>/Plugins/UnrealMCP/UnrealMCP.uplugin"` should succeed
2. **Server exists:** `ls "<UE_PROJECT>/Plugins/UnrealMCP/Server/unreal_mcp_server.py"` should succeed
3. **Venv works:** Run the Python import check from Step 2
4. **MCP config exists:** `ls "<UE_PROJECT>/.mcp.json"` should succeed
5. **No path issues:** Open `.mcp.json` and verify all paths point to real files

### Post-Install Summary

Tell the user:
1. **Rebuild the project** - The C++ plugin needs to be compiled. Regenerate project files (right-click `.uproject` > "Generate Visual Studio project files") and build
2. **Open the Editor** - The plugin will start its TCP server on port 55557 automatically
3. **Restart Claude Code** - So it picks up the new `.mcp.json` configuration
4. **Test it** - Ask Claude to "spawn a cube at the origin" to verify the connection works

---

## Manual Installation

If you prefer to install manually (without AI assistance):

### 1. Clone into Plugins

```bash
cd YourProject/Plugins
git clone <REPO_URL> UnrealMCP
```

Or download and extract this repo as `YourProject/Plugins/UnrealMCP/`.

Then regenerate your project files and rebuild.

### 2. Set Up the Python Server

```bash
cd YourProject/Plugins/UnrealMCP/Server

# Create virtual environment (requires Python 3.10+)
uv venv

# Activate it
# Windows:
.venv\Scripts\activate
# macOS/Linux:
source .venv/bin/activate

# Install dependencies
uv pip install -e .
```

### 3. Configure Your MCP Client

Copy `Claude/.mcp.json.template` to your project root as `.mcp.json` and update the paths to point to `Plugins/UnrealMCP/Server/.venv/...` and `Plugins/UnrealMCP/Server/unreal_mcp_server.py`.

For **Claude Code**: Place `.mcp.json` at your project root.
For **Claude Desktop**: Add the server config to `claude_desktop_config.json`.
For **Cursor/Windsurf**: Follow their respective MCP configuration docs.

### 4. (Optional) Claude Code Agents

Copy the agent definitions from `Claude/agents/` to your project's `.claude/agents/` directory:

```
YourProject/
  .claude/
    agents/
      mcp-widget-expert.md      <-- Widget creation specialist
```

## Security

The C++ plugin runs a **localhost-only TCP server** on port `127.0.0.1:55557` with **no authentication**. Any process on your machine can send commands and have them executed in the Unreal Editor. This is standard for local developer tools, but be aware:

- **Do not run on machines where untrusted code may execute.** Any local process can spawn actors, modify blueprints, or read project data through the TCP socket.
- Screenshot commands are restricted to writing files under the project directory only.
- Asset commands have protected-path validation to prevent accidental modification of engine content.

This plugin is designed for local development use. Do not expose port 55557 to a network.

## Usage

1. Open your Unreal project in the Editor
2. The plugin starts its TCP server automatically on port 55557
3. Start your MCP client (Claude Code, Cursor, etc.) - it will connect via the Python server
4. Ask the AI to create things in Unreal!

**Example prompts:**
- "Create a blueprint actor with a rotating mesh component"
- "Make a translucent UI material with a radial gradient"
- "Build a lobby HUD with player name list and ready button"
- "Create a Niagara spark particle effect"
- "Add a MetaSound with an oscillator and low-pass filter"
- "Build a behavior tree for an AI that patrols and chases the player"
- "Create an EQS query to find cover positions"
- "Generate a spiral staircase with 20 steps"
- "Analyze the complexity of my blueprint graph"

## Testing

The `Server/scripts/` directory contains standalone test scripts that connect directly to the C++ plugin (no MCP server needed):

```bash
cd Plugins/UnrealMCP/Server
python scripts/actors/test_cube.py
python scripts/blueprints/test_create_and_spawn_cube_blueprint.py
python scripts/node/test_create_bird_blueprint_with_input_and_camera.py
```

Make sure the Unreal Editor is running with the plugin loaded before running tests.

## Contributing

1. Fork the repository
2. Create your feature branch
3. Follow the existing code patterns:
   - C++ command headers go in `Source/UnrealMCP/Public/Commands/`
   - C++ command implementations go in `Source/UnrealMCP/Private/Commands/`
   - Python tools go in `Server/tools/` (one module per domain)
   - Register new C++ command handlers in `Source/UnrealMCP/Private/UnrealMCPBridge.cpp`
   - Register new Python tool modules in `Server/unreal_mcp_server.py` (import + call `register_*_tools`)
4. Submit a pull request

## Credits

Originally inspired by [chongdashu/unreal-mcp](https://github.com/chongdashu/unreal-mcp) (MIT), which provided the initial proof-of-concept for Python FastMCP -> TCP -> C++ plugin architecture. This project has been substantially rewritten and expanded since:

- New command architecture with 20+ command modules (vs original scope)
- 430+ MCP tools across Editor, Blueprints, Materials, UMG, MetaSounds, Niagara, Behavior Trees, Blackboards, EQS, PCG, Input, Assets, Procedural generation, and Project settings
- Blueprint graph builder, material graph builder, blueprint inspection and intelligence tooling
- Full session-based editing model for deterministic multi-step operations

## License

MIT License - see [LICENSE](LICENSE) for details.
