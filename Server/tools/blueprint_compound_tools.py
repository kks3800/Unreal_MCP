"""
Blueprint Compound Tools for Unreal MCP.

These tools combine multiple MCP operations into single calls,
reducing round-trips and context window usage.
They use execute_blueprint_batch internally for atomic operations.
"""

import logging
from typing import Dict, List, Any, Optional
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("UnrealMCP")


def register_blueprint_compound_tools(mcp: FastMCP):
    """Register compound Blueprint tools with the MCP server."""

    @mcp.tool()
    def get_blueprint_snapshot(
        ctx: Context,
        blueprint_name: str,
        detail_level: str = "full"
    ) -> Dict[str, Any]:
        """
        Get a snapshot of a Blueprint in a single call.
        Combines: get_blueprint_info + get_blueprint_variables + get_blueprint_graphs + get_compile_status.

        TIERED OUTPUT (detail_level):
            - "minimal":  name, parent_class, compile_status, + counts only (variable_count,
                          component_count, graph_count). Smallest payload — use to check
                          "does this blueprint exist / is it compiled" without pulling the
                          whole structure.
            - "standard": Adds variable/component/graph lists with NAMES + TYPES only.
                          No default values, no exposure flags, no per-graph node counts.
                          Roughly 40-60% smaller than full. Recommended default for
                          large blueprints (50+ variables/components).
            - "full" (DEFAULT): Everything — variables with defaults/exposure/category,
                          components with parent, graphs with node counts.

        BACKWARD COMPAT: default is "full" to preserve behavior for existing callers
        (blueprint_intelligence relies on full variable/component detail). Pass
        "standard" or "minimal" explicitly for token-efficient inspection.

        Unknown detail_level values fall back to "full" with a warning field in the
        response (data.detail_level_warning).

        Args:
            blueprint_name: Name of the Blueprint to inspect.
            detail_level: "minimal" | "standard" | "full" (default: "full").

        Returns:
            Blueprint snapshot. Shape varies by detail_level (see above).
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_blueprint_snapshot", {
                "blueprint_name": blueprint_name,
                "detail_level": detail_level
            })

            if not response:
                return {"success": False, "error": "No response from Unreal Engine"}

            return response

        except Exception as e:
            return {"success": False, "error": f"Error getting blueprint snapshot: {e}"}

    @mcp.tool()
    def get_graph_snapshot(
        ctx: Context,
        blueprint_name: str,
        graph_name: str = "EventGraph",
        detail_level: str = "full"
    ) -> Dict[str, Any]:
        """
        Get a snapshot of a Blueprint graph's nodes with tiered detail.
        Combines: get_graph_nodes + get_blueprint_connections + get_unconnected_pins.

        TIERED OUTPUT (detail_level) — rough per-node token cost:
            - "minimal"  (~50 tok/node):  {guid, title, class} only. No pins, no positions,
                          no connections. A 100-node graph ~5k tokens. Use for
                          "what nodes exist?" / identity-only scans.
            - "standard" (~150 tok/node): Adds pos_x/pos_y and pin names + directions
                          (input/output). NO pin types, NO connected_to lists, NO
                          default values. A 100-node graph ~15k tokens. Use for
                          layout inspection or pin-name discovery without the wire
                          topology.
            - "full"    (~400 tok/node, DEFAULT): Everything — pin types, connected_to
                          arrays ("NodeGUID:PinName"), and all metadata needed to
                          reconstruct wiring. A 100-node graph ~40k tokens. Use when
                          you need to trace execution or understand connections.

        BACKWARD COMPAT: default is "full" to preserve existing callers
        (blueprint_intelligence's graph analysis requires connected_to + pin types).
        Pass "standard" or "minimal" explicitly for token-efficient inspection of
        large graphs.

        Unknown detail_level values fall back to "full" with a warning field in the
        response (data.detail_level_warning).

        Args:
            blueprint_name: Name of the Blueprint.
            graph_name: Name of the graph (default: "EventGraph").
            detail_level: "minimal" | "standard" | "full" (default: "full").

        Returns:
            Graph snapshot. Top-level envelope unchanged: {success, data: {graph_name,
            node_count, detail_level, nodes: [...]}}. Per-node shape varies by tier.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_graph_snapshot", {
                "blueprint_name": blueprint_name,
                "graph_name": graph_name,
                "detail_level": detail_level
            })

            if not response:
                return {"success": False, "error": "No response from Unreal Engine"}

            return response

        except Exception as e:
            return {"success": False, "error": f"Error getting graph snapshot: {e}"}

    @mcp.tool()
    def create_variable_full(
        ctx: Context,
        blueprint_name: str,
        variable_name: str,
        variable_type: str,
        default_value: str = "",
        is_exposed: bool = True,
        category: str = "",
        tooltip: str = "",
        place_getter_in: str = "",
        place_setter_in: str = "",
        getter_position: Optional[List[int]] = None,
        setter_position: Optional[List[int]] = None,
        connect_getter_to: Optional[Dict[str, str]] = None
    ) -> Dict[str, Any]:
        """
        Create a Blueprint variable with full configuration in a single call.
        Optionally places getter/setter nodes and connects them.
        Replaces 3-6 separate MCP calls with 1.

        Args:
            blueprint_name: Name of the target Blueprint.
            variable_name: Name of the variable.
            variable_type: Type (Boolean, Integer, Float, String, Vector, Rotator, Transform, Object, etc.)
            default_value: Default value as string (e.g., "100.0", "true", "(X=1,Y=2,Z=3)").
            is_exposed: Instance-editable (default: True).
            category: Variable category for organization.
            tooltip: Tooltip text for the variable.
            place_getter_in: Graph name to place a getter node (empty = don't place).
            place_setter_in: Graph name to place a setter node (empty = don't place).
            getter_position: [X, Y] position for getter node.
            setter_position: [X, Y] position for setter node.
            connect_getter_to: {"node_id": "GUID", "pin_name": "PinName"} to connect getter output.

        Returns:
            Results of all operations including variable creation and node placement.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            operations = []

            # Op 0: Create the variable
            operations.append({
                "op": "add_blueprint_variable_extended",
                "variable_name": variable_name,
                "variable_type": variable_type,
                "is_exposed": is_exposed,
                "category": category
            })

            # Op 1: Set default value (if provided)
            if default_value:
                operations.append({
                    "op": "set_variable_default_value",
                    "variable_name": variable_name,
                    "default_value": default_value
                })

            # Op 2: Set tooltip metadata (if provided)
            if tooltip:
                operations.append({
                    "op": "set_variable_metadata",
                    "variable_name": variable_name,
                    "metadata_key": "tooltip",
                    "metadata_value": tooltip
                })

            # Op N: Place getter node (if requested)
            getter_op_index = None
            if place_getter_in:
                getter_op_index = len(operations)
                getter_op = {
                    "op": "add_variable_get_node",
                    "variable_name": variable_name,
                    "graph_name": place_getter_in
                }
                if getter_position:
                    getter_op["node_position"] = getter_position
                operations.append(getter_op)

            # Op N+1: Place setter node (if requested)
            if place_setter_in:
                setter_op = {
                    "op": "add_variable_set_node",
                    "variable_name": variable_name,
                    "graph_name": place_setter_in
                }
                if setter_position:
                    setter_op["node_position"] = setter_position
                operations.append(setter_op)

            # Op N+2: Connect getter to target (if requested)
            if connect_getter_to and getter_op_index is not None:
                operations.append({
                    "op": "connect_blueprint_nodes",
                    "source_node_id": f"${getter_op_index}.node_id",
                    "source_pin": variable_name,
                    "target_node_id": connect_getter_to["node_id"],
                    "target_pin": connect_getter_to["pin_name"]
                })

            response = unreal.send_command("execute_blueprint_batch", {
                "blueprint_name": blueprint_name,
                "auto_compile": True,
                "operations": operations
            })

            if not response:
                return {"success": False, "error": "No response from Unreal Engine"}

            return response

        except Exception as e:
            return {"success": False, "error": f"Error creating variable: {e}"}

    @mcp.tool()
    def create_function_skeleton(
        ctx: Context,
        blueprint_name: str,
        function_name: str,
        inputs: Optional[List[Dict[str, str]]] = None,
        outputs: Optional[List[Dict[str, str]]] = None,
        is_pure: bool = False,
        access: str = "Public",
        initial_nodes: Optional[List[Dict[str, Any]]] = None
    ) -> Dict[str, Any]:
        """
        Create a function graph with entry/result nodes and optional initial nodes.
        Replaces 3-8 separate calls with 1.

        Args:
            blueprint_name: Name of the target Blueprint.
            function_name: Name of the function to create.
            inputs: List of {"name": "ParamName", "type": "Float"} for function inputs.
            outputs: List of {"name": "ReturnName", "type": "Boolean"} for function outputs.
            is_pure: If True, function has no exec pins.
            access: "Public", "Protected", or "Private".
            initial_nodes: Optional list of nodes to place in the function graph.

        Returns:
            Function graph details and any placed nodes.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            operations = []

            # Op 0: Create the function graph
            create_op = {
                "op": "create_function_graph",
                "function_name": function_name
            }
            if inputs:
                create_op["inputs"] = inputs
            if outputs:
                create_op["outputs"] = outputs
            operations.append(create_op)

            # Op 1: Set access specifier if not Public
            if access != "Public":
                operations.append({
                    "op": "set_function_access_specifier",
                    "function_name": function_name,
                    "access": access
                })

            # Op 2: Set pure flag if needed
            if is_pure:
                operations.append({
                    "op": "set_function_flags",
                    "function_name": function_name,
                    "is_pure": True
                })

            # Op 3+: Place initial nodes
            if initial_nodes:
                for node_def in initial_nodes:
                    node_op = dict(node_def)
                    node_op["graph_name"] = function_name
                    operations.append(node_op)

            response = unreal.send_command("execute_blueprint_batch", {
                "blueprint_name": blueprint_name,
                "auto_compile": True,
                "auto_layout": function_name,
                "operations": operations
            })

            if not response:
                return {"success": False, "error": "No response from Unreal Engine"}

            return response

        except Exception as e:
            return {"success": False, "error": f"Error creating function: {e}"}

    @mcp.tool()
    def batch_create_nodes(
        ctx: Context,
        blueprint_name: str,
        graph_name: str,
        nodes: List[Dict[str, Any]],
        auto_compile: bool = True
    ) -> Dict[str, Any]:
        """
        Create multiple Blueprint nodes in a single call.
        Each node definition maps to an existing MCP command.

        Args:
            blueprint_name: Name of the target Blueprint.
            graph_name: Graph to place nodes in (e.g., "EventGraph").
            nodes: Array of node definitions. Each must have an "op" field:
                   [
                     {"op": "add_variable_get_node", "variable_name": "Health"},
                     {"op": "add_blueprint_function_node", "function_name": "PrintString", "target": "KismetSystemLibrary"},
                     {"op": "add_branch_node"},
                     {"op": "add_blueprint_event_node", "event_name": "ReceiveBeginPlay"}
                   ]
            auto_compile: Compile after creating all nodes (default: True).

        Returns:
            Array of results with node_id, title, and pins for each created node.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            operations = []
            for node_def in nodes:
                op = dict(node_def)
                # Inject graph_name if not present
                if "graph_name" not in op:
                    op["graph_name"] = graph_name
                operations.append(op)

            response = unreal.send_command("execute_blueprint_batch", {
                "blueprint_name": blueprint_name,
                "auto_compile": auto_compile,
                "operations": operations
            })

            if not response:
                return {"success": False, "error": "No response from Unreal Engine"}

            return response

        except Exception as e:
            return {"success": False, "error": f"Error batch creating nodes: {e}"}

    @mcp.tool()
    def batch_connect_nodes(
        ctx: Context,
        blueprint_name: str,
        graph_name: str,
        connections: List[Dict[str, str]],
        auto_compile: bool = True
    ) -> Dict[str, Any]:
        """
        Connect multiple Blueprint node pins in a single call.

        Args:
            blueprint_name: Name of the target Blueprint.
            graph_name: Graph containing the nodes.
            connections: Array of connection definitions:
                        [
                          {"src": "GUID1", "src_pin": "ReturnValue", "dst": "GUID2", "dst_pin": "Value"},
                          {"src": "GUID1", "src_pin": "Then", "dst": "GUID3", "dst_pin": "execute"}
                        ]
                        You can use $N.node_id references for nodes created in prior batch operations.
            auto_compile: Compile after connecting (default: True).

        Returns:
            Array of per-connection results.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            operations = []
            for conn in connections:
                operations.append({
                    "op": "connect_blueprint_nodes",
                    "source_node_id": conn.get("src", ""),
                    "source_pin": conn.get("src_pin", ""),
                    "target_node_id": conn.get("dst", ""),
                    "target_pin": conn.get("dst_pin", ""),
                    "graph_name": graph_name
                })

            response = unreal.send_command("execute_blueprint_batch", {
                "blueprint_name": blueprint_name,
                "auto_compile": auto_compile,
                "operations": operations
            })

            if not response:
                return {"success": False, "error": "No response from Unreal Engine"}

            return response

        except Exception as e:
            return {"success": False, "error": f"Error batch connecting nodes: {e}"}

    @mcp.tool()
    def begin_blueprint_edit(
        ctx: Context,
        blueprint_name: str
    ) -> Dict[str, Any]:
        """
        Begin batch editing a Blueprint. Defers compilation until end_blueprint_edit is called.
        Use this when you need to make many individual changes and want to compile only once at the end.
        For most cases, prefer execute_blueprint_batch which handles begin/end automatically.

        Args:
            blueprint_name: Name of the Blueprint to edit.

        Returns:
            Success status.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("begin_blueprint_edit", {
                "blueprint_name": blueprint_name
            })

            if not response:
                return {"success": False, "error": "No response from Unreal Engine"}

            return response

        except Exception as e:
            return {"success": False, "error": f"Error beginning blueprint edit: {e}"}

    @mcp.tool()
    def end_blueprint_edit(
        ctx: Context,
        auto_compile: bool = True,
        auto_save: bool = False,
        auto_layout: str = ""
    ) -> Dict[str, Any]:
        """
        End batch editing a Blueprint. Triggers single compile and optional save.

        Args:
            auto_compile: Compile the Blueprint (default: True).
            auto_save: Save the Blueprint asset (default: False).
            auto_layout: Graph name to auto-layout (empty = skip).

        Returns:
            Success status.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            params = {
                "auto_compile": auto_compile,
                "auto_save": auto_save
            }
            if auto_layout:
                params["auto_layout"] = auto_layout

            response = unreal.send_command("end_blueprint_edit", params)

            if not response:
                return {"success": False, "error": "No response from Unreal Engine"}

            return response

        except Exception as e:
            return {"success": False, "error": f"Error ending blueprint edit: {e}"}

    @mcp.tool()
    def execute_blueprint_batch(
        ctx: Context,
        blueprint_name: str,
        operations: List[Dict[str, Any]],
        auto_compile: bool = True,
        auto_save: bool = False,
        auto_layout: str = ""
    ) -> Dict[str, Any]:
        """
        Execute multiple Blueprint operations in a single call with deferred compilation.
        This is the primary batch tool - use it for any multi-step Blueprint editing.

        Operations are executed sequentially. Use $N.field syntax to reference results
        from previous operations (zero-indexed). For example, $0.node_id references
        the node_id from operation 0's result.

        Args:
            blueprint_name: Name of the target Blueprint.
            operations: Array of operations, each with an "op" field:
                       [
                         {"op": "add_blueprint_variable_extended", "variable_name": "Health", "variable_type": "Float"},
                         {"op": "set_variable_default_value", "variable_name": "Health", "default_value": "100.0"},
                         {"op": "add_variable_get_node", "variable_name": "Health", "graph_name": "EventGraph"},
                         {"op": "connect_blueprint_nodes", "source_node_id": "$2.node_id", "source_pin": "Health",
                          "target_node_id": "ABCD1234", "target_pin": "Value"}
                       ]
            auto_compile: Compile after all operations (default: True).
            auto_save: Save the Blueprint after operations (default: False).
            auto_layout: Graph name to auto-layout after operations (empty = skip).

        Returns:
            {success, success_count, fail_count, total_operations, results: [...per-op results...]}
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            params = {
                "blueprint_name": blueprint_name,
                "operations": operations,
                "auto_compile": auto_compile,
                "auto_save": auto_save
            }
            if auto_layout:
                params["auto_layout"] = auto_layout

            response = unreal.send_command("execute_blueprint_batch", params)

            if not response:
                return {"success": False, "error": "No response from Unreal Engine"}

            return response

        except Exception as e:
            return {"success": False, "error": f"Error executing blueprint batch: {e}"}

    @mcp.tool()
    def build_blueprint_graph(
        ctx: Context,
        blueprint_name: str,
        graph_name: str,
        nodes: Dict[str, Dict[str, Any]],
        connections: List,
        pin_defaults: Optional[Dict[str, Dict[str, str]]] = None,
        auto_compile: bool = True,
        auto_save: bool = False,
        auto_layout: str = "",
        open_editor: bool = True,
    ) -> Dict[str, Any]:
        """
        Build a complete Blueprint graph in one declarative call.

        This is the Blueprint equivalent of build_material — specify all nodes and
        connections symbolically, and the system handles search resolution, placement,
        wiring, layout, and compilation in a single operation.

        Args:
            blueprint_name: Name or path of the target Blueprint.
            graph_name: Target graph name ("EventGraph", "UserConstructionScript",
                       or a custom function name).
            nodes: Dict mapping symbolic names to node specs. Each spec has:
                   - "type": Node type. Options:
                     * Direct types: "Branch", "Sequence", "ForEachLoop", "WhileLoop",
                       "Gate", "DoOnce", "FlipFlop", "Delay", "Timeline",
                       "SwitchOnInt", "SwitchOnString", "SwitchOnEnum",
                       "MakeArray", "MakeMap", "MakeSet", "SpawnActor",
                       "ConstructObject", "FormatText", "Select", "Comment", "Reroute"
                     * "Function" — needs "target" and "function_name"
                     * "VariableGet" — needs "variable_name"
                     * "VariableSet" — needs "variable_name"
                     * "Event" — needs "event_name" (e.g. "ReceiveBeginPlay")
                     * "CustomEvent" — needs "event_name"
                     * "ComponentRef" — needs "component_name"
                     * "SelfRef" — self reference
                     * "CastTo" — needs "class_name"
                     * "SearchAction" — needs "search" keyword, optional "class_filter",
                       "search_index" (default 0)
                     * Any other string — auto-searched in the action database
                   - "position": Optional [x, y] override (auto-layout if omitted)
                   - Other fields depend on type (see above)

            connections: List of connections. Each is either:
                        - ["NodeA.PinName", "NodeB.PinName"] (dot notation)
                        - ["NodeA", "PinA", "NodeB", "PinB"] (explicit)

            pin_defaults: Optional dict of {node_name: {pin_name: "value"}}.
                         Sets default values on pins (e.g. for literals, enums).

            auto_compile: Compile the Blueprint after building (default True).
            auto_save: Save the Blueprint after building (default False).
            auto_layout: Graph name to auto-layout (empty = skip).
            open_editor: Open the Blueprint editor for live viewing (default True).

        Returns:
            Dict with success, node count, connection count, and any errors.

        Example:
            build_blueprint_graph(
                blueprint_name="BP_Example",
                graph_name="EventGraph",
                nodes={
                    "BeginPlay": {"type": "Event", "event_name": "ReceiveBeginPlay"},
                    "PrintHello": {"type": "SearchAction", "search": "Print String"},
                },
                connections=[
                    ["BeginPlay.then", "PrintHello.execute"],
                ],
                pin_defaults={
                    "PrintHello": {"InString": "Hello World!"},
                },
            )
        """
        from unreal_mcp_server import get_unreal_connection
        from tools.blueprint_graph_builder import BlueprintGraphBuilder

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            # Open the Blueprint editor for live viewing
            if open_editor:
                unreal.send_command("open_blueprint", {
                    "blueprint_name": blueprint_name
                })

            # Phase 10 idempotency: fetch a snapshot of the target graph so the
            # builder can skip add-ops for nodes that already exist by signature
            # (Event/VariableGet/VariableSet/Function/SearchAction). If the
            # snapshot call fails (graph doesn't exist yet, etc.), we silently
            # fall back to non-idempotent behavior — the build still works, it
            # just won't dedupe.
            snapshot_result = None
            try:
                snapshot_response = unreal.send_command("get_graph_snapshot", {
                    "blueprint_name": blueprint_name,
                    "graph_name": graph_name,
                    "detail_level": "minimal",
                })
                if (
                    isinstance(snapshot_response, dict)
                    and snapshot_response.get("status") != "error"
                ):
                    snapshot_result = snapshot_response
            except Exception as snap_err:
                logger.info(
                    f"Graph snapshot skipped (continuing without dedup): {snap_err}"
                )
                snapshot_result = None

            # Build the operation list
            builder = BlueprintGraphBuilder()
            builder.load_spec(
                nodes=nodes,
                connections=connections,
                pin_defaults=pin_defaults,
                blueprint_name=blueprint_name,
                graph_name=graph_name,
            )
            if snapshot_result is not None:
                builder.apply_graph_snapshot(snapshot_result)
            ops = builder.generate_ops()

            if not ops and builder.reused_count == 0:
                return {"success": False, "error": "No operations generated from spec"}

            # Idempotent fast path: every spec node already exists AND there
            # are no pin_defaults / new connections to emit. Nothing to do.
            if not ops:
                response = {
                    "success": True,
                    "success_count": 0,
                    "fail_count": 0,
                    "total_operations": 0,
                    "results": [],
                    "nodes_specified": len(nodes),
                    "connections_specified": len(connections),
                    "ops_generated": 0,
                    "reused_count": builder.reused_count,
                    "message": "All spec nodes already present; no changes needed.",
                }
            else:
                # Execute as a single batch
                params = {
                    "blueprint_name": blueprint_name,
                    "operations": ops,
                    "auto_compile": auto_compile,
                    "auto_save": auto_save,
                }
                if auto_layout:
                    params["auto_layout"] = auto_layout

                # Disable batch auto_compile — we'll compile separately for detailed results
                params["auto_compile"] = False

                response = unreal.send_command("execute_blueprint_batch", params)

                if not response:
                    return {"success": False, "error": "No response from Unreal Engine"}

                # Add builder stats to response
                if isinstance(response, dict):
                    result = response.get("result", response)
                    if isinstance(result, dict):
                        result["nodes_specified"] = len(nodes)
                        result["connections_specified"] = len(connections)
                        result["ops_generated"] = len(ops)
                        # Phase 10: surface dedup count to the caller so the LLM
                        # can see that idempotency kicked in.
                        if builder.reused_count > 0:
                            result["reused_count"] = builder.reused_count

            # Compile with detailed results so errors are surfaced
            if auto_compile:
                compile_result = unreal.send_command("compile_blueprint_detailed", {
                    "blueprint_name": blueprint_name
                })
                if isinstance(response, dict):
                    result = response.get("result", response)
                    if isinstance(result, dict) and isinstance(compile_result, dict):
                        cr = compile_result.get("result", compile_result)
                        result["compile_status"] = cr.get("status", "unknown")
                        result["compile_errors"] = cr.get("errors", [])
                        result["compile_warnings"] = cr.get("warnings", [])
                        result["compile_notes"] = cr.get("notes", [])
                        error_count = cr.get("error_count", 0)
                        warning_count = cr.get("warning_count", 0)
                        if error_count > 0:
                            result["compile_summary"] = (
                                f"COMPILE FAILED: {error_count} error(s), "
                                f"{warning_count} warning(s). See compile_errors for details."
                            )
                        elif warning_count > 0:
                            result["compile_summary"] = (
                                f"Compiled with {warning_count} warning(s). "
                                f"See compile_warnings for details."
                            )
                        else:
                            result["compile_summary"] = "Compiled successfully."

            return response

        except Exception as e:
            logger.error(f"Error in build_blueprint_graph: {e}")
            return {"success": False, "error": str(e)}
