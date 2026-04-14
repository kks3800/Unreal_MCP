"""
Unreal Engine MCP Server

A simple MCP server for interacting with Unreal Engine.
"""

import logging
import socket
import sys
import json
from contextlib import asynccontextmanager
from typing import AsyncIterator, Dict, Any, Optional
from mcp.server.fastmcp import FastMCP

# Configure logging with more detailed format
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - [%(filename)s:%(lineno)d] - %(message)s',
    handlers=[
        logging.FileHandler('unreal_mcp.log'),
        # logging.StreamHandler(sys.stdout) # Remove this handler to unexpected non-whitespace characters in JSON
    ]
)
logger = logging.getLogger("UnrealMCP")

# Configuration
UNREAL_HOST = "127.0.0.1"
UNREAL_PORT = 55557

class UnrealConnection:
    """Connection to an Unreal Engine instance."""

    # Timeout tiers (seconds) — short for read-only queries, long for mutations/batches
    TIMEOUT_CONNECT = 3
    TIMEOUT_FAST = 10       # is_pie_active, list_assets, get_*
    TIMEOUT_NORMAL = 30     # compile, create, delete, set_*
    TIMEOUT_BATCH = 90      # execute_*_batch, build_*
    MAX_RETRIES = 2         # auto-retry on transient connection failures

    # Commands that are known to be fast read-only queries
    _FAST_COMMANDS = frozenset([
        "is_pie_active", "list_assets", "get_asset_info", "get_asset_class",
        "get_folder_structure", "find_asset_references", "is_protected_path",
        "get_actors_in_level", "get_actor_properties", "get_actor_material_info",
        "find_actors_by_name",
        "get_blueprint_info", "get_blueprint_variables", "get_blueprint_graphs",
        "get_blueprint_connections", "get_compile_status", "get_blueprint_snapshot",
        "get_class_functions", "get_graph_nodes", "get_graph_snapshot",
        "get_node_info", "get_node_pins", "get_node_type_info",
        "get_unconnected_pins", "find_blueprint_nodes", "search_blueprint_actions",
        "get_material_nodes", "get_material_hierarchy", "get_material_function_info",
        "get_widget_hierarchy", "get_widget_properties", "get_all_widgets",
        "get_widget_bounds", "get_widget_animations", "find_widget", "find_widgets",
        "get_children", "get_parent", "list_visible_widgets",
        "get_behavior_tree_info", "get_bt_tree_structure", "get_bt_node_info",
        "get_blackboard_info", "get_blackboard_state", "get_blackboard_value",
        "list_behavior_trees", "list_blackboards",
        "get_metasound_info", "get_metasound_nodes", "get_metasound_inputs",
        "get_metasound_outputs", "list_metasound_assets",
        "get_niagara_system_info", "list_niagara_systems",
        "get_emitter_info", "list_emitters", "get_renderers",
        "get_eqs_query_info", "list_eqs_queries", "get_eqs_tests",
        "get_common_ui_config", "get_animation_state",
        "read_blueprint_content", "analyze_blueprint_graph",
        "analyze_folder_organization", "analyze_graph_complexity",
        "analyze_material_graph", "validate_asset_move", "validate_connection",
    ])

    def __init__(self):
        """Initialize the connection."""
        self.socket = None
        self.connected = False

    def _close_socket(self):
        """Safely close and clear the socket."""
        if self.socket:
            try:
                self.socket.close()
            except Exception:
                pass
        self.socket = None
        self.connected = False

    def connect(self) -> bool:
        """Connect to the Unreal Engine instance."""
        try:
            self._close_socket()

            logger.info(f"Connecting to Unreal at {UNREAL_HOST}:{UNREAL_PORT}...")
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.settimeout(self.TIMEOUT_CONNECT)

            self.socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 65536)
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 65536)

            self.socket.connect((UNREAL_HOST, UNREAL_PORT))
            self.connected = True
            logger.info("Connected to Unreal Engine")
            return True

        except Exception as e:
            logger.error(f"Failed to connect to Unreal: {e}")
            self._close_socket()
            return False

    def disconnect(self):
        """Disconnect from the Unreal Engine instance."""
        self._close_socket()

    def _timeout_for_command(self, command: str) -> int:
        """Return the appropriate recv timeout for a given command."""
        if "batch" in command.lower() or command.startswith("build_"):
            return self.TIMEOUT_BATCH
        if command in self._FAST_COMMANDS:
            return self.TIMEOUT_FAST
        return self.TIMEOUT_NORMAL

    def receive_full_response(self, sock, buffer_size=4096, timeout=30) -> bytes:
        """Receive a complete JSON response from Unreal, handling chunked data."""
        chunks = []
        sock.settimeout(timeout)
        try:
            while True:
                chunk = sock.recv(buffer_size)
                if not chunk:
                    if not chunks:
                        raise Exception("Connection closed before receiving data")
                    break
                chunks.append(chunk)

                data = b''.join(chunks)
                try:
                    json.loads(data.decode('utf-8'))
                    logger.info(f"Received complete response ({len(data)} bytes)")
                    return data
                except json.JSONDecodeError:
                    continue
        except socket.timeout:
            logger.warning(f"Socket timeout after {timeout}s during receive")
            if chunks:
                data = b''.join(chunks)
                try:
                    json.loads(data.decode('utf-8'))
                    logger.info(f"Using partial response after timeout ({len(data)} bytes)")
                    return data
                except Exception:
                    pass
            raise Exception(f"Timeout receiving Unreal response after {timeout}s")
        except Exception as e:
            logger.error(f"Error during receive: {str(e)}")
            raise

    def send_command(self, command: str, params: Dict[str, Any] = None) -> Optional[Dict[str, Any]]:
        """Send a command to Unreal Engine and get the response.

        Reconnects per-command (Unreal closes the socket after each response).
        Retries once on transient connection/timeout failures.
        """
        last_error = None
        recv_timeout = self._timeout_for_command(command)

        for attempt in range(1, self.MAX_RETRIES + 1):
            # Fresh connection for every attempt
            self._close_socket()

            if not self.connect():
                last_error = "Failed to connect to Unreal Engine"
                logger.error(f"[attempt {attempt}/{self.MAX_RETRIES}] {last_error}")
                continue

            try:
                command_obj = {
                    "type": command,
                    "params": params or {}
                }
                command_json = json.dumps(command_obj)
                logger.info(f"[attempt {attempt}] Sending: {command} (timeout={recv_timeout}s)")
                self.socket.sendall(command_json.encode('utf-8'))

                response_data = self.receive_full_response(self.socket, timeout=recv_timeout)
                response = json.loads(response_data.decode('utf-8'))

                logger.info(f"Response from Unreal: {json.dumps(response)[:500]}")

                # Normalise error formats
                if response.get("status") == "error":
                    error_message = response.get("error") or response.get("message", "Unknown Unreal error")
                    logger.error(f"Unreal error (status=error): {error_message}")
                    if "error" not in response:
                        response["error"] = error_message
                elif response.get("success") is False:
                    error_message = response.get("error") or response.get("message", "Unknown Unreal error")
                    logger.error(f"Unreal error (success=false): {error_message}")
                    response = {"status": "error", "error": error_message}

                return response

            except Exception as e:
                last_error = str(e)
                logger.warning(f"[attempt {attempt}/{self.MAX_RETRIES}] {command} failed: {last_error}")
            finally:
                self._close_socket()

        logger.error(f"All {self.MAX_RETRIES} attempts failed for '{command}': {last_error}")
        return {"status": "error", "error": f"Failed after {self.MAX_RETRIES} attempts: {last_error}"}

# Global connection state
_unreal_connection: UnrealConnection = None

def get_unreal_connection() -> Optional[UnrealConnection]:
    """Get or create the global UnrealConnection singleton.

    No socket health-check here — send_command() reconnects per-command anyway.
    This just ensures the object exists.
    """
    global _unreal_connection
    try:
        if _unreal_connection is None:
            _unreal_connection = UnrealConnection()
            # Try an initial connect to verify Unreal is reachable
            if not _unreal_connection.connect():
                logger.warning("Could not connect to Unreal Engine on startup")
                # Keep the object — send_command will retry on each call
            else:
                # Close immediately; send_command reconnects per-command
                _unreal_connection._close_socket()
        return _unreal_connection
    except Exception as e:
        logger.error(f"Error getting Unreal connection: {e}")
        return None

@asynccontextmanager
async def server_lifespan(server: FastMCP) -> AsyncIterator[Dict[str, Any]]:
    """Handle server startup and shutdown."""
    global _unreal_connection
    logger.info("UnrealMCP server starting up")
    try:
        _unreal_connection = get_unreal_connection()
        if _unreal_connection:
            logger.info("Connected to Unreal Engine on startup")
        else:
            logger.warning("Could not connect to Unreal Engine on startup")
    except Exception as e:
        logger.error(f"Error connecting to Unreal Engine on startup: {e}")
        _unreal_connection = None
    
    try:
        yield {}
    finally:
        if _unreal_connection:
            _unreal_connection.disconnect()
            _unreal_connection = None
        logger.info("Unreal MCP server shut down")

# Initialize server
mcp = FastMCP(
    "UnrealMCP",
    instructions="Unreal Engine integration via Model Context Protocol",
    lifespan=server_lifespan
)

# ---------------------------------------------------------------------------
# Module Registration (controlled by ENABLED_MODULES env var)
# ---------------------------------------------------------------------------
# Set ENABLED_MODULES in .mcp.json to a comma-separated list of module names
# to load only those modules. Use "all" or omit the variable to load everything.
#
# Available modules:
#   editor, blueprint, blueprint_nodes, project, material, metasound,
#   niagara, asset, blueprint_inspect, blueprint_search, blueprint_graph,
#   blueprint_compound, blueprint_intelligence, behavior_tree, blackboard,
#   eqs
#
# Widget modules (use "widget" to load all, or pick individually):
#   widget, widget_core, widget_commonui, widget_batch, widget_discovery,
#   widget_style, widget_input, widget_animation, widget_commonui_ext,
#   widget_readonly (read-only subset: get/find/list only, no mutations)
# ---------------------------------------------------------------------------

import os

from tools.editor_tools import register_editor_tools
from tools.blueprint_tools import register_blueprint_tools
from tools.node_tools import register_blueprint_node_tools
from tools.project_tools import register_project_tools
from tools.umg_tools import (
    register_umg_tools,
    register_widget_core_tools,
    register_widget_commonui_tools,
    register_widget_batch_tools,
    register_widget_discovery_tools,
    register_widget_style_tools,
    register_widget_input_tools,
    register_widget_animation_tools,
    register_widget_commonui_ext_tools,
    register_widget_readonly_tools,
)
from tools.material_tools import register_material_tools
from tools.material_graph_tools import register_material_graph_tools
from tools.metasound_tools import register_metasound_tools
from tools.niagara_tools import register_niagara_tools
from tools.asset_tools import register_asset_tools
from tools.blueprint_inspect_tools import register_blueprint_inspect_tools
from tools.blueprint_search_tools import register_blueprint_search_tools
from tools.blueprint_graph_tools import register_blueprint_graph_tools
from tools.blueprint_compound_tools import register_blueprint_compound_tools
from tools.blueprint_intelligence import register_blueprint_intelligence_tools
from tools.behavior_tree_tools import register_behavior_tree_tools
from tools.blackboard_tools import register_blackboard_tools
from tools.eqs_tools import register_eqs_tools
from tools.input_tools import register_input_tools
from tools.procedural_tools import register_procedural_tools
from tools.pcg_tools import register_pcg_tools

MODULE_REGISTRY = {
    "editor":                register_editor_tools,
    "blueprint":             register_blueprint_tools,
    "blueprint_nodes":       register_blueprint_node_tools,
    "project":               register_project_tools,
    "widget":                register_umg_tools,
    "widget_core":           register_widget_core_tools,
    "widget_commonui":       register_widget_commonui_tools,
    "widget_batch":          register_widget_batch_tools,
    "widget_discovery":      register_widget_discovery_tools,
    "widget_style":          register_widget_style_tools,
    "widget_input":          register_widget_input_tools,
    "widget_animation":      register_widget_animation_tools,
    "widget_commonui_ext":   register_widget_commonui_ext_tools,
    "widget_readonly":       register_widget_readonly_tools,
    "material":              register_material_tools,
    "material_graph":        register_material_graph_tools,
    "metasound":             register_metasound_tools,
    "niagara":               register_niagara_tools,
    "asset":                 register_asset_tools,
    "blueprint_inspect":     register_blueprint_inspect_tools,
    "blueprint_search":      register_blueprint_search_tools,
    "blueprint_graph":       register_blueprint_graph_tools,
    "blueprint_compound":    register_blueprint_compound_tools,
    "blueprint_intelligence": register_blueprint_intelligence_tools,
    "behavior_tree":         register_behavior_tree_tools,
    "blackboard":            register_blackboard_tools,
    "eqs":                   register_eqs_tools,
    "input":                 register_input_tools,
    "procedural":            register_procedural_tools,
    "pcg":                   register_pcg_tools,
}

# Sub-modules that are covered by their parent (e.g. "widget" loads all widget_* sub-modules).
# When "all" is used, only the parent is loaded to avoid double-registration.
_PARENT_MODULES = {
    "widget": {"widget_core", "widget_commonui", "widget_batch", "widget_discovery",
               "widget_style", "widget_input", "widget_animation", "widget_commonui_ext",
               "widget_readonly"},
}
_ALL_SUB_MODULES = set()
for _subs in _PARENT_MODULES.values():
    _ALL_SUB_MODULES |= _subs

_enabled_env = os.environ.get("ENABLED_MODULES", "all").strip()
if _enabled_env.lower() == "all" or _enabled_env == "":
    _enabled_modules = set(MODULE_REGISTRY.keys()) - _ALL_SUB_MODULES
else:
    _enabled_modules = {m.strip() for m in _enabled_env.split(",")}
    _unknown = _enabled_modules - set(MODULE_REGISTRY.keys())
    if _unknown:
        logger.warning(f"Unknown modules in ENABLED_MODULES (ignored): {', '.join(sorted(_unknown))}")
        _enabled_modules -= _unknown
    # Resolve parent/sub-module conflicts to avoid double-registration:
    # - If parent is listed, remove sub-modules (parent covers them all)
    # - If only sub-modules are listed (no parent), keep as-is
    for _parent, _subs in _PARENT_MODULES.items():
        if _parent in _enabled_modules:
            _overlap = _enabled_modules & _subs
            if _overlap:
                _enabled_modules -= _overlap
                logger.info(f"Removed sub-modules {sorted(_overlap)} — parent '{_parent}' covers them")

# ---------------------------------------------------------------------------
# DYNAMIC_MODE — opt-in dynamic toolset registration
# ---------------------------------------------------------------------------
# When set (DYNAMIC_MODE=1 in .mcp.json env), each module is registered
# through a RecordingProxy that captures tools into a ToolRegistry instead
# of exposing them to the MCP client. Only the ~20 core tools + 3 meta-tools
# (search/describe/execute) are exposed; the rest are reached dynamically.
# Rationale and design: see Docs/DYNAMIC_TOOLSETS_PLAN.md.
# Falls back automatically to legacy all-tools mode if setup fails.

_dynamic_mode_enabled = os.environ.get("DYNAMIC_MODE", "0") == "1"

if _dynamic_mode_enabled:
    try:
        from tool_registry import ToolRegistry, RecordingProxy, register_meta_tools
        from tool_categories import MODULE_CATEGORIES, CORE_TOOLS, KEYWORD_OVERRIDES

        _registry = ToolRegistry()
        _capture_failures: list[str] = []

        for _mod_name, _register_fn in MODULE_REGISTRY.items():
            if _mod_name not in _enabled_modules:
                logger.info(f"Skipped module: {_mod_name}")
                continue
            _category = MODULE_CATEGORIES.get(_mod_name, "misc")
            _proxy = RecordingProxy(
                registry=_registry,
                category=_category,
                core_names=CORE_TOOLS,
                keyword_overrides=KEYWORD_OVERRIDES,
            )
            try:
                _register_fn(_proxy)
                logger.info(f"Captured module: {_mod_name}")
            except Exception as e:
                _capture_failures.append(_mod_name)
                logger.warning(f"DYNAMIC_MODE: failed to capture module {_mod_name}: {e}")

        if len(_registry) == 0:
            raise RuntimeError("registry is empty after capture")

        # Expose only the core tools to the real mcp
        _core_count = 0
        for _name, _meta in _registry._tools.items():
            if _meta.is_core:
                # Use FastMCP's registration with the captured callable
                mcp.tool(name=_name, description=_meta.description)(_meta.func)
                _core_count += 1

        register_meta_tools(mcp, _registry)

        _dyn_count = len(_registry) - _core_count
        logger.info(
            f"DYNAMIC_MODE enabled: {_core_count} core + 3 meta + {_dyn_count} dynamic "
            f"(captured from {len(_enabled_modules) - len(_capture_failures)}/"
            f"{len(_enabled_modules)} modules)"
        )
        if _capture_failures:
            logger.warning(f"DYNAMIC_MODE: these modules failed to capture: {_capture_failures}")

    except Exception as e:
        logger.error(f"DYNAMIC_MODE setup failed, falling back to legacy all-tools mode: {e}")
        _dynamic_mode_enabled = False

if not _dynamic_mode_enabled:
    # Legacy path — register every enabled module directly on the real mcp
    for _mod_name, _register_fn in MODULE_REGISTRY.items():
        if _mod_name in _enabled_modules:
            _register_fn(mcp)
            logger.info(f"Registered module: {_mod_name}")
        else:
            logger.info(f"Skipped module: {_mod_name}")

    logger.info(f"Modules loaded: {len(_enabled_modules)}/{len(MODULE_REGISTRY)}")

@mcp.prompt()
def info():
    """Information about available Unreal MCP tools and best practices."""
    return """
    # Unreal MCP Server Tools and Best Practices
    
    ## UMG (Widget Blueprint) Tools
    - `create_umg_widget_blueprint(widget_name, parent_class="UserWidget", path="/Game/UI")` 
      Create a new UMG Widget Blueprint
    - `add_text_block_to_widget(widget_name, text_block_name, text="", position=[0,0], size=[200,50], font_size=12, color=[1,1,1,1])`
      Add a Text Block widget with customizable properties
    - `add_button_to_widget(widget_name, button_name, text="", position=[0,0], size=[200,50], font_size=12, color=[1,1,1,1], background_color=[0.1,0.1,0.1,1])`
      Add a Button widget with text and styling
    - `bind_widget_event(widget_name, widget_component_name, event_name, function_name="")`
      Bind events like OnClicked to functions
    - `add_widget_to_viewport(widget_name, z_order=0)`
      Add widget instance to game viewport
    - `set_text_block_binding(widget_name, text_block_name, binding_property, binding_type="Text")`
      Set up dynamic property binding for text blocks

    ## Editor Tools
    ### Viewport and Screenshots
    - `focus_viewport(target, location, distance, orientation)` - Focus viewport
    - `take_screenshot(filename, show_ui, resolution)` - Capture screenshots

    ### Actor Management
    - `get_actors_in_level()` - List all actors in current level
    - `find_actors_by_name(pattern)` - Find actors by name pattern
    - `spawn_actor(name, type, location=[0,0,0], rotation=[0,0,0], scale=[1,1,1])` - Create actors
    - `delete_actor(name)` - Remove actors
    - `set_actor_transform(name, location, rotation, scale)` - Modify actor transform
    - `get_actor_properties(name)` - Get actor properties
    
    ## Blueprint Management
    - `create_blueprint(name, parent_class)` - Create new Blueprint classes
    - `add_component_to_blueprint(blueprint_name, component_type, component_name)` - Add components
    - `set_static_mesh_properties(blueprint_name, component_name, static_mesh)` - Configure meshes
    - `set_physics_properties(blueprint_name, component_name)` - Configure physics
    - `compile_blueprint(blueprint_name)` - Compile Blueprint changes
    - `set_blueprint_property(blueprint_name, property_name, property_value)` - Set properties
    - `set_pawn_properties(blueprint_name)` - Configure Pawn settings
    - `spawn_blueprint_actor(blueprint_name, actor_name)` - Spawn Blueprint actors
    
    ## Blueprint Node Management
    - `add_blueprint_event_node(blueprint_name, event_type)` - Add event nodes
    - `add_blueprint_input_action_node(blueprint_name, action_name)` - Add input nodes
    - `add_blueprint_function_node(blueprint_name, target, function_name)` - Add function nodes
    - `connect_blueprint_nodes(blueprint_name, source_node_id, source_pin, target_node_id, target_pin)` - Connect nodes
    - `add_blueprint_variable(blueprint_name, variable_name, variable_type)` - Add variables
    - `add_blueprint_get_self_component_reference(blueprint_name, component_name)` - Add component refs
    - `add_blueprint_self_reference(blueprint_name)` - Add self references
    - `find_blueprint_nodes(blueprint_name, node_type, event_type)` - Find nodes
    
    ## Project Tools
    - `create_input_mapping(action_name, key, input_type)` - Create input mappings
    
    ## Best Practices
    
    ### UMG Widget Development
    - Create widgets with descriptive names that reflect their purpose
    - Use consistent naming conventions for widget components
    - Organize widget hierarchy logically
    - Set appropriate anchors and alignment for responsive layouts
    - Use property bindings for dynamic updates instead of direct setting
    - Handle widget events appropriately with meaningful function names
    - Clean up widgets when no longer needed
    - Test widget layouts at different resolutions
    
    ### Editor and Actor Management
    - Use unique names for actors to avoid conflicts
    - Clean up temporary actors
    - Validate transforms before applying
    - Check actor existence before modifications
    - Take regular viewport screenshots during development
    - Keep the viewport focused on relevant actors during operations
    
    ### Blueprint Development
    - Compile Blueprints after changes
    - Use meaningful names for variables and functions
    - Organize nodes logically
    - Test functionality in isolation
    - Consider performance implications
    - Document complex setups
    
    ### Error Handling
    - Check command responses for success
    - Handle errors gracefully
    - Log important operations
    - Validate parameters
    - Clean up resources on errors
    """

# Run the server
if __name__ == "__main__":
    logger.info("Starting MCP server with stdio transport")
    mcp.run(transport='stdio') 