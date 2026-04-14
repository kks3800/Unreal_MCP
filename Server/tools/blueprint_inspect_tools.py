"""
Blueprint inspection tools for Unreal MCP.

Provides read-only inspection of Blueprint graphs, nodes, pins,
variables, connections, and compile status via MCP commands.
"""

import logging
from mcp.server.fastmcp import FastMCP, Context
from typing import Dict, Any, Optional

# Get logger
logger = logging.getLogger("UnrealMCP")


def register_blueprint_inspect_tools(mcp: FastMCP):
    """Register all blueprint inspection MCP tools."""

    @mcp.tool()
    def get_blueprint_info(
        ctx: Context,
        blueprint_name: str
    ) -> Dict[str, Any]:
        """
        Get high-level information about a Blueprint.

        Returns parent class, compile status, list of graphs, variable count,
        and component count.

        Args:
            blueprint_name: Blueprint name or full asset path
                (e.g., "BP_MyActor" or "/Game/Blueprints/BP_MyActor")

        Returns:
            Dict with parent_class, compile_status, graphs array, variable_count, component_count
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_blueprint_info", {
                "blueprint_name": blueprint_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return response

        except Exception as e:
            logger.error(f"Error getting blueprint info: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_blueprint_graphs(
        ctx: Context,
        blueprint_name: str
    ) -> Dict[str, Any]:
        """
        Get all graphs in a Blueprint.

        Returns an array of graphs with name, type (EventGraph/FunctionGraph/MacroGraph),
        node count, and schema.

        Args:
            blueprint_name: Blueprint name or full asset path

        Returns:
            Dict with graphs array containing name, type, node_count for each graph
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_blueprint_graphs", {
                "blueprint_name": blueprint_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return response

        except Exception as e:
            logger.error(f"Error getting blueprint graphs: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_graph_nodes(
        ctx: Context,
        blueprint_name: str,
        graph_name: str
    ) -> Dict[str, Any]:
        """
        Get all nodes in a specific Blueprint graph.

        Returns an array of nodes with guid, class, title, position, and comment.

        Args:
            blueprint_name: Blueprint name or full asset path
            graph_name: Name of the graph (e.g., "EventGraph", function name)

        Returns:
            Dict with nodes array containing guid, class, title, pos_x, pos_y for each node
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_graph_nodes", {
                "blueprint_name": blueprint_name,
                "graph_name": graph_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return response

        except Exception as e:
            logger.error(f"Error getting graph nodes: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_node_info(
        ctx: Context,
        blueprint_name: str,
        graph_name: str,
        node_guid: str
    ) -> Dict[str, Any]:
        """
        Get detailed information about a specific node, including all its pins.

        Args:
            blueprint_name: Blueprint name or full asset path
            graph_name: Name of the graph containing the node
            node_guid: GUID of the node (from get_graph_nodes results)

        Returns:
            Dict with full node details including guid, class, title, position,
            and a pins array with name, direction, type, connections, default values
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_node_info", {
                "blueprint_name": blueprint_name,
                "graph_name": graph_name,
                "node_guid": node_guid
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return response

        except Exception as e:
            logger.error(f"Error getting node info: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_node_pins(
        ctx: Context,
        blueprint_name: str,
        graph_name: str,
        node_guid: str
    ) -> Dict[str, Any]:
        """
        Get all pins for a specific node with full type and connection details.

        Args:
            blueprint_name: Blueprint name or full asset path
            graph_name: Name of the graph containing the node
            node_guid: GUID of the node

        Returns:
            Dict with pins array containing name, direction, category, default_value,
            connections, hidden status, etc. for each pin
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_node_pins", {
                "blueprint_name": blueprint_name,
                "graph_name": graph_name,
                "node_guid": node_guid
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return response

        except Exception as e:
            logger.error(f"Error getting node pins: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_blueprint_variables(
        ctx: Context,
        blueprint_name: str
    ) -> Dict[str, Any]:
        """
        Get all variables defined in a Blueprint.

        Returns name, type, category, default value, property flags,
        and replication info for each variable.

        Args:
            blueprint_name: Blueprint name or full asset path

        Returns:
            Dict with variables array containing name, type, category, default_value,
            is_replicated, rep_notify_func, container_type, property_flags
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_blueprint_variables", {
                "blueprint_name": blueprint_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return response

        except Exception as e:
            logger.error(f"Error getting blueprint variables: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_blueprint_connections(
        ctx: Context,
        blueprint_name: str,
        graph_name: str
    ) -> Dict[str, Any]:
        """
        Get all connections (wires) in a Blueprint graph.

        Returns source and target node GUIDs, pin names, and node titles
        for every connection in the graph.

        Args:
            blueprint_name: Blueprint name or full asset path
            graph_name: Name of the graph (e.g., "EventGraph")

        Returns:
            Dict with connections array containing source_node_guid, source_pin,
            target_node_guid, target_pin, and node titles
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_blueprint_connections", {
                "blueprint_name": blueprint_name,
                "graph_name": graph_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return response

        except Exception as e:
            logger.error(f"Error getting blueprint connections: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_compile_status(
        ctx: Context,
        blueprint_name: str
    ) -> Dict[str, Any]:
        """
        Compile a Blueprint and return the compile status with any messages.

        Triggers a full compile and captures all errors, warnings, and info messages.

        Args:
            blueprint_name: Blueprint name or full asset path

        Returns:
            Dict with status (up_to_date/dirty/error/warnings), num_errors,
            num_warnings, and messages array with severity and message text
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_compile_status", {
                "blueprint_name": blueprint_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return response

        except Exception as e:
            logger.error(f"Error getting compile status: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_unconnected_pins(
        ctx: Context,
        blueprint_name: str,
        graph_name: str,
        filter: Optional[str] = "all"
    ) -> Dict[str, Any]:
        """
        Find all unconnected (dangling) pins in a Blueprint graph.

        Useful for identifying incomplete wiring in a graph.

        Args:
            blueprint_name: Blueprint name or full asset path
            graph_name: Name of the graph (e.g., "EventGraph")
            filter: Optional filter - "exec" for exec pins only, "data" for data pins only,
                   "all" for both (default)

        Returns:
            Dict with unconnected_pins array containing node_guid, node_title,
            pin_name, pin_direction, pin_category, is_exec
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            params = {
                "blueprint_name": blueprint_name,
                "graph_name": graph_name
            }
            if filter and filter != "all":
                params["filter"] = filter

            response = unreal.send_command("get_unconnected_pins", params)

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return response

        except Exception as e:
            logger.error(f"Error getting unconnected pins: {e}")
            return {"status": "error", "error": str(e)}

    # ---- Blueprint Analysis Tools ----

    @mcp.tool()
    def read_blueprint_content(
        ctx: Context,
        blueprint_name: str,
        include_event_graph: bool = True,
        include_functions: bool = True,
        include_variables: bool = True,
        include_components: bool = True,
        include_interfaces: bool = True,
    ) -> Dict[str, Any]:
        """Full blueprint content dump with semantic node data.

        Returns everything about a Blueprint in a single call: parent class,
        compile status, implemented interfaces, event dispatchers, variables
        (with replication/exposure flags), components, and all graphs with
        semantic node identity (which function is called, which variable is
        read/set, which event fires, cast targets, etc.).

        Args:
            blueprint_name: Name of the Blueprint asset.
            include_event_graph: Include UbergraphPages (event graphs).
            include_functions: Include function and macro graphs.
            include_variables: Include variable definitions.
            include_components: Include SCS component hierarchy.
            include_interfaces: Include implemented interfaces.
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Not connected to Unreal Engine"}

            params = {
                "blueprint_name": blueprint_name,
                "include_event_graph": include_event_graph,
                "include_functions": include_functions,
                "include_variables": include_variables,
                "include_components": include_components,
                "include_interfaces": include_interfaces,
            }

            response = unreal.send_command("read_blueprint_content", params)
            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}
            return response

        except Exception as e:
            logger.error(f"Error reading blueprint content: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def analyze_blueprint_graph(
        ctx: Context,
        blueprint_name: str,
        graph_name: str = "EventGraph",
        include_node_details: bool = True,
        include_pin_connections: bool = True,
        trace_execution_flow: bool = True,
    ) -> Dict[str, Any]:
        """Analyze a specific Blueprint graph with execution flow tracing.

        Returns all nodes with semantic identity (what each node does),
        full pin/connection data with type classification (exec vs data),
        and traced execution paths from every entry point (Event, CustomEvent,
        FunctionEntry) through the graph following exec wires.

        Args:
            blueprint_name: Name of the Blueprint asset.
            graph_name: Name of the graph to analyze (default: EventGraph).
            include_node_details: Include pin details per node.
            include_pin_connections: Include connection info per pin.
            trace_execution_flow: Trace and return execution paths from entry nodes.
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Not connected to Unreal Engine"}

            params = {
                "blueprint_name": blueprint_name,
                "graph_name": graph_name,
                "include_node_details": include_node_details,
                "include_pin_connections": include_pin_connections,
                "trace_execution_flow": trace_execution_flow,
            }

            response = unreal.send_command("analyze_blueprint_graph", params)
            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}
            return response

        except Exception as e:
            logger.error(f"Error analyzing blueprint graph: {e}")
            return {"status": "error", "error": str(e)}
