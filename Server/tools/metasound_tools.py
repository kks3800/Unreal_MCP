"""
MetaSound creation and editing tools for Unreal MCP.

Provides full MetaSound Source graph creation via MCP commands:
- Create MetaSound Source assets with configurable output formats
- Add and connect audio nodes (oscillators, filters, effects, etc.)
- Define graph inputs and outputs
- Build and save MetaSound assets
"""

import logging
from mcp.server.fastmcp import FastMCP, Context
from typing import List, Optional, Dict, Any

# Get logger
logger = logging.getLogger("UnrealMCP")


def register_metasound_tools(mcp: FastMCP):
    """Register all MetaSound-related MCP tools."""

    #=========================================================================
    # Asset Creation Commands
    #=========================================================================

    @mcp.tool()
    def create_metasound_source(
        ctx: Context,
        sound_name: str,
        path: str = "/Game/MetaSounds",
        output_format: str = "Mono",
        is_one_shot: bool = True
    ) -> Dict[str, Any]:
        """
        Create a new MetaSound Source asset.

        This creates a builder for the MetaSound. After adding nodes and connections,
        call build_metasound to save the asset.

        Args:
            sound_name: Name for the new MetaSound
            path: Content path (default: /Game/MetaSounds)
            output_format: Audio output format - Mono, Stereo, Quad, FiveDotOne (default: Mono)
            is_one_shot: Whether this is a one-shot sound (default: True)

        Returns:
            Dict with success status and builder info
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.warning("Failed to connect to Unreal Engine")
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("create_metasound_source", {
                "sound_name": sound_name,
                "path": path,
                "output_format": output_format,
                "is_one_shot": is_one_shot
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error creating MetaSound source: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def create_metasound_preset(
        ctx: Context,
        preset_name: str,
        source_name: str,
        path: str = "/Game/MetaSounds"
    ) -> Dict[str, Any]:
        """
        Create a MetaSound preset from an existing source.

        Args:
            preset_name: Name for the new preset
            source_name: Name of source MetaSound to copy from
            path: Content path (default: /Game/MetaSounds)

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("create_metasound_preset", {
                "preset_name": preset_name,
                "source_name": source_name,
                "path": path
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error creating MetaSound preset: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def delete_metasound(
        ctx: Context,
        sound_name: str
    ) -> Dict[str, Any]:
        """
        Delete a MetaSound asset.

        Args:
            sound_name: Name of MetaSound to delete

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("delete_metasound", {
                "sound_name": sound_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error deleting MetaSound: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_metasound_info(
        ctx: Context,
        sound_name: str
    ) -> Dict[str, Any]:
        """
        Get information about a MetaSound asset.

        Args:
            sound_name: Name of MetaSound to query

        Returns:
            Dict with MetaSound properties
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_metasound_info", {
                "sound_name": sound_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error getting MetaSound info: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def list_metasound_assets(
        ctx: Context,
        path: str = "/Game",
        recursive: bool = True
    ) -> Dict[str, Any]:
        """
        List MetaSound assets in a path.

        Args:
            path: Content path to search (default: /Game)
            recursive: Search recursively (default: True)

        Returns:
            Dict with list of MetaSound assets
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("list_metasound_assets", {
                "path": path,
                "recursive": recursive
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error listing MetaSound assets: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def open_metasound(
        ctx: Context,
        sound_name: str,
        builder_name: Optional[str] = None
    ) -> Dict[str, Any]:
        """
        Open an existing MetaSound for editing.

        Creates a builder from the asset so you can add/modify nodes, then save.
        This is much more efficient than recreating from scratch.

        Args:
            sound_name: Name or path of the MetaSound to open
            builder_name: Name for the builder (default: same as asset name)

        Returns:
            Dict with builder info and cached node/input counts
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            params = {
                "sound_name": sound_name
            }
            if builder_name is not None:
                params["builder_name"] = builder_name

            response = unreal.send_command("open_metasound", params)

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error opening MetaSound: {e}")
            return {"status": "error", "error": str(e)}

    #=========================================================================
    # Graph Input/Output Commands
    #=========================================================================

    @mcp.tool()
    def add_metasound_input(
        ctx: Context,
        sound_name: str,
        input_name: str,
        data_type: str,
        default_value: Optional[Any] = None,
        is_constructor: bool = False
    ) -> Dict[str, Any]:
        """
        Add an input to the MetaSound graph.

        Args:
            sound_name: Target MetaSound builder name
            input_name: Name for the input
            data_type: Data type - Float, Int32, Bool, String, Audio, Time, Trigger, WaveAsset
            default_value: Default value for the input (optional)
            is_constructor: Whether this is a constructor input (default: False)

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            params = {
                "sound_name": sound_name,
                "input_name": input_name,
                "data_type": data_type,
                "is_constructor": is_constructor
            }
            if default_value is not None:
                params["default_value"] = default_value

            response = unreal.send_command("add_metasound_input", params)

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding MetaSound input: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def add_metasound_output(
        ctx: Context,
        sound_name: str,
        output_name: str,
        data_type: str
    ) -> Dict[str, Any]:
        """
        Add an output to the MetaSound graph.

        Args:
            sound_name: Target MetaSound builder name
            output_name: Name for the output
            data_type: Data type for the output

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_metasound_output", {
                "sound_name": sound_name,
                "output_name": output_name,
                "data_type": data_type
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding MetaSound output: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_metasound_inputs(
        ctx: Context,
        sound_name: str
    ) -> Dict[str, Any]:
        """
        Get all inputs in a MetaSound graph.

        Args:
            sound_name: Target MetaSound builder name

        Returns:
            Dict with list of inputs
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_metasound_inputs", {
                "sound_name": sound_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error getting MetaSound inputs: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_metasound_outputs(
        ctx: Context,
        sound_name: str
    ) -> Dict[str, Any]:
        """
        Get all outputs in a MetaSound graph.

        Args:
            sound_name: Target MetaSound builder name

        Returns:
            Dict with list of outputs
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_metasound_outputs", {
                "sound_name": sound_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error getting MetaSound outputs: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def set_metasound_input_default(
        ctx: Context,
        sound_name: str,
        input_name: str,
        value: Any,
        data_type: str = "Float"
    ) -> Dict[str, Any]:
        """
        Set the default value for a graph input.

        Args:
            sound_name: Target MetaSound builder name
            input_name: Name of input
            value: New default value
            data_type: Data type (default: Float)

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("set_metasound_input_default", {
                "sound_name": sound_name,
                "input_name": input_name,
                "value": value,
                "data_type": data_type
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error setting MetaSound input default: {e}")
            return {"status": "error", "error": str(e)}

    #=========================================================================
    # Node Management Commands
    #=========================================================================

    @mcp.tool()
    def add_metasound_node(
        ctx: Context,
        sound_name: str,
        node_type: str,
        node_name: Optional[str] = None,
        position: Optional[List[float]] = None
    ) -> Dict[str, Any]:
        """
        Add a node to the MetaSound graph.

        Common node types:
        - Generators: UE.Generators.Sine, UE.Generators.Saw, UE.Generators.Square,
                     UE.Generators.Triangle, UE.Generators.Noise
        - Envelopes: UE.Generators.ADSR, UE.Generators.Envelope
        - Filters: UE.Filters.Biquad, UE.Filters.OnePole, UE.Filters.StateVariable
        - Math: UE.Math.Add, UE.Math.Multiply, UE.Math.Clamp, UE.Math.Lerp
        - Audio: UE.Audio.Gain, UE.Audio.Delay, UE.WavePlayer
        - Triggers: UE.Trigger.Repeat, UE.Trigger.Gate

        Args:
            sound_name: Target MetaSound builder name
            node_type: Node class name (e.g., "UE.Generators.Sine")
            node_name: Display name for the node (optional, defaults to node_type)
            position: [X, Y] position in graph (optional)

        Returns:
            Dict with node_id for future connections
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            params = {
                "sound_name": sound_name,
                "node_type": node_type
            }
            if node_name is not None:
                params["node_name"] = node_name
            if position is not None:
                params["position"] = position

            response = unreal.send_command("add_metasound_node", params)

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding MetaSound node: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_metasound_nodes(
        ctx: Context,
        sound_name: str
    ) -> Dict[str, Any]:
        """
        Get all nodes in a MetaSound graph.

        Args:
            sound_name: Target MetaSound builder name

        Returns:
            Dict with list of nodes
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_metasound_nodes", {
                "sound_name": sound_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error getting MetaSound nodes: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_metasound_node_inputs(
        ctx: Context,
        sound_name: str,
        node_id: str
    ) -> Dict[str, Any]:
        """
        Get input pins for a node.

        Args:
            sound_name: Target MetaSound builder name
            node_id: Target node ID/name

        Returns:
            Dict with list of input pins
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_metasound_node_inputs", {
                "sound_name": sound_name,
                "node_id": node_id
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error getting node inputs: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_metasound_node_outputs(
        ctx: Context,
        sound_name: str,
        node_id: str
    ) -> Dict[str, Any]:
        """
        Get output pins for a node.

        Args:
            sound_name: Target MetaSound builder name
            node_id: Target node ID/name

        Returns:
            Dict with list of output pins
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_metasound_node_outputs", {
                "sound_name": sound_name,
                "node_id": node_id
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error getting node outputs: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def set_metasound_node_input_default(
        ctx: Context,
        sound_name: str,
        node_id: str,
        input_name: str,
        value: Any,
        data_type: str = "Float"
    ) -> Dict[str, Any]:
        """
        Set the default value for a node input pin.

        Args:
            sound_name: Target MetaSound builder name
            node_id: Target node ID/name
            input_name: Name of input pin
            value: New default value
            data_type: Data type (default: Float)

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("set_metasound_node_input_default", {
                "sound_name": sound_name,
                "node_id": node_id,
                "input_name": input_name,
                "value": value,
                "data_type": data_type
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error setting node input default: {e}")
            return {"status": "error", "error": str(e)}

    #=========================================================================
    # Connection Commands
    #=========================================================================

    @mcp.tool()
    def connect_metasound_nodes(
        ctx: Context,
        sound_name: str,
        from_node: str,
        from_output: str,
        to_node: str,
        to_input: str
    ) -> Dict[str, Any]:
        """
        Connect two nodes in the MetaSound graph.

        Args:
            sound_name: Target MetaSound builder name
            from_node: Source node ID/name
            from_output: Output pin name on source node
            to_node: Destination node ID/name
            to_input: Input pin name on destination node

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("connect_metasound_nodes", {
                "sound_name": sound_name,
                "from_node": from_node,
                "from_output": from_output,
                "to_node": to_node,
                "to_input": to_input
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error connecting MetaSound nodes: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def disconnect_metasound_nodes(
        ctx: Context,
        sound_name: str,
        from_node: str,
        from_output: str,
        to_node: str,
        to_input: str
    ) -> Dict[str, Any]:
        """
        Disconnect two nodes in the MetaSound graph.

        Args:
            sound_name: Target MetaSound builder name
            from_node: Source node ID/name
            from_output: Output pin name
            to_node: Destination node ID/name
            to_input: Input pin name

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("disconnect_metasound_nodes", {
                "sound_name": sound_name,
                "from_node": from_node,
                "from_output": from_output,
                "to_node": to_node,
                "to_input": to_input
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error disconnecting MetaSound nodes: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def connect_to_metasound_output(
        ctx: Context,
        sound_name: str,
        from_node: str,
        from_output: str,
        graph_output: str
    ) -> Dict[str, Any]:
        """
        Connect a node to a graph output.

        Args:
            sound_name: Target MetaSound builder name
            from_node: Source node ID/name
            from_output: Output pin name on source node
            graph_output: Graph output name (e.g., "Audio Out Left")

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("connect_to_metasound_output", {
                "sound_name": sound_name,
                "from_node": from_node,
                "from_output": from_output,
                "graph_output": graph_output
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error connecting to graph output: {e}")
            return {"status": "error", "error": str(e)}

    #=========================================================================
    # Variable Commands
    #=========================================================================

    @mcp.tool()
    def add_metasound_variable(
        ctx: Context,
        sound_name: str,
        variable_name: str,
        data_type: str,
        default_value: Optional[Any] = None
    ) -> Dict[str, Any]:
        """
        Add a variable to the MetaSound graph.

        Args:
            sound_name: Target MetaSound builder name
            variable_name: Name for the variable
            data_type: Data type for the variable
            default_value: Default value (optional)

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            params = {
                "sound_name": sound_name,
                "variable_name": variable_name,
                "data_type": data_type
            }
            if default_value is not None:
                params["default_value"] = default_value

            response = unreal.send_command("add_metasound_variable", params)

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding MetaSound variable: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def add_metasound_variable_getter(
        ctx: Context,
        sound_name: str,
        variable_name: str
    ) -> Dict[str, Any]:
        """
        Add a variable getter node.

        Args:
            sound_name: Target MetaSound builder name
            variable_name: Variable to get

        Returns:
            Dict with node_id of getter node
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_metasound_variable_getter", {
                "sound_name": sound_name,
                "variable_name": variable_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding variable getter: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def add_metasound_variable_setter(
        ctx: Context,
        sound_name: str,
        variable_name: str
    ) -> Dict[str, Any]:
        """
        Add a variable setter node.

        Args:
            sound_name: Target MetaSound builder name
            variable_name: Variable to set

        Returns:
            Dict with node_id of setter node
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("add_metasound_variable_setter", {
                "sound_name": sound_name,
                "variable_name": variable_name
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error adding variable setter: {e}")
            return {"status": "error", "error": str(e)}

    #=========================================================================
    # Build Commands
    #=========================================================================

    @mcp.tool()
    def build_metasound(
        ctx: Context,
        sound_name: str,
        asset_name: Optional[str] = None,
        path: str = "/Game/MetaSounds"
    ) -> Dict[str, Any]:
        """
        Build the MetaSound and save as a new asset.

        Args:
            sound_name: Builder name to build from
            asset_name: Name for saved asset (default: same as builder)
            path: Save path (default: /Game/MetaSounds)

        Returns:
            Dict with asset path
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            params = {
                "sound_name": sound_name,
                "path": path
            }
            if asset_name is not None:
                params["asset_name"] = asset_name

            response = unreal.send_command("build_metasound", params)

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error building MetaSound: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def build_overwrite_metasound(
        ctx: Context,
        sound_name: str,
        target_asset: str
    ) -> Dict[str, Any]:
        """
        Build and overwrite an existing MetaSound asset.

        Args:
            sound_name: Builder name to build from
            target_asset: Asset to overwrite

        Returns:
            Dict with success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("build_overwrite_metasound", {
                "sound_name": sound_name,
                "target_asset": target_asset
            })

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error overwriting MetaSound: {e}")
            return {"status": "error", "error": str(e)}

    #=========================================================================
    # Utility Commands
    #=========================================================================

    @mcp.tool()
    def list_metasound_node_types(
        ctx: Context,
        category: Optional[str] = None
    ) -> Dict[str, Any]:
        """
        List all registered MetaSound node types.

        Args:
            category: Filter by category (Generators, Filters, Math, etc.)

        Returns:
            Dict with list of node types
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            params = {}
            if category is not None:
                params["category"] = category

            response = unreal.send_command("list_metasound_node_types", params)

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error listing node types: {e}")
            return {"status": "error", "error": str(e)}

    @mcp.tool()
    def get_metasound_data_types(
        ctx: Context
    ) -> Dict[str, Any]:
        """
        Get available MetaSound data types.

        Returns:
            Dict with list of data types (Float, Int32, Bool, Audio, etc.)
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"status": "error", "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_metasound_data_types", {})

            if not response:
                return {"status": "error", "error": "No response from Unreal Engine"}

            return {"status": "success", "result": response}

        except Exception as e:
            logger.error(f"Error getting data types: {e}")
            return {"status": "error", "error": str(e)}
