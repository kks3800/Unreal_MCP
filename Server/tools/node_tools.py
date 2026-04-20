"""
Blueprint Node Tools for Unreal MCP.

This module provides tools for manipulating Blueprint graph nodes and connections.
"""

import logging
from typing import Dict, List, Any, Optional
from mcp.server.fastmcp import FastMCP, Context

# Get logger
logger = logging.getLogger("UnrealMCP")

def register_blueprint_node_tools(mcp: FastMCP):
    """Register Blueprint node manipulation tools with the MCP server."""
    
    @mcp.tool()
    def add_blueprint_event_node(
        ctx: Context,
        blueprint_name: str,
        event_name: str,
        node_position = None
    ) -> Dict[str, Any]:
        """
        Add an event node to a Blueprint's event graph.
        
        Args:
            blueprint_name: Name of the target Blueprint
            event_name: Name of the event. Use 'Receive' prefix for standard events:
                       - 'ReceiveBeginPlay' for Begin Play
                       - 'ReceiveTick' for Tick
                       - etc.
            node_position: Optional [X, Y] position in the graph
            
        Returns:
            Response containing the node ID and success status
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            # Handle default value within the method body
            if node_position is None:
                node_position = [0, 0]
            
            params = {
                "blueprint_name": blueprint_name,
                "event_name": event_name,
                "node_position": node_position
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Adding event node '{event_name}' to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_blueprint_event_node", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Event node creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding event node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def add_blueprint_input_action_node(
        ctx: Context,
        blueprint_name: str,
        action_name: str,
        node_position = None
    ) -> Dict[str, Any]:
        """
        Add an input action event node to a Blueprint's event graph.
        
        Args:
            blueprint_name: Name of the target Blueprint
            action_name: Name of the input action to respond to
            node_position: Optional [X, Y] position in the graph
            
        Returns:
            Response containing the node ID and success status
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            # Handle default value within the method body
            if node_position is None:
                node_position = [0, 0]
            
            params = {
                "blueprint_name": blueprint_name,
                "action_name": action_name,
                "node_position": node_position
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Adding input action node for '{action_name}' to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_blueprint_input_action_node", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Input action node creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding input action node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def add_blueprint_function_node(
        ctx: Context,
        blueprint_name: str,
        target: str,
        function_name: str,
        params = None,
        node_position = None
    ) -> Dict[str, Any]:
        """
        Add a function call node to a Blueprint's event graph.
        
        Args:
            blueprint_name: Name of the target Blueprint
            target: Target object for the function (component name or self)
            function_name: Name of the function to call
            params: Optional parameters to set on the function node
            node_position: Optional [X, Y] position in the graph
            
        Returns:
            Response containing the node ID and success status
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            # Handle default values within the method body
            if params is None:
                params = {}
            if node_position is None:
                node_position = [0, 0]
            
            command_params = {
                "blueprint_name": blueprint_name,
                "target": target,
                "function_name": function_name,
                "params": params,
                "node_position": node_position
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Adding function node '{function_name}' to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_blueprint_function_node", command_params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Function node creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding function node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
            
    @mcp.tool()
    def connect_blueprint_nodes(
        ctx: Context,
        blueprint_name: str,
        source_node_id: str,
        source_pin: str,
        target_node_id: str,
        target_pin: str,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """
        Connect two nodes in a Blueprint graph.

        Args:
            blueprint_name: Name of the target Blueprint
            source_node_id: ID of the source node
            source_pin: Name of the output pin on the source node
            target_node_id: ID of the target node
            target_pin: Name of the input pin on the target node
            graph_name: Optional graph name (defaults to EventGraph).
                        Use "UserConstructionScript" for Construction Script.

        Returns:
            Response indicating success or failure
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            params = {
                "blueprint_name": blueprint_name,
                "source_node_id": source_node_id,
                "source_pin": source_pin,
                "target_node_id": target_node_id,
                "target_pin": target_pin
            }
            if graph_name:
                params["graph_name"] = graph_name
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Connecting nodes in blueprint '{blueprint_name}'")
            response = unreal.send_command("connect_blueprint_nodes", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Node connection response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error connecting nodes: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def add_blueprint_variable(
        ctx: Context,
        blueprint_name: str,
        variable_name: str,
        variable_type: str,
        is_exposed: bool = True
    ) -> Dict[str, Any]:
        """
        Simplified wrapper around add_blueprint_variable_extended.

        Adds a variable to a Blueprint using the extended variable command internally.
        For full control (containers, replication, categories), use add_blueprint_variable_extended directly.

        Args:
            blueprint_name: Name of the target Blueprint
            variable_name: Name of the variable
            variable_type: Type of the variable (Boolean, Integer, Float, Vector, etc.)
            is_exposed: Whether to expose the variable to the editor (default True)

        Returns:
            Response indicating success or failure
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            params = {
                "blueprint_name": blueprint_name,
                "variable_name": variable_name,
                "variable_type": variable_type,
                "is_exposed": is_exposed
            }

            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            logger.info(f"Adding variable '{variable_name}' to blueprint '{blueprint_name}' (via extended)")
            response = unreal.send_command("add_blueprint_variable_extended", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Variable creation response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error adding variable: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def add_blueprint_get_self_component_reference(
        ctx: Context,
        blueprint_name: str,
        component_name: str,
        node_position = None
    ) -> Dict[str, Any]:
        """
        Add a node that gets a reference to a component owned by the current Blueprint.
        This creates a node similar to what you get when dragging a component from the Components panel.
        
        Args:
            blueprint_name: Name of the target Blueprint
            component_name: Name of the component to get a reference to
            node_position: Optional [X, Y] position in the graph
            
        Returns:
            Response containing the node ID and success status
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            # Handle None case explicitly in the function
            if node_position is None:
                node_position = [0, 0]
            
            params = {
                "blueprint_name": blueprint_name,
                "component_name": component_name,
                "node_position": node_position
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Adding self component reference node for '{component_name}' to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_blueprint_get_self_component_reference", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Self component reference node creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding self component reference node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def add_blueprint_self_reference(
        ctx: Context,
        blueprint_name: str,
        node_position = None
    ) -> Dict[str, Any]:
        """
        Add a 'Get Self' node to a Blueprint's event graph that returns a reference to this actor.
        
        Args:
            blueprint_name: Name of the target Blueprint
            node_position: Optional [X, Y] position in the graph
            
        Returns:
            Response containing the node ID and success status
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            if node_position is None:
                node_position = [0, 0]
                
            params = {
                "blueprint_name": blueprint_name,
                "node_position": node_position
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Adding self reference node to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_blueprint_self_reference", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Self reference node creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding self reference node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def find_blueprint_nodes(
        ctx: Context,
        blueprint_name: str,
        node_type = None,
        event_type = None
    ) -> Dict[str, Any]:
        """
        Find nodes in a Blueprint's event graph.
        
        Args:
            blueprint_name: Name of the target Blueprint
            node_type: Optional type of node to find (Event, Function, Variable, etc.)
            event_type: Optional specific event type to find (BeginPlay, Tick, etc.)
            
        Returns:
            Response containing array of found node IDs and success status
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            params = {
                "blueprint_name": blueprint_name,
                "node_type": node_type,
                "event_type": event_type
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Finding nodes in blueprint '{blueprint_name}'")
            response = unreal.send_command("find_blueprint_nodes", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Node find response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error finding nodes: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def delete_node(
        ctx: Context,
        blueprint_name: str,
        node_guid: str,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """
        Delete a node from a Blueprint graph.

        Args:
            blueprint_name: Name of the target Blueprint
            node_guid: GUID of the node to delete
            graph_name: Optional graph name (defaults to EventGraph)

        Returns:
            Response indicating success or failure
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            params = {
                "blueprint_name": blueprint_name,
                "node_guid": node_guid,
                "graph_name": graph_name
            }

            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            logger.info(f"Deleting node '{node_guid}' from blueprint '{blueprint_name}'")
            response = unreal.send_command("delete_node", params)

            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Delete node response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error deleting node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def set_node_position(
        ctx: Context,
        blueprint_name: str,
        node_guid: str,
        position: list,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """
        Set the position of a node in a Blueprint graph.

        Args:
            blueprint_name: Name of the target Blueprint
            node_guid: GUID of the node to move
            position: [X, Y] position in the graph
            graph_name: Optional graph name (defaults to EventGraph)

        Returns:
            Response with the new position
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            params = {
                "blueprint_name": blueprint_name,
                "node_guid": node_guid,
                "position": position,
                "graph_name": graph_name
            }

            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            logger.info(f"Setting node '{node_guid}' position to {position}")
            response = unreal.send_command("set_node_position", params)

            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Set node position response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error setting node position: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def add_comment_node(
        ctx: Context,
        blueprint_name: str,
        text: str,
        position: list = None,
        graph_name: str = "",
        size: list = None,
        color: list = None
    ) -> Dict[str, Any]:
        """
        Add a comment box node to a Blueprint graph.

        Args:
            blueprint_name: Name of the target Blueprint
            text: Comment text
            position: Optional [X, Y] position in the graph
            graph_name: Optional graph name (defaults to EventGraph)
            size: Optional [width, height] of the comment box
            color: Optional [R, G, B, A] color (0.0-1.0 range)

        Returns:
            Response with node_guid of the created comment
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            if position is None:
                position = [0, 0]

            params = {
                "blueprint_name": blueprint_name,
                "text": text,
                "position": position,
                "graph_name": graph_name
            }
            if size is not None:
                params["size"] = size
            if color is not None:
                params["color"] = color

            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            logger.info(f"Adding comment node to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_comment_node", params)

            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Add comment node response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error adding comment node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def disconnect_pins(
        ctx: Context,
        blueprint_name: str,
        source_node_guid: str,
        source_pin_name: str,
        target_node_guid: str,
        target_pin_name: str,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """
        Disconnect a specific pin link between two nodes.

        Args:
            blueprint_name: Name of the target Blueprint
            source_node_guid: GUID of the source node
            source_pin_name: Name of the output pin on the source node
            target_node_guid: GUID of the target node
            target_pin_name: Name of the input pin on the target node
            graph_name: Optional graph name (defaults to EventGraph)

        Returns:
            Response indicating success or failure
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            params = {
                "blueprint_name": blueprint_name,
                "source_node_guid": source_node_guid,
                "source_pin_name": source_pin_name,
                "target_node_guid": target_node_guid,
                "target_pin_name": target_pin_name,
                "graph_name": graph_name
            }

            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            logger.info(f"Disconnecting pins in blueprint '{blueprint_name}'")
            response = unreal.send_command("disconnect_pins", params)

            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Disconnect pins response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error disconnecting pins: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def disconnect_all_pins(
        ctx: Context,
        blueprint_name: str,
        node_guid: str,
        pin_name: str = "",
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """
        Disconnect all links from a node or a specific pin.

        Args:
            blueprint_name: Name of the target Blueprint
            node_guid: GUID of the node
            pin_name: Optional specific pin name. If empty, disconnects ALL pins on the node
            graph_name: Optional graph name (defaults to EventGraph)

        Returns:
            Response with disconnected_count
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            params = {
                "blueprint_name": blueprint_name,
                "node_guid": node_guid,
                "pin_name": pin_name,
                "graph_name": graph_name
            }

            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            logger.info(f"Disconnecting all pins for node '{node_guid}' in blueprint '{blueprint_name}'")
            response = unreal.send_command("disconnect_all_pins", params)

            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Disconnect all pins response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error disconnecting all pins: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def set_pin_default_value(
        ctx: Context,
        blueprint_name: str,
        node_guid: str,
        pin_name: str,
        value: str,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """
        Set the default value of a pin on a node.

        Args:
            blueprint_name: Name of the target Blueprint
            node_guid: GUID of the node containing the pin
            pin_name: Name of the pin to set the value on
            value: String representation of the value to set.
                   For simple types: "42", "3.14", "true", "Hello"
                   For vectors: "(X=1.0,Y=2.0,Z=3.0)"
                   For object refs: full asset path "/Script/Engine.ClassName"
            graph_name: Optional graph name (defaults to EventGraph)

        Returns:
            Response indicating success or failure
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            params = {
                "blueprint_name": blueprint_name,
                "node_guid": node_guid,
                "pin_name": pin_name,
                "value": value,
                "graph_name": graph_name
            }

            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            logger.info(f"Setting pin '{pin_name}' default value to '{value}' on node '{node_guid}'")
            response = unreal.send_command("set_pin_default_value", params)

            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Set pin default value response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error setting pin default value: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    # =========================================================================
    # Sprint 4: Flow Control Nodes
    # =========================================================================

    @mcp.tool()
    def add_branch_node(
        ctx: Context,
        blueprint_name: str,
        position: list = None,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """
        Add a Branch (If/Then/Else) node to a Blueprint graph.

        Args:
            blueprint_name: Name of the target Blueprint
            position: Optional [X, Y] position in the graph
            graph_name: Optional graph name (defaults to EventGraph)

        Returns:
            Response containing the node_id
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            if position is None:
                position = [0, 0]
            params = {"blueprint_name": blueprint_name, "position": position, "graph_name": graph_name}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("add_branch_node", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def add_sequence_node(
        ctx: Context,
        blueprint_name: str,
        position: list = None,
        graph_name: str = "",
        num_outputs: int = 2
    ) -> Dict[str, Any]:
        """
        Add a Sequence node to a Blueprint graph.

        Args:
            blueprint_name: Name of the target Blueprint
            position: Optional [X, Y] position in the graph
            graph_name: Optional graph name (defaults to EventGraph)
            num_outputs: Number of output execution pins (default 2)

        Returns:
            Response containing the node_id
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            if position is None:
                position = [0, 0]
            params = {"blueprint_name": blueprint_name, "position": position, "graph_name": graph_name, "num_outputs": num_outputs}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("add_sequence_node", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def add_switch_on_int_node(
        ctx: Context,
        blueprint_name: str,
        position: list = None,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """
        Add a Switch on Int node to a Blueprint graph.

        Args:
            blueprint_name: Name of the target Blueprint
            position: Optional [X, Y] position in the graph
            graph_name: Optional graph name (defaults to EventGraph)

        Returns:
            Response containing the node_id
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            if position is None:
                position = [0, 0]
            params = {"blueprint_name": blueprint_name, "position": position, "graph_name": graph_name}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("add_switch_on_int_node", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def add_switch_on_string_node(
        ctx: Context,
        blueprint_name: str,
        position: list = None,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """
        Add a Switch on String node to a Blueprint graph.

        Args:
            blueprint_name: Name of the target Blueprint
            position: Optional [X, Y] position in the graph
            graph_name: Optional graph name (defaults to EventGraph)

        Returns:
            Response containing the node_id
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            if position is None:
                position = [0, 0]
            params = {"blueprint_name": blueprint_name, "position": position, "graph_name": graph_name}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("add_switch_on_string_node", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def add_switch_on_enum_node(
        ctx: Context,
        blueprint_name: str,
        enum_name: str,
        position: list = None,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """
        Add a Switch on Enum node to a Blueprint graph.

        Args:
            blueprint_name: Name of the target Blueprint
            enum_name: Full path or name of the enum type (e.g. '/Script/Engine.ECollisionChannel')
            position: Optional [X, Y] position in the graph
            graph_name: Optional graph name (defaults to EventGraph)

        Returns:
            Response containing the node_id
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            if position is None:
                position = [0, 0]
            params = {"blueprint_name": blueprint_name, "enum_name": enum_name, "position": position, "graph_name": graph_name}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("add_switch_on_enum_node", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def add_for_each_loop_node(
        ctx: Context,
        blueprint_name: str,
        position: list = None,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """
        Add a ForEachLoop macro node to a Blueprint graph.

        Args:
            blueprint_name: Name of the target Blueprint
            position: Optional [X, Y] position in the graph
            graph_name: Optional graph name (defaults to EventGraph)

        Returns:
            Response containing the node_id
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            if position is None:
                position = [0, 0]
            params = {"blueprint_name": blueprint_name, "position": position, "graph_name": graph_name}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("add_for_each_loop_node", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def add_while_loop_node(
        ctx: Context,
        blueprint_name: str,
        position: list = None,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """
        Add a WhileLoop macro node to a Blueprint graph.

        Args:
            blueprint_name: Name of the target Blueprint
            position: Optional [X, Y] position in the graph
            graph_name: Optional graph name (defaults to EventGraph)

        Returns:
            Response containing the node_id
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            if position is None:
                position = [0, 0]
            params = {"blueprint_name": blueprint_name, "position": position, "graph_name": graph_name}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("add_while_loop_node", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def add_gate_node(
        ctx: Context,
        blueprint_name: str,
        position: list = None,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """
        Add a Gate macro node to a Blueprint graph.

        Args:
            blueprint_name: Name of the target Blueprint
            position: Optional [X, Y] position in the graph
            graph_name: Optional graph name (defaults to EventGraph)

        Returns:
            Response containing the node_id
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            if position is None:
                position = [0, 0]
            params = {"blueprint_name": blueprint_name, "position": position, "graph_name": graph_name}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("add_gate_node", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def add_do_once_node(
        ctx: Context,
        blueprint_name: str,
        position: list = None,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """
        Add a DoOnce macro node to a Blueprint graph.

        Args:
            blueprint_name: Name of the target Blueprint
            position: Optional [X, Y] position in the graph
            graph_name: Optional graph name (defaults to EventGraph)

        Returns:
            Response containing the node_id
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            if position is None:
                position = [0, 0]
            params = {"blueprint_name": blueprint_name, "position": position, "graph_name": graph_name}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("add_do_once_node", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def add_flip_flop_node(
        ctx: Context,
        blueprint_name: str,
        position: list = None,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """
        Add a FlipFlop macro node to a Blueprint graph.

        Args:
            blueprint_name: Name of the target Blueprint
            position: Optional [X, Y] position in the graph
            graph_name: Optional graph name (defaults to EventGraph)

        Returns:
            Response containing the node_id
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            if position is None:
                position = [0, 0]
            params = {"blueprint_name": blueprint_name, "position": position, "graph_name": graph_name}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("add_flip_flop_node", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def add_delay_node(
        ctx: Context,
        blueprint_name: str,
        duration: float = 0.2,
        position: list = None,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """
        Add a Delay node to a Blueprint graph.

        Args:
            blueprint_name: Name of the target Blueprint
            duration: Delay duration in seconds (default 0.2)
            position: Optional [X, Y] position in the graph
            graph_name: Optional graph name (defaults to EventGraph)

        Returns:
            Response containing the node_id
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            if position is None:
                position = [0, 0]
            params = {"blueprint_name": blueprint_name, "duration": duration, "position": position, "graph_name": graph_name}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("add_delay_node", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def add_variable_get_node(
        ctx: Context,
        blueprint_name: str,
        variable_name: str,
        position: list = None,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """
        Add a Variable Get node to a Blueprint graph.

        Args:
            blueprint_name: Name of the target Blueprint
            variable_name: Name of the variable to get
            position: Optional [X, Y] position in the graph
            graph_name: Optional graph name (defaults to EventGraph)

        Returns:
            Response containing the node_id
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            if position is None:
                position = [0, 0]
            params = {"blueprint_name": blueprint_name, "variable_name": variable_name, "position": position, "graph_name": graph_name}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("add_variable_get_node", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def add_variable_set_node(
        ctx: Context,
        blueprint_name: str,
        variable_name: str,
        position: list = None,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """
        Add a Variable Set node to a Blueprint graph.

        Args:
            blueprint_name: Name of the target Blueprint
            variable_name: Name of the variable to set
            position: Optional [X, Y] position in the graph
            graph_name: Optional graph name (defaults to EventGraph)

        Returns:
            Response containing the node_id
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            if position is None:
                position = [0, 0]
            params = {"blueprint_name": blueprint_name, "variable_name": variable_name, "position": position, "graph_name": graph_name}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("add_variable_set_node", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def add_custom_event_node(
        ctx: Context,
        blueprint_name: str,
        event_name: str,
        position: list = None,
        graph_name: str = "",
        parameters: list = None
    ) -> Dict[str, Any]:
        """
        Add a Custom Event node to a Blueprint graph.

        Args:
            blueprint_name: Name of the target Blueprint
            event_name: Name of the custom event
            position: Optional [X, Y] position in the graph
            graph_name: Optional graph name (defaults to EventGraph)
            parameters: Optional list of parameter dicts [{"name": "Param1", "type": "Float"}, ...]
                        Supported types: Boolean, Integer, Float, String, Vector, Rotator, Object

        Returns:
            Response containing the node_id
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            if position is None:
                position = [0, 0]
            params = {"blueprint_name": blueprint_name, "event_name": event_name, "position": position, "graph_name": graph_name}
            if parameters is not None:
                params["parameters"] = parameters
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("add_custom_event_node", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def add_cast_node(
        ctx: Context,
        blueprint_name: str,
        target_class: str,
        position: list = None,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """
        Add a Cast To node to a Blueprint graph.

        Args:
            blueprint_name: Name of the target Blueprint
            target_class: Class to cast to (e.g. '/Script/Engine.Character' or 'Character')
            position: Optional [X, Y] position in the graph
            graph_name: Optional graph name (defaults to EventGraph)

        Returns:
            Response containing the node_id
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            if position is None:
                position = [0, 0]
            params = {"blueprint_name": blueprint_name, "target_class": target_class, "position": position, "graph_name": graph_name}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("add_cast_node", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    # =========================================================================
    # Sprint 5: Extended Variables & Struct/Container Operations
    # =========================================================================

    @mcp.tool()
    def add_blueprint_variable_extended(
        ctx: Context,
        blueprint_name: str,
        variable_name: str,
        variable_type: str,
        sub_type: str = "",
        container_type: str = "",
        is_exposed: bool = True,
        is_replicated: bool = False,
        category: str = ""
    ) -> Dict[str, Any]:
        """
        Add a variable to a Blueprint with extended type support (all primitive types,
        structs, objects, classes, and container types).

        Args:
            blueprint_name: Name of the target Blueprint
            variable_name: Name of the variable
            variable_type: Type: Boolean, Byte, Integer, Integer64, Float, Double,
                          Name, String, Text, Vector, Rotator, Transform, Object, Class, Struct
            sub_type: For Object/Class/Struct types - the specific subtype
                     (e.g. 'Actor' for Object, 'Vector' for Struct)
            container_type: Optional container: 'Array', 'Set', or 'Map'
            is_exposed: Whether to expose in the editor details panel
            is_replicated: Whether to replicate over network
            category: Optional category for organizing in the details panel

        Returns:
            Response with variable_name and variable_type
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            params = {
                "blueprint_name": blueprint_name,
                "variable_name": variable_name,
                "variable_type": variable_type,
                "sub_type": sub_type,
                "container_type": container_type,
                "is_exposed": is_exposed,
                "is_replicated": is_replicated,
                "category": category
            }
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("add_blueprint_variable_extended", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def set_variable_default_value(
        ctx: Context,
        blueprint_name: str,
        variable_name: str,
        default_value: str
    ) -> Dict[str, Any]:
        """
        Set the default value of a Blueprint variable.

        Args:
            blueprint_name: Name of the target Blueprint
            variable_name: Name of the variable
            default_value: String representation of the default value
                          (e.g. "42", "3.14", "true", "Hello", "(X=1.0,Y=2.0,Z=3.0)")

        Returns:
            Response indicating success
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            params = {"blueprint_name": blueprint_name, "variable_name": variable_name, "default_value": default_value}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("set_variable_default_value", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def set_variable_metadata(
        ctx: Context,
        blueprint_name: str,
        variable_name: str,
        category: str = "",
        tooltip: str = "",
        is_exposed: bool = None,
        is_read_only: bool = None,
        is_replicated: bool = None,
        rep_notify_func: str = ""
    ) -> Dict[str, Any]:
        """
        Set metadata on a Blueprint variable (category, tooltip, flags, replication).

        Args:
            blueprint_name: Name of the target Blueprint
            variable_name: Name of the variable
            category: Optional category for organizing in details panel
            tooltip: Optional tooltip text
            is_exposed: Whether to expose in editor (None = don't change)
            is_read_only: Whether the variable is read-only in Blueprints (None = don't change)
            is_replicated: Whether to replicate (None = don't change)
            rep_notify_func: Name of the RepNotify function (sets replication automatically)

        Returns:
            Response indicating success
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            params = {"blueprint_name": blueprint_name, "variable_name": variable_name}
            if category:
                params["category"] = category
            if tooltip:
                params["tooltip"] = tooltip
            if is_exposed is not None:
                params["is_exposed"] = is_exposed
            if is_read_only is not None:
                params["is_read_only"] = is_read_only
            if is_replicated is not None:
                params["is_replicated"] = is_replicated
            if rep_notify_func:
                params["rep_notify_func"] = rep_notify_func
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("set_variable_metadata", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def delete_variable(
        ctx: Context,
        blueprint_name: str,
        variable_name: str
    ) -> Dict[str, Any]:
        """
        Delete a variable from a Blueprint.

        Args:
            blueprint_name: Name of the target Blueprint
            variable_name: Name of the variable to delete

        Returns:
            Response indicating success
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            params = {"blueprint_name": blueprint_name, "variable_name": variable_name}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("delete_variable", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def set_blueprint_variable_properties(
        ctx: Context,
        blueprint_name: str,
        variable_name: str,
        is_blueprint_readable: bool = None,
        is_blueprint_writable: bool = None,
        is_exposed: bool = None,
        is_read_only: bool = None,
        is_editable_in_instance: bool = None,
        expose_on_spawn: bool = None,
        expose_to_cinematics: bool = None,
        replication_enabled: bool = None,
        replication_condition: int = None,
        rep_notify_func: str = None,
        category: str = None,
        tooltip: str = None,
        default_value: str = None,
        slider_range_min: float = None,
        slider_range_max: float = None,
        value_range_min: float = None,
        value_range_max: float = None,
        units: str = None,
        bitmask: bool = None,
        bitmask_enum: str = None,
    ) -> Dict[str, Any]:
        """Set comprehensive properties on a Blueprint variable in a single call.

        All parameters except blueprint_name and variable_name are optional.
        Only properties that are explicitly provided will be changed.

        Args:
            blueprint_name: Name of the target Blueprint.
            variable_name: Name of the variable to modify.
            is_blueprint_readable: Whether the variable is visible to Blueprints.
            is_blueprint_writable: Whether the variable can be set from Blueprints (clears ReadOnly).
            is_exposed: Whether the variable is exposed as public/editable in the editor.
            is_read_only: Whether the variable is read-only in Blueprints.
            is_editable_in_instance: Whether the variable can be edited per-instance in the level.
            expose_on_spawn: Whether the variable appears as a pin on SpawnActor nodes.
            expose_to_cinematics: Whether the variable is available to Sequencer (Interp flag).
            replication_enabled: Whether the variable replicates over the network.
            replication_condition: Replication condition (0=None, 1=InitialOnly, 2=OwnerOnly,
                3=SkipOwner, 4=SimulatedOnly, 5=AutonomousOnly, 6=SimulatedOrPhysics,
                7=InitialOrOwner). Also enables replication.
            rep_notify_func: Name of the RepNotify function. Also enables replication.
            category: Variable category in the Details panel.
            tooltip: Tooltip text shown on hover.
            default_value: Default value as a string.
            slider_range_min: Minimum value shown on the UI slider (UIMin metadata).
            slider_range_max: Maximum value shown on the UI slider (UIMax metadata).
            value_range_min: Hard minimum clamp value (ClampMin metadata).
            value_range_max: Hard maximum clamp value (ClampMax metadata).
            units: Display units string (e.g. "Centimeters", "Degrees", "Seconds").
            bitmask: Whether this integer variable is treated as a bitmask.
            bitmask_enum: Full path to the enum for bitmask display (e.g. "/Script/MyModule.EMyFlags").
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            params = {"blueprint_name": blueprint_name, "variable_name": variable_name}

            # Only send params that were explicitly provided
            bool_fields = {
                "is_blueprint_readable": is_blueprint_readable,
                "is_blueprint_writable": is_blueprint_writable,
                "is_exposed": is_exposed,
                "is_read_only": is_read_only,
                "is_editable_in_instance": is_editable_in_instance,
                "expose_on_spawn": expose_on_spawn,
                "expose_to_cinematics": expose_to_cinematics,
                "replication_enabled": replication_enabled,
                "bitmask": bitmask,
            }
            for key, val in bool_fields.items():
                if val is not None:
                    params[key] = val

            if replication_condition is not None:
                params["replication_condition"] = replication_condition
            if rep_notify_func is not None:
                params["rep_notify_func"] = rep_notify_func
            if category is not None:
                params["category"] = category
            if tooltip is not None:
                params["tooltip"] = tooltip
            if default_value is not None:
                params["default_value"] = default_value
            if units is not None:
                params["units"] = units
            if bitmask_enum is not None:
                params["bitmask_enum"] = bitmask_enum

            float_fields = {
                "slider_range_min": slider_range_min,
                "slider_range_max": slider_range_max,
                "value_range_min": value_range_min,
                "value_range_max": value_range_max,
            }
            for key, val in float_fields.items():
                if val is not None:
                    params[key] = val

            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("set_blueprint_variable_properties", params) or {
                "success": False, "message": "No response"
            }
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def add_make_struct_node(
        ctx: Context,
        blueprint_name: str,
        struct_type: str,
        position: list = None,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """
        Add a Make Struct node to a Blueprint graph.

        Args:
            blueprint_name: Name of the target Blueprint
            struct_type: Full path or name of the struct (e.g. '/Script/CoreUObject.Vector')
            position: Optional [X, Y] position in the graph
            graph_name: Optional graph name (defaults to EventGraph)

        Returns:
            Response containing the node_id
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            if position is None:
                position = [0, 0]
            params = {"blueprint_name": blueprint_name, "struct_type": struct_type, "position": position, "graph_name": graph_name}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("add_make_struct_node", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def add_break_struct_node(
        ctx: Context,
        blueprint_name: str,
        struct_type: str,
        position: list = None,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """
        Add a Break Struct node to a Blueprint graph.

        Args:
            blueprint_name: Name of the target Blueprint
            struct_type: Full path or name of the struct (e.g. '/Script/CoreUObject.Vector')
            position: Optional [X, Y] position in the graph
            graph_name: Optional graph name (defaults to EventGraph)

        Returns:
            Response containing the node_id
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            if position is None:
                position = [0, 0]
            params = {"blueprint_name": blueprint_name, "struct_type": struct_type, "position": position, "graph_name": graph_name}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("add_break_struct_node", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def add_make_array_node(
        ctx: Context,
        blueprint_name: str,
        position: list = None,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """
        Add a Make Array node to a Blueprint graph.

        Args:
            blueprint_name: Name of the target Blueprint
            position: Optional [X, Y] position in the graph
            graph_name: Optional graph name (defaults to EventGraph)

        Returns:
            Response containing the node_id
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            if position is None:
                position = [0, 0]
            params = {"blueprint_name": blueprint_name, "position": position, "graph_name": graph_name}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("add_make_array_node", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def add_make_map_node(
        ctx: Context,
        blueprint_name: str,
        position: list = None,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """
        Add a Make Map node to a Blueprint graph.

        Args:
            blueprint_name: Name of the target Blueprint
            position: Optional [X, Y] position in the graph
            graph_name: Optional graph name (defaults to EventGraph)

        Returns:
            Response containing the node_id
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            if position is None:
                position = [0, 0]
            params = {"blueprint_name": blueprint_name, "position": position, "graph_name": graph_name}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("add_make_map_node", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def add_make_set_node(
        ctx: Context,
        blueprint_name: str,
        position: list = None,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """
        Add a Make Set node to a Blueprint graph.

        Args:
            blueprint_name: Name of the target Blueprint
            position: Optional [X, Y] position in the graph
            graph_name: Optional graph name (defaults to EventGraph)

        Returns:
            Response containing the node_id
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            if position is None:
                position = [0, 0]
            params = {"blueprint_name": blueprint_name, "position": position, "graph_name": graph_name}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("add_make_set_node", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def add_literal_node(
        ctx: Context,
        blueprint_name: str,
        type: str,
        value: str,
        position: list = None,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """
        Add a literal/constant value node to a Blueprint graph.

        Args:
            blueprint_name: Name of the target Blueprint
            type: Literal type: Float, Double, Integer, Int, Boolean, Bool, String, Name, Text
            value: String representation of the value (e.g. "3.14", "42", "true", "Hello")
            position: Optional [X, Y] position in the graph
            graph_name: Optional graph name (defaults to EventGraph)

        Returns:
            Response containing the node_id
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            if position is None:
                position = [0, 0]
            params = {"blueprint_name": blueprint_name, "type": type, "value": value, "position": position, "graph_name": graph_name}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("add_literal_node", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    # =========================================================================
    # Sprint 6: QoL Node Operations
    # =========================================================================

    @mcp.tool()
    def validate_connection(
        ctx: Context,
        blueprint_name: str,
        source_node_guid: str,
        source_pin_name: str,
        target_node_guid: str,
        target_pin_name: str,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """Check if two pins can be connected (type compatibility check)."""
        from unreal_mcp_server import get_unreal_connection
        try:
            params = {"blueprint_name": blueprint_name, "source_node_guid": source_node_guid, "source_pin_name": source_pin_name, "target_node_guid": target_node_guid, "target_pin_name": target_pin_name, "graph_name": graph_name}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("validate_connection", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def auto_connect_nodes(
        ctx: Context,
        blueprint_name: str,
        source_node_guid: str,
        target_node_guid: str,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """Auto-connect compatible pins between two nodes (exec first, then data types)."""
        from unreal_mcp_server import get_unreal_connection
        try:
            params = {"blueprint_name": blueprint_name, "source_node_guid": source_node_guid, "target_node_guid": target_node_guid, "graph_name": graph_name}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("auto_connect_nodes", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def duplicate_node(
        ctx: Context,
        blueprint_name: str,
        node_guid: str,
        offset: list = None,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """Duplicate a node in the same graph. Returns the new node's GUID."""
        from unreal_mcp_server import get_unreal_connection
        try:
            if offset is None:
                offset = [200, 50]
            params = {"blueprint_name": blueprint_name, "node_guid": node_guid, "offset": offset, "graph_name": graph_name}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("duplicate_node", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def copy_nodes_to_graph(
        ctx: Context,
        blueprint_name: str,
        source_graph_name: str,
        target_graph_name: str,
        node_guids: list = None
    ) -> Dict[str, Any]:
        """Copy nodes from one graph to another within the same blueprint."""
        from unreal_mcp_server import get_unreal_connection
        try:
            if node_guids is None:
                node_guids = []
            params = {"blueprint_name": blueprint_name, "source_graph_name": source_graph_name, "target_graph_name": target_graph_name, "node_guids": node_guids}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("copy_nodes_to_graph", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def add_reroute_node(
        ctx: Context,
        blueprint_name: str,
        position: list = None,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """Add a reroute (knot) node for cleaner wire routing."""
        from unreal_mcp_server import get_unreal_connection
        try:
            if position is None:
                position = [0, 0]
            params = {"blueprint_name": blueprint_name, "position": position, "graph_name": graph_name}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("add_reroute_node", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    # =========================================================================
    # Sprint 7: Timelines, Delegates & Advanced Nodes
    # =========================================================================

    @mcp.tool()
    def add_timeline_node(
        ctx: Context,
        blueprint_name: str,
        timeline_name: str,
        auto_play: bool = False,
        loop: bool = False,
        position: list = None,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """Add a Timeline node. Pins: Play, PlayFromStart, Stop, Reverse, Update, Finished, Direction."""
        from unreal_mcp_server import get_unreal_connection
        try:
            if position is None:
                position = [0, 0]
            params = {"blueprint_name": blueprint_name, "timeline_name": timeline_name, "auto_play": auto_play, "loop": loop, "position": position, "graph_name": graph_name}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("add_timeline_node", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def add_event_dispatcher(
        ctx: Context,
        blueprint_name: str,
        dispatcher_name: str,
        parameters: Optional[List[Dict[str, str]]] = None
    ) -> Dict[str, Any]:
        """
        Add an event dispatcher (multicast delegate) variable to the blueprint.

        Creates the delegate variable AND its signature graph so it compiles cleanly.
        Optionally accepts parameters that define the delegate signature.

        Args:
            blueprint_name: Name of the target Blueprint
            dispatcher_name: Name for the event dispatcher
            parameters: Optional list of parameter dicts [{"name": "Damage", "type": "Float"}, ...]
                        Supported types: Boolean, Integer, Float, Double, String, Name, Text,
                        Vector, Rotator, Object

        Returns:
            Response with dispatcher_name, parameter_count, and message
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            params = {"blueprint_name": blueprint_name, "dispatcher_name": dispatcher_name}
            if parameters is not None:
                params["parameters"] = parameters
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("add_event_dispatcher", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def add_call_dispatcher_node(
        ctx: Context,
        blueprint_name: str,
        dispatcher_name: str,
        position: list = None,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """Add a Call node for an event dispatcher (broadcasts the delegate)."""
        from unreal_mcp_server import get_unreal_connection
        try:
            if position is None:
                position = [0, 0]
            params = {"blueprint_name": blueprint_name, "dispatcher_name": dispatcher_name, "position": position, "graph_name": graph_name}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("add_call_dispatcher_node", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def add_bind_dispatcher_node(
        ctx: Context,
        blueprint_name: str,
        dispatcher_name: str,
        position: list = None,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """Add a Bind Event node for an event dispatcher."""
        from unreal_mcp_server import get_unreal_connection
        try:
            if position is None:
                position = [0, 0]
            params = {"blueprint_name": blueprint_name, "dispatcher_name": dispatcher_name, "position": position, "graph_name": graph_name}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("add_bind_dispatcher_node", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def add_create_delegate_node(
        ctx: Context,
        blueprint_name: str,
        function_name: str = "",
        position: list = None,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """Add a Create Delegate node. Optionally bind it to a function."""
        from unreal_mcp_server import get_unreal_connection
        try:
            if position is None:
                position = [0, 0]
            params = {"blueprint_name": blueprint_name, "function_name": function_name, "position": position, "graph_name": graph_name}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("add_create_delegate_node", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def add_spawn_actor_node(
        ctx: Context,
        blueprint_name: str,
        position: list = None,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """Add a SpawnActorFromClass node. Set class via the Class pin."""
        from unreal_mcp_server import get_unreal_connection
        try:
            if position is None:
                position = [0, 0]
            params = {"blueprint_name": blueprint_name, "position": position, "graph_name": graph_name}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("add_spawn_actor_node", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def add_construct_object_node(
        ctx: Context,
        blueprint_name: str,
        position: list = None,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """Add a Construct Object From Class node."""
        from unreal_mcp_server import get_unreal_connection
        try:
            if position is None:
                position = [0, 0]
            params = {"blueprint_name": blueprint_name, "position": position, "graph_name": graph_name}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("add_construct_object_node", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def add_format_text_node(
        ctx: Context,
        blueprint_name: str,
        position: list = None,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """Add a Format Text node with dynamic argument pins."""
        from unreal_mcp_server import get_unreal_connection
        try:
            if position is None:
                position = [0, 0]
            params = {"blueprint_name": blueprint_name, "position": position, "graph_name": graph_name}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("add_format_text_node", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def add_select_node(
        ctx: Context,
        blueprint_name: str,
        position: list = None,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """Add a Select node (value choice by index/condition)."""
        from unreal_mcp_server import get_unreal_connection
        try:
            if position is None:
                position = [0, 0]
            params = {"blueprint_name": blueprint_name, "position": position, "graph_name": graph_name}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("add_select_node", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def add_interface_message_node(
        ctx: Context,
        blueprint_name: str,
        function_name: str,
        interface_name: str = "",
        position: list = None,
        graph_name: str = ""
    ) -> Dict[str, Any]:
        """Add an interface function call (message) node."""
        from unreal_mcp_server import get_unreal_connection
        try:
            if position is None:
                position = [0, 0]
            params = {"blueprint_name": blueprint_name, "function_name": function_name, "interface_name": interface_name, "position": position, "graph_name": graph_name}
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("add_interface_message_node", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    # =========================================================================
    # Rework Phases 5-7: pin introspection + dynamic cast + component bound event
    # =========================================================================

    @mcp.tool()
    def describe_node_pins(
        ctx: Context,
        node_class: str,
        struct_type: str = "",
        enum_type: str = "",
        target_class: str = "",
        is_pure: bool = False,
    ) -> Dict[str, Any]:
        """Return the default pin layout for a K2 node class WITHOUT spawning it.

        Use this before wiring to learn exact pin names, types, and directions.
        The node is instantiated in a transient graph and discarded — no blueprint
        is modified.

        Args:
            node_class: Short name like "Branch", "MakeArray", "Switch on Int",
                        "Dynamic Cast", "MakeStruct", "Self", "ComponentBoundEvent",
                        or the raw UClass name like "K2Node_IfThenElse".
            struct_type: For MakeStruct / BreakStruct — the struct to preview
                         (e.g. "FVector", "MyCustomStruct", "/Script/Engine.HitResult").
            enum_type: For SwitchOnEnum — the enum to preview (e.g. "EInputEvent").
            target_class: For DynamicCast — the class to cast to.
            is_pure: For DynamicCast — preview the pure variant (no exec pins).

        Returns:
            Dict with fields:
                node_class: resolved UClass short name
                resolved_name: editor display title
                pins: list of pin dicts (name, direction, category, sub_category,
                      sub_category_object, container_type, is_reference, is_const,
                      hidden, default_value, ...)
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            params: Dict[str, Any] = {"node_class": node_class}
            if struct_type:
                params["struct_type"] = struct_type
            if enum_type:
                params["enum_type"] = enum_type
            if target_class:
                params["target_class"] = target_class
            if is_pure:
                params["is_pure"] = is_pure
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("describe_node_pins", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def add_dynamic_cast_node(
        ctx: Context,
        blueprint_name: str,
        target_class: str,
        is_pure: bool = False,
        position: list = None,
        graph_name: str = "",
    ) -> Dict[str, Any]:
        """Spawn a Dynamic Cast (aka Cast To <Class>) node with explicit purity.

        Unlike add_cast_node, this variant honors the is_pure flag so the caller
        can request the pure (no exec pins) form up front.

        Args:
            blueprint_name: Target Blueprint name.
            target_class: Class to cast to (e.g. "Character", "/Script/Engine.PlayerController").
            is_pure: True = pure cast (output recomputed on read, no exec pins).
            position: Optional [X, Y] position.
            graph_name: Optional graph name (defaults to EventGraph).

        Returns:
            Node info dict including node_id and pins.
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            if position is None:
                position = [0, 0]
            params = {
                "blueprint_name": blueprint_name,
                "target_class": target_class,
                "is_pure": is_pure,
                "position": position,
                "graph_name": graph_name,
            }
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("add_dynamic_cast_node", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    @mcp.tool()
    def add_component_bound_event_node(
        ctx: Context,
        blueprint_name: str,
        component_name: str,
        delegate_name: str,
        position: list = None,
        graph_name: str = "",
    ) -> Dict[str, Any]:
        """Spawn a Component-Bound Event node (auto-binds to a component's multicast delegate).

        This is how you get the green "OnClicked" / "OnBeginOverlap" event nodes that
        come pre-wired to a specific component instance in the blueprint.

        Args:
            blueprint_name: Target Blueprint name.
            component_name: Name of the component UPROPERTY (e.g. "Mesh", "Trigger").
            delegate_name: Name of the multicast delegate on that component class
                           (e.g. "OnComponentBeginOverlap", "OnClicked").
            position: Optional [X, Y] position.
            graph_name: Optional graph name (defaults to EventGraph).

        Returns:
            Node info dict including node_id and pins.
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            if position is None:
                position = [0, 0]
            params = {
                "blueprint_name": blueprint_name,
                "component_name": component_name,
                "delegate_name": delegate_name,
                "position": position,
                "graph_name": graph_name,
            }
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            return unreal.send_command("add_component_bound_event_node", params) or {"success": False, "message": "No response"}
        except Exception as e:
            return {"success": False, "message": f"Error: {e}"}

    logger.info("Blueprint node tools registered successfully")