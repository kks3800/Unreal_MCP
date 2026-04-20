"""
Blueprint Multi-Graph Authoring Tools for Unreal MCP (Phase 8).

This module provides full-surface Blueprint authoring beyond the EventGraph:
  - Full-spec function graph creation with pure/const/call_in_editor/replication
  - Macro graph creation with tunnel pin population (inputs and outputs)
  - Event dispatcher authoring with signature parameter support
  - Interface implementation (generates implementation graphs automatically)
  - Local variables with rich type support (Object, Class, Interface, Struct)
  - Delegate node spawning (Bind / Call / Remove)

Type strings supported by these tools:
  Primitives:
    Boolean / Bool, Integer / Int / Int32, Int64, Byte,
    Float, Double, String, Name, Text
  Structs (shortcuts):
    Vector, Vector2D, Vector4, Rotator, Transform, Color, LinearColor
  Prefixed forms:
    "Object:ClassName"    hard object reference to ClassName
    "Class:ClassName"     TSubclassOf<ClassName>
    "Interface:ClassName" interface reference
    "Struct:StructName"   explicit script-struct
  Unprefixed names fall back to class-scan then struct-scan.

Asset-registry paths (e.g. /Script/Engine.Actor, /Game/Path/MyBPI.MyBPI_C)
are accepted wherever a class or interface is expected.
"""

import logging
from typing import Dict, List, Any, Optional
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("UnrealMCP")


def register_blueprint_multigraph_tools(mcp: FastMCP):
    """Register Phase 8 Blueprint multi-graph authoring tools."""

    @mcp.tool()
    def create_function_graph_ex(
        ctx: Context,
        blueprint_name: str,
        function_name: str,
        inputs: Optional[List[Dict[str, Any]]] = None,
        outputs: Optional[List[Dict[str, str]]] = None,
        pure: bool = False,
        const: bool = False,
        call_in_editor: bool = False,
        replication: str = "none",
    ) -> Dict[str, Any]:
        """
        Create a function graph with the full flag surface.

        Args:
            blueprint_name: Target Blueprint name.
            function_name: New function name.
            inputs: Input params, each {"name", "type", "default_value" (optional)}.
            outputs: Output params, each {"name", "type"}. A Result node is
                created only when outputs are present.
            pure: If True, function has no exec pins and no side effects.
            const: If True, function is marked FUNC_Const.
            call_in_editor: If True, function can be called from the details panel.
            replication: "none" | "multicast" | "reliable_multicast" | "server" | "client".

        Returns:
            graph_name, entry_node_guid, result_node_guid (if outputs given),
            applied flag echoes, and a message.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            params: Dict[str, Any] = {
                "blueprint_name": blueprint_name,
                "function_name": function_name,
                "pure": pure,
                "const": const,
                "call_in_editor": call_in_editor,
                "replication": replication,
            }
            if inputs is not None:
                params["inputs"] = inputs
            if outputs is not None:
                params["outputs"] = outputs

            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            logger.info(
                "create_function_graph_ex: blueprint=%s function=%s pure=%s const=%s",
                blueprint_name, function_name, pure, const,
            )
            response = unreal.send_command("create_function_graph_ex", params)
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response
        except Exception as exc:
            logger.error("create_function_graph_ex failed: %s", exc)
            return {"success": False, "message": str(exc)}

    @mcp.tool()
    def create_macro_graph_ex(
        ctx: Context,
        blueprint_name: str,
        macro_name: str,
        inputs: Optional[List[Dict[str, str]]] = None,
        outputs: Optional[List[Dict[str, str]]] = None,
    ) -> Dict[str, Any]:
        """
        Create a macro graph and populate its entry/exit tunnels with the
        requested inputs and outputs.

        Args:
            blueprint_name: Target Blueprint name.
            macro_name: New macro name.
            inputs: Input pins on the macro's entry tunnel, each {"name", "type"}.
            outputs: Output pins on the macro's exit tunnel, each {"name", "type"}.

        Returns:
            graph_name, entry_tunnel_guid, exit_tunnel_guid, input_count,
            output_count, message.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            params: Dict[str, Any] = {
                "blueprint_name": blueprint_name,
                "macro_name": macro_name,
            }
            if inputs is not None:
                params["inputs"] = inputs
            if outputs is not None:
                params["outputs"] = outputs

            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            logger.info("create_macro_graph_ex: blueprint=%s macro=%s", blueprint_name, macro_name)
            response = unreal.send_command("create_macro_graph_ex", params)
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response
        except Exception as exc:
            logger.error("create_macro_graph_ex failed: %s", exc)
            return {"success": False, "message": str(exc)}

    @mcp.tool()
    def create_event_dispatcher(
        ctx: Context,
        blueprint_name: str,
        dispatcher_name: str,
        signature_params: Optional[List[Dict[str, str]]] = None,
    ) -> Dict[str, Any]:
        """
        Create an event dispatcher (BlueprintAssignable multicast delegate)
        with an optional signature.

        Adds a PC_MCDelegate member variable, builds its signature graph with
        function terminators, and populates the entry node with the requested
        params. Regenerates the Blueprint skeleton so downstream Bind / Call /
        Remove delegate nodes can resolve the new property without a full
        compile.

        Args:
            blueprint_name: Target Blueprint name.
            dispatcher_name: New dispatcher variable name.
            signature_params: Optional list of {"name", "type"} entries the
                dispatcher will pass to its listeners.

        Returns:
            dispatcher_name, parameter_count, signature_entry_guid, message.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            params: Dict[str, Any] = {
                "blueprint_name": blueprint_name,
                "dispatcher_name": dispatcher_name,
            }
            if signature_params is not None:
                params["signature_params"] = signature_params

            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            logger.info(
                "create_event_dispatcher: blueprint=%s dispatcher=%s params=%d",
                blueprint_name, dispatcher_name, len(signature_params or []),
            )
            response = unreal.send_command("create_event_dispatcher", params)
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response
        except Exception as exc:
            logger.error("create_event_dispatcher failed: %s", exc)
            return {"success": False, "message": str(exc)}

    @mcp.tool()
    def implement_interface(
        ctx: Context,
        blueprint_name: str,
        interface_path: str,
    ) -> Dict[str, Any]:
        """
        Implement a Blueprint interface on the target Blueprint.

        Accepts a full asset path ("/Script/Engine.MyInterface",
        "/Game/Path/MyBPI.MyBPI_C") or a short class name ("MyInterface").
        Short names are resolved against loaded UClasses and the asset
        registry for Blueprint-defined interfaces. Functions that
        CanKismetOverrideFunction && !FunctionCanBePlacedAsEvent get fresh
        implementation graphs; event-style interface methods remain in the
        EventGraph palette.

        Args:
            blueprint_name: Target Blueprint name.
            interface_path: Full path or short name of the interface.

        Returns:
            interface_path, interfaces_added, graphs_added, new_graph_names, message.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            params = {
                "blueprint_name": blueprint_name,
                "interface_path": interface_path,
            }
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            logger.info("implement_interface: blueprint=%s interface=%s", blueprint_name, interface_path)
            response = unreal.send_command("implement_interface", params)
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response
        except Exception as exc:
            logger.error("implement_interface failed: %s", exc)
            return {"success": False, "message": str(exc)}

    @mcp.tool()
    def add_local_variable_ex(
        ctx: Context,
        blueprint_name: str,
        function_graph_name: str,
        variable_name: str,
        variable_type: str,
        default_value: str = "",
    ) -> Dict[str, Any]:
        """
        Add a local variable with full type support to a function graph.

        Unlike the legacy add_local_variable, this accepts rich type strings
        including Object/Class/Interface/Struct prefixed forms. Only works on
        function graphs; errors clearly if called on a macro graph.

        Args:
            blueprint_name: Target Blueprint name.
            function_graph_name: Name of the owning function graph.
            variable_name: New local variable name.
            variable_type: Type string — see module docstring for supported forms.
            default_value: Optional default literal.

        Returns:
            variable_name, variable_type, function_graph_name, message.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            params: Dict[str, Any] = {
                "blueprint_name": blueprint_name,
                "function_graph_name": function_graph_name,
                "variable_name": variable_name,
                "variable_type": variable_type,
            }
            if default_value:
                params["default_value"] = default_value

            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            logger.info(
                "add_local_variable_ex: blueprint=%s function=%s var=%s type=%s",
                blueprint_name, function_graph_name, variable_name, variable_type,
            )
            response = unreal.send_command("add_local_variable_ex", params)
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response
        except Exception as exc:
            logger.error("add_local_variable_ex failed: %s", exc)
            return {"success": False, "message": str(exc)}

    @mcp.tool()
    def add_bind_delegate_node(
        ctx: Context,
        blueprint_name: str,
        dispatcher_name: str,
        graph_name: Optional[str] = None,
        position: Optional[List[float]] = None,
    ) -> Dict[str, Any]:
        """
        Spawn a Bind Event (UK2Node_AddDelegate) node wired to the named
        dispatcher on the self context.

        Args:
            blueprint_name: Target Blueprint name.
            dispatcher_name: Multicast delegate variable on the Blueprint.
            graph_name: Optional graph to spawn into; defaults to EventGraph.
            position: Optional [x, y] node position.

        Returns:
            NodeToCompactJson payload plus dispatcher_resolved flag.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            params: Dict[str, Any] = {
                "blueprint_name": blueprint_name,
                "dispatcher_name": dispatcher_name,
            }
            if graph_name:
                params["graph_name"] = graph_name
            if position:
                params["position"] = position

            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_bind_delegate_node", params)
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response
        except Exception as exc:
            logger.error("add_bind_delegate_node failed: %s", exc)
            return {"success": False, "message": str(exc)}

    @mcp.tool()
    def add_call_delegate_node(
        ctx: Context,
        blueprint_name: str,
        dispatcher_name: str,
        graph_name: Optional[str] = None,
        position: Optional[List[float]] = None,
    ) -> Dict[str, Any]:
        """
        Spawn a Call Dispatcher (UK2Node_CallDelegate) node wired to the named
        dispatcher on the self context.

        Args:
            blueprint_name: Target Blueprint name.
            dispatcher_name: Multicast delegate variable on the Blueprint.
            graph_name: Optional graph to spawn into; defaults to EventGraph.
            position: Optional [x, y] node position.

        Returns:
            NodeToCompactJson payload plus dispatcher_resolved flag.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            params: Dict[str, Any] = {
                "blueprint_name": blueprint_name,
                "dispatcher_name": dispatcher_name,
            }
            if graph_name:
                params["graph_name"] = graph_name
            if position:
                params["position"] = position

            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_call_delegate_node", params)
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response
        except Exception as exc:
            logger.error("add_call_delegate_node failed: %s", exc)
            return {"success": False, "message": str(exc)}

    @mcp.tool()
    def add_remove_delegate_node(
        ctx: Context,
        blueprint_name: str,
        dispatcher_name: str,
        graph_name: Optional[str] = None,
        position: Optional[List[float]] = None,
    ) -> Dict[str, Any]:
        """
        Spawn an Unbind Event (UK2Node_RemoveDelegate) node wired to the named
        dispatcher on the self context.

        Args:
            blueprint_name: Target Blueprint name.
            dispatcher_name: Multicast delegate variable on the Blueprint.
            graph_name: Optional graph to spawn into; defaults to EventGraph.
            position: Optional [x, y] node position.

        Returns:
            NodeToCompactJson payload plus dispatcher_resolved flag.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            params: Dict[str, Any] = {
                "blueprint_name": blueprint_name,
                "dispatcher_name": dispatcher_name,
            }
            if graph_name:
                params["graph_name"] = graph_name
            if position:
                params["position"] = position

            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_remove_delegate_node", params)
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response
        except Exception as exc:
            logger.error("add_remove_delegate_node failed: %s", exc)
            return {"success": False, "message": str(exc)}

    logger.info("Blueprint multigraph tools registered successfully")
