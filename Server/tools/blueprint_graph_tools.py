"""
Blueprint Graph Tools for Unreal MCP.

This module provides tools for managing Blueprint graphs (functions, macros, layout).
"""

import logging
from typing import Dict, List, Any, Optional
from mcp.server.fastmcp import FastMCP, Context

# Get logger
logger = logging.getLogger("UnrealMCP")

def register_blueprint_graph_tools(mcp: FastMCP):
    """Register Blueprint graph management tools with the MCP server."""

    @mcp.tool()
    def create_function_graph(
        ctx: Context,
        blueprint_name: str,
        function_name: str,
        inputs: Optional[List[Dict[str, str]]] = None,
        outputs: Optional[List[Dict[str, str]]] = None,
        is_pure: bool = False
    ) -> Dict[str, Any]:
        """
        Create a new function graph in a Blueprint.

        Args:
            blueprint_name: Name of the target Blueprint
            function_name: Name for the new function
            inputs: Optional list of input parameters, each with 'name' and 'type'
                   (e.g. [{"name": "Speed", "type": "Float"}, {"name": "Name", "type": "String"}])
                   Supported types: Boolean, Integer/Int, Float, Double, String, Name, Text, Vector, Rotator, Transform
            outputs: Optional list of output parameters, same format as inputs
            is_pure: If True, creates a pure function (no exec pins)

        Returns:
            Response with graph_name, entry_node_guid, and result_node_guid
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            params = {
                "blueprint_name": blueprint_name,
                "function_name": function_name,
                "is_pure": is_pure
            }
            if inputs is not None:
                params["inputs"] = inputs
            if outputs is not None:
                params["outputs"] = outputs

            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            logger.info(f"Creating function graph '{function_name}' in blueprint '{blueprint_name}'")
            response = unreal.send_command("create_function_graph", params)

            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Create function graph response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error creating function graph: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def delete_function_graph(
        ctx: Context,
        blueprint_name: str,
        function_name: str
    ) -> Dict[str, Any]:
        """
        Delete a function graph from a Blueprint.

        Args:
            blueprint_name: Name of the target Blueprint
            function_name: Name of the function graph to delete

        Returns:
            Response indicating success or failure
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            params = {
                "blueprint_name": blueprint_name,
                "function_name": function_name
            }

            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            logger.info(f"Deleting function graph '{function_name}' from blueprint '{blueprint_name}'")
            response = unreal.send_command("delete_function_graph", params)

            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Delete function graph response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error deleting function graph: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def create_macro_graph(
        ctx: Context,
        blueprint_name: str,
        macro_name: str
    ) -> Dict[str, Any]:
        """
        Create a new macro graph in a Blueprint.

        Args:
            blueprint_name: Name of the target Blueprint
            macro_name: Name for the new macro

        Returns:
            Response with graph_name
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            params = {
                "blueprint_name": blueprint_name,
                "macro_name": macro_name
            }

            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            logger.info(f"Creating macro graph '{macro_name}' in blueprint '{blueprint_name}'")
            response = unreal.send_command("create_macro_graph", params)

            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Create macro graph response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error creating macro graph: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def delete_macro_graph(
        ctx: Context,
        blueprint_name: str,
        macro_name: str
    ) -> Dict[str, Any]:
        """
        Delete a macro graph from a Blueprint.

        Args:
            blueprint_name: Name of the target Blueprint
            macro_name: Name of the macro graph to delete

        Returns:
            Response indicating success or failure
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            params = {
                "blueprint_name": blueprint_name,
                "macro_name": macro_name
            }

            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            logger.info(f"Deleting macro graph '{macro_name}' from blueprint '{blueprint_name}'")
            response = unreal.send_command("delete_macro_graph", params)

            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Delete macro graph response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error deleting macro graph: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def auto_layout_graph(
        ctx: Context,
        blueprint_name: str,
        graph_name: str
    ) -> Dict[str, Any]:
        """
        Auto-layout nodes in a Blueprint graph using column-based BFS arrangement.
        Nodes are arranged left-to-right based on execution flow depth.

        Args:
            blueprint_name: Name of the target Blueprint
            graph_name: Name of the graph to layout (e.g. 'EventGraph', function name, macro name)

        Returns:
            Response with number of nodes positioned
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            params = {
                "blueprint_name": blueprint_name,
                "graph_name": graph_name
            }

            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            logger.info(f"Auto-layout graph '{graph_name}' in blueprint '{blueprint_name}'")
            response = unreal.send_command("auto_layout_graph", params)

            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Auto-layout graph response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error auto-laying out graph: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def set_function_access_specifier(
        ctx: Context,
        blueprint_name: str,
        function_name: str,
        access: str
    ) -> Dict[str, Any]:
        """
        Set the access specifier of a Blueprint function.

        Args:
            blueprint_name: Name of the target Blueprint
            function_name: Name of the function graph
            access: Access level - "Public", "Protected", or "Private"

        Returns:
            Response indicating success or failure
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("set_function_access_specifier", {
                "blueprint_name": blueprint_name,
                "function_name": function_name,
                "access": access
            })

            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}

            return response

        except Exception as e:
            logger.error(f"Error setting function access: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_function_flags(
        ctx: Context,
        blueprint_name: str,
        function_name: str,
        is_pure: Optional[bool] = None,
        is_const: Optional[bool] = None,
        is_static: Optional[bool] = None,
        is_callable_in_editor: Optional[bool] = None
    ) -> Dict[str, Any]:
        """
        Set flags on a Blueprint function (pure, const, static, callable in editor).

        Args:
            blueprint_name: Name of the target Blueprint
            function_name: Name of the function graph
            is_pure: If True, function has no exec pins and no side effects
            is_const: If True, function cannot modify the object
            is_static: If True, function is static
            is_callable_in_editor: If True, function can be called from editor details panel

        Returns:
            Response indicating which flags were set
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "blueprint_name": blueprint_name,
                "function_name": function_name
            }
            if is_pure is not None:
                params["is_pure"] = is_pure
            if is_const is not None:
                params["is_const"] = is_const
            if is_static is not None:
                params["is_static"] = is_static
            if is_callable_in_editor is not None:
                params["is_callable_in_editor"] = is_callable_in_editor

            response = unreal.send_command("set_function_flags", params)

            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}

            return response

        except Exception as e:
            logger.error(f"Error setting function flags: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_local_variable(
        ctx: Context,
        blueprint_name: str,
        function_name: str,
        variable_name: str,
        variable_type: str,
        default_value: Optional[str] = None
    ) -> Dict[str, Any]:
        """
        Add a local variable to a Blueprint function.

        Args:
            blueprint_name: Name of the target Blueprint
            function_name: Name of the function graph
            variable_name: Name for the local variable
            variable_type: Type of the variable (Boolean, Integer, Float, Double, String,
                          Name, Text, Vector, Rotator, Transform)
            default_value: Optional default value as string

        Returns:
            Response with variable_name
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "blueprint_name": blueprint_name,
                "function_name": function_name,
                "variable_name": variable_name,
                "variable_type": variable_type
            }
            if default_value is not None:
                params["default_value"] = default_value

            response = unreal.send_command("add_local_variable", params)

            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}

            return response

        except Exception as e:
            logger.error(f"Error adding local variable: {e}")
            return {"success": False, "message": str(e)}

    logger.info("Blueprint graph tools registered successfully")
