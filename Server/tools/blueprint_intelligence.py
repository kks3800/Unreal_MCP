"""
Blueprint Intelligence Tools for Unreal MCP.

Provides graph analysis and function extraction capabilities.
These tools help Claude make intelligent decisions about Blueprint organization.
"""

import logging
from typing import Dict, List, Any, Optional
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("UnrealMCP")


def register_blueprint_intelligence_tools(mcp: FastMCP):
    """Register Blueprint intelligence tools with the MCP server."""

    @mcp.tool()
    def analyze_graph_complexity(
        ctx: Context,
        blueprint_name: str,
        graph_name: str = "EventGraph"
    ) -> Dict[str, Any]:
        """
        Analyze the complexity of a Blueprint graph and suggest function extraction.
        Returns node count, execution chain depth, disconnected clusters, and suggestions.

        Args:
            blueprint_name: Name of the Blueprint to analyze.
            graph_name: Graph to analyze (default: "EventGraph").

        Returns:
            Complexity analysis with extraction suggestions.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            # Get graph snapshot with full node and connection data
            snapshot = unreal.send_command("get_graph_snapshot", {
                "blueprint_name": blueprint_name,
                "graph_name": graph_name
            })

            if not snapshot or snapshot.get("status") == "error":
                return {"success": False, "error": f"Failed to get graph snapshot: {snapshot}"}

            result_data = snapshot.get("result", snapshot)
            nodes = result_data.get("nodes", [])
            node_count = len(nodes)

            if node_count == 0:
                return {
                    "success": True,
                    "node_count": 0,
                    "exec_chain_depth": 0,
                    "disconnected_clusters": 0,
                    "suggestion": "Graph is empty.",
                    "cluster_analysis": []
                }

            # Build adjacency maps
            node_map = {}  # guid -> node data
            exec_graph = {}  # guid -> [connected guids via exec pins]
            data_graph = {}  # guid -> [connected guids via data pins]

            for node in nodes:
                guid = node.get("guid", "")
                node_map[guid] = node
                exec_graph[guid] = []
                data_graph[guid] = []

                for pin in node.get("pins", []):
                    if pin.get("direction") == "output":
                        for conn in pin.get("connected_to", []):
                            # Connection format: "GUID:PinName"
                            parts = conn.split(":")
                            target_guid = parts[0] if parts else ""
                            if target_guid in node_map or target_guid:
                                if pin.get("type") == "exec":
                                    exec_graph[guid].append(target_guid)
                                else:
                                    data_graph[guid].append(target_guid)

            # Calculate exec chain depth using BFS from event nodes
            max_depth = 0
            event_nodes = []
            for node in nodes:
                title = node.get("title", "").lower()
                node_class = node.get("class", "")
                if "event" in title or "Event" in node_class or "K2Node_Event" in node_class:
                    event_nodes.append(node.get("guid", ""))

            for start_guid in event_nodes:
                visited = set()
                queue = [(start_guid, 0)]
                while queue:
                    current, depth = queue.pop(0)
                    if current in visited:
                        continue
                    visited.add(current)
                    max_depth = max(max_depth, depth)
                    for neighbor in exec_graph.get(current, []):
                        if neighbor not in visited:
                            queue.append((neighbor, depth + 1))

            # Find disconnected clusters using Union-Find
            all_connections = {}  # Merge exec and data connections
            for guid in node_map:
                all_connections[guid] = set()
                all_connections[guid].update(exec_graph.get(guid, []))
                all_connections[guid].update(data_graph.get(guid, []))

            # Simple BFS-based cluster detection
            visited_global = set()
            clusters = []
            for guid in node_map:
                if guid in visited_global:
                    continue
                cluster = []
                queue = [guid]
                while queue:
                    current = queue.pop(0)
                    if current in visited_global:
                        continue
                    visited_global.add(current)
                    cluster.append(current)
                    # Follow all connections (both directions)
                    for neighbor in all_connections.get(current, []):
                        if neighbor not in visited_global:
                            queue.append(neighbor)
                    # Also check reverse connections
                    for other_guid, conns in all_connections.items():
                        if current in conns and other_guid not in visited_global:
                            queue.append(other_guid)

                if cluster:
                    clusters.append(cluster)

            # Analyze clusters for extraction suggestions
            cluster_analysis = []
            for cluster_guids in clusters:
                if len(cluster_guids) < 3:
                    continue  # Too small to extract

                # Find the "entry" node (event or first exec node)
                start_node = None
                for guid in cluster_guids:
                    node = node_map.get(guid, {})
                    title = node.get("title", "")
                    if "Event" in title or "event" in node.get("class", "").lower():
                        start_node = node
                        break

                if not start_node:
                    start_node = node_map.get(cluster_guids[0], {})

                # Suggest a name based on the event/first node
                suggested_name = _suggest_function_name(start_node, cluster_guids, node_map)

                cluster_analysis.append({
                    "start_node": start_node.get("guid", ""),
                    "start_title": start_node.get("title", "Unknown"),
                    "node_count": len(cluster_guids),
                    "suggested_name": suggested_name,
                    "node_guids": cluster_guids
                })

            # Build suggestion text
            suggestion = ""
            if node_count > 20:
                suggestion = f"Graph has {node_count} nodes - consider breaking into functions."
            elif node_count > 10:
                suggestion = f"Graph has {node_count} nodes - moderate complexity."
            else:
                suggestion = f"Graph has {node_count} nodes - complexity is manageable."

            if len(clusters) > 2:
                suggestion += f" Found {len(clusters)} disconnected clusters that could be separate functions."

            if max_depth > 8:
                suggestion += f" Exec chain depth is {max_depth} - consider extracting deep chains."

            return {
                "success": True,
                "node_count": node_count,
                "exec_chain_depth": max_depth,
                "disconnected_clusters": len(clusters),
                "suggestion": suggestion,
                "cluster_analysis": cluster_analysis
            }

        except Exception as e:
            return {"success": False, "error": f"Error analyzing graph: {e}"}

    @mcp.tool()
    def extract_to_function(
        ctx: Context,
        blueprint_name: str,
        source_graph: str,
        node_guids: List[str],
        function_name: str
    ) -> Dict[str, Any]:
        """
        Extract selected nodes from a graph into a new function.
        Creates the function graph, copies nodes, detects boundary pins for function I/O,
        and replaces the original nodes with a function call.

        This is a complex multi-step operation performed atomically.

        Args:
            blueprint_name: Name of the Blueprint.
            source_graph: Source graph name (e.g., "EventGraph").
            node_guids: List of node GUIDs to extract.
            function_name: Name for the new function.

        Returns:
            Details of the created function and call node.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            # Step 1: Get graph snapshot to understand connections
            snapshot = unreal.send_command("get_graph_snapshot", {
                "blueprint_name": blueprint_name,
                "graph_name": source_graph
            })

            if not snapshot or snapshot.get("status") == "error":
                return {"success": False, "error": "Failed to get graph snapshot"}

            result_data = snapshot.get("result", snapshot)
            nodes = result_data.get("nodes", [])

            # Build node map
            node_map = {n.get("guid", ""): n for n in nodes}
            selected_set = set(node_guids)

            # Find boundary pins (connections that cross the selection boundary)
            function_inputs = []  # Pins on selected nodes that receive data from outside
            function_outputs = []  # Pins on selected nodes that send data to outside
            has_exec_input = False
            has_exec_output = False

            for guid in node_guids:
                node = node_map.get(guid, {})
                for pin in node.get("pins", []):
                    for conn in pin.get("connected_to", []):
                        conn_guid = conn.split(":")[0]
                        conn_pin = conn.split(":")[1] if ":" in conn else ""

                        if conn_guid not in selected_set:
                            # This connection crosses the boundary
                            if pin.get("direction") == "input":
                                if pin.get("type") == "exec":
                                    has_exec_input = True
                                else:
                                    function_inputs.append({
                                        "name": pin.get("name", "Input"),
                                        "type": pin.get("type", "object"),
                                        "from_node": conn_guid,
                                        "from_pin": conn_pin,
                                        "to_node": guid,
                                        "to_pin": pin.get("name", "")
                                    })
                            elif pin.get("direction") == "output":
                                if pin.get("type") == "exec":
                                    has_exec_output = True
                                else:
                                    function_outputs.append({
                                        "name": pin.get("name", "Output"),
                                        "type": pin.get("type", "object"),
                                        "from_node": guid,
                                        "from_pin": pin.get("name", ""),
                                        "to_node": conn_guid,
                                        "to_pin": conn_pin
                                    })

            # Step 2: Create function graph with detected I/O
            inputs_spec = [{"name": inp["name"], "type": _map_pin_type(inp["type"])}
                          for inp in function_inputs]
            outputs_spec = [{"name": out["name"], "type": _map_pin_type(out["type"])}
                           for out in function_outputs]

            operations = []

            # Create the function graph
            create_func_op = {
                "op": "create_function_graph",
                "function_name": function_name
            }
            if inputs_spec:
                create_func_op["inputs"] = inputs_spec
            if outputs_spec:
                create_func_op["outputs"] = outputs_spec
            operations.append(create_func_op)

            # Copy nodes to the new function graph
            operations.append({
                "op": "copy_nodes_to_graph",
                "source_graph": source_graph,
                "target_graph": function_name,
                "node_guids": node_guids
            })

            # Delete original nodes from source graph
            for guid in node_guids:
                operations.append({
                    "op": "delete_node",
                    "graph_name": source_graph,
                    "node_id": guid
                })

            # Place a function call node in the source graph
            operations.append({
                "op": "add_blueprint_function_node",
                "function_name": function_name,
                "graph_name": source_graph
            })

            # Auto-layout the new function
            response = unreal.send_command("execute_blueprint_batch", {
                "blueprint_name": blueprint_name,
                "auto_compile": True,
                "auto_layout": function_name,
                "operations": operations
            })

            if not response:
                return {"success": False, "error": "No response from Unreal Engine"}

            return {
                "success": True,
                "function_name": function_name,
                "inputs_detected": len(function_inputs),
                "outputs_detected": len(function_outputs),
                "nodes_extracted": len(node_guids),
                "batch_result": response
            }

        except Exception as e:
            return {"success": False, "error": f"Error extracting to function: {e}"}


def _suggest_function_name(start_node: Dict, cluster_guids: List[str],
                           node_map: Dict) -> str:
    """Suggest a function name based on the nodes in a cluster."""
    title = start_node.get("title", "")

    if "BeginPlay" in title:
        return "InitializeComponents"
    elif "Tick" in title:
        return "UpdateLogic"
    elif "InputAction" in title:
        action = title.replace("InputAction ", "").replace(" ", "")
        return f"Handle{action}"
    elif "Overlap" in title:
        return "HandleOverlap"
    elif "Hit" in title:
        return "HandleHit"
    else:
        # Use the most common function call in the cluster
        func_counts = {}
        for guid in cluster_guids:
            node = node_map.get(guid, {})
            if "CallFunction" in node.get("class", ""):
                name = node.get("title", "Unknown")
                func_counts[name] = func_counts.get(name, 0) + 1

        if func_counts:
            most_common = max(func_counts, key=func_counts.get)
            return f"Do{most_common.replace(' ', '')}"

    return f"ProcessCluster_{len(cluster_guids)}Nodes"


def _map_pin_type(pin_type: str) -> str:
    """Map a pin type string to a variable type name."""
    type_map = {
        "bool": "Boolean",
        "int": "Integer",
        "int64": "Integer64",
        "real": "Float",
        "real/double": "Float",
        "float": "Float",
        "double": "Float",
        "string": "String",
        "text": "Text",
        "name": "Name",
        "struct": "Struct",
        "object": "Object",
        "class": "Class",
        "exec": "Exec",
        "byte": "Byte",
    }
    return type_map.get(pin_type.lower(), "Object")
